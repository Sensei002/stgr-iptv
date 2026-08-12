#include "ui/ChannelView.h"

#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>

#include "services/FavoritesStore.h"
#include "services/LogoCache.h"
#include "ui/ChannelDelegate.h"
#include "ui/ChannelListModel.h"
#include "ui/Theme.h"

ChannelView::ChannelView(QWidget* parent)
    : QWidget(parent)
{
    m_model = new ChannelListModel(this);
    m_delegate = new ChannelDelegate(this);
    m_delegate->setMode(ChannelDelegate::Mode::Grid);

    m_list = new QListView(this);
    m_list->setModel(m_model);
    m_list->setItemDelegate(m_delegate);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setMouseTracking(true);
    m_list->setUniformItemSizes(true);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->installEventFilter(this);

    // Empty state page
    m_emptyPage = new QWidget(this);
    auto* emptyLayout = new QVBoxLayout(m_emptyPage);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(10);

    auto* emptyIcon = new QLabel(m_emptyPage);
    emptyIcon->setPixmap(Theme::icon(QStringLiteral("tv"), Theme::colors().textDim, 64).pixmap(64, 64));
    emptyIcon->setAlignment(Qt::AlignCenter);

    m_emptyTitle = new QLabel(m_emptyPage);
    m_emptyTitle->setAlignment(Qt::AlignCenter);
    m_emptyTitle->setProperty("stgrClass", QStringLiteral("pageTitle"));
    m_emptyTitle->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 600;"));

    m_emptySubtitle = new QLabel(m_emptyPage);
    m_emptySubtitle->setAlignment(Qt::AlignCenter);
    m_emptySubtitle->setProperty("stgrClass", QStringLiteral("dim"));
    m_emptySubtitle->setWordWrap(true);

    m_emptyAction = new QPushButton(m_emptyPage);
    m_emptyAction->setProperty("accent", true);
    m_emptyAction->setVisible(false);

    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(m_emptyTitle);
    emptyLayout->addWidget(m_emptySubtitle);
    emptyLayout->addWidget(m_emptyAction, 0, Qt::AlignHCenter);

    auto* stack = new QStackedLayout(this);
    stack->addWidget(m_list);
    stack->addWidget(m_emptyPage);
    stack->setCurrentWidget(m_list);

    connect(m_list, &QListView::doubleClicked, this, [this](const QModelIndex& idx) {
        onActivated(idx.row());
    });
    connect(m_list, &QWidget::customContextMenuRequested,
            this, &ChannelView::showContextMenu);
    connect(m_emptyAction, &QPushButton::clicked,
            this, &ChannelView::emptyActionRequested);

    connect(LogoCache::instance(), &LogoCache::logoReady, this, [this](const QString&) {
        m_list->viewport()->update();
    });

    // Forward "empty state" action click.
    m_emptyAction->hide();
}

void ChannelView::setChannels(const QVector<Channel>& channels)
{
    m_model->setChannels(channels);
    const bool empty = channels.isEmpty();
    static_cast<QStackedLayout*>(layout())->setCurrentWidget(empty ? m_emptyPage : m_list);
}

void ChannelView::setGridMode(bool grid)
{
    if (m_gridMode == grid)
        return;
    m_gridMode = grid;
    m_delegate->setMode(grid ? ChannelDelegate::Mode::Grid : ChannelDelegate::Mode::List);

    if (grid) {
        m_list->setViewMode(QListView::IconMode);
        m_list->setMovement(QListView::Static);
        m_list->setResizeMode(QListView::Adjust);
        m_list->setLayoutMode(QListView::Batched);
        m_list->setBatchSize(200);
        m_list->setGridSize(QSize(172, 110));
        m_list->setSpacing(4);
    } else {
        m_list->setViewMode(QListView::ListMode);
        m_list->setMovement(QListView::Static);
        m_list->setSpacing(2);
    }
}

void ChannelView::setHorizontalStrip(bool strip)
{
    if (m_horizontalStrip == strip)
        return;
    m_horizontalStrip = strip;
    m_delegate->setMode(ChannelDelegate::Mode::Grid);
    m_list->setViewMode(QListView::IconMode);
    m_list->setMovement(QListView::Static);
    m_list->setResizeMode(QListView::Adjust);
    m_list->setLayoutMode(QListView::Batched);
    m_list->setBatchSize(200);
    m_list->setGridSize(QSize(152, 106));
    m_list->setSpacing(6);
    if (strip) {
        m_list->setFlow(QListView::LeftToRight);
        m_list->setWrapping(false);
    } else {
        m_list->setFlow(QListView::TopToBottom);
        m_list->setWrapping(true);
    }
}

void ChannelView::setShowLogos(bool show)
{
    m_delegate->setShowLogos(show);
    m_list->viewport()->update();
}

void ChannelView::setFavoriteKeys(const QSet<QString>& keys)
{
    m_model->setFavoriteKeys(keys);
}

void ChannelView::setCurrentKey(const QString& key)
{
    m_model->setCurrentKey(key);
}

void ChannelView::setEmptyState(const QString& title, const QString& subtitle,
                                const QString& actionText)
{
    m_emptyTitle->setText(title);
    m_emptySubtitle->setText(subtitle);
    m_emptyAction->setText(actionText);
    m_emptyAction->setVisible(!actionText.isEmpty());
}

void ChannelView::setEmptyActionVisible(bool visible)
{
    m_emptyAction->setVisible(visible);
}

QVector<Channel> ChannelView::channels() const
{
    return m_model->channels();
}

int ChannelView::count() const
{
    return m_model->rowCount();
}

void ChannelView::onActivated(int row)
{
    const Channel ch = m_model->channelAt(row);
    if (ch.isValid())
        emit channelActivated(ch);
}

void ChannelView::showContextMenu(const QPoint& pos)
{
    const QModelIndex idx = m_list->indexAt(pos);
    if (!idx.isValid())
        return;

    const Channel ch = m_model->channelAt(idx.row());
    if (!ch.isValid())
        return;

    QMenu menu(this);
    const bool isFav = FavoritesStore::instance()->isFavorite(ch.stableKey());

    QAction* playAction = menu.addAction(Theme::icon(QStringLiteral("play"), Theme::colors().text, 16),
                                         tr("Play channel"));
    QAction* favAction = menu.addAction(Theme::icon(QStringLiteral("star"), Theme::colors().gold, 16),
                                        isFav ? tr("Remove from favorites") : tr("Add to favorites"));

    QAction* chosen = menu.exec(m_list->viewport()->mapToGlobal(pos));
    if (chosen == playAction) {
        emit channelActivated(ch);
    } else if (chosen == favAction) {
        const bool now = FavoritesStore::instance()->toggle(ch);
        emit favoriteToggled(ch, now);
    }
}

bool ChannelView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_list && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            const QModelIndex idx = m_list->currentIndex();
            if (idx.isValid())
                onActivated(idx.row());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
