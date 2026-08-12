#pragma once

#include "playback/IPlaybackEngine.h"

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;
struct libvlc_event_t;

// ---------------------------------------------------------------------------
// VlcPlaybackEngine - libVLC playback backend.
//
// * All libvlc calls happen on the UI thread (they are non-blocking);
//   libvlc event callbacks arrive on VLC threads and are forwarded to Qt via
//   signal emission (queued automatically).
// * Hardware acceleration, network caching and reconnection behavior are
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

    libvlc_instance_t* m_vlc = nullptr;
    libvlc_media_player_t* m_player = nullptr;
    libvlc_media_t* m_media = nullptr;

    QWidget* m_surface = nullptr;
    QString m_lastError;
    int m_hwMode = 0;
    bool m_loading = false;
    int m_volume = 100;
    bool m_muted = false;
};
