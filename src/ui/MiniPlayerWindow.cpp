#include "ui/MiniPlayerWindow.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include "playback/EngineFactory.h"
#include "settings/Settings.h"
#include "ui/Theme.h"

MiniPlayerWindow::MiniPlayerWindow(QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
    setWindowTitle(tr("STGR IpTV \u2014 Mini Player"));
    setWindowIcon(Theme::appIcon());
    resize(400, 260);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto* header = new QHBoxLayout();
    m_channelLabel = new QLabel(tr("No channel"), this);
    m_channelLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));

    m_alwaysOnTop = new QCheckBox(tr("Always on top"), this);
    m_alwaysOnTop->setChecked(true);
    connect(m_alwaysOnTop, &QCheckBox::toggled, this, [this](bool on) {
        setWindowFlag(Qt::WindowStaysOnTopHint, on);
        show();
    });

    header->addWidget(m_channelLabel, 1);
    header->addWidget(m_alwaysOnTop);
    root->addLayout(header);

    m_videoSurface = new QWidget(this);
    m_videoSurface->setStyleSheet(QStringLiteral("background: #000000;"));
    root->addWidget(m_videoSurface, 1);

    auto* controls = new QHBoxLayout();
    controls->setSpacing(6);

    m_playPause = new QToolButton(this);
    m_playPause->setIcon(Theme::icon(QStringLiteral("pause"), Theme::colors().text, 22));
    m_playPause->setAutoRaise(true);
    connect(m_playPause, &QToolButton::clicked, this, [this]() { togglePlayPause(); });

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    controls->addWidget(m_playPause);
    controls->addStretch(1);
    controls->addWidget(closeBtn);
    root->addLayout(controls);

    // Create the playback engine for this window.
    const Settings* s = Settings::instance();
    m_engine = EngineFactory::create(m_videoSurface, s->bufferSizeMs(),
                                     s->hardwareAcceleration(), this);

    if (m_engine) {
        connect(m_engine, &IPlaybackEngine::stateChanged, this,
                [this](IPlaybackEngine::State state) {
                    m_playPause->setIcon(Theme::icon(
                        state == IPlaybackEngine::State::Playing
                            ? QStringLiteral("pause") : QStringLiteral("play"),
                        Theme::colors().text, 22));
                });
    } else {
        m_playPause->setEnabled(false);
    }
}

MiniPlayerWindow::~MiniPlayerWindow()
{
    // m_engine is a child of this window and is torn down automatically.
}

bool MiniPlayerWindow::playChannel(const Channel& channel)
{
    if (!m_engine || !channel.isValid())
        return false;

    m_channelLabel->setText(channel.displayName());
    m_channel = channel;
    m_engine->stop();
    m_engine->load(QUrl(channel.url));
    show();
    raise();
    return true;
}

void MiniPlayerWindow::togglePlayPause()
{
    if (!m_engine)
        return;
    if (m_engine->state() == IPlaybackEngine::State::Playing)
        m_engine->pause();
    else
        m_engine->play();
}
