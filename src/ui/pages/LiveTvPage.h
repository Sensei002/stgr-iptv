#pragma once

#include <QSet>
#include <QWidget>

#include "models/Channel.h"

class QComboBox;
class QToolButton;
class ChannelView;
class PlayerPanel;
class PlaybackController;

// ---------------------------------------------------------------------------
// LiveTvPage - the main Live TV screen: virtualized channel grid/list on the
// left, embedded player panel on the right. Supports sorting and category
// filtering plus grid/list view switching.
// ---------------------------------------------------------------------------
class LiveTvPage : public QWidget
{
    Q_OBJECT

public:
    explicit LiveTvPage(PlaybackController* controller, QWidget* parent = nullptr);

    void setAllChannels(const QVector<Channel>& channels);
    void setCurrentKey(const QString& key);
    void setFavoriteKeys(const QSet<QString>& keys);
    void setGridMode(bool grid);
    void setShowLogos(bool show);
    void applySettings();
    void focusChannelList();

    PlayerPanel* playerPanel() const { return m_player; }
    QVector<Channel> visibleChannels() const { return m_visible; }

signals:
    void channelActivated(const Channel& channel, const QVector<Channel>& sourcePool);
    void refreshRequested();

private:
    void rebuildCategoryFilter();
    void applySortAndFilter();

    PlaybackController* m_controller = nullptr;
    ChannelView* m_view = nullptr;
    PlayerPanel* m_player = nullptr;
    QComboBox* m_sortBox = nullptr;
    QComboBox* m_categoryBox = nullptr;
    QToolButton* m_gridButton = nullptr;
    QToolButton* m_listButton = nullptr;
    QToolButton* m_refreshButton = nullptr;

    QVector<Channel> m_all;
    QVector<Channel> m_visible;
    QString m_currentKey;
    bool m_viewModeInitialized = false;
};
