#include "core/AppPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace AppPaths {

static QString s_appDataDir;

void setAppDataDirOverride(const QString& path)
{
    s_appDataDir = QDir::cleanPath(path);
}

QString appDataDir()
{
    if (!s_appDataDir.isEmpty())
        return s_appDataDir;

    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        // Fallback: %APPDATA%/<org>/<app>
        base = QDir::home().filePath(QStringLiteral("AppData/Roaming"));
        const QString org = QCoreApplication::organizationName();
        const QString app = QCoreApplication::applicationName();
        if (!org.isEmpty())
            base = QDir(base).filePath(org);
        if (!app.isEmpty())
            base = QDir(base).filePath(app);
    }
    s_appDataDir = QDir::cleanPath(base);
    return s_appDataDir;
}

QString settingsDir()     { return appDataDir() + QStringLiteral("/settings"); }
QString settingsFile()    { return settingsDir() + QStringLiteral("/settings.json"); }
QString playlistsDir()    { return appDataDir() + QStringLiteral("/playlists"); }
QString playlistImportsDir() { return playlistsDir() + QStringLiteral("/imports"); }
QString cacheDir()        { return appDataDir() + QStringLiteral("/cache"); }
QString logosDir()        { return appDataDir() + QStringLiteral("/logos"); }
QString historyFile()     { return appDataDir() + QStringLiteral("/history/history.json"); }
QString logsDir()         { return appDataDir() + QStringLiteral("/logs"); }
QString logFile()         { return logsDir() + QStringLiteral("/stgr-iptv.log"); }

bool ensureDirs()
{
    bool ok = true;
    ok &= QDir().mkpath(settingsDir());
    ok &= QDir().mkpath(playlistsDir());
    ok &= QDir().mkpath(playlistImportsDir());
    ok &= QDir().mkpath(cacheDir());
    ok &= QDir().mkpath(logosDir());
    ok &= QDir().mkpath(QFileInfo(historyFile()).absolutePath());
    ok &= QDir().mkpath(logsDir());
    return ok;
}

QString cacheFileForPlaylist(const QString& playlistId)
{
    return cacheDir() + QStringLiteral("/channels_") + playlistId + QStringLiteral(".json");
}

} // namespace AppPaths
