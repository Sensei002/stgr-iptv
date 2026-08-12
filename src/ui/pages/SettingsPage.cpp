#include "ui/pages/SettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QUrl>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "core/AppPaths.h"
#include "core/version.h"
#include "playlist/PlaylistManager.h"
#include "services/UpdateService.h"
#include "settings/Settings.h"
#include "ui/dialogs/PlaylistDialog.h"
#include "ui/Theme.h"

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);

    auto* title = new QLabel(tr("Settings"), this);
    title->setProperty("stgrClass", QStringLiteral("pageTitle"));
    root->addWidget(title);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildGeneralTab(), tr("General"));
    m_tabs->addTab(buildPlaybackTab(), tr("Playback"));
    m_tabs->addTab(buildInterfaceTab(), tr("Interface"));
    m_tabs->addTab(buildPlaylistsTab(), tr("Playlists"));
    m_tabs->addTab(buildPrivacyTab(), tr("Privacy"));
    m_tabs->addTab(buildDiagnosticsTab(), tr("Diagnostics"));
    root->addWidget(m_tabs, 1);
}

void SettingsPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    refreshPlaylistsTab();
}

void SettingsPage::selectPlaylistsTab()
{
    // The playlists tab is index 3 in the tab widget.
    if (m_tabs && m_tabs->count() > 3)
        m_tabs->setCurrentIndex(3);
}

// ---------------------------------------------------------------------------
// Tabs
// ---------------------------------------------------------------------------
QWidget* SettingsPage::buildGeneralTab()
{
    auto* w = new QWidget(this);
    auto* form = new QFormLayout(w);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    Settings* s = Settings::instance();

    auto* startMax = new QCheckBox(tr("Start maximized"), w);
    startMax->setChecked(s->startMaximized());
    connect(startMax, &QCheckBox::toggled, this, [s](bool v) { s->setStartMaximized(v); s->save(); });
    form->addRow(startMax);

    auto* rememberSize = new QCheckBox(tr("Remember window size and position"), w);
    rememberSize->setChecked(s->rememberWindowSize());
    connect(rememberSize, &QCheckBox::toggled, this, [s](bool v) { s->setRememberWindowSize(v); s->save(); });
    form->addRow(rememberSize);

    auto* rememberChannel = new QCheckBox(tr("Resume the last channel on startup"), w);
    rememberChannel->setChecked(s->rememberLastChannel());
    connect(rememberChannel, &QCheckBox::toggled, this, [s](bool v) { s->setRememberLastChannel(v); s->save(); });
    form->addRow(rememberChannel);

    auto* launch = new QCheckBox(tr("Launch STGR IpTV when Windows starts"), w);
    launch->setChecked(s->launchOnStartup());
    connect(launch, &QCheckBox::toggled, this, [s](bool v) {
        s->setLaunchOnStartup(v);
        s->save();
        s->applyLaunchOnStartup();
    });
    form->addRow(launch);

    return w;
}

QWidget* SettingsPage::buildPlaybackTab()
{
    auto* w = new QWidget(this);
    auto* form = new QFormLayout(w);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    Settings* s = Settings::instance();

    auto* hw = new QComboBox(w);
    hw->addItem(tr("Automatic"), 0);
    hw->addItem(tr("Enabled"), 1);
    hw->addItem(tr("Disabled"), 2);
    hw->setCurrentIndex(hw->findData(s->hardwareAcceleration()));
    connect(hw, &QComboBox::currentIndexChanged, this, [s, hw](int) {
        s->setHardwareAcceleration(hw->currentData().toInt());
        s->save();
    });
    form->addRow(tr("Hardware acceleration:"), hw);

    auto* buffer = new QSpinBox(w);
    buffer->setRange(0, 10000);
    buffer->setSingleStep(100);
    buffer->setSuffix(QStringLiteral(" ms"));
    buffer->setValue(s->bufferSizeMs());
    connect(buffer, &QSpinBox::valueChanged, this, [s](int v) { s->setBufferSizeMs(v); s->save(); });
    form->addRow(tr("Stream buffer size:"), buffer);

    auto* timeout = new QSpinBox(w);
    timeout->setRange(3, 120);
    timeout->setSuffix(QStringLiteral(" s"));
    timeout->setValue(s->networkTimeoutSec());
    connect(timeout, &QSpinBox::valueChanged, this, [s](int v) { s->setNetworkTimeoutSec(v); s->save(); });
    form->addRow(tr("Network timeout:"), timeout);

    auto* reconnect = new QCheckBox(tr("Automatically reconnect failed streams"), w);
    reconnect->setChecked(s->autoReconnect());
    connect(reconnect, &QCheckBox::toggled, this, [s](bool v) { s->setAutoReconnect(v); s->save(); });
    form->addRow(reconnect);

    auto* retries = new QSpinBox(w);
    retries->setRange(0, 10);
    retries->setValue(s->maxRetries());
    connect(retries, &QSpinBox::valueChanged, this, [s](int v) { s->setMaxRetries(v); s->save(); });
    form->addRow(tr("Maximum reconnect attempts:"), retries);

    auto* volume = new QSlider(Qt::Horizontal, w);
    volume->setRange(0, 100);
    volume->setValue(s->defaultVolume());
    connect(volume, &QSlider::valueChanged, this, [s](int v) { s->setDefaultVolume(v); s->save(); });
    form->addRow(tr("Default volume:"), volume);

    auto* hint = new QLabel(tr("Playback settings apply to the next channel you open."), w);
    hint->setProperty("stgrClass", QStringLiteral("dim"));
    form->addRow(hint);

    return w;
}

QWidget* SettingsPage::buildInterfaceTab()
{
    auto* w = new QWidget(this);
    auto* form = new QFormLayout(w);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    Settings* s = Settings::instance();

    auto* theme = new QComboBox(w);
    theme->addItem(tr("Dark (Dojo)"), QStringLiteral("dark"));
    theme->setCurrentIndex(theme->findData(s->theme()));
    connect(theme, &QComboBox::currentIndexChanged, this, [s, theme](int) {
        s->setTheme(theme->currentData().toString());
        s->save();
    });
    form->addRow(tr("Theme:"), theme);

    auto* compact = new QCheckBox(tr("Compact mode"), w);
    compact->setChecked(s->compactMode());
    connect(compact, &QCheckBox::toggled, this, [s](bool v) { s->setCompactMode(v); s->save(); });
    form->addRow(compact);

    auto* logos = new QCheckBox(tr("Show channel logos"), w);
    logos->setChecked(s->showLogos());
    connect(logos, &QCheckBox::toggled, this, [this, s](bool v) {
        s->setShowLogos(v);
        s->save();
        emit showLogosChanged(v);
    });
    form->addRow(logos);

    auto* viewMode = new QComboBox(w);
    viewMode->addItem(tr("Grid"), QStringLiteral("grid"));
    viewMode->addItem(tr("List"), QStringLiteral("list"));
    viewMode->setCurrentIndex(viewMode->findData(s->viewMode()));
    connect(viewMode, &QComboBox::currentIndexChanged, this, [this, s, viewMode](int) {
        const QString mode = viewMode->currentData().toString();
        s->setViewMode(mode);
        s->save();
        emit viewModeChanged(mode != QLatin1String("list"));
    });
    form->addRow(tr("Channel view:"), viewMode);

    auto* animations = new QCheckBox(tr("Enable animations"), w);
    animations->setChecked(s->animationsEnabled());
    connect(animations, &QCheckBox::toggled, this, [s](bool v) { s->setAnimationsEnabled(v); s->save(); });
    form->addRow(animations);

    return w;
}

QWidget* SettingsPage::buildPlaylistsTab()
{
    auto* w = new QWidget(this);
    auto* root = new QVBoxLayout(w);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(10);

    auto* hint = new QLabel(tr(
        "STGR IpTV plays M3U/M3U8 playlists you provide. The bundled IPTV-org "
        "entry is a publicly maintained community playlist \u2014 remove it any time."), w);
    hint->setWordWrap(true);
    hint->setProperty("stgrClass", QStringLiteral("dim"));
    root->addWidget(hint);

    m_playlistList = new QListWidget(w);
    m_playlistList->setAlternatingRowColors(true);
    root->addWidget(m_playlistList, 1);

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(8);

    m_playlistAddBtn = new QPushButton(tr("Add Playlist\u2026"), w);
    m_playlistAddBtn->setProperty("accent", true);
    auto* importBtn = new QPushButton(tr("Import M3U\u2026"), w);
    m_refreshBtn = new QPushButton(tr("Refresh"), w);
    auto* editBtn = new QPushButton(tr("Edit\u2026"), w);
    auto* enableBtn = new QPushButton(tr("Enable / Disable"), w);
    auto* deleteBtn = new QPushButton(tr("Delete"), w);
    auto* restoreBtn = new QPushButton(tr("Restore IPTV-org"), w);

    buttons->addWidget(m_playlistAddBtn);
    buttons->addWidget(importBtn);
    buttons->addStretch(1);
    buttons->addWidget(m_refreshBtn);
    buttons->addWidget(editBtn);
    buttons->addWidget(enableBtn);
    buttons->addWidget(deleteBtn);
    buttons->addWidget(restoreBtn);
    root->addLayout(buttons);

    connect(m_playlistAddBtn, &QPushButton::clicked, this, &SettingsPage::addPlaylistDialog);
    connect(importBtn, &QPushButton::clicked, this, &SettingsPage::importPlaylistFile);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SettingsPage::refreshSelectedPlaylist);
    connect(editBtn, &QPushButton::clicked, this, &SettingsPage::editSelectedPlaylist);
    connect(enableBtn, &QPushButton::clicked, this, &SettingsPage::toggleSelectedPlaylist);
    connect(deleteBtn, &QPushButton::clicked, this, &SettingsPage::deleteSelectedPlaylist);
    connect(restoreBtn, &QPushButton::clicked, this, [this]() {
        if (m_manager) {
            m_manager->restoreBuiltInPlaylists();
            refreshPlaylistsTab();
        }
    });

    connect(m_playlistList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        editSelectedPlaylist();
    });

    return w;
}

QWidget* SettingsPage::buildPrivacyTab()
{
    auto* w = new QWidget(this);
    auto* root = new QVBoxLayout(w);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(12);

    auto* title = new QLabel(tr("Privacy-first by design"), w);
    title->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));

    auto* body = new QLabel(tr(
        "STGR IpTV has no telemetry, no analytics, no advertising SDK and no "
        "tracking of any kind. Nothing is uploaded automatically.\n\n"
        "Network requests only happen when you ask for them:\n"
        "  \u2022 Loading or refreshing a playlist you added,\n"
        "  \u2022 Playing a channel you selected,\n"
        "  \u2022 Downloading channel logos for visible channels,\n"
        "  \u2022 Optional EPG data you configured,\n"
        "  \u2022 The optional update check (GitHub Releases, if enabled).\n\n"
        "All data (playlists, caches, favorites, history, logs) stays in your "
        "user profile folder and is never shared."), w);
    body->setWordWrap(true);
    body->setProperty("stgrClass", QStringLiteral("dim"));

    auto* dataFolder = new QPushButton(tr("Open data folder"), w);
    connect(dataFolder, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(AppPaths::appDataDir()));
    });

    root->addWidget(title);
    root->addWidget(body);
    root->addWidget(dataFolder);
    root->addStretch(1);
    return w;
}

QWidget* SettingsPage::buildDiagnosticsTab()
{
    auto* w = new QWidget(this);
    auto* root = new QVBoxLayout(w);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(12);

    auto* logsBtn = new QPushButton(tr("Open logs folder"), w);
    connect(logsBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(AppPaths::logsDir()));
    });
    root->addWidget(logsBtn);

    auto* dataBtn = new QPushButton(tr("Open data folder"), w);
    connect(dataBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(AppPaths::appDataDir()));
    });
    root->addWidget(dataBtn);

    auto* updatesBtn = new QPushButton(tr("Check for updates"), w);
    connect(updatesBtn, &QPushButton::clicked, this, [this]() {
        emit checkForUpdatesRequested();
    });
    root->addWidget(updatesBtn);

    auto* version = new QLabel(tr("Version %1").arg(QStringLiteral(STGR_VERSION_STRING)), w);
    version->setProperty("stgrClass", QStringLiteral("dim"));
    root->addWidget(version);

    root->addStretch(1);
    return w;
}

// ---------------------------------------------------------------------------
// Playlist management
// ---------------------------------------------------------------------------
void SettingsPage::setPlaylistManager(PlaylistManager* manager)
{
    m_manager = manager;
    if (!m_manager)
        return;

    connect(m_manager, &PlaylistManager::playlistsChanged, this, [this]() { refreshPlaylistsTab(); });
    connect(m_manager, &PlaylistManager::refreshStarted, this, [this](const QString& id) {
        Q_UNUSED(id);
        m_refreshBtn->setEnabled(false);
        m_refreshBtn->setText(tr("Refreshing\u2026"));
    });
    connect(m_manager, &PlaylistManager::refreshFinished, this,
            [this](const QString&, bool, const QString&, int) {
                m_refreshBtn->setEnabled(true);
                m_refreshBtn->setText(tr("Refresh"));
                refreshPlaylistsTab();
            });
    refreshPlaylistsTab();
}

void SettingsPage::refreshPlaylistsTab()
{
    if (!m_playlistList || !m_manager)
        return;
    refreshPlaylistList();
}

void SettingsPage::refreshPlaylistList()
{
    m_playlistList->clear();
    const QVector<Playlist> playlists = m_manager->playlists();

    for (const Playlist& p : playlists) {
        QString status = tr("no channels");
        if (p.channelCount > 0)
            status = tr("%n channel(s)", nullptr, p.channelCount);

        QString state = p.enabled ? QString() : tr(" [disabled]");
        QString when;
        if (p.lastUpdated.isValid())
            when = tr(" \u00b7 updated %1").arg(p.lastUpdated.toString(QStringLiteral("yyyy-MM-dd HH:mm")));

        QString error = p.errorMessage.isEmpty() ? QString() : tr(" \u00b7 %1").arg(p.errorMessage);

        auto* item = new QListWidgetItem(
            QStringLiteral("%1%2\n%3 \u00b7 %4%5%6")
                .arg(p.name, state, status, p.url, when, error));
        item->setData(Qt::UserRole, p.id);
        item->setToolTip(p.url);
        m_playlistList->addItem(item);
    }
}

void SettingsPage::addPlaylistDialog()
{
    PlaylistDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString name = dialog.name().trimmed();
    const QString url = dialog.url().trimmed();
    if (name.isEmpty() || url.isEmpty())
        return;

    const QString id = m_manager->addPlaylist(name, url, dialog.epgUrl(), dialog.isBuiltInChoice());
    if (dialog.refreshAfterAdd())
        m_manager->refresh(id);
    emit playlistsChanged();
}

void SettingsPage::editSelectedPlaylist()
{
    QListWidgetItem* item = m_playlistList->currentItem();
    if (!item || !m_manager)
        return;

    const QString id = item->data(Qt::UserRole).toString();
    const Playlist* p = m_manager->findPlaylist(id);
    if (!p)
        return;

    PlaylistDialog dialog(this);
    dialog.setMode(PlaylistDialog::Mode::Edit);
    dialog.setName(p->name);
    dialog.setUrl(p->url);
    dialog.setEpgUrl(p->epgUrl);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_manager->renamePlaylist(id, dialog.name());
    m_manager->setPlaylistUrl(id, dialog.url());
    m_manager->setPlaylistEpgUrl(id, dialog.epgUrl());
    if (dialog.refreshAfterAdd())
        m_manager->refresh(id);
    emit playlistsChanged();
}

void SettingsPage::deleteSelectedPlaylist()
{
    QListWidgetItem* item = m_playlistList->currentItem();
    if (!item || !m_manager)
        return;

    const QString id = item->data(Qt::UserRole).toString();
    const Playlist* p = m_manager->findPlaylist(id);
    if (!p)
        return;

    const auto answer = QMessageBox::question(
        this, tr("Delete playlist"),
        tr("Delete the playlist \u201c%1\u201d? Its cached channels will be removed too.").arg(p->name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    m_manager->removePlaylist(id);
    emit playlistsChanged();
}

void SettingsPage::toggleSelectedPlaylist()
{
    QListWidgetItem* item = m_playlistList->currentItem();
    if (!item || !m_manager)
        return;

    const QString id = item->data(Qt::UserRole).toString();
    const Playlist* p = m_manager->findPlaylist(id);
    if (p)
        m_manager->setPlaylistEnabled(id, !p->enabled);
    refreshPlaylistList();
}

void SettingsPage::refreshSelectedPlaylist()
{
    QListWidgetItem* item = m_playlistList->currentItem();
    if (!item || !m_manager)
        return;
    m_manager->refresh(item->data(Qt::UserRole).toString());
}

void SettingsPage::importPlaylistFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Import M3U playlist"), QString(),
        tr("Playlists (*.m3u *.m3u8);;All files (*.*)"));
    if (path.isEmpty() || !m_manager)
        return;

    if (!m_manager->importLocalFile(path)) {
        QMessageBox::warning(this, tr("Import failed"),
                             tr("The selected file could not be imported."));
        return;
    }
    emit playlistsChanged();
}
