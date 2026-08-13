#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QListWidget;
class QPushButton;
class QSlider;
class QSpinBox;
class QTabWidget;
class PlaylistManager;
class UpdateService;

// ---------------------------------------------------------------------------
// SettingsPage - General / Playback / Interface / Playlists / Privacy /
// Diagnostics tabs. Every change is written to Settings immediately.
// ---------------------------------------------------------------------------
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    void setPlaylistManager(PlaylistManager* manager);
    void refreshPlaylistsTab();
    void selectPlaylistsTab();

signals:
    void showLogosChanged(bool show);
    void playlistsChanged();       // registry mutated here
    void openPlaylistsTabRequested();
    void checkForUpdatesRequested();

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* buildGeneralTab();
    QWidget* buildPlaybackTab();
    QWidget* buildInterfaceTab();
    QWidget* buildPlaylistsTab();
    QWidget* buildPrivacyTab();
    QWidget* buildDiagnosticsTab();

    void refreshPlaylistList();
    void addPlaylistDialog();
    void editSelectedPlaylist();
    void deleteSelectedPlaylist();
    void toggleSelectedPlaylist();
    void refreshSelectedPlaylist();
    void importPlaylistFile();

    QTabWidget* m_tabs = nullptr;

    // Playlists tab
    QListWidget* m_playlistList = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_playlistAddBtn = nullptr;

    PlaylistManager* m_manager = nullptr;
};
