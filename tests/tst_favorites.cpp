#include <QtTest>

#include <QCoreApplication>
#include <QTemporaryDir>

#include "core/AppPaths.h"
#include "models/Channel.h"
#include "services/FavoritesStore.h"

namespace {
Channel makeChannel(const QString& name, const QString& url, const QString& playlistId)
{
    Channel c;
    c.name = name;
    c.url = url;
    c.playlistId = playlistId;
    return c;
}
} // namespace

class FavoritesStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void addAndToggle();
    void persistsAcrossRestart();
    void survivesReorder();
    void removeAndClear();

private:
    QTemporaryDir m_dir; // outlives every test body, like tst_playlistmanager
};

void FavoritesStoreTest::init()
{
    // A local QTemporaryDir here would be destroyed when init() returns,
    // deleting the storage the test bodies write to.
    if (!m_dir.isValid())
        m_dir = QTemporaryDir();
    QVERIFY(m_dir.isValid());
    AppPaths::setAppDataDirOverride(m_dir.path());
    QVERIFY(AppPaths::ensureDirs());

    FavoritesStore* store = FavoritesStore::instance();
    store->clear();
    store->load();
}

void FavoritesStoreTest::addAndToggle()
{
    FavoritesStore* store = FavoritesStore::instance();
    const Channel c = makeChannel(QStringLiteral("BBC News"),
                                  QStringLiteral("http://host/bbc"), QStringLiteral("p1"));

    QVERIFY(!store->isFavorite(c.stableKey()));
    QVERIFY(store->toggle(c));            // now favorite
    QVERIFY(store->isFavorite(c.stableKey()));
    QVERIFY(!store->toggle(c));           // removed again
    QVERIFY(!store->isFavorite(c.stableKey()));
}

void FavoritesStoreTest::persistsAcrossRestart()
{
    FavoritesStore* store = FavoritesStore::instance();
    const Channel c = makeChannel(QStringLiteral("NHK World"),
                                  QStringLiteral("http://host/nhk"), QStringLiteral("p1"));
    store->add(c);
    QCOMPARE(store->favorites().size(), 1);

    // Simulate a restart: reload from disk.
    store->load();
    QCOMPARE(store->favorites().size(), 1);
    QCOMPARE(store->favorites().first().name, QStringLiteral("NHK World"));
    QVERIFY(store->isFavorite(c.stableKey()));
}

void FavoritesStoreTest::survivesReorder()
{
    FavoritesStore* store = FavoritesStore::instance();

    // The same channel in a different position still has the same key.
    const Channel c1 = makeChannel(QStringLiteral("Al Jazeera"),
                                   QStringLiteral("http://host/aj"), QStringLiteral("p1"));
    store->add(c1);

    Channel c2 = c1;
    c2.number = 999; // playlist order changed
    QCOMPARE(c1.stableKey(), c2.stableKey());
    QVERIFY(store->isFavorite(c2.stableKey()));

    Channel c3 = c1;
    c3.url = QStringLiteral("http://host/aj-new"); // stream changed
    QVERIFY(c1.stableKey() != c3.stableKey());
    QVERIFY(!store->isFavorite(c3.stableKey()));
}

void FavoritesStoreTest::removeAndClear()
{
    FavoritesStore* store = FavoritesStore::instance();
    store->add(makeChannel(QStringLiteral("DW"), QStringLiteral("http://host/dw"), QStringLiteral("p1")));
    store->add(makeChannel(QStringLiteral("RT"), QStringLiteral("http://host/rt"), QStringLiteral("p1")));
    QCOMPARE(store->favorites().size(), 2);

    store->remove(store->favorites().first().key);
    QCOMPARE(store->favorites().size(), 1);

    store->clear();
    QVERIFY(store->favorites().isEmpty());
}

QTEST_GUILESS_MAIN(FavoritesStoreTest)
#include "tst_favorites.moc"
