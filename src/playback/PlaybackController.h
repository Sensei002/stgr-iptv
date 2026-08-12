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

    // Re-starts the current channel. Used after the video surface's native
    // window handle is recreated (e.g. fullscreen toggling) so the video
    // output initializes on the new window. No-op when idle.
    void reload();

    // TV-style navigation through the currently set channel pool.
    void playPrevious();
    void playNext();
    void setChannelPool(const QVector<Channel>& pool);

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
