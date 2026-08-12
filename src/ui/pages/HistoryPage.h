#pragma once

#include <QHash>
#include <QWidget>

#include "models/Channel.h"

class ChannelView;
class QLabel;
class QPushButton;

// ---------------------------------------------------------------------------
// HistoryPage - recently watched channels (most recent first).
// ---------------------------------------------------------------------------
class HistoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPage(QWidget* parent = nullptr);

    void setAllChannels(const QVector<Channel>& pool);
    void refresh();
    void setCurrentKey(const QString& key);

signals:
    void channelActivated(const Channel& channel, const QVector<Channel>& sourcePool);

private:
    QHash<QString, Channel> m_byKey;
    QVector<Channel> m_resolved;
    ChannelView* m_view = nullptr;
    QPushButton* m_clearButton = nullptr;
};
