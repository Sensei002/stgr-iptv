#include "services/FavoritesStore.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "core/AppPaths.h"

FavoritesStore* FavoritesStore::instance()
{
    static FavoritesStore s;
    return &s;
}

FavoritesStore::FavoritesStore(QObject* parent)
    : QObject(parent)
    , m_file(QStringLiteral("__unset__"))
{
}

void FavoritesStore::load()
{
    m_items.clear();
    m_keys.clear();
    m_file = AppPaths::settingsDir() + QStringLiteral("/favorites.json");

    QFile f(m_file);
    if (!f.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray())
        return;

    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        ChannelRef ref;
        ref.key = o.value(QStringLiteral("key")).toString();
        ref.name = o.value(QStringLiteral("name")).toString();
        ref.logo = o.value(QStringLiteral("logo")).toString();
        ref.playlistId = o.value(QStringLiteral("playlistId")).toString();
        ref.playlistName = o.value(QStringLiteral("playlistName")).toString();
        if (ref.key.isEmpty())
            continue;
        m_items.append(ref);
        m_keys.insert(ref.key);
    }
    emit favoritesChanged();
}

void FavoritesStore::save()
{
    if (m_file == QStringLiteral("__unset__"))
        m_file = AppPaths::settingsDir() + QStringLiteral("/favorites.json");

    QJsonArray arr;
    for (const ChannelRef& ref : m_items) {
        QJsonObject o;
        o.insert(QStringLiteral("key"), ref.key);
        o.insert(QStringLiteral("name"), ref.name);
        o.insert(QStringLiteral("logo"), ref.logo);
        o.insert(QStringLiteral("playlistId"), ref.playlistId);
        o.insert(QStringLiteral("playlistName"), ref.playlistName);
        arr.append(o);
    }

    QSaveFile out(m_file);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        out.commit();
    }
}

void FavoritesStore::add(const Channel& channel)
{
    const ChannelRef ref(channel);
    if (m_keys.contains(ref.key))
        return;
    m_items.prepend(ref);
    m_keys.insert(ref.key);
    save();
    emit favoritesChanged();
}

void FavoritesStore::remove(const QString& stableKey)
{
    if (!m_keys.contains(stableKey))
        return;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).key == stableKey) {
            m_items.removeAt(i);
            break;
        }
    }
    m_keys.remove(stableKey);
    save();
    emit favoritesChanged();
}

bool FavoritesStore::toggle(const Channel& channel)
{
    const ChannelRef ref(channel);
    if (m_keys.contains(ref.key)) {
        remove(ref.key);
        return false;
    }
    add(channel);
    return true;
}

void FavoritesStore::clear()
{
    m_items.clear();
    m_keys.clear();
    save();
    emit favoritesChanged();
}
