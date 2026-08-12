#include "ui/ChannelListModel.h"

ChannelListModel::ChannelListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void ChannelListModel::setChannels(const QVector<Channel>& channels)
{
    beginResetModel();
    m_channels = channels;
    endResetModel();
}

void ChannelListModel::setFavoriteKeys(const QSet<QString>& keys)
{
    if (m_favorites == keys)
        return;
    m_favorites = keys;
    if (!m_channels.isEmpty()) {
        emit dataChanged(index(0, 0),
                         index(m_channels.size() - 1, 0),
                         { FavoriteRole });
    }
}

void ChannelListModel::setCurrentKey(const QString& key)
{
    if (m_currentKey == key)
        return;
    const int oldIdx = indexOfKey(m_currentKey);
    const int newIdx = indexOfKey(key);
    m_currentKey = key;
    if (oldIdx >= 0)
        emit dataChanged(index(oldIdx, 0), index(oldIdx, 0), { IsPlayingRole });
    if (newIdx >= 0 && newIdx != oldIdx)
        emit dataChanged(index(newIdx, 0), index(newIdx, 0), { IsPlayingRole });
}

void ChannelListModel::clear()
{
    beginResetModel();
    m_channels.clear();
    endResetModel();
}

int ChannelListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_channels.size();
}

QVariant ChannelListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_channels.size())
        return {};

    const Channel& c = m_channels.at(index.row());
    switch (role) {
    case ChannelRole:
        return QVariant::fromValue(c);
    case NameRole:
        return c.displayName();
    case LogoUrlRole:
        return c.logo;
    case CountryRole:
        return c.country;
    case CategoryRole:
        return c.category();
    case LanguageRole:
        return c.language;
    case NumberRole:
        return c.number;
    case FavoriteRole:
        return m_favorites.contains(c.stableKey());
    case IsPlayingRole:
        return !m_currentKey.isEmpty() && c.stableKey() == m_currentKey;
    case SubtitleRole: {
        QStringList parts;
        if (!c.country.isEmpty()) parts << c.country;
        if (!c.category().isEmpty()) parts << c.category();
        if (!c.language.isEmpty()) parts << c.language;
        return parts.join(QStringLiteral("  \u00b7  "));
    }
    default:
        return {};
    }
}

Channel ChannelListModel::channelAt(int row) const
{
    if (row < 0 || row >= m_channels.size())
        return {};
    return m_channels.at(row);
}

int ChannelListModel::indexOfKey(const QString& key) const
{
    for (int i = 0; i < m_channels.size(); ++i) {
        if (m_channels.at(i).stableKey() == key)
            return i;
    }
    return -1;
}
