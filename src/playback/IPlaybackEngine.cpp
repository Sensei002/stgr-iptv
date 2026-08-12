#include "playback/IPlaybackEngine.h"

IPlaybackEngine::IPlaybackEngine(QObject* parent)
    : QObject(parent)
{
}

void IPlaybackEngine::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(m_state);
}

void IPlaybackEngine::setPosition(qint64 ms)
{
    if (m_positionMs == ms)
        return;
    m_positionMs = ms;
    emit positionChanged(m_positionMs);
}

void IPlaybackEngine::setDuration(qint64 ms)
{
    if (m_durationMs == ms)
        return;
    m_durationMs = ms;
    emit durationChanged(m_durationMs);
}

void IPlaybackEngine::setBufferingPercent(int percent)
{
    percent = qBound(0, percent, 100);
    if (m_bufferingPercent == percent)
        return;
    m_bufferingPercent = percent;
    emit bufferingChanged(m_bufferingPercent);
}
