#include "ui/dialogs/UpdateDialog.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

#include "ui/Theme.h"

UpdateDialog::UpdateDialog(const QString& version, const QString& releaseUrl,
                           const QString& notes, QWidget* parent)
    : QDialog(parent)
    , m_releaseUrl(releaseUrl)
{
    setWindowTitle(tr("Update Available"));
    setMinimumWidth(480);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 20);
    root->setSpacing(10);

    auto* logo = new QLabel(this);
    logo->setPixmap(Theme::appIcon().pixmap(64, 64));
    logo->setAlignment(Qt::AlignCenter);
    root->addWidget(logo);

    auto* title = new QLabel(tr("STGR IpTV %1 is available").arg(version), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 700; color: #ef4651;"));

    auto* sub = new QLabel(tr("A newer version of STGR IpTV was released."), this);
    sub->setAlignment(Qt::AlignCenter);
    sub->setProperty("stgrClass", QStringLiteral("dim"));

    root->addWidget(title);
    root->addWidget(sub);

    if (!notes.trimmed().isEmpty()) {
        auto* releaseNotes = new QTextBrowser(this);
        releaseNotes->setOpenExternalLinks(true);
        releaseNotes->setPlainText(notes.trimmed());
        releaseNotes->setFixedHeight(160);
        releaseNotes->setStyleSheet(QStringLiteral(
            "QTextBrowser { background: #121216; border: 1px solid #26262f; border-radius: 6px; color: #c9c9d0; }"));
        root->addWidget(releaseNotes);
    }

    auto* buttons = new QHBoxLayout();
    buttons->addStretch();

    auto* later = new QPushButton(tr("Later"), this);
    auto* view = new QPushButton(tr("View Release"), this);
    auto* update = new QPushButton(tr("Update"), this);
    update->setProperty("accent", true);

    connect(later, &QPushButton::clicked, this, &QDialog::reject);
    connect(view, &QPushButton::clicked, this, [this]() {
        if (!m_releaseUrl.isEmpty())
            QDesktopServices::openUrl(QUrl(m_releaseUrl));
    });
    connect(update, &QPushButton::clicked, this, [this]() {
        // Opens the official release page where the signed installer lives.
        if (!m_releaseUrl.isEmpty())
            QDesktopServices::openUrl(QUrl(m_releaseUrl));
        accept();
    });

    buttons->addWidget(later);
    buttons->addWidget(view);
    buttons->addWidget(update);
    buttons->addStretch();
    root->addLayout(buttons);
}
