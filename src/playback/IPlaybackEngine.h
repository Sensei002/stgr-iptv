#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QWidget>

// ---------------------------------------------------------------------------
// IPlaybackEngine - the playback backend abstraction.
//
// The UI (and PlaybackController) only ever talk to this interface, so the
// underlying engine (libVLC today) can be replaced later without touching
// any UI code. Implementations:
//   * must be safe to call from the UI thread,
//   * may emit signals from their own threads (Qt queues them automatically).
//
// State/position/duration/buffering are tracked in the base class; derived
// engines feed them through the protected setters.
// ---------------------------------------------------------------------------
class IPlaybackEngine : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Loading,
        Buffering,
        Playing,
        Paused,
        Stopped,
        Error
    };
    Q_ENUM(State)

    explicit IPlaybackEngine(QObject* parent = nullptr);

    // -- media control -------------------------------------------------------
    virtual void load(const QUrl& url) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(qint64 positionMs) = 0;

    // -- audio ---------------------------------------------------------------
    virtual void setVolume(int percent) = 0;   // 0..100
    virtual int volume() const = 0;
    virtual void setMuted(bool muted) = 0;
    virtual bool muted() const = 0;

    // -- video ----------------------------------------------------------------
    // Attaches the widget the video is rendered into (re-attachable after the
    // widget has been reparented, e.g. for fullscreen).
    virtual void attachSurface(QWidget* widget) = 0;
    // 0 = auto, 1 = 16:9, 2 = 4:3
    virtual void setAspectRatio(int mode) = 0;
    // 0 = automatic, 1 = enabled, 2 = disabled
    virtual void setHardwareAcceleration(int mode) = 0;

    // Human-readable stream summary (resolution, track counts, ...).
    virtual QString videoInfo() const = 0;

    // -- observable state -----------------------------------------------------
    State state() const { return m_state; }
    qint64 positionMs() const { return m_positionMs; }
    qint64 durationMs() const { return m_durationMs; }
    int bufferingPercent() const { return m_bufferingPercent; }

signals:
    void stateChanged(IPlaybackEngine::State state);
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void bufferingChanged(int percent);
    void errorOccurred(const QString& message);
    void ended();

protected:
    void setState(State state);
    void setPosition(qint64 ms);
    void setDuration(qint64 ms);
    void setBufferingPercent(int percent);

private:
    State m_state = State::Idle;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    int m_bufferingPercent = 0;
};
