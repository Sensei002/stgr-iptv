#pragma once

#include <QHash>
#include <QWidget>

#include "models/Channel.h"

class ChannelView;
class QLabel;

// ---------------------------------------------------------------------------
// FavoritesPage - the user's favorite channels.
//
// Favorites are stored by stable key; the page resolves them against the
// current channel pool (so a refresh or reorder of the playlist never breaks
// them). Entries whose playlist is not loaded are hidden with a hint.
// ---------------------------------------------------------------------------
class FavoritesPage : public QWidget
{
    Q_OBJECT

public:
    explicit FavoritesPage(QWidget* parent = nullptr);

    void setAllChannels(const QVector<Channel>& pool);
    void refresh();
    void setCurrentKey(const QString& key);

signals:
    void channelActivated(const Channel& channel, const QVector<Channel>& sourcePool);

private:
    QHash<QString, Channel> m_byKey;
    QVector<Channel> m_resolved;
    ChannelView* m_view = nullptr;
    QLabel* m_hint = nullptr;
};
