#include "models/Playlist.h"

#include <QJsonArray>
#include <QUuid>

QJsonObject playlistToJson(const Playlist& p)
{
    QJsonObject o;
    o.insert(QStringLiteral("id"), p.id);
    o.insert(QStringLiteral("name"), p.name);
    o.insert(QStringLiteral("url"), p.url);
    o.insert(QStringLiteral("epgUrl"), p.epgUrl);
    o.insert(QStringLiteral("enabled"), p.enabled);
    o.insert(QStringLiteral("builtIn"), p.builtIn);
    o.insert(QStringLiteral("lastUpdated"), p.lastUpdated.toString(Qt::ISODate));
    o.insert(QStringLiteral("channelCount"), p.channelCount);
    o.insert(QStringLiteral("errorMessage"), p.errorMessage);
    return o;
}

Playlist playlistFromJson(const QJsonObject& o)
{
    Playlist p;
    p.id = o.value(QStringLiteral("id")).toString();
    p.name = o.value(QStringLiteral("name")).toString();
    p.url = o.value(QStringLiteral("url")).toString();
    p.epgUrl = o.value(QStringLiteral("epgUrl")).toString();
    p.enabled = o.value(QStringLiteral("enabled")).toBool(true);
    p.builtIn = o.value(QStringLiteral("builtIn")).toBool(false);
    p.lastUpdated = QDateTime::fromString(o.value(QStringLiteral("lastUpdated")).toString(), Qt::ISODate);
    p.channelCount = o.value(QStringLiteral("channelCount")).toInt(0);
    p.errorMessage = o.value(QStringLiteral("errorMessage")).toString();

    if (p.id.isEmpty())
        p.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return p;
}
