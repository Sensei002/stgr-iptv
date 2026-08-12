#include "ui/pages/SearchPage.h"

#include <QLabel>
#include <QVBoxLayout>

#include "ui/ChannelView.h"

SearchPage::SearchPage(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(10);

    auto* title = new QLabel(tr("Search"), this);
    title->setProperty("stgrClass", QStringLiteral("pageTitle"));
    root->addWidget(title);

    m_resultLabel = new QLabel(tr("Type to search across all channels."), this);
    m_resultLabel->setProperty("stgrClass", QStringLiteral("dim"));
    root->addWidget(m_resultLabel);

    m_view = new ChannelView(this);
    m_view->setEmptyState(tr("No results"),
                          tr("Try a different search term."));
    root->addWidget(m_view, 1);

    connect(m_view, &ChannelView::channelActivated,
            this, [this](const Channel& ch) {
                emit channelActivated(ch, m_view->channels());
            });

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(120);
    connect(&m_debounce, &QTimer::timeout, this, &SearchPage::performSearch);
}

void SearchPage::setAllChannels(const QVector<Channel>& channels)
{
    m_pool = channels;
    m_index.rebuild(channels);
    performSearch();
}

void SearchPage::setSearchText(const QString& text)
{
    m_pendingQuery = text.trimmed();
    m_debounce.start();
}

void SearchPage::setCurrentKey(const QString& key)
{
    m_view->setCurrentKey(key);
}

void SearchPage::clear()
{
    m_pendingQuery.clear();
    m_debounce.stop();
    m_view->setChannels({});
    m_resultLabel->setText(tr("Type to search across all channels."));
}

void SearchPage::performSearch()
{
    const QString query = m_pendingQuery.trimmed();
    if (query.isEmpty()) {
        m_view->setChannels({});
        m_resultLabel->setText(tr("Type to search across all channels."));
        return;
    }

    const QVector<int> hits = m_index.search(query);
    QVector<Channel> results;
    results.reserve(hits.size());
    for (int idx : hits)
        results.append(m_pool.at(idx));

    m_view->setChannels(results);
    m_resultLabel->setText(results.isEmpty()
                               ? tr("No results for \u201c%1\u201d").arg(query)
                               : tr("%1 result(s) for \u201c%2\u201d").arg(results.size()).arg(query));
}
