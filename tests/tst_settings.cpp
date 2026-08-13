#include <QtTest>

#include <QCoreApplication>
#include <QTemporaryDir>

#include "core/AppPaths.h"
#include "settings/Settings.h"

class SettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void defaultsAreSane();
    void roundTrip();
    void persistsAcrossReload();
    void clampsValues();
};

void SettingsTest::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("stgr-test"));
    QCoreApplication::setApplicationName(QStringLiteral("stgr-test"));
}

void SettingsTest::defaultsAreSane()
{
    Settings* s = Settings::instance();
    s->load();

    QCOMPARE(s->hardwareAcceleration(), 2); // software decode by default
    QCOMPARE(s->bufferSizeMs(), 1500);
    QCOMPARE(s->networkTimeoutSec(), 10);
    QCOMPARE(s->maxRetries(), 3);
    QVERIFY(s->autoReconnect());
    QVERIFY(s->rememberLastChannel());
    QVERIFY(!s->firstRunDone());
    QVERIFY(!s->launchOnStartup());
    QCOMPARE(s->theme(), QStringLiteral("dark"));
}

void SettingsTest::roundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    AppPaths::setAppDataDirOverride(dir.path());
    QVERIFY(AppPaths::ensureDirs());

    Settings* s = Settings::instance();
    s->load();

    s->setStartMaximized(true);
    s->setHardwareAcceleration(1);
    s->setBufferSizeMs(2000);
    s->setDefaultVolume(42);
    s->setViewMode(QStringLiteral("list"));
    s->setFirstRunDone(true);
    s->save();

    QCOMPARE(s->startMaximized(), true);
    QCOMPARE(s->hardwareAcceleration(), 1);
    QCOMPARE(s->bufferSizeMs(), 2000);
    QCOMPARE(s->defaultVolume(), 42);
    QCOMPARE(s->viewMode(), QStringLiteral("list"));
    QCOMPARE(s->firstRunDone(), true);
}

void SettingsTest::persistsAcrossReload()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    AppPaths::setAppDataDirOverride(dir.path());
    QVERIFY(AppPaths::ensureDirs());

    Settings* s = Settings::instance();
    s->load();
    s->setStartMaximized(true);
    s->setMaxRetries(7);
    s->save();

    // Fresh load from disk.
    s->load();
    QCOMPARE(s->startMaximized(), true);
    QCOMPARE(s->maxRetries(), 7);
}

void SettingsTest::clampsValues()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    AppPaths::setAppDataDirOverride(dir.path());
    QVERIFY(AppPaths::ensureDirs());

    Settings* s = Settings::instance();
    s->load();

    s->setHardwareAcceleration(99);
    QCOMPARE(s->hardwareAcceleration(), 2);
    s->setHardwareAcceleration(-5);
    QCOMPARE(s->hardwareAcceleration(), 0);
    s->setDefaultVolume(500);
    QCOMPARE(s->defaultVolume(), 100);
    s->setMaxRetries(-1);
    QCOMPARE(s->maxRetries(), 0);
}

QTEST_GUILESS_MAIN(SettingsTest)
#include "tst_settings.moc"
