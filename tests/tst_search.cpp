#include <QtTest>

#include "services/SearchIndex.h"

namespace {

QVector<Channel> makePool()
{
    QVector<Channel> pool;

    Channel bbc;
    bbc.name = QStringLiteral("BBC News");
    bbc.country = QStringLiteral("GB");
    bbc.language = QStringLiteral("eng");
    bbc.group = QStringLiteral("News");
    bbc.network = QStringLiteral("BBC Network"); // tokens used by searchesNetwork()
    bbc.url = QStringLiteral("http://host/bbc");
    bbc.playlistId = QStringLiteral("p1");

    Channel cnn;
    cnn.name = QStringLiteral("CNN International");
    cnn.country = QStringLiteral("US");
    cnn.language = QStringLiteral("eng");
    cnn.group = QStringLiteral("News");
    cnn.url = QStringLiteral("http://host/cnn");
    cnn.playlistId = QStringLiteral("p1");

    Channel sputnik;
    sputnik.name = QStringLiteral("Спутник");
    sputnik.country = QStringLiteral("RU");
    sputnik.language = QStringLiteral("rus");
    sputnik.group = QStringLiteral("News");
    sputnik.url = QStringLiteral("http://host/sp");
    sputnik.playlistId = QStringLiteral("p1");

    Channel sport;
    sport.name = QStringLiteral("Sky Sports");
    sport.country = QStringLiteral("GB");
    sport.group = QStringLiteral("Sports");
    sport.url = QStringLiteral("http://host/sky");
    sport.playlistId = QStringLiteral("p1");

    pool << bbc << cnn << sputnik << sport;
    return pool;
}

} // namespace

class SearchIndexTest : public QObject
{
    Q_OBJECT

private slots:
    void exactMatch();
    void partialMatch();
    void caseInsensitive();
    void searchesCountry();
    void searchesCategory();
    void searchesNetwork();
    void multiTokenQuery();
    void emptyQueryReturnsNothing();
    void rankByNameFirst();
};

void SearchIndexTest::exactMatch()
{
    SearchIndex index;
    index.rebuild(makePool());
    const auto hits = index.search(QStringLiteral("BBC News"));
    QCOMPARE(hits.size(), 1);
    QCOMPARE(makePool().at(hits.first()).name, QStringLiteral("BBC News"));
}

void SearchIndexTest::partialMatch()
{
    SearchIndex index;
    index.rebuild(makePool());
    const auto hits = index.search(QStringLiteral("bbc"));
    QCOMPARE(hits.size(), 1);
}

void SearchIndexTest::caseInsensitive()
{
    SearchIndex index;
    index.rebuild(makePool());
    QCOMPARE(index.search(QStringLiteral("CNN INTERNAtionaL")).size(), 1);
}

void SearchIndexTest::searchesCountry()
{
    SearchIndex index;
    index.rebuild(makePool());
    const auto hits = index.search(QStringLiteral("GB"));
    QCOMPARE(hits.size(), 2); // BBC News + Sky Sports
}

void SearchIndexTest::searchesCategory()
{
    SearchIndex index;
    index.rebuild(makePool());
    const auto hits = index.search(QStringLiteral("sports"));
    QCOMPARE(hits.size(), 1);
    QCOMPARE(makePool().at(hits.first()).name, QStringLiteral("Sky Sports"));
}

void SearchIndexTest::searchesNetwork()
{
    SearchIndex index;
    index.rebuild(makePool());
    const auto hits = index.search(QStringLiteral("bbc network"));
    QCOMPARE(hits.size(), 1);
}

void SearchIndexTest::multiTokenQuery()
{
    SearchIndex index;
    index.rebuild(makePool());
    const auto hits = index.search(QStringLiteral("sky sports"));
    QCOMPARE(hits.size(), 1);
    QCOMPARE(makePool().at(hits.first()).name, QStringLiteral("Sky Sports"));

    QVERIFY(index.search(QStringLiteral("bbc sports")).isEmpty());
}

void SearchIndexTest::emptyQueryReturnsNothing()
{
    SearchIndex index;
    index.rebuild(makePool());
    QVERIFY(index.search(QString()).isEmpty());
    QVERIFY(index.search(QStringLiteral("   ")).isEmpty());
}

void SearchIndexTest::rankByNameFirst()
{
    SearchIndex index;
    index.rebuild(makePool());
    const auto hits = index.search(QStringLiteral("sport"));
    QVERIFY(!hits.isEmpty());
    // "Sky Sports" matches on name and should outrank any metadata match.
    QCOMPARE(makePool().at(hits.first()).name, QStringLiteral("Sky Sports"));
}

QTEST_GUILESS_MAIN(SearchIndexTest)
#include "tst_search.moc"
