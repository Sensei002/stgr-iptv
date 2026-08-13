#include "ui/PlayerPanel.h"

#include <QAction>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSlider>
#include <QStackedLayout>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>

#include "core/Log.h"
#include "playback/PlaybackController.h"
#include "services/EpgManager.h"
#include "services/FavoritesStore.h"
#include "services/LogoCache.h"
#include "ui/Theme.h"

namespace {
const char kPropertyAccent[] = "accent";
}

PlayerPanel::PlayerPanel(PlaybackController* controller, QWidget* parent)
    : QFrame(parent)
    , m_controller(controller)
{
    setProperty("stgrClass", QStringLiteral("panel"));
    setMinimumWidth(360);
    // Native from birth keeps the video surface's window handle stable.
    winId();
    // Filter our own events so Escape exits fullscreen when the panel holds
    // focus.
    installEventFilter(this);

    buildUi();
    buildOverlays();
    buildControls();

    // Fullscreen: the control bar auto-hides after a few seconds of no mouse
    // movement and reappears on hover (VLC-style).
    m_controlsHideTimer.setSingleShot(true);
    connect(&m_controlsHideTimer, &QTimer::timeout, this, [this]() {
        if (m_fullscreenActive)
            m_controlsWidget->hide();
    });

    connect(m_controller, &PlaybackController::stateChanged,
            this, &PlayerPanel::onStateChanged);
    connect(m_controller, &PlaybackController::errorOccurred,
            this, &PlayerPanel::onError);
    connect(m_controller, &PlaybackController::reconnecting,
            this, &PlayerPanel::onReconnecting);
    connect(m_controller, &PlaybackController::channelChanged,
            this, &PlayerPanel::onChannelChanged);
    connect(m_controller, &PlaybackController::videoInfoChanged,
            this, [this](const QString& info) { m_videoInfo->setText(info); });
    connect(m_controller, &PlaybackController::streamSwitched,
            this, [this](const QString& title) {
                m_switchBanner->setText(tr("Switched to backup stream \u2014 %1").arg(title));
                m_switchPage->show();
                m_switchPage->raise();
                QTimer::singleShot(4000, this, [this]() { m_switchPage->hide(); });
            });

    // When the iptv-org mirror data arrives (cache load or background
    // refresh), the quality menu gains the API entries.
    connect(IptvOrgApi::instance(), &IptvOrgApi::ready, this, [this]() {
        if (m_current.isValid())
            rebuildQualityMenu();
    });

    connect(&m_infoRefreshTimer, &QTimer::timeout, this, [this]() {
        const QString info = m_controller->videoInfo();
        if (!info.isEmpty())
            m_videoInfo->setText(info);
    });
    m_infoRefreshTimer.setInterval(3000);

    onStateChanged(IPlaybackEngine::State::Idle);
}

void PlayerPanel::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    // --- channel info header ------------------------------------------------
    // Wrapped in a widget so fullscreen can hide the whole row at once.
    m_headerWidget = new QWidget(this);
    auto* header = new QHBoxLayout(m_headerWidget);
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(10);

    m_channelLogo = new QLabel(this);
    m_channelLogo->setFixedSize(40, 40);
    m_channelLogo->setAlignment(Qt::AlignCenter);
    m_channelLogo->setStyleSheet(QStringLiteral("background: #1b1b22; border-radius: 8px;"));

    auto* nameBox = new QVBoxLayout();
    nameBox->setSpacing(2);
    m_channelName = new QLabel(tr("No channel selected"), this);
    m_channelName->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));
    m_channelMeta = new QLabel(QString(), this);
    m_channelMeta->setProperty("stgrClass", QStringLiteral("dim"));
    m_channelMeta->setStyleSheet(QStringLiteral("font-size: 11px;"));
    nameBox->addWidget(m_channelName);
    nameBox->addWidget(m_channelMeta);

    m_liveBadge = new QLabel(QStringLiteral("LIVE"), this);
    m_liveBadge->setStyleSheet(QStringLiteral(
        "color: white; background: #e2343f; border-radius: 4px; padding: 2px 8px; font-size: 10px; font-weight: 700;"));
    m_liveBadge->setVisible(false);

    m_favoriteButton = new QToolButton(this);
    m_favoriteButton->setIcon(Theme::icon(QStringLiteral("star-outline"), Theme::colors().textDim, 20));
    m_favoriteButton->setToolTip(tr("Add to favorites"));
    m_favoriteButton->setCheckable(true);
    connect(m_favoriteButton, &QToolButton::clicked, this, [this]() {
        if (!m_current.isValid())
            return;
        const bool now = FavoritesStore::instance()->toggle(m_current);
        emit favoriteToggled(m_current, now);
        refreshEpgLabel();
    });

    m_miniButton = new QToolButton(this);
    m_miniButton->setIcon(Theme::icon(QStringLiteral("tv"), Theme::colors().textDim, 20));
    m_miniButton->setToolTip(tr("Open mini player"));
    m_miniButton->setAutoRaise(true);
    connect(m_miniButton, &QToolButton::clicked, this, [this]() {
        if (m_current.isValid())
            emit miniPlayerRequested(m_current);
    });

    header->addWidget(m_channelLogo);
    header->addLayout(nameBox, 1);
    header->addWidget(m_liveBadge);
    header->addWidget(m_favoriteButton);
    header->addWidget(m_miniButton);

    // --- video area ---------------------------------------------------------
    m_videoSurface = new QWidget(this);
    m_videoSurface->setStyleSheet(QStringLiteral("background: #000000;"));
    m_videoSurface->setMinimumHeight(200);
    // Safety net: if the OS gives the surface HWND keyboard focus while in
    // fullscreen, Escape must still exit.
    m_videoSurface->installEventFilter(this);
    m_controller->setVideoSurface(m_videoSurface);

    // Overlays stack (video + loading + error + reconnect banner).
    m_videoContainer = new QWidget(this);
    // The container is made native BEFORE the surface is added to it, so the
    // surface's HWND (the one libVLC renders into) is born as a child of the
    // container's HWND. When the panel moves (fullscreen, window drags), the
    // video follows through this permanent native parent/child link.
    m_videoContainer->winId();
    // Escape while in fullscreen must exit even when the video container has
    // keyboard focus.
    m_videoContainer->installEventFilter(this);
    m_overlayLayout = new QStackedLayout(m_videoContainer);
    m_overlayLayout->setStackingMode(QStackedLayout::StackAll);
    m_overlayLayout->addWidget(m_videoSurface);

    m_loadingPage = new QWidget(m_videoContainer);
    m_loadingPage->setStyleSheet(QStringLiteral("background: rgba(10,10,13,0.78);"));
    m_loadingLogo = new QLabel(m_loadingPage);
    m_loadingLogo->setAlignment(Qt::AlignCenter);
    m_loadingLogo->setPixmap(Theme::appIcon().pixmap(48, 48));
    m_loadingTitle = new QLabel(tr("Loading channel\u2026"), m_loadingPage);
    m_loadingTitle->setAlignment(Qt::AlignCenter);
    m_loadingTitle->setStyleSheet(QStringLiteral("font-size: 14px; color: #e9e9ec;"));
    auto* loadingLayout = new QVBoxLayout(m_loadingPage);
    loadingLayout->setSpacing(14);
    loadingLayout->addStretch();
    loadingLayout->addWidget(m_loadingLogo);
    loadingLayout->addWidget(m_loadingTitle);
    loadingLayout->addStretch();

    m_errorPage = new QWidget(m_videoContainer);
    m_errorPage->setStyleSheet(QStringLiteral("background: rgba(10,10,13,0.88);"));
    m_errorLogo = new QLabel(m_errorPage);
    m_errorLogo->setAlignment(Qt::AlignCenter);
    m_errorLogo->setPixmap(Theme::appIcon().pixmap(44, 44));
    auto* errorLogoEffect = new QGraphicsOpacityEffect(m_errorLogo);
    errorLogoEffect->setOpacity(0.55);
    m_errorLogo->setGraphicsEffect(errorLogoEffect);
    m_errorTitle = new QLabel(tr("Unable to play this channel."), m_errorPage);
    m_errorTitle->setAlignment(Qt::AlignCenter);
    m_errorTitle->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 600; color: #ef4651;"));
    m_errorSubtitle = new QLabel(m_errorPage);
    m_errorSubtitle->setAlignment(Qt::AlignCenter);
    m_errorSubtitle->setWordWrap(true);
    m_errorSubtitle->setProperty("stgrClass", QStringLiteral("dim"));

    auto* errorLayout = new QVBoxLayout(m_errorPage);
    errorLayout->setSpacing(12);
    errorLayout->addStretch();
    errorLayout->addWidget(m_errorLogo);
    errorLayout->addWidget(m_errorTitle);
    errorLayout->addWidget(m_errorSubtitle);
    auto* errorButtons = new QHBoxLayout();
    errorButtons->setSpacing(8);
    errorButtons->addStretch();
    m_retryButton = new QPushButton(tr("Retry"), m_errorPage);
    m_retryButton->setProperty(kPropertyAccent, true);
    m_prevButton = new QPushButton(tr("Previous Channel"), m_errorPage);
    m_backButton = new QPushButton(tr("Back to Channel List"), m_errorPage);
    errorButtons->addWidget(m_retryButton);
    errorButtons->addWidget(m_prevButton);
    errorButtons->addWidget(m_backButton);
    errorButtons->addStretch();
    errorLayout->addLayout(errorButtons);
    errorLayout->addStretch();

    m_reconnectPage = new QWidget(m_videoContainer);
    m_reconnectPage->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* reconnectLayout = new QHBoxLayout(m_reconnectPage);
    reconnectLayout->addStretch();
    m_reconnectLabel = new QLabel(m_reconnectPage);
    m_reconnectLabel->setStyleSheet(QStringLiteral(
        "color: #e9e9ec; background: #3a141a; border: 1px solid #e2343f; border-radius: 6px; padding: 6px 12px; font-size: 12px;"));
    reconnectLayout->addWidget(m_reconnectLabel);
    reconnectLayout->addStretch();
    m_reconnectPage->setVisible(false);

    // Transient "switched to backup mirror" notice (auto-hidden after a few
    // seconds; ignores mouse so it never blocks the video). A transparent
    // wrapper page top-centers the label so only the pill has a background.
    m_switchPage = new QWidget(m_videoContainer);
    m_switchPage->setStyleSheet(QStringLiteral("background: transparent;"));
    m_switchBanner = new QLabel(m_switchPage);
    m_switchBanner->setWordWrap(true);
    m_switchBanner->setStyleSheet(QStringLiteral(
        "color: #e9e9ec; background: rgba(24,42,31,0.92); border: 1px solid #3ecf8e; border-radius: 6px; padding: 6px 12px; font-size: 12px;"));
    m_switchBanner->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* switchLayout = new QVBoxLayout(m_switchPage);
    switchLayout->setContentsMargins(8, 16, 8, 8);
    auto* switchRow = new QHBoxLayout();
    switchRow->addStretch();
    switchRow->addWidget(m_switchBanner);
    switchRow->addStretch();
    switchLayout->addLayout(switchRow);
    switchLayout->addStretch();
    m_switchPage->hide();

    m_overlayLayout->addWidget(m_reconnectPage);
    m_overlayLayout->addWidget(m_loadingPage);
    m_overlayLayout->addWidget(m_errorPage);
    m_overlayLayout->addWidget(m_switchPage);

    root->addWidget(m_headerWidget);
    root->addWidget(m_videoContainer, 1);
}

void PlayerPanel::buildOverlays()
{
    m_videoInfo = new QLabel(QString(), this);
    m_videoInfo->setProperty("stgrClass", QStringLiteral("dim"));
    m_videoInfo->setStyleSheet(QStringLiteral("font-size: 11px;"));

    m_epgLabel = new QLabel(QString(), this);
    m_epgLabel->setWordWrap(true);
    m_epgLabel->setStyleSheet(QStringLiteral(
        "color: #999aa4; font-size: 11px; background: #121216; border: 1px solid #26262f; border-radius: 6px; padding: 6px 10px;"));
    m_epgLabel->setVisible(false);

    connect(m_retryButton, &QPushButton::clicked, this, [this]() { m_controller->retry(); });
    connect(m_prevButton, &QPushButton::clicked, this, [this]() {
        m_controller->playPrevious();
    });
    connect(m_backButton, &QPushButton::clicked, this, [this]() {
        emit backToChannelsRequested();
    });
}

void PlayerPanel::buildControls()
{
    // The control bar lives in its own widget so fullscreen can overlay it on
    // the video (m_controlsWidget is added to the overlay layout) and hide it
    // when idle. In the normal layout the widget takes its natural height.
    m_controlsWidget = new QWidget(this);
    m_controlsWidget->installEventFilter(this); // keep visible while hovering
    m_controlsLayout = new QVBoxLayout(m_controlsWidget);
    m_controlsLayout->setContentsMargins(0, 0, 0, 0);
    m_controlsLayout->setSpacing(0);

    m_controlsBar = new QWidget(m_controlsWidget);
    auto* bar = new QHBoxLayout(m_controlsBar);
    bar->setContentsMargins(0, 0, 0, 0);
    bar->setSpacing(6);

    auto* prev = new QToolButton(this);
    prev->setIcon(Theme::icon(QStringLiteral("prev"), Theme::colors().text, 22));
    prev->setToolTip(tr("Previous channel (Page Up)"));
    prev->setAutoRaise(true);
    connect(prev, &QToolButton::clicked, this, [this]() { playPrevious(); });

    m_playPause = new QToolButton(this);
    m_playPause->setIcon(Theme::icon(QStringLiteral("play"), Theme::colors().text, 24));
    m_playPause->setToolTip(tr("Play / pause (Space)"));
    m_playPause->setAutoRaise(true);
    connect(m_playPause, &QToolButton::clicked, this, [this]() { togglePlayPause(); });

    auto* next = new QToolButton(this);
    next->setIcon(Theme::icon(QStringLiteral("next"), Theme::colors().text, 22));
    next->setToolTip(tr("Next channel (Page Down)"));
    next->setAutoRaise(true);
    connect(next, &QToolButton::clicked, this, [this]() { playNext(); });

    auto* stop = new QToolButton(this);
    stop->setIcon(Theme::icon(QStringLiteral("stop"), Theme::colors().text, 20));
    stop->setToolTip(tr("Stop"));
    stop->setAutoRaise(true);
    connect(stop, &QToolButton::clicked, this, [this]() { m_controller->stop(); });

    m_liveButton = new QToolButton(this);
    m_liveButton->setText(QStringLiteral("LIVE"));
    m_liveButton->setToolTip(tr("Jump back to live (reloads the stream)"));
    m_liveButton->setAutoRaise(true);
    m_liveButton->setStyleSheet(QStringLiteral(
        "color: #e2343f; font-weight: 700; font-size: 11px; padding: 2px 8px;"));
    connect(m_liveButton, &QToolButton::clicked, this, [this]() {
        m_controller->jumpToLive();
    });

    m_qualityButton = new QToolButton(this);
    m_qualityButton->setPopupMode(QToolButton::InstantPopup);
    m_qualityButton->setToolTip(tr("Stream quality"));
    m_qualityButton->setAutoRaise(true);
    m_qualityButton->setStyleSheet(QStringLiteral(
        "font-size: 11px; font-weight: 600; padding: 2px 8px;"));
    connect(m_qualityButton, &QToolButton::triggered, this, [this](QAction* action) {
        if (!action)
            return;
        // API mirror entries carry an IptvStream; playlist entries a Channel.
        if (action->data().canConvert<IptvStream>()) {
            const IptvStream s = action->data().value<IptvStream>();
            if (!s.url.isEmpty())
                m_controller->playStream(s);
            return;
        }
        const Channel ch = action->data().value<Channel>();
        if (ch.isValid())
            m_controller->playChannel(ch);
    });

    m_muteButton = new QToolButton(this);
    m_muteButton->setIcon(Theme::icon(QStringLiteral("volume"), Theme::colors().text, 20));
    m_muteButton->setToolTip(tr("Mute (M)"));
    m_muteButton->setAutoRaise(true);
    m_muteButton->setCheckable(true);
    connect(m_muteButton, &QToolButton::clicked, this, [this]() { toggleMute(); });

    m_volumeLabel = new QLabel(this);
    m_volumeLabel->setProperty("stgrClass", QStringLiteral("dim"));
    m_volumeLabel->setFixedWidth(34);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setFixedWidth(110);
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int v) {
        m_controller->setVolume(v);
        m_volumeLabel->setText(QStringLiteral("%1%").arg(v));
    });
    // Set the value AFTER the label exists and the connection is live, so a
    // non-zero persisted volume can't fire valueChanged into a null label.
    m_volumeSlider->setValue(qBound(0, m_controller->volume(), 100));
    m_volumeLabel->setText(QStringLiteral("%1%").arg(m_volumeSlider->value()));

    m_aspectButton = new QToolButton(this);
    m_aspectButton->setIcon(Theme::icon(QStringLiteral("aspect"), Theme::colors().text, 20));
    m_aspectButton->setToolTip(tr("Aspect ratio (A)"));
    m_aspectButton->setAutoRaise(true);
    connect(m_aspectButton, &QToolButton::clicked, this, [this]() { cycleAspectRatio(); });

    m_fullscreenButton = new QToolButton(this);
    m_fullscreenButton->setIcon(Theme::icon(QStringLiteral("fullscreen"), Theme::colors().text, 20));
    m_fullscreenButton->setToolTip(tr("Fullscreen (F)"));
    m_fullscreenButton->setAutoRaise(true);
    connect(m_fullscreenButton, &QToolButton::clicked, this, [this]() { toggleFullscreen(); });

    bar->addWidget(prev);
    bar->addWidget(m_playPause);
    bar->addWidget(next);
    bar->addWidget(stop);
    bar->addSpacing(10);
    bar->addWidget(m_liveButton);
    bar->addWidget(m_qualityButton);
    bar->addSpacing(10);
    bar->addWidget(m_muteButton);
    bar->addWidget(m_volumeSlider);
    bar->addWidget(m_volumeLabel);
    bar->addStretch(1);
    bar->addWidget(m_videoInfo);
    bar->addStretch(1);
    bar->addWidget(m_aspectButton);
    bar->addWidget(m_fullscreenButton);

    m_controlsLayout->addStretch(1); // pushes the pill to the bottom when overlaid
    m_controlsLayout->addWidget(m_controlsBar);

    auto* root = static_cast<QVBoxLayout*>(layout());
    root->addWidget(m_epgLabel);
    root->addWidget(m_controlsWidget);

    // Keyboard shortcut for mute handled by MainWindow; also accept it here.
    setFocusPolicy(Qt::ClickFocus);
}

// ---------------------------------------------------------------------------
// State handling
// ---------------------------------------------------------------------------
void PlayerPanel::onStateChanged(IPlaybackEngine::State state)
{
    const bool playing = state == IPlaybackEngine::State::Playing;
    const bool loading = state == IPlaybackEngine::State::Loading
        || state == IPlaybackEngine::State::Buffering;

    m_playPause->setIcon(Theme::icon(playing ? QStringLiteral("pause") : QStringLiteral("play"),
                                     Theme::colors().text, 24));

    m_loadingPage->setVisible(loading);
    m_errorPage->setVisible(state == IPlaybackEngine::State::Error);

    if (loading && m_current.isValid())
        m_loadingTitle->setText(tr("Loading %1\u2026").arg(m_current.displayName()));

    m_liveBadge->setVisible(playing);
    if (playing) {
        m_reconnectPage->setVisible(false);
        m_infoRefreshTimer.start();
    } else {
        m_infoRefreshTimer.stop();
    }

    if (state == IPlaybackEngine::State::Stopped || state == IPlaybackEngine::State::Idle) {
        m_errorPage->setVisible(false);
        m_loadingPage->setVisible(false);
    }
}

void PlayerPanel::onError(const QString& message, bool fatal)
{
    m_loadingPage->setVisible(false);
    m_reconnectPage->setVisible(false);

    if (!fatal)
        return; // transient failure - reconnect banner covers it

    m_errorTitle->setText(tr("Unable to play this channel."));
    m_errorSubtitle->setText(message.isEmpty()
                                 ? tr("The stream may be offline or unreachable.")
                                 : message);
    m_errorPage->setVisible(true);
}

void PlayerPanel::onReconnecting(int attempt, int maxAttempts)
{
    m_errorPage->setVisible(false);
    m_reconnectLabel->setText(tr("Stream lost \u2014 reconnecting (%1/%2)\u2026").arg(attempt).arg(maxAttempts));
    m_reconnectPage->setVisible(true);
}

void PlayerPanel::onChannelChanged(const Channel& channel)
{
    m_current = channel;
    m_channelName->setText(channel.displayName());

    QString meta;
    QStringList parts;
    if (!channel.country.isEmpty()) parts << channel.country;
    if (!channel.category().isEmpty()) parts << channel.category();
    if (!channel.language.isEmpty()) parts << channel.language;
    if (!channel.playlistName.isEmpty()) parts << channel.playlistName;
    m_channelMeta->setText(parts.join(QStringLiteral("  \u00b7  ")));

    const bool fav = FavoritesStore::instance()->isFavorite(channel.stableKey());
    m_favoriteButton->setChecked(fav);
    m_favoriteButton->setIcon(Theme::icon(fav ? QStringLiteral("star") : QStringLiteral("star-outline"),
                                          fav ? Theme::colors().gold : Theme::colors().textDim, 20));

    // Logo
    m_channelLogo->setPixmap(QPixmap());
    const QString logoKey = channel.logo;
    if (!logoKey.isEmpty()) {
        const QPixmap pm = LogoCache::instance()->cachedPixmap(logoKey);
        if (!pm.isNull()) {
            m_channelLogo->setPixmap(pm.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            LogoCache::instance()->request(logoKey, logoKey);
        }
    }
    if (m_channelLogo->pixmap().isNull()) {
        m_channelLogo->setText(channel.displayName().left(1).toUpper());
        m_channelLogo->setStyleSheet(QStringLiteral(
            "color: #ef4651; background: #3a141a; border-radius: 8px; font-size: 16px; font-weight: 700;"));
    }

    m_videoInfo->clear();
    m_epgLabel->setVisible(false);
    m_switchPage->hide();
    rebuildQualityMenu();
    refreshEpgLabel();
}

void PlayerPanel::rebuildQualityMenu()
{
    if (!m_qualityButton)
        return;

    const QVector<Channel> variants = m_controller->qualityVariants();
    const QVector<IptvStream> api = m_controller->apiStreams();
    const bool hasApi = !api.isEmpty();
    m_qualityButton->setText(PlaybackController::qualityLabel(m_current));

    if (variants.size() <= 1 && !hasApi) {
        m_qualityButton->setMenu(nullptr);
        m_qualityButton->setEnabled(false);
        m_qualityButton->setToolTip(tr("No alternate qualities in this playlist"));
        return;
    }

    auto* menu = new QMenu(this);
    const QString currentKey = m_current.stableKey();
    const QString activeUrl = m_controller->activeUrl();

    // Playlist variants first (they are the canonical entries).
    for (const Channel& v : variants) {
        QAction* action = menu->addAction(PlaybackController::qualityLabel(v));
        action->setCheckable(true);
        action->setChecked(v.stableKey() == currentKey);
        action->setData(QVariant::fromValue(v));
        action->setToolTip(v.displayName());
    }

    // Then the iptv-org API mirrors, labelled with their real quality.
    if (hasApi) {
        menu->addSeparator();
        for (const IptvStream& s : api) {
            QString label = s.quality.isEmpty() ? tr("Auto") : s.quality;
            if (!s.title.isEmpty())
                label += QStringLiteral("  \u00b7  %1").arg(s.title);
            QAction* action = menu->addAction(label);
            action->setCheckable(true);
            action->setChecked(s.url == activeUrl);
            action->setData(QVariant::fromValue(s));
            action->setToolTip(s.url);
        }
    }

    m_qualityButton->setMenu(menu);
    m_qualityButton->setEnabled(true);
    m_qualityButton->setToolTip(tr("Select stream quality"));
}

void PlayerPanel::setChannelPool(const QVector<Channel>& pool)
{
    m_controller->setChannelPool(pool);
}

void PlayerPanel::updateEpgFor(const Channel& channel)
{
    if (!channel.id.isEmpty())
        refreshEpgLabel();
}

void PlayerPanel::refreshEpgLabel()
{
    if (!m_current.isValid() || m_current.id.isEmpty()) {
        m_epgLabel->setVisible(false);
        return;
    }

    const EpgManager* epg = EpgManager::instance();
    const XmltvParser::Program now = epg->currentProgram(m_current.id);
    const XmltvParser::Program next = epg->nextProgram(m_current.id);

    QString text;
    if (now.isValid()) {
        const QDateTime local = now.startUtc.toLocalTime();
        text += tr("Now (%1): %2").arg(local.time().toString(QStringLiteral("HH:mm")), now.title);
        if (next.isValid()) {
            text += QStringLiteral("\n");
            text += tr("Next (%1): %2").arg(next.startUtc.toLocalTime().time().toString(QStringLiteral("HH:mm")),
                                            next.title);
        }
    } else if (next.isValid()) {
        text += tr("Up next (%1): %2")
                    .arg(next.startUtc.toLocalTime().time().toString(QStringLiteral("HH:mm")),
                         next.title);
    }

    m_epgLabel->setText(text);
    m_epgLabel->setVisible(!text.isEmpty());
}

// ---------------------------------------------------------------------------
// Controls
// ---------------------------------------------------------------------------
void PlayerPanel::setFullscreenMode(bool active)
{
    if (m_fullscreenActive == active)
        return;
    m_fullscreenActive = active;

    auto* root = static_cast<QVBoxLayout*>(layout());

    if (active) {
        // Hide the chrome: channel header and EPG go away so the video is
        // truly edge to edge. The control bar is overlaid on the video and
        // auto-hides until the mouse moves.
        m_headerWidget->hide();
        m_epgLabel->hide();
        root->removeWidget(m_controlsWidget);
        m_overlayLayout->addWidget(m_controlsWidget);
        m_controlsWidget->raise();
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);
        m_controlsBar->setStyleSheet(QStringLiteral(
            "background: rgba(12,12,16,0.65); border: 1px solid rgba(255,255,255,0.10);"
            "border-radius: 10px;"));
        // Track the mouse so hovering the video brings the controls back.
        setMouseTracking(true);
        m_videoContainer->setMouseTracking(true);
        m_videoSurface->setMouseTracking(true);
        showControlsTemporarily();
    } else {
        m_overlayLayout->removeWidget(m_controlsWidget);
        root->addWidget(m_controlsWidget);
        root->setContentsMargins(10, 10, 10, 10);
        root->setSpacing(8);
        m_headerWidget->show();
        refreshEpgLabel(); // restores the EPG row only when there is program data
        m_controlsBar->setStyleSheet(QString());
        m_controlsHideTimer.stop();
        m_controlsWidget->show();
        m_controlsBar->show();
        setMouseTracking(false);
        m_videoContainer->setMouseTracking(false);
        m_videoSurface->setMouseTracking(false);
    }
}

void PlayerPanel::showControlsTemporarily()
{
    if (!m_fullscreenActive)
        return;
    m_controlsWidget->show();
    m_controlsWidget->raise();
    m_controlsHideTimer.start(3000);
}

void PlayerPanel::togglePlayPause() { m_controller->togglePlayPause(); }
void PlayerPanel::toggleMute()
{
    m_controller->toggleMute();
    const bool muted = m_controller->muted();
    m_muteButton->setChecked(muted);
    m_muteButton->setIcon(Theme::icon(muted ? QStringLiteral("mute") : QStringLiteral("volume"),
                                      Theme::colors().text, 20));
}
void PlayerPanel::cycleAspectRatio() { m_controller->cycleAspectRatio(); }
void PlayerPanel::playPrevious() { m_controller->playPrevious(); }
void PlayerPanel::playNext() { m_controller->playNext(); }
void PlayerPanel::retry() { m_controller->retry(); }

void PlayerPanel::volumeUp()
{
    m_volumeSlider->setValue(m_volumeSlider->value() + 5);
}

void PlayerPanel::volumeDown()
{
    m_volumeSlider->setValue(m_volumeSlider->value() - 5);
}

void PlayerPanel::toggleFullscreen()
{
    // The MainWindow owns the actual fullscreen state; it listens to this
    // signal and calls setFullscreenMode() accordingly.
    emit fullscreenRequested();
}

bool PlayerPanel::eventFilter(QObject* watched, QEvent* event)
{
    // While in fullscreen, moving the mouse over the video brings the
    // auto-hidden control bar back (and restarts its hide timer).
    if (m_fullscreenActive && event->type() == QEvent::MouseMove) {
        if (watched == this || watched == m_videoContainer || watched == m_videoSurface
            || watched == m_controlsWidget)
            showControlsTemporarily();
    }

    // Escape exits fullscreen whether the focus lands on the panel, the video
    // container or the video surface.
    if (event->type() == QEvent::KeyPress
        && (watched == this || watched == m_videoContainer || watched == m_videoSurface)) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape && m_fullscreenActive) {
            emit fullscreenRequested();
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}
