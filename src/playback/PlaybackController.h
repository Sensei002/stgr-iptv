#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVector>

#include "models/Channel.h"
#include "playback/IPlaybackEngine.h"
#include "services/IptvOrgApi.h"

class QWidget;

// ---------------------------------------------------------------------------
// PlaybackController - the single playback facade used by the UI.
//
// Responsibilities:
//  * owns the IPlaybackEngine instance (created through EngineFactory),
//  * fast channel switching with a clean stop -> load -> play sequence,
//  * automatic reconnect with backoff when a stream fails or stalls,
//  * friendly, never-crashing error handling (Retry / Previous / Back).
//
// The controller relays engine signals so the UI never touches the engine
// directly.
// ---------------------------------------------------------------------------
class PlaybackController : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackController(QObject* parent = nullptr);
    ~PlaybackController() override;

    bool backendAvailable() const { return m_engine != nullptr; }
    void setVideoSurface(QWidget* widget);

    // -- channel control ------------------------------------------------------
    void playChannel(const Channel& channel);
    void play();
    void pause();
    void togglePlayPause();
    void stop();
    void retry();

    // TV-style navigation through the currently set channel pool.
    void playPrevious();
    void playNext();
    void setChannelPool(const QVector<Channel>& pool);

    // Reloads the current stream, snapping playback back to the live edge
    // (the IPTV equivalent of YouTube's "jump to live").
    void jumpToLive();

    // Alternate qualities of the current channel found in the pool (same
    // playlist, same name minus the quality suffix, e.g. "CNN HD" / "CNN SD").
    // The current channel is always included first.
    QVector<Channel> qualityVariants() const;

    // All known mirrors of the current channel from the iptv-org API
    // (best-effort; empty when the API has not loaded or has no entry).
    QVector<IptvStream> apiStreams() const;

    // URL currently being played (may be a backup mirror, not the channel's
    // playlist URL) - used by the quality menu to mark the active entry.
    QString activeUrl() const { return m_activeUrl; }

    // Plays a specific mirror (from the API or the quality menu) with its
    // required Referer/User-Agent headers, keeping the channel identity.
    void playStream(const IptvStream& stream);

    // Human-readable quality label extracted from a channel name
    // ("1080p60", "720", "HD", "4K", ...) or "Auto" when none is present.
    static QString qualityLabel(const Channel& channel);

    // -- audio / video ----------------------------------------------------------
    void setVolume(int percent);
    void toggleMute();
    int volume() const;
    bool muted() const;
    void cycleAspectRatio();
    int aspectMode() const { return m_aspectMode; }
    QString videoInfo() const;

    // -- state -------------------------------------------------------------------
    IPlaybackEngine::State state() const
    {
        return m_engine ? m_engine->state() : IPlaybackEngine::State::Idle;
    }
    bool isPlaying() const { return state() == IPlaybackEngine::State::Playing; }
    bool hasChannel() const { return m_channel.isValid(); }
    Channel currentChannel() const { return m_channel; }

signals:
    void channelChanged(const Channel& channel);
    void stateChanged(IPlaybackEngine::State state);
    void errorOccurred(const QString& message, bool fatal);
    void reconnecting(int attempt, int maxAttempts);
    void bufferingChanged(int percent);
    void videoInfoChanged(const QString& info);
    // Emitted when playback transparently switches to a backup mirror URL.
    void streamSwitched(const QString& title);

private:
    void connectEngine();
    void handleFailure(const QString& message);
    bool tryNextMirror();
    void buildFailover();
    void scheduleReconnect();
    void clearReconnect();
    void startLoadTimer();
    void stopLoadTimer();
    void updatePoolIndex();

    Channel m_channel;
    QVector<Channel> m_pool;
    int m_poolIndex = -1;

    IPlaybackEngine* m_engine = nullptr;
    QTimer m_retryTimer;
    QTimer m_loadTimer;
    QTimer m_stallTimer;   // mid-playback rebuffer watchdog -> triggers failover
    QElapsedTimer m_lastSwitch;
    bool m_wasPlaying = false; // stall watchdog only arms after first Playing

    // Backup mirrors for the current channel (from the iptv-org API),
    // tried in order when the current URL stalls or fails.
    QVector<IptvStream> m_failover;
    int m_failoverIndex = 0;
    QString m_activeUrl;   // URL actually loaded (primary or a mirror)

    int m_retryCount = 0;
    bool m_autoReconnect = true;
    int m_maxRetries = 3;
    int m_aspectMode = 0;
};
