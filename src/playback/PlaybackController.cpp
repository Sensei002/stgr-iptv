#include "playback/PlaybackController.h"

#include <QRegularExpression>
#include <QUrl>

#include <algorithm>

#include "core/Log.h"
#include "playback/EngineFactory.h"
#include "settings/Settings.h"
#include "utils/UrlUtils.h"

namespace {
constexpr int kLoadTimeoutMs = 20000;

// Quality tokens stripped when matching channel variants ("CNN HD" vs
// "CNN SD") and used to label the quality picker. Longest alternatives first
// so "1080p60" matches before "1080p".
const QRegularExpression& qualityTokenRe()
{
    static const QRegularExpression re(QStringLiteral(
        "\\b(2160p|1080p\\d*|1080p|1080i|1080|720p|720|576p|576|540p|480p|480|"
        "360p|240p|144p|4k|uhd|fhd|qhd|hd|sd|60fps|50fps|30fps|25fps|24fps)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    return re;
}

QString normalizedName(const Channel& c)
{
    QString n = c.displayName().toLower();
    n.remove(qualityTokenRe());
    n.remove(QRegularExpression(QStringLiteral("[[\\](){}]")));
    return n.simplified();
}
}

PlaybackController::PlaybackController(QObject* parent)
    : QObject(parent)
{
    const Settings* s = Settings::instance();
    m_autoReconnect = s->autoReconnect();
    m_maxRetries = s->maxRetries();
    m_aspectMode = 0;

    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout, this, [this]() {
        if (m_engine && m_channel.isValid()) {
            qInfo() << "playback: reconnect attempt" << m_retryCount;
            m_engine->load(QUrl(m_activeUrl));
            startLoadTimer();
        }
    });

    m_loadTimer.setSingleShot(true);
    connect(&m_loadTimer, &QTimer::timeout, this, [this]() {
        const IPlaybackEngine::State st = state();
        if (st == IPlaybackEngine::State::Loading || st == IPlaybackEngine::State::Buffering)
            handleFailure(tr("The stream is not responding."));
    });

    // Mid-playback rebuffer watchdog: if a live stream keeps buffering without
    // recovering (the classic stutter), switch to the next known mirror
    // instead of waiting for the load timer.
    m_stallTimer.setSingleShot(true);
    connect(&m_stallTimer, &QTimer::timeout, this, [this]() {
        const IPlaybackEngine::State st = state();
        if (st == IPlaybackEngine::State::Buffering || st == IPlaybackEngine::State::Loading)
            handleFailure(tr("The stream is stalling."));
    });
    m_lastSwitch.start();

    m_engine = EngineFactory::create(nullptr, s->bufferSizeMs(), s->hardwareAcceleration(), this);
    if (m_engine) {
        connectEngine();
        m_engine->setVolume(s->defaultVolume());
        m_engine->setMuted(false);
    } else {
        qWarning() << "playback: no backend available (libVLC not compiled in)";
    }
}

PlaybackController::~PlaybackController()
{
    stopLoadTimer();
    clearReconnect();
    m_stallTimer.stop();
}

void PlaybackController::connectEngine()
{
    connect(m_engine, &IPlaybackEngine::stateChanged, this, [this](IPlaybackEngine::State st) {
        emit stateChanged(st);
        if (st == IPlaybackEngine::State::Playing) {
            m_stallTimer.stop();
            m_wasPlaying = true;
            emit videoInfoChanged(m_engine->videoInfo());
        } else if (st == IPlaybackEngine::State::Buffering && m_channel.isValid() && m_wasPlaying) {
            // Mid-playback rebuffer: a stall that outlives the timeout means
            // the mirror is dying -> fail over. The initial load is left to
            // the load timer so slow-but-working streams are not yanked.
            m_stallTimer.start(8000);
        }
    });
    connect(m_engine, &IPlaybackEngine::bufferingChanged,
            this, &PlaybackController::bufferingChanged);
    connect(m_engine, &IPlaybackEngine::errorOccurred,
            this, [this](const QString& msg) { handleFailure(msg); });
    connect(m_engine, &IPlaybackEngine::ended,
            this, [this]() { handleFailure(tr("The stream ended.")); });
}

void PlaybackController::setVideoSurface(QWidget* widget)
{
    if (m_engine)
        m_engine->attachSurface(widget);
}

void PlaybackController::playChannel(const Channel& channel)
{
    if (!channel.isValid())
        return;

    m_channel = channel;
    m_activeUrl = channel.url;
    m_retryCount = 0;
    m_wasPlaying = false;
    clearReconnect();
    updatePoolIndex();
    buildFailover();
    m_lastSwitch.restart(); // fresh pacing clock for this channel

    if (!m_engine) {
        stopLoadTimer();
        emit stateChanged(IPlaybackEngine::State::Error);
        emit errorOccurred(tr("Playback is unavailable \u2014 the libVLC runtime could not be loaded."), true);
        return;
    }

    qInfo() << "playback: switching to channel" << channel.displayName();
    m_engine->stop();
    m_engine->setAspectRatio(m_aspectMode);
    m_engine->load(QUrl(m_activeUrl));
    startLoadTimer();
    emit channelChanged(channel);
}

void PlaybackController::play()
{
    if (m_engine && m_channel.isValid())
        m_engine->play();
}

void PlaybackController::pause()
{
    if (m_engine && m_channel.isValid())
        m_engine->pause();
}

void PlaybackController::togglePlayPause()
{
    if (state() == IPlaybackEngine::State::Playing)
        pause();
    else if (m_channel.isValid())
        play();
}

void PlaybackController::stop()
{
    stopLoadTimer();
    clearReconnect();
    m_stallTimer.stop();
    if (m_engine)
        m_engine->stop();
}

void PlaybackController::retry()
{
    if (m_channel.isValid()) {
        m_retryCount = 0;
        clearReconnect();
        m_failoverIndex = 0;
        playChannel(m_channel);
    }
}

void PlaybackController::playStream(const IptvStream& stream)
{
    if (!m_channel.isValid() || stream.url.isEmpty())
        return;

    qInfo() << "playback: selecting stream" << (stream.title.isEmpty() ? stream.url : stream.title);
    m_activeUrl = stream.url;
    m_retryCount = 0;
    clearReconnect();
    m_stallTimer.stop();
    buildFailover(); // reseed mirrors excluding the URL just selected
    m_lastSwitch.restart();

    if (!m_engine) {
        emit stateChanged(IPlaybackEngine::State::Error);
        emit errorOccurred(tr("Playback is unavailable \u2014 the libVLC runtime could not be loaded."), true);
        return;
    }

    m_engine->stop();
    m_engine->setAspectRatio(m_aspectMode);
    m_engine->load(QUrl(stream.url), stream.referrer, stream.userAgent);
    startLoadTimer();
    emit channelChanged(m_channel);
}

void PlaybackController::handleFailure(const QString& message)
{
    if (!m_channel.isValid())
        return;

    // Prefer a working backup mirror before retrying the same URL.
    if (m_autoReconnect && tryNextMirror())
        return;

    // All mirrors exhausted: retry the channel's own playlist URL.
    m_activeUrl = m_channel.url;
    if (m_autoReconnect && m_retryCount < m_maxRetries) {
        scheduleReconnect();
        return;
    }

    stopLoadTimer();
    emit errorOccurred(message, true);
}

bool PlaybackController::tryNextMirror()
{
    if (!m_engine)
        return false;

    // Pace mirror switches so a burst of error events cannot thrash through
    // the whole list in one frame.
    if (m_lastSwitch.isValid() && m_lastSwitch.elapsed() < 800)
        return false;

    while (m_failoverIndex < m_failover.size()) {
        const IptvStream s = m_failover.at(m_failoverIndex++);
        if (s.url.isEmpty() || s.url == m_activeUrl)
            continue;

        qInfo() << "playback: switching to backup mirror"
                << (s.title.isEmpty() ? Log::redactUrl(s.url) : s.title);
        m_activeUrl = s.url;
        m_lastSwitch.restart();
        m_stallTimer.stop();
        m_engine->stop();
        m_engine->load(QUrl(s.url), s.referrer, s.userAgent);
        startLoadTimer();
        emit streamSwitched(s.title.isEmpty() ? tr("backup stream") : s.title);
        return true;
    }
    return false;
}

void PlaybackController::buildFailover()
{
    m_failover.clear();
    m_failoverIndex = 0;
    if (!m_channel.isValid())
        return;

    const QVector<IptvStream> mirrors = IptvOrgApi::instance()->streamsFor(m_channel);
    for (const IptvStream& s : mirrors) {
        if (s.url.isEmpty() || s.url == m_activeUrl)
            continue;
        m_failover.append(s);
    }
}

QVector<IptvStream> PlaybackController::apiStreams() const
{
    if (!m_channel.isValid())
        return {};
    return IptvOrgApi::instance()->streamsFor(m_channel);
}

void PlaybackController::scheduleReconnect()
{
    ++m_retryCount;
    const int delayMs = qMin(5000, 1500 * m_retryCount);
    emit reconnecting(m_retryCount, m_maxRetries);
    qInfo() << "playback: stream failed, reconnecting in" << delayMs << "ms (attempt"
            << m_retryCount << "/" << m_maxRetries << ")";
    m_retryTimer.start(delayMs);
}

void PlaybackController::clearReconnect()
{
    m_retryTimer.stop();
    m_retryCount = 0;
}

void PlaybackController::startLoadTimer()
{
    m_loadTimer.stop();
    m_loadTimer.start(kLoadTimeoutMs);
}

void PlaybackController::stopLoadTimer()
{
    m_loadTimer.stop();
}

void PlaybackController::setVolume(int percent)
{
    if (m_engine)
        m_engine->setVolume(percent);
    if (percent > 0 && m_engine)
        m_engine->setMuted(false);
}

int PlaybackController::volume() const
{
    return m_engine ? m_engine->volume() : Settings::instance()->defaultVolume();
}

void PlaybackController::toggleMute()
{
    if (m_engine) {
        const bool muted = !m_engine->muted();
        m_engine->setMuted(muted);
    }
}

bool PlaybackController::muted() const
{
    return m_engine && m_engine->muted();
}

void PlaybackController::cycleAspectRatio()
{
    m_aspectMode = (m_aspectMode + 1) % 3;
    if (m_engine)
        m_engine->setAspectRatio(m_aspectMode);
}

QString PlaybackController::videoInfo() const
{
    return m_engine ? m_engine->videoInfo() : QString();
}

void PlaybackController::setChannelPool(const QVector<Channel>& pool)
{
    m_pool = pool;
    updatePoolIndex();
}

void PlaybackController::updatePoolIndex()
{
    m_poolIndex = -1;
    if (!m_channel.isValid())
        return;
    const QString key = m_channel.stableKey();
    for (int i = 0; i < m_pool.size(); ++i) {
        if (m_pool.at(i).stableKey() == key) {
            m_poolIndex = i;
            break;
        }
    }
}

void PlaybackController::jumpToLive()
{
    if (m_channel.isValid()) {
        m_failoverIndex = 0;
        playChannel(m_channel);
    }
}

QVector<Channel> PlaybackController::qualityVariants() const
{
    if (!m_channel.isValid())
        return {};

    const QString base = normalizedName(m_channel);
    QVector<Channel> variants;
    variants.reserve(m_pool.size());
    for (const Channel& c : m_pool) {
        if (c.playlistId == m_channel.playlistId && normalizedName(c) == base)
            variants.append(c);
    }

    // Put the currently playing channel first.
    const QString key = m_channel.stableKey();
    std::stable_partition(variants.begin(), variants.end(),
                          [&key](const Channel& c) { return c.stableKey() != key; });
    return variants;
}

QString PlaybackController::qualityLabel(const Channel& channel)
{
    const QRegularExpressionMatch m = qualityTokenRe().match(channel.displayName());
    if (m.hasMatch())
        return m.captured(1).toUpper();
    return QStringLiteral("Auto");
}

void PlaybackController::playPrevious()
{
    if (m_pool.isEmpty() || m_poolIndex < 0)
        return;
    const int idx = (m_poolIndex - 1 + m_pool.size()) % m_pool.size();
    playChannel(m_pool.at(idx));
}

void PlaybackController::playNext()
{
    if (m_pool.isEmpty() || m_poolIndex < 0)
        return;
    const int idx = (m_poolIndex + 1) % m_pool.size();
    playChannel(m_pool.at(idx));
}
