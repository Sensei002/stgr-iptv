#include "ui/dialogs/FirstRunDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "ui/Theme.h"

FirstRunDialog::FirstRunDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Welcome to STGR IpTV"));
    setMinimumWidth(460);
    setModal(true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 24);
    root->setSpacing(10);

    auto* logo = new QLabel(this);
    logo->setPixmap(Theme::appIcon().pixmap(72, 72));
    logo->setAlignment(Qt::AlignCenter);
    root->addWidget(logo);

    auto* title = new QLabel(tr("Welcome to STGR IpTV"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size: 21px; font-weight: 700;"));
    root->addWidget(title);

    auto* subtitle = new QLabel(tr("Watch live television from your own playlists."), this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setProperty("stgrClass", QStringLiteral("dim"));
    root->addWidget(subtitle);
    root->addSpacing(8);

    m_iptvOrgButton = new QPushButton(tr("Use the IPTV-org Public Playlist"), this);
    m_iptvOrgButton->setProperty("accent", true);
    m_iptvOrgButton->setCursor(Qt::PointingHandCursor);

    m_addPlaylistButton = new QPushButton(tr("Add M3U Playlist"), this);
    m_addPlaylistButton->setCursor(Qt::PointingHandCursor);

    m_importButton = new QPushButton(tr("Import M3U File"), this);
    m_importButton->setCursor(Qt::PointingHandCursor);

    auto* continueButton = new QPushButton(tr("Continue"), this);
    continueButton->setFlat(true);
    continueButton->setCursor(Qt::PointingHandCursor);

    root->addWidget(m_iptvOrgButton);
    root->addWidget(m_addPlaylistButton);
    root->addWidget(m_importButton);
    root->addSpacing(6);
    root->addWidget(continueButton);

    auto* privacy = new QLabel(tr("No account. No ads. No tracking."), this);
    privacy->setAlignment(Qt::AlignCenter);
    privacy->setProperty("stgrClass", QStringLiteral("gold"));
    root->addWidget(privacy);

    connect(m_iptvOrgButton, &QPushButton::clicked, this, [this]() {
        m_choice = Choice::IptvOrg;
        accept();
    });
    connect(m_addPlaylistButton, &QPushButton::clicked, this, [this]() {
        m_choice = Choice::AddPlaylist;
        accept();
    });
    connect(m_importButton, &QPushButton::clicked, this, [this]() {
        m_choice = Choice::ImportFile;
        accept();
    });
    connect(continueButton, &QPushButton::clicked, this, [this]() {
        m_choice = Choice::Continue;
        accept();
    });
}
