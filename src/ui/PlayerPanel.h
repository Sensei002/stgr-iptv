#pragma once

#include <QFrame>
#include <QList>
#include <QStackedWidget>
#include <QTimer>

#include "models/Channel.h"
#include "playback/IPlaybackEngine.h"
#include "services/IptvOrgApi.h"

class QLabel;
class QPushButton;
class QSlider;
class QStackedLayout;
class QToolButton;
class QVBoxLayout;
class QWidget;
class PlaybackController;

// ---------------------------------------------------------------------------
// PlayerPanel - the central video area with its chrome:
//
//   * video surface (attached to the playback engine),
//   * loading overlay (spinner + channel name),
//   * error overlay with Retry / Previous Channel / Back to Channel List,
//   * reconnect banner,
//   * control bar: play/pause, LIVE, quality, stop, channel prev/next, volume,
//     mute, aspect, favorite, fullscreen,
//   * channel info header (logo, name, LIVE badge, EPG now/next).
//
// Fullscreen is handled by the MainWindow (the app window itself goes full
// screen, VLC-style); the panel only adapts its chrome via setFullscreenMode()
// - hiding the channel header/EPG, overlaying the control bar on the video and
// auto-hiding it until the mouse moves.
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

    // Adapts the panel chrome for fullscreen: hides the channel header and
    // EPG, overlays the control bar on the video and auto-hides it until the
    // mouse moves. The MainWindow owns the actual window fullscreen state.
    void setFullscreenMode(bool active);
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
    // The user pressed the fullscreen button (or F/Escape); the MainWindow
    // owns the actual fullscreen state and calls setFullscreenMode().
    void fullscreenRequested();

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

    void showControlsTemporarily();

    PlaybackController* m_controller = nullptr;

    QWidget* m_videoSurface = nullptr;
    QWidget* m_videoContainer = nullptr;
    QStackedLayout* m_overlayLayout = nullptr;
    QWidget* m_headerWidget = nullptr;      // channel info row (hidden in fullscreen)
    QWidget* m_controlsWidget = nullptr;    // overlay host for the control bar
    QWidget* m_controlsBar = nullptr;       // the pill that holds the buttons
    QVBoxLayout* m_controlsLayout = nullptr;
    QTimer m_controlsHideTimer;             // auto-hide the controls in fullscreen
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
    QWidget* m_switchPage = nullptr;    // transparent wrapper (top-centered)
    QLabel* m_switchBanner = nullptr;   // "switched to backup stream" notice

    QLabel* m_channelLogo = nullptr;
    QLabel* m_channelName = nullptr;
    QLabel* m_channelMeta = nullptr;
    QLabel* m_liveBadge = nullptr;
    QLabel* m_videoInfo = nullptr;
    QLabel* m_epgLabel = nullptr;

    QToolButton* m_playPause = nullptr;
    QToolButton* m_liveButton = nullptr;
    QToolButton* m_qualityButton = nullptr;
    QToolButton* m_muteButton = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QLabel* m_volumeLabel = nullptr;
    QToolButton* m_aspectButton = nullptr;
    QToolButton* m_fullscreenButton = nullptr;
    QToolButton* m_favoriteButton = nullptr;
    QToolButton* m_miniButton = nullptr;

    void rebuildQualityMenu();

    bool m_fullscreenActive = false;
    QTimer m_infoRefreshTimer;

    Channel m_current;
};
