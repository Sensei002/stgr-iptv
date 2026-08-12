#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

// ---------------------------------------------------------------------------
// Playlist - a user-managed M3U/M3U8 source (remote URL or local file).
// ---------------------------------------------------------------------------
struct Playlist {
    QString id;               // stable identifier (UUID)
    QString name;             // display name
    QString url;              // remote http(s) URL or absolute local file path
    QString epgUrl;           // optional XMLTV URL for EPG data
    bool    enabled = true;   // excluded from the channel pool when false
    bool    builtIn = false;  // provided by the app (e.g. IPTV-org)
    QDateTime lastUpdated;
    int     channelCount = 0;
    QString errorMessage;     // last refresh failure, cleared on success

    bool isLocal() const
    {
        const QString u = url.trimmed().toLower();
        return !(u.startsWith(QLatin1String("http://")) || u.startsWith(QLatin1String("https://")));
    }
};

// JSON <-> Playlist (used by PlaylistManager for persistence).
QJsonObject playlistToJson(const Playlist& p);
Playlist playlistFromJson(const QJsonObject& obj);
