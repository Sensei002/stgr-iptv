#pragma once

#include <QSet>
#include <QWidget>

#include "models/Channel.h"

class QLabel;
class QListView;
class QPushButton;
class ChannelListModel;
class ChannelDelegate;

// ---------------------------------------------------------------------------
// ChannelView - the reusable channel browser widget (grid or list).
//
// * Virtualized rendering via QListView + model (thousands of channels OK).
// * Built-in empty state with optional action button.
// * Double-click / Enter plays; right-click menu for favorite actions.
// * Hooks into LogoCache so visible logos are requested lazily.
// ---------------------------------------------------------------------------
class ChannelView : public QWidget
{
    Q_OBJECT

public:
    explicit ChannelView(QWidget* parent = nullptr);

    void setChannels(const QVector<Channel>& channels);
    void setGridMode(bool grid);
    bool isGridMode() const { return m_gridMode; }
    // Horizontal single-row strip of small cards (used on the Home screen).
    void setHorizontalStrip(bool strip);
    void setShowLogos(bool show);
    void setFavoriteKeys(const QSet<QString>& keys);
    void setCurrentKey(const QString& key);
    void setEmptyState(const QString& title, const QString& subtitle,
                       const QString& actionText = QString());
    void setEmptyActionVisible(bool visible);

    QVector<Channel> channels() const;
    int count() const;

signals:
    void channelActivated(const Channel& channel);
    void favoriteToggled(const Channel& channel, bool nowFavorite);
    // Fired when the user clicks the empty-state action button.
    void emptyActionRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void onActivated(int row);
    void showContextMenu(const QPoint& pos);

    QListView* m_list = nullptr;
    ChannelListModel* m_model = nullptr;
    ChannelDelegate* m_delegate = nullptr;
    QWidget* m_emptyPage = nullptr;
    QLabel* m_emptyTitle = nullptr;
    QLabel* m_emptySubtitle = nullptr;
    QPushButton* m_emptyAction = nullptr;
    bool m_gridMode = false;
    bool m_horizontalStrip = false;
};
