#include <QtTest>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "core/AppPaths.h"
#include "playlist/PlaylistManager.h"
#include "services/PlaylistFetcher.h"

namespace {

class FakePlaylistSource : public IPlaylistSource
{
    Q_OBJECT

public:
    void fetch(const Playlist& playlist, int timeoutMs) override
    {
        Q_UNUSED(timeoutMs);
        const auto it = m_data.constFind(playlist.url);
        if (it == m_data.constEnd()) {
            emit finished(playlist.id, false, QByteArray(), QStringLiteral("not found"));
            return;
        }
        emit finished(playlist.id, true, it.value().toUtf8(), QString());
    }

    QHash<QString, QString> m_data;
};

const char kSamplePlaylist[] =
    "#EXTM3U\n"
    "#EXTINF:-1 tvg-id=\"bbc\" group-title=\"News\",BBC News\n"
    "http://host/bbc.m3u8\n"
    "#EXTINF:-1 tvg-id=\"sky\" group-title=\"Sports\",Sky Sports\n"
    "http://host/sky.ts\n";

} // namespace

class PlaylistManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void addAndRemove();
    void refreshParsesChannels();
    void refreshFailureSetsError();
    void disabledPlaylistsExcludedFromPool();
    void renameAndPersist();
    void importLocalFileCopiesAndRegisters();
    void builtInsAreProvided();

private:
    QTemporaryDir m_dir;
    FakePlaylistSource m_source;
};

void PlaylistManagerTest::init()
{
    QVERIFY(m_dir.isValid());
    AppPaths::setAppDataDirOverride(m_dir.path());
    QVERIFY(AppPaths::ensureDirs());

    // Fresh registry + cache for every test (tests must be isolated).
    QFile::remove(AppPaths::playlistsDir() + QStringLiteral("/playlists.json"));
    QDir cache(AppPaths::cacheDir());
    if (cache.exists()) {
        const QStringList files = cache.entryList(QDir::Files);
        for (const QString& f : files)
            QFile::remove(cache.filePath(f));
    }

    m_source.m_data.clear();
}

void PlaylistManagerTest::addAndRemove()
{
    PlaylistManager manager(&m_source);
    manager.load();

    const QString id = manager.addPlaylist(QStringLiteral("Test"), QStringLiteral("http://host/list.m3u"));
    QVERIFY(!id.isEmpty());
    QCOMPARE(manager.playlists().size(), 1);
    QVERIFY(manager.findPlaylist(id) != nullptr);

    QVERIFY(manager.removePlaylist(id));
    QCOMPARE(manager.playlists().size(), 0);
    QVERIFY(manager.findPlaylist(id) == nullptr);
}

void PlaylistManagerTest::refreshParsesChannels()
{
    m_source.m_data.insert(QStringLiteral("http://host/list.m3u"), QString::fromLatin1(kSamplePlaylist));

    PlaylistManager manager(&m_source);
    manager.load();
    const QString id = manager.addPlaylist(QStringLiteral("Test"), QStringLiteral("http://host/list.m3u"));

    QSignalSpy spy(&manager, &PlaylistManager::refreshFinished);
    QVERIFY(manager.refresh(id));
    QTRY_COMPARE(spy.count(), 1);

    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), id);
    QCOMPARE(args.at(1).toBool(), true);
    QCOMPARE(args.at(2).toString(), QString());
    QCOMPARE(args.at(3).toInt(), 2);

    const QVector<Channel> channels = manager.channelsFor(id);
    QCOMPARE(channels.size(), 2);
    QCOMPARE(channels.at(0).name, QStringLiteral("BBC News"));
    QCOMPARE(channels.at(1).group, QStringLiteral("Sports"));

    // Cache file was written.
    QVERIFY(QFile::exists(AppPaths::cacheFileForPlaylist(id)));

    const Playlist* p = manager.findPlaylist(id);
    QVERIFY(p != nullptr);
    QCOMPARE(p->channelCount, 2);
    QVERIFY(p->lastUpdated.isValid());
}

void PlaylistManagerTest::refreshFailureSetsError()
{
    PlaylistManager manager(&m_source);
    manager.load();
    const QString id = manager.addPlaylist(QStringLiteral("Bad"), QStringLiteral("http://host/missing.m3u"));

    QSignalSpy spy(&manager, &PlaylistManager::refreshFinished);
    QVERIFY(manager.refresh(id));
    QTRY_COMPARE(spy.count(), 1);

    const QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(1).toBool(), false);
    QVERIFY(!args.at(2).toString().isEmpty());

    const Playlist* p = manager.findPlaylist(id);
    QVERIFY(p != nullptr);
    QVERIFY(!p->errorMessage.isEmpty());
}

void PlaylistManagerTest::disabledPlaylistsExcludedFromPool()
{
    m_source.m_data.insert(QStringLiteral("http://host/a.m3u"), QString::fromLatin1(kSamplePlaylist));
    m_source.m_data.insert(QStringLiteral("http://host/b.m3u"),
                           QStringLiteral("#EXTM3U\n#EXTINF:-1,Other\nhttp://host/other.ts\n"));

    PlaylistManager manager(&m_source);
    manager.load();
    const QString a = manager.addPlaylist(QStringLiteral("A"), QStringLiteral("http://host/a.m3u"));
    const QString b = manager.addPlaylist(QStringLiteral("B"), QStringLiteral("http://host/b.m3u"));

    manager.refresh(a);
    manager.refresh(b);
    QTRY_VERIFY(manager.channelsFor(a).size() == 2);
    QTRY_VERIFY(manager.channelsFor(b).size() == 1);
    QCOMPARE(manager.allChannels().size(), 3);

    QVERIFY(manager.setPlaylistEnabled(b, false));
    QCOMPARE(manager.allChannels().size(), 2);
}

void PlaylistManagerTest::renameAndPersist()
{
    PlaylistManager manager(&m_source);
    manager.load();
    const QString id = manager.addPlaylist(QStringLiteral("Old"), QStringLiteral("http://host/x.m3u"));
    QVERIFY(manager.renamePlaylist(id, QStringLiteral("New")));
    QCOMPARE(manager.findPlaylist(id)->name, QStringLiteral("New"));
    manager.save();

    // Reload in a fresh instance: registry persisted.
    PlaylistManager reloaded(&m_source);
    reloaded.load();
    const Playlist* p = reloaded.findPlaylist(id);
    QVERIFY(p != nullptr);
    QCOMPARE(p->name, QStringLiteral("New"));
    QCOMPARE(p->url, QStringLiteral("http://host/x.m3u"));
}

void PlaylistManagerTest::importLocalFileCopiesAndRegisters()
{
    const QString sourceFile = m_dir.path() + QStringLiteral("/my-list.m3u");
    QFile f(sourceFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(kSamplePlaylist);
    f.close();

    PlaylistManager manager(&m_source);
    manager.load();
    QVERIFY(manager.importLocalFile(sourceFile));
    QCOMPARE(manager.playlists().size(), 1);

    const Playlist& p = manager.playlists().first();
    QVERIFY(p.isLocal());
    QVERIFY(QFileInfo::exists(p.url));
    QCOMPARE(QFileInfo(p.url).fileName(), QStringLiteral("my-list.m3u"));

    // The import directory got a copy.
    QVERIFY(QFile::exists(p.url));
}

void PlaylistManagerTest::builtInsAreProvided()
{
    const QVector<Playlist> builtIns = PlaylistManager::builtInPlaylists();
    QVERIFY(builtIns.size() >= 5);
    for (const Playlist& p : builtIns)
        QVERIFY(p.url.startsWith(QStringLiteral("https://iptv-org.github.io/")));

    PlaylistManager manager(&m_source);
    manager.load();
    manager.restoreBuiltInPlaylists();
    QCOMPARE(manager.playlists().size(), builtIns.size());
}

QTEST_GUILESS_MAIN(PlaylistManagerTest)
#include "tst_playlistmanager.moc"
