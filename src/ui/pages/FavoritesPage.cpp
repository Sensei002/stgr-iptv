#include "ui/pages/FavoritesPage.h"

#include <QLabel>
#include <QVBoxLayout>

#include "services/FavoritesStore.h"
#include "ui/ChannelView.h"

FavoritesPage::FavoritesPage(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(10);

    auto* title = new QLabel(tr("Favorites"), this);
    title->setProperty("stgrClass", QStringLiteral("pageTitle"));
    root->addWidget(title);

    m_hint = new QLabel(this);
    m_hint->setProperty("stgrClass", QStringLiteral("dim"));
    m_hint->setVisible(false);
    root->addWidget(m_hint);

    m_view = new ChannelView(this);
    m_view->setEmptyState(tr("No favorites yet"),
                          tr("Tap the star on any channel to pin it here."));
    root->addWidget(m_view, 1);

    connect(m_view, &ChannelView::channelActivated,
            this, [this](const Channel& ch) {
                emit channelActivated(ch, m_resolved);
            });
    connect(m_view, &ChannelView::favoriteToggled, this,
            [this](const Channel&, bool) { refresh(); });
}

void FavoritesPage::setAllChannels(const QVector<Channel>& pool)
{
    m_byKey.clear();
    for (const Channel& c : pool)
        m_byKey.insert(c.stableKey(), c);
    refresh();
}

void FavoritesPage::refresh()
{
    m_resolved.clear();
    int missing = 0;

    const QVector<ChannelRef> refs = FavoritesStore::instance()->favorites();
    for (const ChannelRef& ref : refs) {
        const auto it = m_byKey.constFind(ref.key);
        if (it != m_byKey.constEnd()) {
            m_resolved.append(it.value());
        } else {
            ++missing;
        }
    }

    m_view->setChannels(m_resolved);
    m_view->setFavoriteKeys(FavoritesStore::instance()->keys());
    m_hint->setVisible(missing > 0);
    if (missing > 0)
        m_hint->setText(tr("%1 favorite(s) are waiting for their playlist to be loaded.").arg(missing));
}

void FavoritesPage::setCurrentKey(const QString& key)
{
    m_view->setCurrentKey(key);
}
