#pragma once

#include <QWidget>

#include "models/Channel.h"
#include "playback/IPlaybackEngine.h"

class QCheckBox;
class QLabel;
class QToolButton;

// ---------------------------------------------------------------------------
// MiniPlayerWindow - compact always-on-top window with its own playback
// engine instance (the main player keeps running independently).
// ---------------------------------------------------------------------------
class MiniPlayerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MiniPlayerWindow(QWidget* parent = nullptr);
    ~MiniPlayerWindow() override;

    bool playChannel(const Channel& channel);
    void togglePlayPause();

private:
    IPlaybackEngine* m_engine = nullptr;
    QWidget* m_videoSurface = nullptr;
    QLabel* m_channelLabel = nullptr;
    QToolButton* m_playPause = nullptr;
    QCheckBox* m_alwaysOnTop = nullptr;
    Channel m_channel;
};
