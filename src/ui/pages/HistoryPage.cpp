#include "ui/pages/HistoryPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "services/HistoryStore.h"
#include "ui/ChannelView.h"

HistoryPage::HistoryPage(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(10);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel(tr("Recently Watched"), this);
    title->setProperty("stgrClass", QStringLiteral("pageTitle"));
    m_clearButton = new QPushButton(tr("Clear History"), this);
    m_clearButton->setFlat(true);
    m_clearButton->setProperty("flat", true);
    header->addWidget(title);
    header->addStretch(1);
    header->addWidget(m_clearButton);
    root->addLayout(header);

    m_view = new ChannelView(this);
    m_view->setEmptyState(tr("Nothing watched yet"),
                          tr("Channels you watch will appear here."));
    root->addWidget(m_view, 1);

    connect(m_view, &ChannelView::channelActivated,
            this, [this](const Channel& ch) {
                emit channelActivated(ch, m_resolved);
            });
    connect(m_clearButton, &QPushButton::clicked, this, [this]() {
        HistoryStore::instance()->clear();
        refresh();
    });
}

void HistoryPage::setAllChannels(const QVector<Channel>& pool)
{
    m_byKey.clear();
    for (const Channel& c : pool)
        m_byKey.insert(c.stableKey(), c);
    refresh();
}

void HistoryPage::refresh()
{
    m_resolved.clear();
    const QVector<ChannelRef> refs = HistoryStore::instance()->history();
    for (const ChannelRef& ref : refs) {
        const auto it = m_byKey.constFind(ref.key);
        if (it != m_byKey.constEnd())
            m_resolved.append(it.value());
    }
    m_view->setChannels(m_resolved);
}

void HistoryPage::setCurrentKey(const QString& key)
{
    m_view->setCurrentKey(key);
}
