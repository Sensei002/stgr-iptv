#include <QtTest>

#include <QCoreApplication>
#include <QTemporaryDir>

#include "core/AppPaths.h"
#include "services/HistoryStore.h"

namespace {
Channel makeChannel(const QString& name, int id)
{
    Channel c;
    c.name = name;
    c.url = QStringLiteral("http://host/%1").arg(id);
    c.playlistId = QStringLiteral("p1");
    return c;
}
} // namespace

class HistoryStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void recordsMostRecentFirst();
    void deduplicates();
    void capsAtMaxEntries();
    void clearResets();
    void persists();

private:
    QTemporaryDir m_dir; // outlives every test body, like tst_playlistmanager
};

void HistoryStoreTest::init()
{
    // A local QTemporaryDir here would be destroyed when init() returns,
    // deleting the storage the test bodies write to.
    if (!m_dir.isValid())
        m_dir = QTemporaryDir();
    QVERIFY(m_dir.isValid());
    AppPaths::setAppDataDirOverride(m_dir.path());
    QVERIFY(AppPaths::ensureDirs());

    HistoryStore::instance()->clear();
    HistoryStore::instance()->load();
}

void HistoryStoreTest::recordsMostRecentFirst()
{
    HistoryStore* store = HistoryStore::instance();
    store->record(makeChannel(QStringLiteral("A"), 1));
    store->record(makeChannel(QStringLiteral("B"), 2));
    store->record(makeChannel(QStringLiteral("C"), 3));

    QCOMPARE(store->history().size(), 3);
    QCOMPARE(store->history().at(0).name, QStringLiteral("C"));
    QCOMPARE(store->history().at(2).name, QStringLiteral("A"));
}

void HistoryStoreTest::deduplicates()
{
    HistoryStore* store = HistoryStore::instance();
    store->record(makeChannel(QStringLiteral("X"), 9));
    store->record(makeChannel(QStringLiteral("Y"), 8));
    store->record(makeChannel(QStringLiteral("X"), 9)); // already watched

    QCOMPARE(store->history().size(), 2);
    QCOMPARE(store->history().at(0).name, QStringLiteral("X")); // moved to front
}

void HistoryStoreTest::capsAtMaxEntries()
{
    HistoryStore* store = HistoryStore::instance();
    for (int i = 0; i < HistoryStore::kMaxEntries + 20; ++i)
        store->record(makeChannel(QStringLiteral("CH%1").arg(i), i));

    QCOMPARE(store->history().size(), HistoryStore::kMaxEntries);
    QCOMPARE(store->history().first().name, QStringLiteral("CH%1").arg(HistoryStore::kMaxEntries + 19));
}

void HistoryStoreTest::clearResets()
{
    HistoryStore* store = HistoryStore::instance();
    store->record(makeChannel(QStringLiteral("Z"), 1));
    store->clear();
    QVERIFY(store->history().isEmpty());
}

void HistoryStoreTest::persists()
{
    HistoryStore* store = HistoryStore::instance();
    store->record(makeChannel(QStringLiteral("Persisted"), 42));
    QCOMPARE(store->history().size(), 1);

    store->load();
    QCOMPARE(store->history().size(), 1);
    QCOMPARE(store->history().first().name, QStringLiteral("Persisted"));
}

QTEST_GUILESS_MAIN(HistoryStoreTest)
#include "tst_history.moc"
