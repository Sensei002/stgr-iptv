#include "services/HistoryStore.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "core/AppPaths.h"

HistoryStore* HistoryStore::instance()
{
    static HistoryStore s;
    return &s;
}

HistoryStore::HistoryStore(QObject* parent)
    : QObject(parent)
    , m_file(QStringLiteral("__unset__"))
{
}

void HistoryStore::load()
{
    m_items.clear();
    m_file = AppPaths::historyFile();

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
        if (m_items.size() >= kMaxEntries)
            break;
    }
    emit historyChanged();
}

void HistoryStore::save()
{
    if (m_file == QStringLiteral("__unset__"))
        m_file = AppPaths::historyFile();

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

void HistoryStore::record(const Channel& channel)
{
    if (!channel.isValid())
        return;

    const ChannelRef ref(channel);

    // Move an existing entry to the front instead of duplicating it.
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).key == ref.key) {
            if (i == 0)
                return;
            m_items.removeAt(i);
            break;
        }
    }

    m_items.prepend(ref);
    if (m_items.size() > kMaxEntries)
        m_items.resize(kMaxEntries);

    save();
    emit historyChanged();
}

void HistoryStore::clear()
{
    m_items.clear();
    save();
    emit historyChanged();
}
