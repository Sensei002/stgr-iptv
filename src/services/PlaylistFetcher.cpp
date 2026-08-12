#include "services/PlaylistFetcher.h"

#include <QFile>
#include <QFutureWatcher>
#include <QNetworkReply>
#include <QtConcurrent/QtConcurrent>

#include "core/Log.h"
#include "services/NetworkService.h"

// ---------------------------------------------------------------------------
// HttpPlaylistSource
// ---------------------------------------------------------------------------
HttpPlaylistSource::HttpPlaylistSource(QObject* parent)
    : IPlaylistSource(parent)
{
}

void HttpPlaylistSource::fetch(const Playlist& playlist, int timeoutMs)
{
    const QUrl url(playlist.url);
    if (!url.isValid() || url.scheme() != QLatin1String("https") && url.scheme() != QLatin1String("http")) {
        emit finished(playlist.id, false, QByteArray(), tr("Invalid playlist URL: %1")
                          .arg(Log::redactUrl(playlist.url)));
        return;
    }

    QNetworkReply* reply = NetworkService::instance()->nam()->get(
        NetworkService::instance()->makeRequest(url, timeoutMs));
    m_inflight.insert(reply, playlist.id);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QString id = m_inflight.take(reply);
        if (id.isEmpty())
            return;

        if (reply->error() != QNetworkReply::NoError) {
            NetworkService::instance()->reportNetworkFailure();
            emit finished(id, false, QByteArray(),
                          tr("Network error: %1").arg(reply->errorString()));
            return;
        }
        NetworkService::instance()->reportNetworkSuccess();
        emit finished(id, true, reply->readAll(), QString());
    });
}

// ---------------------------------------------------------------------------
// FilePlaylistSource
// ---------------------------------------------------------------------------
FilePlaylistSource::FilePlaylistSource(QObject* parent)
    : IPlaylistSource(parent)
{
}

void FilePlaylistSource::fetch(const Playlist& playlist, int timeoutMs)
{
    Q_UNUSED(timeoutMs);

    const QString path = playlist.url;
    auto future = QtConcurrent::run([path]() -> QPair<bool, QByteArray> {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return { false, QByteArray() };
        return { true, f.readAll() };
    });

    // Route the result back onto this object's thread via a watcher.
    QObject* self = this;
    auto* watcher = new QFutureWatcher<QPair<bool, QByteArray>>(this);
    connect(watcher, &QFutureWatcher<QPair<bool, QByteArray>>::finished, self, [self, watcher, id = playlist.id]() {
        const auto [ok, data] = watcher->result();
        watcher->deleteLater();
        if (!ok)
            emit self->finished(id, false, QByteArray(),
                                tr("Could not read the playlist file."));
        else
            emit self->finished(id, true, data, QString());
    });
    watcher->setFuture(future);
}

// ---------------------------------------------------------------------------
// CompositePlaylistSource
// ---------------------------------------------------------------------------
CompositePlaylistSource::CompositePlaylistSource(QObject* parent)
    : IPlaylistSource(parent)
{
    connect(&m_http, &IPlaylistSource::finished, this, &IPlaylistSource::finished);
    connect(&m_file, &IPlaylistSource::finished, this, &IPlaylistSource::finished);
}

void CompositePlaylistSource::fetch(const Playlist& playlist, int timeoutMs)
{
    if (playlist.isLocal())
        m_file.fetch(playlist, timeoutMs);
    else
        m_http.fetch(playlist, timeoutMs);
}
