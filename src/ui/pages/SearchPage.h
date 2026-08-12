#pragma once

#include <QTimer>
#include <QWidget>

#include "models/Channel.h"
#include "services/SearchIndex.h"

class ChannelView;
class QLabel;

// ---------------------------------------------------------------------------
// SearchPage - local, instant search across the whole channel pool.
//
// The SearchIndex is rebuilt whenever the pool changes; every keystroke only
// queries in-memory data (debounced by 120 ms, never a network request).
// ---------------------------------------------------------------------------
class SearchPage : public QWidget
{
    Q_OBJECT

public:
    explicit SearchPage(QWidget* parent = nullptr);

    void setAllChannels(const QVector<Channel>& channels);
    void setSearchText(const QString& text);
    void setCurrentKey(const QString& key);
    void clear();

signals:
    void channelActivated(const Channel& channel, const QVector<Channel>& sourcePool);

private:
    void performSearch();

    SearchIndex m_index;
    QVector<Channel> m_pool;
    QString m_pendingQuery;
    QTimer m_debounce;

    ChannelView* m_view = nullptr;
    QLabel* m_resultLabel = nullptr;
};
