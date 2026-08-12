#include "playback/VlcPlaybackEngine.h"

#include <vlc/vlc.h>

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

    createPlayer();
}

VlcPlaybackEngine::~VlcPlaybackEngine()
{
    destroyPlayer();
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
    libvlc_media_t* media = libvlc_media_new_location(m_vlc, urlBytes.constData());
    if (!media) {
        emit errorOccurred(tr("Could not open the stream."));
        return;
    }

    if (m_media)
        libvlc_media_release(m_media);

    m_media = media;
    m_loading = true;
    setPosition(0);
    setDuration(0);
    setState(State::Loading);

    libvlc_media_player_set_media(m_player, m_media);
    libvlc_media_player_play(m_player);

    qInfo() << "playback: loading" << Log::redactUrl(url.toString());
}

void VlcPlaybackEngine::play()
{
    if (m_player)
        libvlc_media_player_play(m_player);
}

void VlcPlaybackEngine::pause()
{
    if (m_player && libvlc_media_player_can_pause(m_player))
        libvlc_media_player_set_pause(m_player, 1);
}

void VlcPlaybackEngine::stop()
{
    m_loading = false;
    if (m_player) {
        libvlc_media_player_stop(m_player);
        setState(State::Stopped);
    }
}

void VlcPlaybackEngine::seek(qint64 positionMs)
{
    if (m_player && positionMs >= 0)
        libvlc_media_player_set_time(m_player, positionMs);
}

void VlcPlaybackEngine::setVolume(int percent)
{
    m_volume = qBound(0, percent, 100);
    if (m_player)
        libvlc_audio_set_volume(m_player, m_volume);
}

int VlcPlaybackEngine::volume() const
{
    return m_volume;
}

void VlcPlaybackEngine::setMuted(bool muted)
{
    m_muted = muted;
    if (m_player)
        libvlc_audio_set_mute(m_player, m_muted ? 1 : 0);
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
    libvlc_media_player_set_hwnd(m_player, reinterpret_cast<void*>(m_surface->winId()));
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
    if (m_player)
        libvlc_video_set_aspect_ratio(m_player, aspect);
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

    unsigned int width = 0;
    unsigned int height = 0;
    libvlc_video_get_size(m_player, 0, &width, &height);

    const int audioTracks = libvlc_audio_get_track_count(m_player);
    const int videoTracks = libvlc_video_get_track_count(m_player);

    QString info;
    if (width > 0 && height > 0)
        info = QStringLiteral("%1x%2").arg(width).arg(height);
    else
        info = tr("stream");

    if (videoTracks > 0)
        info += QStringLiteral(" \u00b7 %1 video").arg(videoTracks);
    if (audioTracks > 0)
        info += QStringLiteral(" \u00b7 %1 audio").arg(audioTracks);
    return info;
}
