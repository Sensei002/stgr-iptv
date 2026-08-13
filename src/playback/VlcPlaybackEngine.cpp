#include "playback/VlcPlaybackEngine.h"

#include <vlc/vlc.h>

#include <chrono>
#include <QList>

#include "core/Log.h"

VlcPlaybackEngine::VlcPlaybackEngine(QWidget* videoSurface, int networkCachingMs,
                                     int hwMode, QObject* parent)
    : IPlaybackEngine(parent)
    , m_surface(videoSurface)
    , m_hwMode(hwMode)
{
    // libVLC instance arguments (everything is derived from settings).
    const QStringList baseArgs = {
        QStringLiteral("--no-video-title-show"),
        QStringLiteral("--no-osd"),
        QStringLiteral("--http-reconnect"),
        QStringLiteral("--network-caching=%1").arg(qMax(0, networkCachingMs)),
#ifdef Q_OS_WIN
        // Windows: render video through the GDI (wingdi) output instead of
        // libVLC's default D3D11 output. The D3D11 video-output thread can
        // stall while its swapchain/device context is presenting (especially
        // around window resizes, e.g. fullscreen toggles); any control call
        // made from the UI thread (set_pause, set_hwnd, stop) then blocks on
        // that thread's lock and the whole app freezes. GDI output has no
        // such lock and handles window resizes trivially, which makes
        // play/pause instant and fullscreen transitions seamless.
        QStringLiteral("--vout=wingdi"),
#endif
        QStringLiteral("--avcodec-hw=%1").arg(hwMode == 2 ? QStringLiteral("none")
                                                          : (hwMode == 1 ? QStringLiteral("d3d11va")
                                                                         : QStringLiteral("any"))),
        QStringLiteral("--audio-resampler=soxr"),
    };

    QList<QByteArray> args;
    args.reserve(baseArgs.size());
    for (const QString& a : baseArgs)
        args.append(a.toUtf8());

    QVector<const char*> argv;
    argv.reserve(args.size());
    for (const QByteArray& a : args)
        argv.append(a.constData());

    m_vlc = libvlc_new(static_cast<int>(argv.size()), argv.constData());
    if (!m_vlc) {
        qWarning() << "libVLC: failed to create instance";
        return;
    }

    // The worker must be running before createPlayer() so the set_hwnd posted
    // from attachSurface() is picked up.
    m_worker = std::thread([this]() { workerLoop(); });
    createPlayer();
}

VlcPlaybackEngine::~VlcPlaybackEngine()
{
    stopWorker();       // waits for pending libvlc calls to finish
    destroyPlayer();    // safe now: no worker thread touches the player
    if (m_vlc) {
        libvlc_release(m_vlc);
        m_vlc = nullptr;
    }
}

void VlcPlaybackEngine::createPlayer()
{
    m_player = libvlc_media_player_new(m_vlc);
    if (!m_player) {
        qWarning() << "libVLC: failed to create media player";
        return;
    }
    attachEvents();
    if (m_surface)
        attachSurface(m_surface);
}

void VlcPlaybackEngine::destroyPlayer()
{
    if (m_player) {
        libvlc_media_player_stop(m_player);
        if (m_media) {
            libvlc_media_release(m_media);
            m_media = nullptr;
        }
        libvlc_media_player_release(m_player);
        m_player = nullptr;
    }
}

void VlcPlaybackEngine::attachEvents()
{
    libvlc_event_manager_t* em = libvlc_media_player_event_manager(m_player);
    if (!em)
        return;

    libvlc_event_attach(em, libvlc_MediaPlayerOpening, &VlcPlaybackEngine::vlcEventCallback, this);
    libvlc_event_attach(em, libvlc_MediaPlayerBuffering, &VlcPlaybackEngine::vlcEventCallback, this);
    libvlc_event_attach(em, libvlc_MediaPlayerPlaying, &VlcPlaybackEngine::vlcEventCallback, this);
    libvlc_event_attach(em, libvlc_MediaPlayerPaused, &VlcPlaybackEngine::vlcEventCallback, this);
    libvlc_event_attach(em, libvlc_MediaPlayerStopped, &VlcPlaybackEngine::vlcEventCallback, this);
    libvlc_event_attach(em, libvlc_MediaPlayerEndReached, &VlcPlaybackEngine::vlcEventCallback, this);
    libvlc_event_attach(em, libvlc_MediaPlayerEncounteredError, &VlcPlaybackEngine::vlcEventCallback, this);
    libvlc_event_attach(em, libvlc_MediaPlayerTimeChanged, &VlcPlaybackEngine::vlcEventCallback, this);
    libvlc_event_attach(em, libvlc_MediaPlayerLengthChanged, &VlcPlaybackEngine::vlcEventCallback, this);
}

void VlcPlaybackEngine::vlcEventCallback(const libvlc_event_t* event, void* userData)
{
    auto* self = static_cast<VlcPlaybackEngine*>(userData);
    self->handleVlcEvent(event);
}

void VlcPlaybackEngine::handleVlcEvent(const libvlc_event_t* event)
{
    if (!event)
        return;

    switch (event->type) {
    case libvlc_MediaPlayerOpening:
        m_loading = true;
        setState(State::Loading);
        break;
    case libvlc_MediaPlayerBuffering:
        setState(State::Buffering);
        setBufferingPercent(static_cast<int>(event->u.media_player_buffering.new_cache * 100.0f));
        break;
    case libvlc_MediaPlayerPlaying:
        m_loading = false;
        setBufferingPercent(100);
        setState(State::Playing);
        break;
    case libvlc_MediaPlayerPaused:
        setState(State::Paused);
        break;
    case libvlc_MediaPlayerStopped:
        if (m_loading)
            break; // transitional stop while switching channels
        setState(State::Stopped);
        break;
    case libvlc_MediaPlayerEndReached:
        setState(State::Stopped);
        emit ended();
        break;
    case libvlc_MediaPlayerEncounteredError:
        m_loading = false;
        setState(State::Error);
        m_lastError = tr("Stream error");
        emit errorOccurred(m_lastError);
        break;
    case libvlc_MediaPlayerTimeChanged:
        setPosition(static_cast<qint64>(event->u.media_player_time_changed.new_time));
        break;
    case libvlc_MediaPlayerLengthChanged:
        setDuration(static_cast<qint64>(event->u.media_player_length_changed.new_length));
        break;
    default:
        break;
    }
}

void VlcPlaybackEngine::load(const QUrl& url)
{
    if (!isValid()) {
        emit errorOccurred(tr("Playback backend is not available."));
        return;
    }
    if (!url.isValid()) {
        emit errorOccurred(tr("Invalid stream URL."));
        return;
    }

    const QByteArray urlBytes = url.toString(QUrl::FullyEncoded).toUtf8();

    // UI-side state updates happen immediately; the actual libvlc calls run on
    // the worker so a stalled input thread can never freeze the UI.
    m_loading = true;
    setPosition(0);
    setDuration(0);
    setState(State::Loading);

    post([this, urlBytes]() {
        if (!m_player || !m_vlc)
            return;

        libvlc_media_t* media = libvlc_media_new_location(m_vlc, urlBytes.constData());
        if (!media) {
            emit errorOccurred(tr("Could not open the stream."));
            return;
        }

        if (m_media)
            libvlc_media_release(m_media);
        m_media = media;

        libvlc_media_player_set_media(m_player, m_media);
        libvlc_media_player_play(m_player);
    });

    qInfo() << "playback: loading" << Log::redactUrl(url.toString());
}

void VlcPlaybackEngine::play()
{
    post([this]() {
        if (m_player)
            libvlc_media_player_play(m_player);
    });
}

void VlcPlaybackEngine::pause()
{
    post([this]() {
        if (m_player && libvlc_media_player_can_pause(m_player))
            libvlc_media_player_set_pause(m_player, 1);
    });
}

void VlcPlaybackEngine::stop()
{
    m_loading = false;
    post([this]() {
        if (m_player)
            libvlc_media_player_stop(m_player);
    });
    setState(State::Stopped);
}

void VlcPlaybackEngine::seek(qint64 positionMs)
{
    post([this, positionMs]() {
        if (m_player && positionMs >= 0)
            libvlc_media_player_set_time(m_player, positionMs);
    });
}

void VlcPlaybackEngine::setVolume(int percent)
{
    m_volume = qBound(0, percent, 100);
    const int vol = m_volume; // snapshot for the worker
    post([this, vol]() {
        if (m_player)
            libvlc_audio_set_volume(m_player, vol);
    });
}

int VlcPlaybackEngine::volume() const
{
    return m_volume;
}

void VlcPlaybackEngine::setMuted(bool muted)
{
    m_muted = muted;
    post([this, muted]() {
        if (m_player)
            libvlc_audio_set_mute(m_player, muted ? 1 : 0);
    });
}

bool VlcPlaybackEngine::muted() const
{
    return m_muted;
}

void VlcPlaybackEngine::attachSurface(QWidget* widget)
{
    m_surface = widget;
    if (!m_player || !m_surface)
        return;

#ifdef Q_OS_WIN
    // winId() must run on the UI thread (QWidget); set_hwnd itself runs on the
    // worker so it is ordered after any pending play/pause/stop commands.
    void* hwnd = reinterpret_cast<void*>(m_surface->winId());
    post([this, hwnd]() {
        if (m_player)
            libvlc_media_player_set_hwnd(m_player, hwnd);
    });
#endif
}

void VlcPlaybackEngine::setAspectRatio(int mode)
{
    const char* aspect = nullptr;
    switch (mode) {
    case 1: aspect = "16:9"; break;
    case 2: aspect = "4:3"; break;
    default: aspect = nullptr; // auto
    }
    post([this, aspect]() {
        if (m_player)
            libvlc_video_set_aspect_ratio(m_player, aspect);
    });
}

void VlcPlaybackEngine::setHardwareAcceleration(int mode)
{
    // Applied at instance creation; a live change requires a new instance and
    // is handled by PlaybackController (which recreates the engine).
    m_hwMode = mode;
}

QString VlcPlaybackEngine::videoInfo() const
{
    if (!m_player)
        return QString();

    QString info;

    // Resolution / FPS / bitrate come from the parsed media track description.
    if (m_media) {
        libvlc_media_track_t** tracks = nullptr;
        const unsigned n = libvlc_media_tracks_get(m_media, &tracks);
        for (unsigned i = 0; i < n && tracks; ++i) {
            if (tracks[i]->i_type != libvlc_track_video)
                continue;
            libvlc_video_track_t* v = tracks[i]->u.video;
            if (v && v->i_width > 0 && v->i_height > 0)
                info = QStringLiteral("%1x%2").arg(v->i_width).arg(v->i_height);
            else
                info = tr("stream");
            if (v && v->i_frame_rate_num > 0 && v->i_frame_rate_den > 0)
                info += QStringLiteral(" \u00b7 %1 fps").arg(
                    v->i_frame_rate_num / static_cast<double>(v->i_frame_rate_den), 0, 'f', 0);
            if (tracks[i]->i_bitrate > 0)
                info += QStringLiteral(" \u00b7 %1 Mbit/s")
                            .arg(tracks[i]->i_bitrate / 1000000.0, 0, 'f', 1);
            break;
        }
        if (tracks)
            libvlc_media_tracks_release(tracks, n);
    }
    if (info.isEmpty())
        info = tr("stream");

    const int audioTracks = libvlc_audio_get_track_count(m_player);
    const int videoTracks = libvlc_video_get_track_count(m_player);
    if (videoTracks > 0)
        info += QStringLiteral(" \u00b7 %1 video").arg(videoTracks);
    if (audioTracks > 0)
        info += QStringLiteral(" \u00b7 %1 audio").arg(audioTracks);
    return info;
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------
void VlcPlaybackEngine::post(std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.push_back(std::move(fn));
    }
    m_cv.notify_one();
}

void VlcPlaybackEngine::workerLoop()
{
    std::unique_lock<std::mutex> lk(m_mutex);
    while (true) {
        m_cv.wait(lk, [this]() { return !m_queue.empty() || m_stopWorker; });
        while (!m_queue.empty()) {
            std::function<void()> fn = std::move(m_queue.front());
            m_queue.pop_front();
            lk.unlock();
            fn();
            lk.lock();
        }
        if (m_stopWorker)
            break;
    }
    m_workerFinished = true;
}

void VlcPlaybackEngine::stopWorker()
{
    if (!m_worker.joinable())
        return;

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_stopWorker = true;
    }
    m_cv.notify_all();

    // The worker normally finishes instantly (the queue is empty and the
    // current command has completed). If a libvlc call is genuinely wedged,
    // don't hang the UI thread at shutdown either - wait a short bounded time,
    // then detach and let the process exit.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!m_workerFinished.load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

    if (m_workerFinished.load()) {
        m_worker.join();
    } else {
        qWarning() << "playback: worker thread still busy; detaching at shutdown";
        m_worker.detach();
    }
}
