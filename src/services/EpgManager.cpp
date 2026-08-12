#include "services/EpgManager.h"

#include <QFutureWatcher>
#include <QNetworkReply>
#include <QtConcurrent/QtConcurrent>

#include "core/Log.h"
#include "services/NetworkService.h"

EpgManager* EpgManager::instance()
{
    static EpgManager s;
    return &s;
}

EpgManager::EpgManager(QObject* parent)
    : QObject(parent)
{
}

void EpgManager::loadForPlaylist(const Playlist& playlist)
{
    if (playlist.epgUrl.trimmed().isEmpty())
        return;

    const QUrl url(playlist.epgUrl.trimmed());
    if (!url.isValid() || (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https"))) {
        emit epgFailed(playlist.id, tr("Invalid EPG URL."));
        return;
    }

    QNetworkReply* reply = NetworkService::instance()->nam()->get(
        NetworkService::instance()->makeRequest(url, 20000));
    connect(reply, &QNetworkReply::finished, this, [this, reply, id = playlist.id]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            NetworkService::instance()->reportNetworkFailure();
            emit epgFailed(id, tr("Could not download EPG data."));
            return;
        }
        NetworkService::instance()->reportNetworkSuccess();

        const QByteArray data = reply->readAll();
        auto future = QtConcurrent::run([data]() { return XmltvParser::parse(data); });

        auto* watcher = new QFutureWatcher<QVector<XmltvParser::Program>>(this);
        connect(watcher, &QFutureWatcher<QVector<XmltvParser::Program>>::finished, this,
                [this, watcher, id]() {
                    onParsed(id, watcher->result());
                    watcher->deleteLater();
                });
        watcher->setFuture(future);
    });
}

void EpgManager::onParsed(const QString& playlistId, const QVector<XmltvParser::Program>& programs)
{
    int added = 0;
    for (const XmltvParser::Program& p : programs) {
        QVector<XmltvParser::Program>& bucket = m_programs[p.channelId];
        if (!bucket.isEmpty() && bucket.last().startUtc > p.startUtc)
            continue; // keep buckets roughly sorted; rare out-of-order skipped
        bucket.append(p);
        ++added;
    }
    qInfo() << "EPG:" << added << "programs loaded for playlist" << playlistId;
    emit epgLoaded(playlistId);
}

XmltvParser::Program EpgManager::currentProgram(const QString& tvgId) const
{
    const auto it = m_programs.constFind(tvgId);
    if (it == m_programs.constEnd())
        return {};

    const QDateTime now = QDateTime::currentDateTimeUtc();
    XmltvParser::Program fallback;
    for (const XmltvParser::Program& p : it.value()) {
        if (p.startUtc <= now && p.endUtc >= now)
            return p;
        if (!fallback.isValid() && p.endUtc >= now)
            fallback = p;
    }
    return fallback; // next upcoming programme if none is airing right now
}

XmltvParser::Program EpgManager::nextProgram(const QString& tvgId) const
{
    const auto it = m_programs.constFind(tvgId);
    if (it == m_programs.constEnd())
        return {};

    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const XmltvParser::Program& p : it.value()) {
        if (p.startUtc > now)
            return p;
    }
    return {};
}
