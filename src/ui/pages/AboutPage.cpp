#include "ui/pages/AboutPage.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "core/version.h"
#include "ui/Theme.h"

AboutPage::AboutPage(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 40, 40, 40);
    root->setSpacing(10);
    root->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    auto* logo = new QLabel(this);
    logo->setPixmap(Theme::appIcon().pixmap(96, 96));
    logo->setAlignment(Qt::AlignCenter);
    root->addWidget(logo);

    auto* name = new QLabel(QStringLiteral("STGR IpTV"), this);
    name->setAlignment(Qt::AlignCenter);
    name->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: 700;"));

    auto* org = new QLabel(QStringLiteral("by STEiGER Dojo"), this);
    org->setAlignment(Qt::AlignCenter);
    org->setProperty("stgrClass", QStringLiteral("gold"));

    auto* version = new QLabel(tr("Version %1").arg(QStringLiteral(STGR_VERSION_STRING)), this);
    version->setAlignment(Qt::AlignCenter);
    version->setProperty("stgrClass", QStringLiteral("dim"));

    root->addWidget(name);
    root->addWidget(org);
    root->addWidget(version);
    root->addSpacing(10);

    auto* description = new QLabel(tr(
        "A lightweight, privacy-first Live TV player for Windows.\n"
        "Reads M3U/M3U8 playlists and plays live television streams."), this);
    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);
    description->setProperty("stgrClass", QStringLiteral("dim"));
    root->addWidget(description);
    root->addSpacing(16);

    auto* legal = new QLabel(tr(
        "STGR IpTV is a player and a client, not an IPTV provider. It does not "
        "host, proxy, download for redistribution, or supply any television "
        "content. Streams are played directly from the playlists you add. The "
        "bundled IPTV-org entry is an external, community-maintained public "
        "playlist source."), this);
    legal->setWordWrap(true);
    legal->setAlignment(Qt::AlignCenter);
    legal->setStyleSheet(QStringLiteral("color: #8a8a96; font-size: 12px; background: #121216;"
                                        " border: 1px solid #26262f; border-radius: 8px; padding: 12px;"));
    root->addWidget(legal);
    root->addSpacing(16);

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(8);
    buttons->addStretch();

    auto* github = new QPushButton(tr("GitHub Repository"), this);
    connect(github, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/%1").arg(QStringLiteral(STGR_UPDATER_REPO))));
    });
    auto* license = new QPushButton(tr("License (MIT)"), this);
    connect(license, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/%1/blob/main/LICENSE").arg(QStringLiteral(STGR_UPDATER_REPO))));
    });
    auto* notices = new QPushButton(tr("Third-Party Notices"), this);
    connect(notices, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/%1/blob/main/THIRD-PARTY-NOTICES.md").arg(QStringLiteral(STGR_UPDATER_REPO))));
    });
    m_updateButton = new QPushButton(tr("Check for Updates"), this);
    m_updateButton->setProperty("accent", true);
    connect(m_updateButton, &QPushButton::clicked, this, [this]() {
        emit checkForUpdatesRequested();
    });

    buttons->addWidget(github);
    buttons->addWidget(license);
    buttons->addWidget(notices);
    buttons->addWidget(m_updateButton);
    buttons->addStretch();
    root->addLayout(buttons);
    root->addStretch(1);
}
