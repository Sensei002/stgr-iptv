#pragma once

#include <QObject>
#include <QSet>
#include <QVector>

#include "models/Channel.h"

// ---------------------------------------------------------------------------
// FavoritesStore - persisted favorite channels.
//
// Favorites survive restarts, playlist refreshes and reorders because they
// are keyed by Channel::stableKey() (a hash over playlistId+name+url), never
// by array position. A ChannelRef snapshot keeps the name/logo available even
// when the source playlist is not loaded.
// ---------------------------------------------------------------------------
class FavoritesStore : public QObject
{
    Q_OBJECT

public:
    static FavoritesStore* instance();

    void load();
    void save();

    QVector<ChannelRef> favorites() const { return m_items; }
    QSet<QString> keys() const { return m_keys; }
    bool isFavorite(const QString& stableKey) const { return m_keys.contains(stableKey); }

    bool toggle(const Channel& channel); // returns the new favorite state
    void add(const Channel& channel);
    void remove(const QString& stableKey);
    void clear();

signals:
    void favoritesChanged();

private:
    explicit FavoritesStore(QObject* parent = nullptr);

    QVector<ChannelRef> m_items;
    QSet<QString> m_keys;
    QString m_file;
};
