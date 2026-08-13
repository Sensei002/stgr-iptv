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
            m_engine->load(QUrl(m_channel.url));
            startLoadTimer();
        }
    });

    m_loadTimer.setSingleShot(true);
    connect(&m_loadTimer, &QTimer::timeout, this, [this]() {
        const IPlaybackEngine::State st = state();
        if (st == IPlaybackEngine::State::Loading || st == IPlaybackEngine::State::Buffering)
            handleFailure(tr("The stream is not responding."));
    });

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
}

void PlaybackController::connectEngine()
{
    connect(m_engine, &IPlaybackEngine::stateChanged, this, [this](IPlaybackEngine::State st) {
        emit stateChanged(st);
        if (st == IPlaybackEngine::State::Playing)
            emit videoInfoChanged(m_engine->videoInfo());
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
    m_retryCount = 0;
    clearReconnect();
    updatePoolIndex();

    if (!m_engine) {
        stopLoadTimer();
        emit stateChanged(IPlaybackEngine::State::Error);
        emit errorOccurred(tr("Playback is unavailable \u2014 the libVLC runtime could not be loaded."), true);
        return;
    }

    qInfo() << "playback: switching to channel" << channel.displayName();
    m_engine->stop();
    m_engine->setAspectRatio(m_aspectMode);
    m_engine->load(QUrl(channel.url));
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
    if (m_engine)
        m_engine->stop();
}

void PlaybackController::retry()
{
    if (m_channel.isValid()) {
        m_retryCount = 0;
        clearReconnect();
        playChannel(m_channel);
    }
}

void PlaybackController::handleFailure(const QString& message)
{
    if (!m_channel.isValid())
        return;

    if (m_autoReconnect && m_retryCount < m_maxRetries) {
        scheduleReconnect();
        return;
    }

    stopLoadTimer();
    emit errorOccurred(message, true);
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
    if (m_channel.isValid())
        playChannel(m_channel);
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
