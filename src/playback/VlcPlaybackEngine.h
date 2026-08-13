#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include "playback/IPlaybackEngine.h"

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;
struct libvlc_event_t;

// ---------------------------------------------------------------------------
// VlcPlaybackEngine - libVLC playback backend.
//
// * ALL libvlc control calls (play, pause, stop, seek, load, set_hwnd) are
//   executed on a dedicated worker thread through a FIFO command queue. libVLC
//   can block for a long time - or stall entirely - inside its own input /
//   decoder / vout locks (especially on live streams), and running those calls
//   on the UI thread would freeze the whole app. The worker makes the UI
//   immune: even if a libvlc call wedges, the app stays responsive.
// * libvlc event callbacks arrive on VLC threads and are forwarded to Qt via
//   signal emission (queued automatically); the UI thread never touches the
//   libvlc API directly.
// * Hardware acceleration, network caching and the video output module are
//   configured through libvlc instance arguments at construction time.
// ---------------------------------------------------------------------------
class VlcPlaybackEngine : public IPlaybackEngine
{
    Q_OBJECT

public:
    explicit VlcPlaybackEngine(QWidget* videoSurface, int networkCachingMs,
                               int hwMode, QObject* parent = nullptr);
    ~VlcPlaybackEngine() override;

    bool isValid() const { return m_vlc != nullptr && m_player != nullptr; }

    void load(const QUrl& url) override;
    void play() override;
    void pause() override;
    void stop() override;
    void seek(qint64 positionMs) override;

    void setVolume(int percent) override;
    int volume() const override;
    void setMuted(bool muted) override;
    bool muted() const override;

    void attachSurface(QWidget* widget) override;
    void setAspectRatio(int mode) override;
    void setHardwareAcceleration(int mode) override;

    QString videoInfo() const override;

private:
    static void vlcEventCallback(const libvlc_event_t* event, void* userData);
    void handleVlcEvent(const libvlc_event_t* event);

    void createPlayer();
    void destroyPlayer();
    void attachEvents();

    // Worker thread plumbing (all libvlc control calls run here).
    void post(std::function<void()> fn);
    void workerLoop();
    void stopWorker();

    libvlc_instance_t* m_vlc = nullptr;
    libvlc_media_player_t* m_player = nullptr;
    libvlc_media_t* m_media = nullptr;

    QWidget* m_surface = nullptr;
    QString m_lastError;
    int m_hwMode = 0;
    std::atomic<bool> m_loading{ false };
    int m_volume = 100;
    bool m_muted = false;

    std::thread m_worker;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<std::function<void()>> m_queue;
    bool m_stopWorker = false;
    std::atomic<bool> m_workerFinished{ false };
};
