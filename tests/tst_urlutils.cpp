#include <QtTest>

#include "utils/UrlUtils.h"

class UrlUtilsTest : public QObject
{
    Q_OBJECT

private slots:
    void acceptsSupportedSchemes();
    void rejectsUnsupportedSchemes();
    void handlesQuotesAndWhitespace();
    void resolvesRelativeUrls();
    void redactsCredentials();
};

void UrlUtilsTest::acceptsSupportedSchemes()
{
    QVERIFY(UrlUtils::isSupportedStreamUrl(QStringLiteral("http://host/stream.ts")));
    QVERIFY(UrlUtils::isSupportedStreamUrl(QStringLiteral("https://host/live.m3u8")));
    QVERIFY(UrlUtils::isSupportedStreamUrl(QStringLiteral("rtsp://host/channel")));
    QVERIFY(UrlUtils::isSupportedStreamUrl(QStringLiteral("udp://@239.0.0.1:1234")));
    QVERIFY(UrlUtils::isSupportedStreamUrl(QStringLiteral("rtmp://host/app/stream")));
}

void UrlUtilsTest::rejectsUnsupportedSchemes()
{
    QVERIFY(!UrlUtils::isSupportedStreamUrl(QStringLiteral("javascript:alert(1)")));
    QVERIFY(!UrlUtils::isSupportedStreamUrl(QStringLiteral("data:text/html;base64,xx")));
    QVERIFY(!UrlUtils::isSupportedStreamUrl(QStringLiteral("mailto:test@example.com")));
    QVERIFY(!UrlUtils::isSupportedStreamUrl(QStringLiteral("ssh://host/x")));
    QVERIFY(!UrlUtils::isSupportedStreamUrl(QString()));          // empty
    QVERIFY(!UrlUtils::isSupportedStreamUrl(QStringLiteral("\t\n  "))); // whitespace only
}

void UrlUtilsTest::handlesQuotesAndWhitespace()
{
    QCOMPARE(UrlUtils::sanitize(QStringLiteral("\"http://host/x.ts\"")),
             QStringLiteral("http://host/x.ts"));
    QCOMPARE(UrlUtils::sanitize(QStringLiteral("  https://host/a.ts  ")),
             QStringLiteral("https://host/a.ts"));
    QVERIFY(UrlUtils::isSupportedStreamUrl(QStringLiteral("\"http://host/x.ts\"")));
}

void UrlUtilsTest::resolvesRelativeUrls()
{
    const QUrl base(QStringLiteral("https://cdn.example/playlists/main.m3u"));
    QCOMPARE(UrlUtils::resolveAgainst(QStringLiteral("live/ch1.ts"), base),
             QUrl(QStringLiteral("https://cdn.example/playlists/live/ch1.ts")));
    QCOMPARE(UrlUtils::resolveAgainst(QStringLiteral("/absolute/x.ts"), base),
             QUrl(QStringLiteral("https://cdn.example/absolute/x.ts")));
    QCOMPARE(UrlUtils::resolveAgainst(QStringLiteral("https://other.example/x.ts"), base),
             QUrl(QStringLiteral("https://other.example/x.ts")));
}

void UrlUtilsTest::redactsCredentials()
{
    QCOMPARE(UrlUtils::redact(QStringLiteral("http://user:secret@host/stream.ts")),
             QStringLiteral("http://host/stream.ts"));
    QCOMPARE(UrlUtils::redact(QStringLiteral("http://host/stream.ts?token=abc")),
             QStringLiteral("http://host/stream.ts?token=abc"));
}

QTEST_GUILESS_MAIN(UrlUtilsTest)
#include "tst_urlutils.moc"
