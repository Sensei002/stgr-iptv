#pragma once

#include <QObject>
#include <QTimer>
#include <QVector>

#include "models/Channel.h"
#include "playback/IPlaybackEngine.h"

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

private:
    void connectEngine();
    void handleFailure(const QString& message);
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

    int m_retryCount = 0;
    bool m_autoReconnect = true;
    int m_maxRetries = 3;
    int m_aspectMode = 0;
};
