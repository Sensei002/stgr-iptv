# Privacy

STGR IpTV is built to be **privacy-first**.

## What STGR IpTV does NOT do

- No telemetry, no analytics (Google Analytics, Sentry, etc.), no advertising
  SDKs, no tracking, no fingerprinting, no behavioral profiling.
- No account, no registration, no cloud dependency.
- No background services or resident daemons.
- No automatic upload of logs, playlists or anything else.

## When does STGR IpTV use the network?

Only when you explicitly ask for it:

| Action | Request |
| --- | --- |
| Loading/refreshing a playlist you added | The playlist URL you configured |
| Playing a channel | The channel's stream URL (played directly by libVLC) |
| Viewing channels | Logo images for channels currently visible |
| Optional EPG | The XMLTV URL you configured per playlist |
| Optional update check | GitHub API for the official STGR IpTV repository (if enabled) |

That's the complete list. There are no other remote calls, hidden or
otherwise.

## Where is your data stored?

Everything stays in your Windows user profile:

```
%APPDATA%\steigerdojo\STGR IpTV\
├── settings\   your preferences and favorites
├── playlists\  playlist registry and imported files
├── cache\      parsed playlist metadata (so offline launch still works)
├── logos\      downloaded channel logos
├── history\    recently watched channels
└── logs\       local diagnostic logs
```

- Data is never uploaded or shared.
- Logs are **local only**, rotated at 2 MB, and stream URLs are redacted
  before writing.
- Uninstalling the app does not delete your data; remove the folder above
  manually if you want a full wipe.

## Update checks

The optional updater asks GitHub for the latest release of this repository
(`api.github.com`). It never auto-downloads or executes anything — it only
opens the official release page if you choose to update.

## External sources

The bundled IPTV-org playlists are community-maintained public playlists.
When you load them, your IP address is visible to the servers that serve the
playlist and the streams, exactly as with any media player or browser.
