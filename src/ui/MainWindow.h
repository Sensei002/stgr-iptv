#pragma once

#include <QHash>
#include <QMainWindow>
#include <QTimer>

#include "models/Channel.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class MiniPlayerWindow;
class AboutPage;
class FavoritesPage;
class FilterPage;
class HistoryPage;
class HomePage;
class LiveTvPage;
class SearchPage;
class SettingsPage;
class Settings;
class PlaybackController;
class PlaylistManager;

// ---------------------------------------------------------------------------
// MainWindow - assembles the whole application: top bar, sidebar, pages and
// every service (playlist manager, playback, stores, updater, offline state).
// ---------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildUi();
    void buildSidebar();
    void wireSignals();
    void refreshAllViews();
    void playChannelFromPool(const Channel& channel, const QVector<Channel>& pool);
    void navigateTo(int page);
    bool textInputFocused() const;
    void updateOnlineIndicator(bool online);
    void checkForUpdates(bool manual = false);
    void showFirstRunIfNeeded();
    void saveWindowState();

    QVector<Channel> resolveFavorites() const;
    QVector<Channel> resolveHistory() const;
    void updateFavoriteKeysOnViews();

    Settings* m_settings = nullptr;
    PlaybackController* m_controller = nullptr;
    PlaylistManager* m_manager = nullptr;
    QVector<Channel> m_pool;
    QHash<QString, Channel> m_poolByKey;

    QListWidget* m_sidebar = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QLabel* m_onlineDot = nullptr;
    QLabel* m_onlineLabel = nullptr;
    QLabel* m_offlineBanner = nullptr;

    HomePage* m_home = nullptr;
    LiveTvPage* m_live = nullptr;
    FavoritesPage* m_favorites = nullptr;
    HistoryPage* m_history = nullptr;
    FilterPage* m_countries = nullptr;
    FilterPage* m_categories = nullptr;
    FilterPage* m_languages = nullptr;
    SettingsPage* m_settingsPage = nullptr;
    AboutPage* m_about = nullptr;
    SearchPage* m_search = nullptr;

    MiniPlayerWindow* m_miniPlayer = nullptr;
    QTimer m_autoRefreshTimer;
    bool m_started = false;
    bool m_manualUpdateCheck = false;
};
