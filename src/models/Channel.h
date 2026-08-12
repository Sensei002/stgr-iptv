#pragma once

#include <QCryptographicHash>
#include <QMetaType>
#include <QString>
#include <QStringList>

// ---------------------------------------------------------------------------
// Channel - one playable channel parsed from an M3U playlist.
//
// Identity: stableKey() is a hash over (playlistId, name, url). It is used to
// persist favorites and history so they survive playlist refreshes, reorders
// and renames (playlistId is stable; array indexes are not).
// ---------------------------------------------------------------------------
struct Channel {
    QString name;          // display name
    QString url;           // stream URL (absolute, validated)
    QString logo;          // logo URL as written in the playlist
    QString id;            // tvg-id
    QString group;         // group-title / category
    QString country;       // tvg-country
    QString language;      // tvg-language
    QString network;       // tvg-network (may be empty)
    int     number = 0;    // tvg-chno, falls back to position in playlist
    QString playlistId;    // owning playlist
    QString playlistName;  // owning playlist display name (snapshot)
    bool    isFavorite = false;

    bool isValid() const { return !name.isEmpty() && !url.isEmpty(); }

    QString displayName() const
    {
        QString n = name.trimmed();
        if (n.isEmpty())
            n = url;
        return n;
    }

    QString lowerName() const { return displayName().toLower(); }

    // Category preference: group-title wins, then tvg-category.
    QString category() const { return group; }

    // Tags used by the search index: name + country + language + category +
    // network, lowercased.
    QString searchText() const
    {
        QStringList parts;
        parts << displayName();
        if (!country.isEmpty()) parts << country;
        if (!language.isEmpty()) parts << language;
        if (!category().isEmpty()) parts << category();
        if (!group.isEmpty()) parts << group;
        if (!network.isEmpty()) parts << network;
        if (!id.isEmpty()) parts << id;
        return parts.join(QLatin1Char(' ')).toLower();
    }

    // Deterministic identity that survives playlist refresh/reorder.
    QString stableKey() const
    {
        const QString raw = playlistId + QLatin1Char('\n')
            + name + QLatin1Char('\n')
            + url;
        return QString::fromLatin1(QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Md5).toHex());
    }

    bool operator==(const Channel& other) const
    {
        return stableKey() == other.stableKey();
    }
};

// -- snapshot stored with favorites/history (works without the playlist) -----
Q_DECLARE_METATYPE(Channel)

struct ChannelRef {
    QString key;         // stableKey()
    QString name;
    QString logo;
    QString playlistId;
    QString playlistName;

    ChannelRef() = default;
    explicit ChannelRef(const Channel& c)
        : key(c.stableKey())
        , name(c.displayName())
        , logo(c.logo)
        , playlistId(c.playlistId)
        , playlistName(c.playlistName)
    {}
};
