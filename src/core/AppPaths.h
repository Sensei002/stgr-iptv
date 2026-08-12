#pragma once

#include <QString>

// ---------------------------------------------------------------------------
// AppPaths - central resolver for every file and folder the application
// stores on disk. Everything lives under the Windows per-user application
// data directory (%APPDATA%\steigerdojo\STGR IpTV), never next to the
// executable, so upgrades and portable copies keep user data intact.
//
//   <appdata>/
//     settings/settings.json
//     playlists/playlists.json
//     playlists/imports/          (copies of imported .m3u files)
//     cache/channels_<id>.json    (parsed playlist caches)
//     logos/                      (downloaded channel logos)
//     history/history.json
//     logs/stgr-iptv.log
// ---------------------------------------------------------------------------
namespace AppPaths {

QString appDataDir();
QString settingsDir();
QString settingsFile();
QString playlistsDir();
QString playlistImportsDir();
QString cacheDir();
QString logosDir();
QString historyFile();
QString logsDir();
QString logFile();

// Returns true when every directory above exists (created on demand).
bool ensureDirs();

// Test hook: redirects all storage to an isolated directory.
void setAppDataDirOverride(const QString& path);

// File used to cache the parsed channel list of a playlist.
QString cacheFileForPlaylist(const QString& playlistId);

} // namespace AppPaths
