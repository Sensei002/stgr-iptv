#include "playlist/PlaylistManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QThreadPool>
#include <QUuid>
#include <QtConcurrent/QtConcurrent>

#include "core/AppPaths.h"
#include "playlist/M3uParser.h"
#include "services/PlaylistFetcher.h"

namespace {

const char kRegistryFile[] = "playlists.json";

QJsonObject channelToJson(const Channel& c)
{
    QJsonObject o;
    o.insert(QStringLiteral("name"), c.name);
    o.insert(QStringLiteral("url"), c.url);
    o.insert(QStringLiteral("logo"), c.logo);
    o.insert(QStringLiteral("id"), c.id);
    o.insert(QStringLiteral("group"), c.group);
    o.insert(QStringLiteral("country"), c.country);
    o.insert(QStringLiteral("language"), c.language);
    o.insert(QStringLiteral("network"), c.network);
    o.insert(QStringLiteral("number"), c.number);
    return o;
}

Channel channelFromJson(const QJsonObject& o)
{
    Channel c;
    c.name = o.value(QStringLiteral("name")).toString();
    c.url = o.value(QStringLiteral("url")).toString();
    c.logo = o.value(QStringLiteral("logo")).toString();
    c.id = o.value(QStringLiteral("id")).toString();
    c.group = o.value(QStringLiteral("group")).toString();
    c.country = o.value(QStringLiteral("country")).toString();
    c.language = o.value(QStringLiteral("language")).toString();
    c.network = o.value(QStringLiteral("network")).toString();
    c.number = o.value(QStringLiteral("number")).toInt(0);
    return c;
}

} // namespace

// ---------------------------------------------------------------------------
// PlaylistManager
// ---------------------------------------------------------------------------
PlaylistManager::PlaylistManager(IPlaylistSource* source, QObject* parent)
    : QObject(parent)
    , m_source(source)
{
    connect(m_source, &IPlaylistSource::finished,
            this, &PlaylistManager::onSourceFinished);
}

void PlaylistManager::load()
{
    m_playlists.clear();
    m_channels.clear();

    QFile f(AppPaths::playlistsDir() + QStringLiteral("/") + QString::fromLatin1(kRegistryFile));
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isArray()) {
            const QJsonArray arr = doc.array();
            for (const QJsonValue& v : arr) {
                if (v.isObject())
                    m_playlists.append(playlistFromJson(v.toObject()));
            }
        }
    }

    loadCachesAsync();
    emit playlistsChanged();
}

void PlaylistManager::loadCachesAsync()
{
    for (const Playlist& p : m_playlists) {
        const QString file = AppPaths::cacheFileForPlaylist(p.id);
        auto future = QtConcurrent::run([file, playlistId = p.id, playlistName = p.name]() {
            QVector<Channel> channels;
            QFile f(file);
            if (f.open(QIODevice::ReadOnly)) {
                const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
                if (doc.isArray()) {
                    const QJsonArray arr = doc.array();
                    channels.reserve(arr.size());
                    for (const QJsonValue& v : arr) {
                        Channel c = channelFromJson(v.toObject());
                        c.playlistId = playlistId;
                        c.playlistName = playlistName;
                        if (c.isValid())
                            channels.append(c);
                    }
                }
            }
            return channels;
        });

        auto* watcher = new QFutureWatcher<QVector<Channel>>(this);
        connect(watcher, &QFutureWatcher<QVector<Channel>>::finished, this,
                [this, watcher, id = p.id]() {
                    onCacheLoaded(id, watcher->result());
                    watcher->deleteLater();
                });
        watcher->setFuture(future);
    }
}

void PlaylistManager::onCacheLoaded(const QString& playlistId, const QVector<Channel>& channels)
{
    if (!m_channels.contains(playlistId))
        m_channels.insert(playlistId, channels);
    emit channelsChanged(playlistId);
    emit allChannelsChanged();
}

void PlaylistManager::save()
{
    persistPlaylists();
}

void PlaylistManager::persistPlaylists()
{
    QJsonArray arr;
    for (const Playlist& p : m_playlists)
        arr.append(playlistToJson(p));

    const QString file = AppPaths::playlistsDir() + QStringLiteral("/") + QString::fromLatin1(kRegistryFile);
    QSaveFile out(file);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        out.commit();
    }
}

const Playlist* PlaylistManager::findPlaylist(const QString& id) const
{
    const int idx = indexOf(id);
    return idx < 0 ? nullptr : &m_playlists.at(idx);
}

int PlaylistManager::indexOf(const QString& id) const
{
    for (int i = 0; i < m_playlists.size(); ++i) {
        if (m_playlists.at(i).id == id)
            return i;
    }
    return -1;
}

QVector<Channel> PlaylistManager::channelsFor(const QString& playlistId) const
{
    return m_channels.value(playlistId);
}

QVector<Channel> PlaylistManager::allChannels() const
{
    QVector<Channel> out;
    for (const Playlist& p : m_playlists) {
        if (!p.enabled)
            continue;
        const QVector<Channel> channels = m_channels.value(p.id);
        out.reserve(out.size() + channels.size());
        for (const Channel& c : channels)
            out.append(c);
    }
    return out;
}

QString PlaylistManager::addPlaylist(const QString& name, const QString& url,
                                     const QString& epgUrl, bool builtIn)
{
    Playlist p;
    p.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    p.name = name.trimmed().isEmpty() ? tr("Playlist") : name.trimmed();
    p.url = url.trimmed();
    p.epgUrl = epgUrl.trimmed();
    p.builtIn = builtIn;
    p.enabled = true;

    m_playlists.append(p);
    persistPlaylists();
    emit playlistsChanged();
    return p.id;
}

bool PlaylistManager::removePlaylist(const QString& id)
{
    const int idx = indexOf(id);
    if (idx < 0)
        return false;

    m_playlists.removeAt(idx);
    m_channels.remove(id);
    QFile::remove(AppPaths::cacheFileForPlaylist(id));

    persistPlaylists();
    emit channelsChanged(id);
    emit playlistsChanged();
    emit allChannelsChanged();
    return true;
}

bool PlaylistManager::renamePlaylist(const QString& id, const QString& newName)
{
    const int idx = indexOf(id);
    if (idx < 0)
        return false;
    const QString name = newName.trimmed();
    if (name.isEmpty() || m_playlists[idx].name == name)
        return false;

    m_playlists[idx].name = name;

    // Keep channel snapshots in sync.
    QVector<Channel> channels = m_channels.value(id);
    for (Channel& c : channels)
        c.playlistName = name;
    m_channels.insert(id, channels);

    persistPlaylists();
    emit playlistsChanged();
    return true;
}

bool PlaylistManager::setPlaylistUrl(const QString& id, const QString& url)
{
    const int idx = indexOf(id);
    if (idx < 0)
        return false;
    if (m_playlists[idx].url == url.trimmed())
        return false;

    m_playlists[idx].url = url.trimmed();
    m_playlists[idx].errorMessage.clear();
    persistPlaylists();
    emit playlistsChanged();
    return true;
}

bool PlaylistManager::setPlaylistEnabled(const QString& id, bool enabled)
{
    const int idx = indexOf(id);
    if (idx < 0 || m_playlists[idx].enabled == enabled)
        return false;

    m_playlists[idx].enabled = enabled;
    persistPlaylists();
    emit playlistsChanged();
    emit allChannelsChanged();
    return true;
}

bool PlaylistManager::setPlaylistEpgUrl(const QString& id, const QString& epgUrl)
{
    const int idx = indexOf(id);
    if (idx < 0)
        return false;
    m_playlists[idx].epgUrl = epgUrl.trimmed();
    persistPlaylists();
    emit playlistsChanged();
    return true;
}

bool PlaylistManager::refresh(const QString& id)
{
    const int idx = indexOf(id);
    if (idx < 0 || m_refreshing.contains(id))
        return false;

    m_refreshing.insert(id);
    emit refreshStarted(id);
    m_source->fetch(m_playlists.at(idx), m_timeoutMs);
    return true;
}

void PlaylistManager::refreshAll()
{
    for (const Playlist& p : m_playlists) {
        if (p.enabled)
            refresh(p.id);
    }
}

void PlaylistManager::onSourceFinished(const QString& playlistId, bool ok,
                                       const QByteArray& data, const QString& errorMessage)
{
    const int idx = indexOf(playlistId);
    if (idx < 0) {
        m_refreshing.remove(playlistId);
        return;
    }

    if (!ok) {
        m_playlists[idx].errorMessage = errorMessage;
        persistPlaylists();
        m_refreshing.remove(playlistId);
        emit playlistsChanged();
        emit refreshFinished(playlistId, false, errorMessage, -1);
        return;
    }

    // Parse on a worker thread.
    const Playlist p = m_playlists.at(idx);
    const QUrl base = p.isLocal() ? QUrl::fromLocalFile(p.url) : QUrl(p.url);
    auto future = QtConcurrent::run([data, base, id = p.id, name = p.name]() {
        return M3uParser::parse(data, base, id, name);
    });

    auto* watcher = new QFutureWatcher<M3uParser::Result>(this);
    connect(watcher, &QFutureWatcher<M3uParser::Result>::finished, this,
            [this, watcher, id = playlistId]() {
                onParsed(id, watcher->result());
                watcher->deleteLater();
            });
    watcher->setFuture(future);
}

void PlaylistManager::onParsed(const QString& playlistId, const M3uParser::Result& result)
{
    const int idx = indexOf(playlistId);
    if (idx < 0) {
        m_refreshing.remove(playlistId);
        return;
    }

    m_playlists[idx].channelCount = result.channels.size();
    m_playlists[idx].lastUpdated = QDateTime::currentDateTime();
    m_playlists[idx].errorMessage.clear();

    m_channels.insert(playlistId, result.channels);
    writeCacheFile(playlistId, result.channels);
    persistPlaylists();

    m_refreshing.remove(playlistId);
    emit channelsChanged(playlistId);
    emit playlistsChanged();
    emit allChannelsChanged();
    emit refreshFinished(playlistId, true, QString(), result.channels.size());
}

void PlaylistManager::writeCacheFile(const QString& playlistId, const QVector<Channel>& channels)
{
    const QString file = AppPaths::cacheFileForPlaylist(playlistId);
    QJsonArray arr;
    for (const Channel& c : channels)
        arr.append(channelToJson(c));

    // Fire-and-forget: the returned QFuture would be discarded, which is a
    // C4858 error under /WX - use QThreadPool::start for void work instead.
    QThreadPool::globalInstance()->start([file, arr]() {
        QSaveFile out(file);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
            out.commit();
        }
    });
}

bool PlaylistManager::importLocalFile(const QString& filePath)
{
    const QFileInfo info(filePath);
    const QString suffix = info.completeSuffix().toLower();
    if (!info.exists() || (suffix != QLatin1String("m3u") && suffix != QLatin1String("m3u8"))) {
        return false;
    }

    QDir imports(AppPaths::playlistImportsDir());
    QString target = imports.filePath(info.fileName());
    if (QFile::exists(target)) {
        const QString stem = info.completeBaseName();
        const QString ext = info.completeSuffix();
        int n = 1;
        do {
            target = imports.filePath(QStringLiteral("%1_%2.%3").arg(stem).arg(n).arg(ext));
            ++n;
        } while (QFile::exists(target));
    }

    if (!QFile::copy(filePath, target))
        return false;

    const QString id = addPlaylist(info.completeBaseName(), target);
    refresh(id);
    return true;
}

void PlaylistManager::restoreBuiltInPlaylists()
{
    for (const Playlist& builtIn : builtInPlaylists()) {
        bool exists = false;
        for (const Playlist& p : m_playlists) {
            if (p.url == builtIn.url) {
                exists = true;
                break;
            }
        }
        if (!exists)
            addPlaylist(builtIn.name, builtIn.url, QString(), true);
    }
}

QVector<Playlist> PlaylistManager::builtInPlaylists()
{
    const QString base = QStringLiteral("https://iptv-org.github.io/iptv/");
    QVector<Playlist> list;

    const auto make = [&base](const QString& name, const QString& path) {
        Playlist p;
        p.name = name;
        p.url = base + path;
        p.builtIn = true;
        return p;
    };

    list.append(make(QStringLiteral("IPTV-org (All Channels)"), QStringLiteral("index.m3u")));
    list.append(make(QStringLiteral("IPTV-org \u2014 News"), QStringLiteral("categories/news.m3u")));
    list.append(make(QStringLiteral("IPTV-org \u2014 Sports"), QStringLiteral("categories/sports.m3u")));
    list.append(make(QStringLiteral("IPTV-org \u2014 Music"), QStringLiteral("categories/music.m3u")));
    list.append(make(QStringLiteral("IPTV-org \u2014 Movies"), QStringLiteral("categories/movies.m3u")));
    list.append(make(QStringLiteral("IPTV-org \u2014 Kids"), QStringLiteral("categories/kids.m3u")));
    return list;
}
