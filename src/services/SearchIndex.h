#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include "models/Channel.h"

// ---------------------------------------------------------------------------
// SearchIndex - local, in-memory search over the full channel pool.
//
// Built once per pool update; every keystroke afterwards only touches memory
// (no network). Matching is case-insensitive over name, country, language,
// category/group, network and tvg-id. Results are ranked by how strongly the
// channel name matches and returned as indexes into the source vector.
// ---------------------------------------------------------------------------
class SearchIndex
{
public:
    void rebuild(const QVector<Channel>& channels);
    void clear();

    int count() const { return m_channels.size(); }

    // Returns up to maxResults matching channel indexes, best matches first.
    QVector<int> search(const QString& query, int maxResults = 1000) const;

private:
    QVector<Channel> m_channels;
    QStringList m_texts; // lowercased searchText() per channel
};
