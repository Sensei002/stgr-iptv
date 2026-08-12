#include "ui/dialogs/PlaylistDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "playlist/PlaylistManager.h"
#include "ui/Theme.h"

PlaylistDialog::PlaylistDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Add Playlist"));
    setMinimumWidth(460);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);

    auto* hint = new QLabel(tr(
        "Add an M3U/M3U8 playlist from a URL or a local file. Public playlists "
        "like the community-maintained IPTV-org list are supported out of the box."), this);
    hint->setWordWrap(true);
    hint->setProperty("stgrClass", QStringLiteral("dim"));
    root->addWidget(hint);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_name = new QLineEdit(this);
    m_name->setPlaceholderText(tr("e.g. My Playlist"));
    form->addRow(tr("Name:"), m_name);

    auto* urlRow = new QHBoxLayout();
    m_url = new QLineEdit(this);
    m_url->setPlaceholderText(tr("https://example.com/playlist.m3u or C:\\path\\list.m3u"));
    auto* browseBtn = new QPushButton(tr("Browse\u2026"), this);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Select M3U playlist"), QString(),
            tr("Playlists (*.m3u *.m3u8);;All files (*.*)"));
        if (!path.isEmpty())
            m_url->setText(QDir::toNativeSeparators(path));
    });
    urlRow->addWidget(m_url, 1);
    urlRow->addWidget(browseBtn);
    form->addRow(tr("URL / file:"), urlRow);

    m_epgUrl = new QLineEdit(this);
    m_epgUrl->setPlaceholderText(tr("Optional XMLTV URL (EPG guide)"));
    form->addRow(tr("EPG URL (optional):"), m_epgUrl);

    root->addLayout(form);

    auto* divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet(QStringLiteral("color: #26262f;"));
    root->addWidget(divider);

    m_builtInLabel = new QLabel(tr("Quick add \u2014 IPTV-org (community playlists):"), this);
    m_builtInLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));
    root->addWidget(m_builtInLabel);

    m_builtIn = new QComboBox(this);
    m_builtIn->addItem(tr("Choose a built-in playlist\u2026"), QString());
    const QVector<Playlist> builtIns = PlaylistManager::builtInPlaylists();
    for (const Playlist& p : builtIns)
        m_builtIn->addItem(p.name, p.url);
    connect(m_builtIn, &QComboBox::currentIndexChanged, this, [this](int idx) {
        const QString url = m_builtIn->itemData(idx).toString();
        if (url.isEmpty())
            return;
        m_url->setText(url);
        if (m_name->text().trimmed().isEmpty())
            m_name->setText(m_builtIn->currentText());
        m_builtInUsed = true;
        m_refreshAfter->setChecked(true);
    });
    root->addWidget(m_builtIn);

    m_refreshAfter = new QCheckBox(tr("Refresh immediately after adding"), this);
    m_refreshAfter->setChecked(true);
    root->addWidget(m_refreshAfter);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Add"));
    buttons->button(QDialogButtonBox::Ok)->setProperty("accent", true);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    connect(m_name, &QLineEdit::textChanged, this, [this, buttons](const QString& text) {
        buttons->button(QDialogButtonBox::Ok)->setEnabled(!text.trimmed().isEmpty());
    });
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void PlaylistDialog::setMode(Mode mode)
{
    if (mode == Mode::Edit) {
        setWindowTitle(tr("Edit Playlist"));
        m_builtIn->setVisible(false);
        if (m_builtInLabel)
            m_builtInLabel->setVisible(false);
    }
}

void PlaylistDialog::setName(const QString& name) { m_name->setText(name); }
void PlaylistDialog::setUrl(const QString& url) { m_url->setText(url); }
void PlaylistDialog::setEpgUrl(const QString& url) { m_epgUrl->setText(url); }

QString PlaylistDialog::name() const { return m_name->text(); }
QString PlaylistDialog::url() const { return m_url->text(); }
QString PlaylistDialog::epgUrl() const { return m_epgUrl->text(); }
bool PlaylistDialog::refreshAfterAdd() const { return m_refreshAfter->isChecked(); }
bool PlaylistDialog::isBuiltInChoice() const { return m_builtInUsed; }
