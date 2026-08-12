#include "services/SearchIndex.h"

#include <QSet>

#include <algorithm>

#include "utils/StringUtils.h"

void SearchIndex::rebuild(const QVector<Channel>& channels)
{
    m_channels = channels;
    m_texts.clear();
    m_texts.reserve(channels.size());
    for (const Channel& c : channels)
        m_texts.append(c.searchText());
}

void SearchIndex::clear()
{
    m_channels.clear();
    m_texts.clear();
}

QVector<int> SearchIndex::search(const QString& query, int maxResults) const
{
    const QStringList tokens = StringUtils::tokenize(query);
    if (tokens.isEmpty() || m_channels.isEmpty())
        return {};

    // First pass: collect every match (cheap substring checks).
    QVector<int> matches;
    for (int i = 0; i < m_channels.size(); ++i) {
        if (StringUtils::matchesAllTokens(m_texts.at(i), tokens))
            matches.append(i);
    }
    if (matches.isEmpty())
        return matches;

    // Second pass: rank. Channel names that start with the query rank first,
    // then names containing it, then metadata-only matches.
    const QString queryLower = query.trimmed().toLower();
    auto score = [this, &queryLower](int i) {
        const QString name = m_channels.at(i).lowerName();
        if (name.startsWith(queryLower))
            return 3;
        if (name.contains(queryLower))
            return 2;
        return 1;
    };

    std::sort(matches.begin(), matches.end(),
              [this, &score](int a, int b) {
                  const int sa = score(a);
                  const int sb = score(b);
                  if (sa != sb)
                      return sa > sb;
                  return a < b; // stable fallback: original order
              });

    if (matches.size() > maxResults)
        matches.resize(maxResults);
    return matches;
}
