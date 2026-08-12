#include "ui/MainWindow.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/AppPaths.h"
#include "core/version.h"
#include "playback/PlaybackController.h"
#include "playlist/PlaylistManager.h"
#include "services/FavoritesStore.h"
#include "services/HistoryStore.h"
#include "services/NetworkService.h"
#include "services/PlaylistFetcher.h"
#include "services/UpdateService.h"
#include "settings/Settings.h"
#include "ui/Theme.h"
#include "ui/MiniPlayerWindow.h"
#include "ui/dialogs/FirstRunDialog.h"
#include "ui/dialogs/PlaylistDialog.h"
#include "ui/dialogs/UpdateDialog.h"
#include "ui/pages/AboutPage.h"
#include "ui/pages/FavoritesPage.h"
#include "ui/pages/FilterPage.h"
#include "ui/pages/HistoryPage.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/LiveTvPage.h"
#include "ui/pages/SearchPage.h"
#include "ui/pages/SettingsPage.h"

namespace {

enum PageIndex {
    PageHome = 0,
    PageLive,
    PageFavorites,
    PageHistory,
    PageCountries,
    PageCategories,
    PageLanguages,
    PageSettings,
    PageAbout,
    PageSearch // not in the sidebar; reached via Ctrl+K
};

struct SidebarEntry {
    QString icon;
    QString label;
    int page;
};

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("STGR IpTV"));
    setWindowIcon(Theme::appIcon());

    m_settings = Settings::instance();
    m_controller = new PlaybackController(this);

    auto* source = new CompositePlaylistSource(this);
    m_manager = new PlaylistManager(source, this);

    FavoritesStore::instance()->load();
    HistoryStore::instance()->load();

    buildUi();
    wireSignals();

    // Restore window state.
    if (m_settings->rememberWindowSize() && !m_settings->windowGeometry().isEmpty()) {
        if (!restoreGeometry(m_settings->windowGeometry()))
            resize(1280, 800);
    } else {
        resize(1280, 800);
    }
    if (m_settings->startMaximized() || m_settings->windowMaximized())
        showMaximized();

    m_manager->load();

    // First run flow.
    if (!m_settings->firstRunDone()) {
        QTimer::singleShot(150, this, [this]() { showFirstRunIfNeeded(); });
    }

    // Optional startup update check.
    if (m_settings->checkUpdatesAutomatically())
        QTimer::singleShot(2000, this, [this]() { checkForUpdates(); });

    // Optional automatic playlist refresh.
    if (m_settings->autoRefreshEnabled() && m_settings->refreshIntervalMin() > 0) {
        m_autoRefreshTimer.setInterval(m_settings->refreshIntervalMin() * 60000);
        m_autoRefreshTimer.start();
    }

    // Offline indicator.
    connect(NetworkService::instance(), &NetworkService::onlineStateChanged,
            this, &MainWindow::updateOnlineIndicator);
    updateOnlineIndicator(NetworkService::instance()->isOnline());
}

MainWindow::~MainWindow()
{
    saveWindowState();
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------
void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralRoot"));
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Top bar
    auto* topBar = new QFrame(central);
    topBar->setProperty("stgrClass", QStringLiteral("topBar"));
    topBar->setFixedHeight(56);
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 0, 16, 0);
    topLayout->setSpacing(12);

    auto* logo = new QLabel(topBar);
    logo->setPixmap(Theme::appIcon().pixmap(34, 34));

    // Brand wordmark: icon + stacked product name / dojo line.
    auto* wordmark = new QWidget(topBar);
    auto* wmLayout = new QVBoxLayout(wordmark);
    wmLayout->setContentsMargins(0, 0, 0, 0);
    wmLayout->setSpacing(0);
    auto* appName = new QLabel(QStringLiteral("STGR IpTV"), wordmark);
    appName->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 700;"));
    auto* appTag = new QLabel(QStringLiteral("STEiGER DOJO"), wordmark);
    appTag->setStyleSheet(QStringLiteral(
        "color: #d9b64a; font-size: 9px; font-weight: 700; letter-spacing: 2px;"));
    wmLayout->addWidget(appName);
    wmLayout->addWidget(appTag);

    m_searchBox = new QLineEdit(topBar);
    m_searchBox->setProperty("search", true);
    m_searchBox->setPlaceholderText(tr("Search channels  (Ctrl+K)"));
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setMaximumWidth(480);

    m_onlineDot = new QLabel(topBar);
    m_onlineDot->setFixedSize(10, 10);
    m_onlineDot->setStyleSheet(QStringLiteral("background: #3ecf8e; border-radius: 5px;"));
    m_onlineLabel = new QLabel(tr("Online"), topBar);
    m_onlineLabel->setProperty("stgrClass", QStringLiteral("dim"));

    auto* settingsBtn = new QToolButton(topBar);
    settingsBtn->setIcon(Theme::icon(QStringLiteral("settings"), Theme::colors().text, 22));
    settingsBtn->setToolTip(tr("Settings"));
    settingsBtn->setAutoRaise(true);
    connect(settingsBtn, &QToolButton::clicked, this, [this]() { navigateTo(PageSettings); });

    topLayout->addWidget(logo);
    topLayout->addWidget(wordmark, 0, Qt::AlignVCenter);
    topLayout->addSpacing(18);
    topLayout->addWidget(m_searchBox, 1);
    topLayout->addStretch(1);
    topLayout->addWidget(m_onlineDot);
    topLayout->addWidget(m_onlineLabel);
    topLayout->addWidget(settingsBtn);

    root->addWidget(topBar);

    // Offline banner
    m_offlineBanner = new QLabel(tr("You're offline \u2014 showing cached playlists. Live streams need a connection."), central);
    m_offlineBanner->setStyleSheet(QStringLiteral(
        "background: #3a2410; color: #e0a030; border-bottom: 1px solid #6b4a1a; padding: 7px 16px; font-size: 12px;"));
    m_offlineBanner->setVisible(false);
    root->addWidget(m_offlineBanner);

    // Body: sidebar + pages
    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    buildSidebar();

    m_stack = new QStackedWidget(central);
    m_home = new HomePage(m_stack);
    m_live = new LiveTvPage(m_controller, m_stack);
    m_favorites = new FavoritesPage(m_stack);
    m_history = new HistoryPage(m_stack);
    m_countries = new FilterPage(FilterPage::Kind::Countries, m_stack);
    m_categories = new FilterPage(FilterPage::Kind::Categories, m_stack);
    m_languages = new FilterPage(FilterPage::Kind::Languages, m_stack);
    m_settingsPage = new SettingsPage(m_stack);
    m_about = new AboutPage(m_stack);
    m_search = new SearchPage(m_stack);

    m_stack->addWidget(m_home);
    m_stack->addWidget(m_live);
    m_stack->addWidget(m_favorites);
    m_stack->addWidget(m_history);
    m_stack->addWidget(m_countries);
    m_stack->addWidget(m_categories);
    m_stack->addWidget(m_languages);
    m_stack->addWidget(m_settingsPage);
    m_stack->addWidget(m_about);
    m_stack->addWidget(m_search);

    body->addWidget(m_sidebar);
    body->addWidget(m_stack, 1);
    root->addLayout(body, 1);

    setCentralWidget(central);

    // Keyboard shortcuts.
    auto* searchShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+K")), this);
    connect(searchShortcut, &QShortcut::activated, this, [this]() {
        m_searchBox->setFocus();
        m_searchBox->selectAll();
    });

    auto* playPauseShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    playPauseShortcut->setContext(Qt::ApplicationShortcut);
    connect(playPauseShortcut, &QShortcut::activated, this, [this]() {
        if (textInputFocused())
            return;
        m_live->playerPanel()->togglePlayPause();
    });

    auto* fullscreenShortcut = new QShortcut(QKeySequence(Qt::Key_F), this);
    fullscreenShortcut->setContext(Qt::ApplicationShortcut);
    connect(fullscreenShortcut, &QShortcut::activated, this, [this]() {
        if (textInputFocused())
            return;
        m_live->playerPanel()->toggleFullscreen();
    });

    auto* muteShortcut = new QShortcut(QKeySequence(Qt::Key_M), this);
    muteShortcut->setContext(Qt::ApplicationShortcut);
    connect(muteShortcut, &QShortcut::activated, this, [this]() {
        if (textInputFocused())
            return;
        m_live->playerPanel()->toggleMute();
    });

    auto* aspectShortcut = new QShortcut(QKeySequence(Qt::Key_A), this);
    aspectShortcut->setContext(Qt::ApplicationShortcut);
    connect(aspectShortcut, &QShortcut::activated, this, [this]() {
        if (textInputFocused())
            return;
        m_live->playerPanel()->cycleAspectRatio();
    });

    auto* prevShortcut = new QShortcut(QKeySequence(Qt::Key_PageUp), this);
    prevShortcut->setContext(Qt::ApplicationShortcut);
    connect(prevShortcut, &QShortcut::activated, this, [this]() {
        m_live->playerPanel()->playPrevious();
    });

    auto* nextShortcut = new QShortcut(QKeySequence(Qt::Key_PageDown), this);
    nextShortcut->setContext(Qt::ApplicationShortcut);
    connect(nextShortcut, &QShortcut::activated, this, [this]() {
        m_live->playerPanel()->playNext();
    });

    auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escShortcut->setContext(Qt::ApplicationShortcut);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (m_live->playerPanel()->isFullscreenActive()) {
            m_live->playerPanel()->toggleFullscreen();
        }
    });

    // Mini player.
    m_miniPlayer = new MiniPlayerWindow(this);
}

void MainWindow::buildSidebar()
{
    m_sidebar = new QListWidget(this);
    m_sidebar->setObjectName(QStringLiteral("sidebar"));
    m_sidebar->setFixedWidth(200);
    m_sidebar->setFocusPolicy(Qt::NoFocus);

    const QVector<SidebarEntry> entries = {
        { QStringLiteral("home"), tr("Home"), PageHome },
        { QStringLiteral("tv"), tr("Live TV"), PageLive },
        { QStringLiteral("star"), tr("Favorites"), PageFavorites },
        { QStringLiteral("history"), tr("Recently Watched"), PageHistory },
        { QStringLiteral("globe"), tr("Countries"), PageCountries },
        { QStringLiteral("grid"), tr("Categories"), PageCategories },
        { QStringLiteral("language"), tr("Languages"), PageLanguages },
        { QStringLiteral("settings"), tr("Settings"), PageSettings },
        { QStringLiteral("info"), tr("About"), PageAbout },
    };

    for (const SidebarEntry& e : entries) {
        auto* item = new QListWidgetItem(Theme::icon(e.icon, Theme::colors().text, 20), e.label);
        item->setData(Qt::UserRole, e.page);
        m_sidebar->addItem(item);
    }
}

// ---------------------------------------------------------------------------
// Signal wiring
// ---------------------------------------------------------------------------
void MainWindow::wireSignals()
{
    // Pool updates.
    connect(m_manager, &PlaylistManager::channelsChanged, this, [this](const QString&) { refreshAllViews(); });
    connect(m_manager, &PlaylistManager::playlistsChanged, this, [this]() { refreshAllViews(); });
    connect(m_manager, &PlaylistManager::allChannelsChanged, this, [this]() { refreshAllViews(); });

    // Favorites / history.
    connect(FavoritesStore::instance(), &FavoritesStore::favoritesChanged, this, [this]() {
        m_favorites->refresh();
        m_home->setFavorites(resolveFavorites());
        updateFavoriteKeysOnViews();
    });
    connect(HistoryStore::instance(), &HistoryStore::historyChanged, this, [this]() {
        m_home->setHistory(resolveHistory());
        m_history->refresh();
    });

    // Sidebar navigation.
    connect(m_sidebar, &QListWidget::currentRowChanged, this, [this](int row) {
        const QListWidgetItem* item = m_sidebar->item(row);
        if (!item)
            return;
        navigateTo(item->data(Qt::UserRole).toInt());
    });

    // Global search box.
    connect(m_searchBox, &QLineEdit::textChanged, this, [this](const QString& text) {
        navigateTo(PageSearch);
        m_search->setSearchText(text);
    });

    // Page signals.
    connect(m_home, &HomePage::channelActivated, this, &MainWindow::playChannelFromPool);
    connect(m_live, &LiveTvPage::channelActivated, this, &MainWindow::playChannelFromPool);
    connect(m_favorites, &FavoritesPage::channelActivated, this, &MainWindow::playChannelFromPool);
    connect(m_history, &HistoryPage::channelActivated, this, &MainWindow::playChannelFromPool);
    connect(m_countries, &FilterPage::channelActivated, this, &MainWindow::playChannelFromPool);
    connect(m_categories, &FilterPage::channelActivated, this, &MainWindow::playChannelFromPool);
    connect(m_languages, &FilterPage::channelActivated, this, &MainWindow::playChannelFromPool);
    connect(m_search, &SearchPage::channelActivated, this, &MainWindow::playChannelFromPool);

    connect(m_live, &LiveTvPage::refreshRequested, m_manager, &PlaylistManager::refreshAll);

    connect(m_home, &HomePage::countryShortcutSelected, this, [this](const QString& v) {
        navigateTo(PageCountries);
        m_countries->selectValue(v);
    });
    connect(m_home, &HomePage::categoryShortcutSelected, this, [this](const QString& v) {
        navigateTo(PageCategories);
        m_categories->selectValue(v);
    });
    connect(m_home, &HomePage::openPlaylistsRequested, this, [this]() {
        navigateTo(PageSettings);
        emit m_settingsPage->openPlaylistsTabRequested();
    });

    connect(m_settingsPage, &SettingsPage::viewModeChanged, m_live, &LiveTvPage::setGridMode);
    connect(m_settingsPage, &SettingsPage::showLogosChanged, m_live, &LiveTvPage::setShowLogos);
    connect(m_settingsPage, &SettingsPage::playlistsChanged, this, [this]() { refreshAllViews(); });
    connect(m_settingsPage, &SettingsPage::checkForUpdatesRequested, this, [this]() { checkForUpdates(true); });
    connect(m_about, &AboutPage::checkForUpdatesRequested, this, [this]() { checkForUpdates(true); });

    connect(m_live->playerPanel(), &PlayerPanel::favoriteToggled, this,
            [this](const Channel& ch, bool) {
                Q_UNUSED(ch);
                m_favorites->refresh();
                m_home->setFavorites(resolveFavorites());
                updateFavoriteKeysOnViews();
            });
    connect(m_live->playerPanel(), &PlayerPanel::backToChannelsRequested, this, [this]() {
        navigateTo(PageLive);
        m_live->focusChannelList();
    });
    connect(m_live->playerPanel(), &PlayerPanel::miniPlayerRequested, this, [this](const Channel& ch) {
        if (m_miniPlayer)
            m_miniPlayer->playChannel(ch);
    });

    // Settings page playlist manager.
    m_settingsPage->setPlaylistManager(m_manager);
    connect(m_settingsPage, &SettingsPage::openPlaylistsTabRequested, this, [this]() {
        m_settingsPage->selectPlaylistsTab();
    });

    // Auto refresh timer.
    connect(&m_autoRefreshTimer, &QTimer::timeout, m_manager, &PlaylistManager::refreshAll);

    // Update service (wired once; manual vs. silent handled via flag).
    connect(UpdateService::instance(), &UpdateService::updateAvailable, this,
            [this](const QString& version, const QString& url, const QString& notes) {
                UpdateDialog dialog(version, url, notes, this);
                dialog.exec();
            });
    connect(UpdateService::instance(), &UpdateService::noUpdateAvailable, this, [this]() {
        if (m_manualUpdateCheck)
            QMessageBox::information(this, tr("Updates"),
                                     tr("You're running the latest version of STGR IpTV."));
    });
    connect(UpdateService::instance(), &UpdateService::updateCheckFailed, this,
            [this](const QString& error) {
                if (m_manualUpdateCheck)
                    QMessageBox::warning(this, tr("Updates"), error);
            });
}

// ---------------------------------------------------------------------------
// Data flow
// ---------------------------------------------------------------------------
void MainWindow::refreshAllViews()
{
    m_pool = m_manager->allChannels();
    m_poolByKey.clear();
    for (const Channel& c : m_pool)
        m_poolByKey.insert(c.stableKey(), c);

    m_home->setAllChannels(m_pool);
    m_home->setFavorites(resolveFavorites());
    m_home->setHistory(resolveHistory());
    m_home->setCurrentKey(m_controller->currentChannel().stableKey());

    m_live->setAllChannels(m_pool);
    m_live->setCurrentKey(m_controller->currentChannel().stableKey());
    m_live->applySettings();
    m_live->setFavoriteKeys(FavoritesStore::instance()->keys());

    m_countries->setAllChannels(m_pool);
    m_categories->setAllChannels(m_pool);
    m_languages->setAllChannels(m_pool);

    m_favorites->setAllChannels(m_pool);
    m_history->setAllChannels(m_pool);
    m_search->setAllChannels(m_pool);

    m_favorites->setCurrentKey(m_controller->currentChannel().stableKey());
    m_history->setCurrentKey(m_controller->currentChannel().stableKey());
    m_search->setCurrentKey(m_controller->currentChannel().stableKey());

    updateFavoriteKeysOnViews();

    // Resume last channel once, when the pool first becomes available.
    if (!m_started && !m_pool.isEmpty()) {
        m_started = true;
        if (m_settings->rememberLastChannel()) {
            const QString key = m_settings->lastChannelKey();
            if (!key.isEmpty()) {
                const auto it = m_poolByKey.constFind(key);
                if (it != m_poolByKey.constEnd()) {
                    QTimer::singleShot(200, this, [this, ch = it.value()]() {
                        playChannelFromPool(ch, m_pool);
                    });
                }
            }
        }
    }
}

QVector<Channel> MainWindow::resolveFavorites() const
{
    QVector<Channel> out;
    const QVector<ChannelRef> refs = FavoritesStore::instance()->favorites();
    for (const ChannelRef& ref : refs) {
        const auto it = m_poolByKey.constFind(ref.key);
        if (it != m_poolByKey.constEnd())
            out.append(it.value());
    }
    return out;
}

QVector<Channel> MainWindow::resolveHistory() const
{
    QVector<Channel> out;
    const QVector<ChannelRef> refs = HistoryStore::instance()->history();
    for (const ChannelRef& ref : refs) {
        const auto it = m_poolByKey.constFind(ref.key);
        if (it != m_poolByKey.constEnd())
            out.append(it.value());
    }
    return out;
}

void MainWindow::updateFavoriteKeysOnViews()
{
    const QSet<QString> keys = FavoritesStore::instance()->keys();
    m_live->setFavoriteKeys(keys);
    m_home->setFavorites(resolveFavorites());
}

void MainWindow::playChannelFromPool(const Channel& channel, const QVector<Channel>& pool)
{
    if (!channel.isValid() || channel.url.trimmed().isEmpty()) {
        QMessageBox::information(this, tr("Channel unavailable"),
                                 tr("This channel is not available right now. "
                                    "Refresh its playlist and try again."));
        return;
    }

    m_controller->setChannelPool(pool.isEmpty() ? m_pool : pool);
    m_controller->playChannel(channel);

    HistoryStore::instance()->record(channel);
    if (m_settings->rememberLastChannel())
        m_settings->setLastChannelKey(channel.stableKey());

    m_live->setCurrentKey(channel.stableKey());
    navigateTo(PageLive);
}

// ---------------------------------------------------------------------------
// Navigation / misc
// ---------------------------------------------------------------------------
void MainWindow::navigateTo(int page)
{
    m_stack->setCurrentIndex(page);
    const int count = m_sidebar->count();
    for (int i = 0; i < count; ++i) {
        const QListWidgetItem* item = m_sidebar->item(i);
        if (item && item->data(Qt::UserRole).toInt() == page) {
            m_sidebar->blockSignals(true);
            m_sidebar->setCurrentRow(i);
            m_sidebar->blockSignals(false);
            break;
        }
    }
}

bool MainWindow::textInputFocused() const
{
    QWidget* focus = QApplication::focusWidget();
    if (!focus)
        return false;
    return qobject_cast<QLineEdit*>(focus) != nullptr
        || qobject_cast<QTextEdit*>(focus) != nullptr
        || qobject_cast<QPlainTextEdit*>(focus) != nullptr;
}

void MainWindow::updateOnlineIndicator(bool online)
{
    m_onlineDot->setStyleSheet(QStringLiteral("background: %1; border-radius: 5px;")
                                   .arg(online ? QStringLiteral("#3ecf8e") : QStringLiteral("#e0a030")));
    m_onlineLabel->setText(online ? tr("Online") : tr("Offline"));
    m_offlineBanner->setVisible(!online);
}

void MainWindow::checkForUpdates(bool manual)
{
    m_manualUpdateCheck = manual;
    UpdateService::instance()->checkForUpdates();
}

void MainWindow::showFirstRunIfNeeded()
{
    FirstRunDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    switch (dialog.choice()) {
    case FirstRunDialog::Choice::IptvOrg: {
        const QVector<Playlist> builtIns = PlaylistManager::builtInPlaylists();
        if (!builtIns.isEmpty()) {
            const QString id = m_manager->addPlaylist(builtIns.first().name,
                                                      builtIns.first().url, QString(), true);
            m_manager->refresh(id);
        }
        break;
    }
    case FirstRunDialog::Choice::AddPlaylist: {
        PlaylistDialog addDialog(this);
        if (addDialog.exec() == QDialog::Accepted && !addDialog.url().trimmed().isEmpty()) {
            const QString id = m_manager->addPlaylist(addDialog.name(), addDialog.url(),
                                                      addDialog.epgUrl(), addDialog.isBuiltInChoice());
            if (addDialog.refreshAfterAdd())
                m_manager->refresh(id);
        }
        break;
    }
    case FirstRunDialog::Choice::ImportFile: {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Import M3U playlist"), QString(),
            tr("Playlists (*.m3u *.m3u8);;All files (*.*)"));
        if (!path.isEmpty())
            m_manager->importLocalFile(path);
        break;
    }
    case FirstRunDialog::Choice::Continue:
    default:
        break;
    }

    m_settings->setFirstRunDone(true);
    m_settings->save();
}

void MainWindow::saveWindowState()
{
    if (m_settings->rememberWindowSize()) {
        m_settings->setWindowGeometry(saveGeometry());
        m_settings->setWindowMaximized(isMaximized());
    }
    m_settings->save();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveWindowState();
    m_controller->stop();
    QMainWindow::closeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    // Volume with arrow keys when the player has focus (or nothing does).
    if ((event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
        && !textInputFocused()) {
        QWidget* focus = QApplication::focusWidget();
        const bool listFocused = focus != nullptr && qobject_cast<QAbstractItemView*>(focus) != nullptr;
        if (!listFocused) {
            if (event->key() == Qt::Key_Up)
                m_live->playerPanel()->volumeUp();
            else
                m_live->playerPanel()->volumeDown();
            event->accept();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}
