#include <QtTest>

#include "playlist/M3uParser.h"

class M3uParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesValidM3u();
    void resolvesRelativeUrls();
    void handlesMalformedLines();
    void handlesUtf8();
    void deduplicatesChannels();
    void handlesMissingMetadata();
    void handlesQuotedEntries();
    void handlesCrlfAndBom();
    void keepsQueryParameters();
    void handlesCommasInQuotedAttributes();
    void rejectsUnsupportedSchemes();
    void capsHugePlaylists();
    void entryWithoutUrlIsDropped();
};

void M3uParserTest::parsesValidM3u()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1 tvg-id=\"arirang\" tvg-name=\"Arirang TV\" tvg-logo=\"http://logos/arirang.png\" group-title=\"General\",Arirang TV\n"
        "http://stream/arirang.m3u8\n"
        "#EXTINF:0 tvg-id=\"bbc-news\" tvg-name=\"BBC News\" group-title=\"News\",BBC News\n"
        "https://stream/bbc.m3u8\n"
        "#EXTINF:-1 tvg-id=\"nasa\" tvg-name=\"NASA TV\" tvg-chno=\"42\",NASA TV\n"
        "https://stream/nasa.ts\n";

    const auto result = M3uParser::parse(data, QUrl(QStringLiteral("https://playlist.example/index.m3u")),
                                         QStringLiteral("p1"), QStringLiteral("Test"));

    QCOMPARE(result.channels.size(), 3);
    QCOMPARE(result.warnings, 0);

    const Channel& a = result.channels.at(0);
    QCOMPARE(a.name, QStringLiteral("Arirang TV"));
    QCOMPARE(a.id, QStringLiteral("arirang"));
    QCOMPARE(a.logo, QStringLiteral("http://logos/arirang.png"));
    QCOMPARE(a.group, QStringLiteral("General"));
    QCOMPARE(a.url, QStringLiteral("http://stream/arirang.m3u8"));
    QCOMPARE(a.number, 1);
    QCOMPARE(a.playlistId, QStringLiteral("p1"));
    QVERIFY(a.isValid());

    const Channel& b = result.channels.at(1);
    QCOMPARE(b.name, QStringLiteral("BBC News"));
    QCOMPARE(b.category(), QStringLiteral("News"));
    QCOMPARE(b.number, 2);

    const Channel& c = result.channels.at(2);
    QCOMPARE(c.name, QStringLiteral("NASA TV"));
    QCOMPARE(c.number, 42); // tvg-chno wins
}

void M3uParserTest::resolvesRelativeUrls()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1,Rel\n"
        "live/chan.ts\n"
        "#EXTINF:-1,Abs\n"
        "https://other.example/x.m3u8\n";

    const auto result = M3uParser::parse(data,
                                         QUrl(QStringLiteral("https://host.example/dir/list.m3u")),
                                         QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 2);
    QCOMPARE(result.channels.at(0).url, QStringLiteral("https://host.example/dir/live/chan.ts"));
    QCOMPARE(result.channels.at(1).url, QStringLiteral("https://other.example/x.m3u8"));
}

void M3uParserTest::handlesMalformedLines()
{
    const QByteArray data =
        "#EXTM3U\n"
        "this is not a url at all\n"
        "#EXTINF:-1 tvg-id=\"unclosed, nothing after\n"
        "#EXTINF:0 group-title=\"News\",Good Channel\n"
        "https://host/good.ts\n"
        "garbage-no-scheme\n"
        "# just a comment\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 1);
    QCOMPARE(result.channels.at(0).name, QStringLiteral("Good Channel"));
    QVERIFY(result.warnings >= 2);
}

void M3uParserTest::handlesUtf8()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1 tvg-name=\"Спорт ТВ\",Спорт ТВ\n"
        "http://host/ru.ts\n"
        "#EXTINF:-1,日本チャンネル\n"
        "http://host/jp.ts\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 2);
    QCOMPARE(result.channels.at(0).name, QString::fromUtf8("Спорт ТВ"));
    QCOMPARE(result.channels.at(1).name, QString::fromUtf8("日本チャンネル"));
}

void M3uParserTest::deduplicatesChannels()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1,First\n"
        "http://host/same.ts\n"
        "#EXTINF:-1,Second (duplicate url)\n"
        "http://host/same.ts\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 1);
    QCOMPARE(result.channels.at(0).name, QStringLiteral("First"));
    QVERIFY(result.warnings >= 1);
}

void M3uParserTest::handlesMissingMetadata()
{
    const QByteArray data =
        "#EXTM3U\n"
        "http://host/plain.ts\n"
        "#EXTINF:-1 tvg-name=\"Named\",\n"
        "http://host/named.ts\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 2);
    QCOMPARE(result.channels.at(0).name, QStringLiteral("http://host/plain.ts")); // derived
    QVERIFY(result.channels.at(0).isValid());
    QCOMPARE(result.channels.at(1).name, QStringLiteral("Named")); // tvg-name fallback
}

void M3uParserTest::handlesQuotedEntries()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1,\"Quoted Channel\"\n"
        "\"http://host/quote.ts\"\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 1);
    QCOMPARE(result.channels.at(0).name, QStringLiteral("Quoted Channel"));
    QCOMPARE(result.channels.at(0).url, QStringLiteral("http://host/quote.ts"));
}

void M3uParserTest::handlesCrlfAndBom()
{
    const QByteArray data = QByteArray("\xEF\xBB\xBF")
        + "#EXTM3U\r\n"
          "#EXTINF:-1,One\r\n"
          "http://host/one.ts\r\n"
          "#EXTINF:-1,Two\r"
          "http://host/two.ts\r\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 2);
    QCOMPARE(result.channels.at(0).name, QStringLiteral("One"));
    QCOMPARE(result.channels.at(1).name, QStringLiteral("Two"));
}

void M3uParserTest::keepsQueryParameters()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1,Tokened\n"
        "http://host/live/stream.m3u8?token=abc123&x=1\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 1);
    QCOMPARE(result.channels.at(0).url,
             QStringLiteral("http://host/live/stream.m3u8?token=abc123&x=1"));
}

void M3uParserTest::handlesCommasInQuotedAttributes()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1 tvg-id=\"x\" group-title=\"News, Sport\",Channel One\n"
        "http://host/one.ts\n"
        "#EXTINF:-1 tvg-id=\"y\",Title, With Comma\n"
        "http://host/two.ts\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 2);
    QCOMPARE(result.channels.at(0).group, QStringLiteral("News, Sport"));
    QCOMPARE(result.channels.at(0).name, QStringLiteral("Channel One"));
    QCOMPARE(result.channels.at(1).name, QStringLiteral("Title, With Comma"));
}

void M3uParserTest::rejectsUnsupportedSchemes()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1,Js\n"
        "javascript:alert(1)\n"
        "#EXTINF:-1,Data\n"
        "data:text/plain;base64,SGVsbG8=\n"
        "#EXTINF:-1,Mail\n"
        "mailto:someone@example.com\n"
        "#EXTINF:-1,Good\n"
        "https://host/good.ts\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 1);
    QCOMPARE(result.channels.at(0).name, QStringLiteral("Good"));
    QVERIFY(result.warnings >= 3);
}

void M3uParserTest::capsHugePlaylists()
{
    QByteArray data = "#EXTM3U\n";
    for (int i = 0; i < 2000; ++i) {
        data += QByteArray("#EXTINF:-1,Channel ") + QByteArray::number(i) + "\n"
                + "http://host/ch" + QByteArray::number(i) + ".ts\n";
    }

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"), 100);

    QCOMPARE(result.channels.size(), 100);
    QCOMPARE(result.truncated, 100);
}

void M3uParserTest::entryWithoutUrlIsDropped()
{
    const QByteArray data =
        "#EXTM3U\n"
        "#EXTINF:-1,Orphan entry with no stream url\n"
        "#EXTINF:-1,Second orphan\n";

    const auto result = M3uParser::parse(data, QUrl(), QStringLiteral("p"), QStringLiteral("P"));

    QCOMPARE(result.channels.size(), 0);
    QVERIFY(result.warnings >= 1);
}

QTEST_GUILESS_MAIN(M3uParserTest)
#include "tst_m3uparser.moc"
