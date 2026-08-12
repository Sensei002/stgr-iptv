#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QVector>

#include "models/Channel.h"
#include "models/Playlist.h"

class IPlaylistSource;
class M3uParser;

// ---------------------------------------------------------------------------
// PlaylistManager - owns the playlist registry and the parsed channel pool.
//
// The refresh pipeline is fully asynchronous: the source is fetched off the
// UI thread, parsing runs on a worker thread, results are cached to disk and
// the UI is notified through signals. Nothing here ever blocks the UI thread.
//
// Favorites/history identity is stable because channels keep the playlist id
// (not the array index) in their stableKey().
// ---------------------------------------------------------------------------
class PlaylistManager : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistManager(IPlaylistSource* source, QObject* parent = nullptr);

    void load();      // registry + cached channel data from disk
    void save();      // persist the registry

    QVector<Playlist> playlists() const { return m_playlists; }
    const Playlist* findPlaylist(const QString& id) const;
    QVector<Channel> channelsFor(const QString& playlistId) const;
    QVector<Channel> allChannels() const;   // merged pool of enabled playlists

    // Registry mutations (all persisted immediately).
    QString addPlaylist(const QString& name, const QString& url,
                        const QString& epgUrl = QString(), bool builtIn = false);
    bool removePlaylist(const QString& id);
    bool renamePlaylist(const QString& id, const QString& newName);
    bool setPlaylistUrl(const QString& id, const QString& url);
    bool setPlaylistEnabled(const QString& id, bool enabled);
    bool setPlaylistEpgUrl(const QString& id, const QString& epgUrl);

    bool refresh(const QString& id);
    void refreshAll();
    bool isRefreshing(const QString& id) const { return m_refreshing.contains(id); }

    bool importLocalFile(const QString& filePath); // copies into imports/, registers, refreshes
    void restoreBuiltInPlaylists();                // re-adds missing IPTV-org entries

    void setFetchTimeoutMs(int ms) { m_timeoutMs = ms; }

    // The publicly maintained iptv-org/iptv sources bundled as optional providers.
    static QVector<Playlist> builtInPlaylists();

signals:
    void playlistsChanged();
    void channelsChanged(const QString& playlistId);
    void allChannelsChanged();                    // merged pool changed
    void refreshStarted(const QString& playlistId);
    void refreshFinished(const QString& playlistId, bool ok,
                         const QString& errorMessage, int channelCount);

private:
    void onSourceFinished(const QString& playlistId, bool ok, const QByteArray& data,
                          const QString& errorMessage);
    void onParsed(const QString& playlistId, const M3uParser::Result& result);
    void onCacheLoaded(const QString& playlistId, const QVector<Channel>& channels);

    int indexOf(const QString& id) const;
    void persistPlaylists();
    void loadCachesAsync();
    void writeCacheFile(const QString& playlistId, const QVector<Channel>& channels);

    QVector<Playlist> m_playlists;
    QHash<QString, QVector<Channel>> m_channels;
    QSet<QString> m_refreshing;
    IPlaylistSource* m_source = nullptr;
    int m_timeoutMs = 20000;
};
