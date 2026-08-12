#pragma once

#include <QAbstractListModel>
#include <QSet>

#include "models/Channel.h"

// ---------------------------------------------------------------------------
// ChannelListModel - the model behind every channel list/grid in the app.
//
// Qt's model/view machinery virtualizes rendering, so playlists with tens of
// thousands of channels never materialize thousands of widgets.
// ---------------------------------------------------------------------------
class ChannelListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ChannelRole = Qt::UserRole + 1,
        NameRole,
        LogoUrlRole,
        CountryRole,
        CategoryRole,
        LanguageRole,
        NumberRole,
        FavoriteRole,
        IsPlayingRole,
        SubtitleRole
    };

    explicit ChannelListModel(QObject* parent = nullptr);

    void setChannels(const QVector<Channel>& channels);
    void setFavoriteKeys(const QSet<QString>& keys);
    void setCurrentKey(const QString& key);
    void clear();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    Channel channelAt(int row) const;
    int indexOfKey(const QString& key) const;
    QVector<Channel> channels() const { return m_channels; }

private:
    QVector<Channel> m_channels;
    QSet<QString> m_favorites;
    QString m_currentKey;
};
