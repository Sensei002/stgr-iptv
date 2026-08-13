#include "services/IptvOrgApi.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>
#include <utility>

#include "core/AppPaths.h"
#include "core/Log.h"
#include "services/NetworkService.h"

namespace {

constexpr int kRefreshHours = 24; // refetch the API data when cached data is older
const QUrl kStreamsUrl(QStringLiteral("https://iptv-org.github.io/api/streams.json"));
const QUrl kChannelsUrl(QStringLiteral("https://iptv-org.github.io/api/channels.json"));

// Mirrors the quality-token stripping used by PlaybackController so channel
// names like "Zee Bangla HD (720p)" match API names like "Zee Bangla HD".
QString normalizedKey(const QString& name)
{
    static const QRegularExpression qualityRe(QStringLiteral(
        "\\b(2160p|1080p\\d*|1080p|1080i|1080|720p|720|576p|576|540p|480p|480|"
        "360p|240p|144p|4k|uhd|fhd|qhd|hd|sd|60fps|50fps|30fps|25fps|24fps)\\b"),
        QRegularExpression::CaseInsensitiveOption);

    QString n = name.toLower();
    n.remove(qualityRe);
    n.remove(QRegularExpression(QStringLiteral("[[\\](){}]")));
    n.remove(QRegularExpression(QStringLiteral("[^a-z0-9]+")));
    return n;
}

} // namespace

IptvOrgApi* IptvOrgApi::instance()
{
    static IptvOrgApi s;
    return &s;
}

IptvOrgApi::IptvOrgApi(QObject* parent)
    : QObject(parent)
{
}

void IptvOrgApi::start()
{
    if (m_started)
        return;
    m_started = true;
    loadCachedAsync();
    refreshIfStale();
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------
QVector<IptvStream> IptvOrgApi::streamsFor(const Channel& channel) const
{
    QVector<IptvStream> out;

    // Exact match on tvg-id when available (the bundled IPTV-org playlist
    // carries tvg-id attributes identical to the API channel ids).
    QSet<QString> ids;
    if (!channel.id.isEmpty() && m_streamsById.contains(channel.id))
        ids.insert(channel.id);

    // Fall back to a normalized-name match via channels.json.
    if (ids.isEmpty()) {
        const auto it = m_idsByName.constFind(normalizedKey(channel.displayName()));
        if (it != m_idsByName.constEnd())
            ids = QSet<QString>(it->begin(), it->end());
    }

    QSet<QString> seenUrls;
    for (const QString& id : std::as_const(ids)) {
        const auto sit = m_streamsById.constFind(id);
        if (sit == m_streamsById.constEnd())
            continue;
        for (const IptvStream& s : sit.value()) {
            if (s.url.isEmpty() || seenUrls.contains(s.url))
                continue;
            seenUrls.insert(s.url);
            out.append(s);
        }
    }

    // Streams with no special label first (label hints at geo-blocks, sign-up
    // requirements, ...) so failover prefers the most likely-to-work mirrors.
    std::stable_partition(out.begin(), out.end(),
                          [](const IptvStream& s) { return s.label.isEmpty(); });
    return out;
}

// ---------------------------------------------------------------------------
// Cached load (async; parse off the UI thread)
// ---------------------------------------------------------------------------
void IptvOrgApi::loadCachedAsync()
{
    const QString streamsPath = cacheStreamsPath();
    const QString channelsPath = cacheChannelsPath();

    auto future = QtConcurrent::run([streamsPath, channelsPath]() {
        QPair<QByteArray, QByteArray> pair;
        QFile fs(streamsPath);
        if (fs.open(QIODevice::ReadOnly))
            pair.first = fs.readAll();
        QFile fc(channelsPath);
        if (fc.open(QIODevice::ReadOnly))
            pair.second = fc.readAll();
        return pair;
    });

    auto* watcher = new QFutureWatcher<QPair<QByteArray, QByteArray>>(this);
    connect(watcher, &QFutureWatcher<QPair<QByteArray, QByteArray>>::finished, this,
            [this, watcher]() {
                const auto [streamsJson, channelsJson] = watcher->result();
                watcher->deleteLater();
                if (streamsJson.isEmpty() && channelsJson.isEmpty())
                    return; // no cache yet; the background refresh will populate it
                parseAsync(streamsJson, channelsJson);
            });
    watcher->setFuture(future);
}

// ---------------------------------------------------------------------------
// Background refresh
// ---------------------------------------------------------------------------
void IptvOrgApi::refreshIfStale()
{
    const QFileInfo streamsInfo(cacheStreamsPath());
    const QFileInfo channelsInfo(cacheChannelsPath());
    if (streamsInfo.exists() && channelsInfo.exists()) {
        const QDateTime cutoff = QDateTime::currentDateTime().addSecs(-kRefreshHours * 3600);
        if (streamsInfo.lastModified() > cutoff && channelsInfo.lastModified() > cutoff)
            return; // fresh enough
    }

    m_fetching = true;
    m_pendingFetches = 2;
    fetchStreams();
    fetchChannels();
}

void IptvOrgApi::fetchStreams()
{
    QNetworkReply* reply = NetworkService::instance()->nam()->get(
        NetworkService::instance()->makeRequest(kStreamsUrl, 60000));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        onFetched(QStringLiteral("streams"),
                  reply->error() == QNetworkReply::NoError, reply->readAll());
    });
}

void IptvOrgApi::fetchChannels()
{
    QNetworkReply* reply = NetworkService::instance()->nam()->get(
        NetworkService::instance()->makeRequest(kChannelsUrl, 60000));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        onFetched(QStringLiteral("channels"),
                  reply->error() == QNetworkReply::NoError, reply->readAll());
    });
}

void IptvOrgApi::onFetched(const QString& which, bool ok, const QByteArray& data)
{
    if (which == QLatin1String("streams")) {
        if (ok)
            m_streamsRaw = data;
    } else {
        if (ok)
            m_channelsRaw = data;
    }

    if (--m_pendingFetches > 0)
        return;

    m_fetching = false;
    if (m_streamsRaw.isEmpty() && m_channelsRaw.isEmpty())
        return; // both failed; keep whatever cache we have
    parseAsync(m_streamsRaw, m_channelsRaw);

    // Persist for the next launch (even a partially successful fetch).
    if (!m_streamsRaw.isEmpty())
        saveCache(QStringLiteral("iptvorg_streams.json"), m_streamsRaw);
    if (!m_channelsRaw.isEmpty())
        saveCache(QStringLiteral("iptvorg_channels.json"), m_channelsRaw);
}

// ---------------------------------------------------------------------------
// Parsing (off the UI thread)
// ---------------------------------------------------------------------------
void IptvOrgApi::parseAsync(const QByteArray& streamsJson, const QByteArray& channelsJson)
{
    auto future = QtConcurrent::run([streamsJson, channelsJson]() {
        ParsedData data;

        if (!streamsJson.isEmpty()) {
            const QJsonArray arr = QJsonDocument::fromJson(streamsJson).array();
            for (const QJsonValue& v : arr) {
                const QJsonObject o = v.toObject();
                const QString id = o.value(QStringLiteral("channel")).toString();
                if (id.isEmpty())
                    continue;
                IptvStream s;
                s.channelId = id;
                s.title = o.value(QStringLiteral("title")).toString();
                s.url = o.value(QStringLiteral("url")).toString();
                s.quality = o.value(QStringLiteral("quality")).toString();
                s.label = o.value(QStringLiteral("label")).toString();
                s.referrer = o.value(QStringLiteral("referrer")).toString();
                s.userAgent = o.value(QStringLiteral("user_agent")).toString();
                if (s.url.isEmpty())
                    continue;
                data.streamsById[id].append(s);
            }
        }

        if (!channelsJson.isEmpty()) {
            const QJsonArray arr = QJsonDocument::fromJson(channelsJson).array();
            for (const QJsonValue& v : arr) {
                const QJsonObject o = v.toObject();
                const QString id = o.value(QStringLiteral("id")).toString();
                if (id.isEmpty())
                    continue;
                const QString name = o.value(QStringLiteral("name")).toString();
                if (!name.isEmpty())
                    data.idsByName[normalizedKey(name)].append(id);
                const QJsonArray alt = o.value(QStringLiteral("alt_names")).toArray();
                for (const QJsonValue& a : alt) {
                    const QString an = a.toString();
                    if (!an.isEmpty())
                        data.idsByName[normalizedKey(an)].append(id);
                }
            }
            // De-duplicate id lists per normalized name.
            for (auto it = data.idsByName.begin(); it != data.idsByName.end(); ++it) {
                it.value().removeDuplicates();
            }
        }

        return data;
    });

    auto* watcher = new QFutureWatcher<ParsedData>(this);
    connect(watcher, &QFutureWatcher<ParsedData>::finished, this, [this, watcher]() {
        applyData(watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void IptvOrgApi::applyData(const ParsedData& data)
{
    m_streamsById = data.streamsById;
    m_idsByName = data.idsByName;
    if (!m_ready && !m_streamsById.isEmpty()) {
        m_ready = true;
        qInfo() << "iptv-org api: loaded" << m_streamsById.size() << "channel(s) of mirrors";
        emit ready();
    } else if (m_ready) {
        emit ready(); // data refreshed; consumers may re-query
    }
}

void IptvOrgApi::saveCache(const QString& fileName, const QByteArray& data) const
{
    const QString path = AppPaths::cacheDir() + QLatin1Char('/') + fileName;
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.commit();
    }
}

QString IptvOrgApi::cacheStreamsPath() const
{
    return AppPaths::cacheDir() + QLatin1String("/iptvorg_streams.json");
}

QString IptvOrgApi::cacheChannelsPath() const
{
    return AppPaths::cacheDir() + QLatin1String("/iptvorg_channels.json");
}
