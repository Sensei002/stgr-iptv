#pragma once

#include <QObject>
#include <QVector>

#include "models/Channel.h"

// ---------------------------------------------------------------------------
// HistoryStore - recently watched channels, most recent first, capped at
// kMaxEntries. Persisted to <appdata>/history/history.json and keyed by the
// same stable identity used by favorites.
// ---------------------------------------------------------------------------
class HistoryStore : public QObject
{
    Q_OBJECT

public:
    static HistoryStore* instance();

    void load();
    void save();

    QVector<ChannelRef> history() const { return m_items; }
    void record(const Channel& channel);
    void clear();

    static constexpr int kMaxEntries = 50;

signals:
    void historyChanged();

private:
    explicit HistoryStore(QObject* parent = nullptr);

    QVector<ChannelRef> m_items;
    QString m_file;
};
