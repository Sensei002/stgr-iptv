#pragma once

#include <QByteArray>
#include <QHash>
#include <QNetworkReply>
#include <QObject>
#include <QString>

#include "models/Playlist.h"

// ---------------------------------------------------------------------------
// Playlist fetching abstraction. PlaylistManager consumes this interface so
// unit tests can inject a fake source and no network is required.
//
// Implementations MUST emit finished() exactly once per fetch() call.
// ---------------------------------------------------------------------------
class IPlaylistSource : public QObject
{
    Q_OBJECT

public:
    explicit IPlaylistSource(QObject* parent = nullptr) : QObject(parent) {}

    virtual void fetch(const Playlist& playlist, int timeoutMs) = 0;

signals:
    void finished(const QString& playlistId, bool ok, const QByteArray& data,
                  const QString& errorMessage);
};

// HTTP(S) playlists via QNetworkAccessManager.
class HttpPlaylistSource : public IPlaylistSource
{
    Q_OBJECT

public:
    explicit HttpPlaylistSource(QObject* parent = nullptr);

    void fetch(const Playlist& playlist, int timeoutMs) override;

private:
    QHash<QNetworkReply*, QString> m_inflight; // reply -> playlistId
};

// Local .m3u files (read on a worker thread so the UI never blocks).
class FilePlaylistSource : public IPlaylistSource
{
    Q_OBJECT

public:
    explicit FilePlaylistSource(QObject* parent = nullptr);

    void fetch(const Playlist& playlist, int timeoutMs) override;
};

// Routes each playlist to the HTTP or the local-file source automatically.
class CompositePlaylistSource : public IPlaylistSource
{
    Q_OBJECT

public:
    explicit CompositePlaylistSource(QObject* parent = nullptr);

    void fetch(const Playlist& playlist, int timeoutMs) override;

private:
    HttpPlaylistSource m_http;
    FilePlaylistSource m_file;
};
