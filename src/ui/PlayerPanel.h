#pragma once

#include <QFrame>
#include <QStackedWidget>
#include <QTimer>

#include "models/Channel.h"
#include "playback/IPlaybackEngine.h"

class QLabel;
class QPushButton;
class QSlider;
class QStackedLayout;
class QToolButton;
class QWidget;
class PlaybackController;

// ---------------------------------------------------------------------------
// PlayerPanel - the central video area with its chrome:
//
//   * video surface (attached to the playback engine),
//   * loading overlay (spinner + channel name),
//   * error overlay with Retry / Previous Channel / Back to Channel List,
//   * reconnect banner,
//   * control bar: play/pause, stop, channel prev/next, volume, mute, aspect,
//     favorite, fullscreen,
//   * channel info header (logo, name, LIVE badge, EPG now/next).
//
// Fullscreen toggling reparents the video surface into a frameless top-level
// frame. Reparenting recreates the native HWND, so the surface is re-attached
// to the engine and the channel is restarted to render on the new window.
// ---------------------------------------------------------------------------
class PlayerPanel : public QFrame
{
    Q_OBJECT

public:
    explicit PlayerPanel(PlaybackController* controller, QWidget* parent = nullptr);

    void setChannelPool(const QVector<Channel>& pool);
    void updateEpgFor(const Channel& channel);

    void togglePlayPause();
    void toggleFullscreen();
    void toggleMute();
    void volumeUp();
    void volumeDown();
    void cycleAspectRatio();
    void playPrevious();
    void playNext();
    void retry();

    bool isFullscreenActive() const { return m_fullscreenActive; }
    QWidget* videoSurface() const { return m_videoSurface; }

signals:
    void backToChannelsRequested();
    void favoriteToggled(const Channel& channel, bool nowFavorite);
    void miniPlayerRequested(const Channel& channel);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildUi();
    void buildOverlays();
    void buildControls();

    void onStateChanged(IPlaybackEngine::State state);
    void onError(const QString& message, bool fatal);
    void onReconnecting(int attempt, int maxAttempts);
    void onChannelChanged(const Channel& channel);
    void refreshEpgLabel();

    void enterFullscreen();
    void exitFullscreen();

    PlaybackController* m_controller = nullptr;

    QWidget* m_videoSurface = nullptr;
    QWidget* m_videoContainer = nullptr;
    QStackedLayout* m_overlayLayout = nullptr;
    QWidget* m_loadingPage = nullptr;
    QWidget* m_errorPage = nullptr;
    QWidget* m_reconnectPage = nullptr;
    QLabel* m_loadingLogo = nullptr;
    QLabel* m_loadingTitle = nullptr;
    QLabel* m_errorLogo = nullptr;
    QLabel* m_errorTitle = nullptr;
    QLabel* m_errorSubtitle = nullptr;
    QPushButton* m_retryButton = nullptr;
    QPushButton* m_prevButton = nullptr;
    QPushButton* m_backButton = nullptr;
    QLabel* m_reconnectLabel = nullptr;

    QLabel* m_channelLogo = nullptr;
    QLabel* m_channelName = nullptr;
    QLabel* m_channelMeta = nullptr;
    QLabel* m_liveBadge = nullptr;
    QLabel* m_videoInfo = nullptr;
    QLabel* m_epgLabel = nullptr;

    QToolButton* m_playPause = nullptr;
    QToolButton* m_muteButton = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QLabel* m_volumeLabel = nullptr;
    QToolButton* m_aspectButton = nullptr;
    QToolButton* m_fullscreenButton = nullptr;
    QToolButton* m_favoriteButton = nullptr;
    QToolButton* m_miniButton = nullptr;

    QWidget* m_fullscreenFrame = nullptr;
    QLabel* m_fullscreenLoading = nullptr;
    bool m_fullscreenActive = false;
    QTimer m_infoRefreshTimer;

    Channel m_current;
};
