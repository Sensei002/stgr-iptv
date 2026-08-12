# Changelog

All notable changes to STGR IpTV are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-08-12

Initial production release.

### Added

- Native Windows x64 application (C++20 / Qt 6 / libVLC, no Electron).
- M3U/M3U8 playback: remote URLs, local imports, robust defensive parsing
  (UTF-8, CRLF/LF, malformed lines, duplicates, huge playlists, query
  parameters, protocol whitelist).
- Optional built-in IPTV-org community playlists (All / News / Sports /
  Music / Movies / Kids) with enable/disable.
- Live TV browser with virtualized grid/list, sorting and category filtering.
- Instant local search (Ctrl+K) over names, countries, categories, languages,
  networks.
- Dynamic Countries / Categories / Languages pages built from playlist
  metadata.
- Favorites and Recently Watched with stable identity across refreshes and
  reorders.
- Player: play/pause/stop, volume, mute, aspect ratio, fullscreen, previous/
  next channel, reconnect with backoff, friendly error states, mini player.
- Optional EPG support: per-playlist XMLTV URL with now/next guide.
- Settings: general, playback, interface, playlists, privacy, diagnostics.
- Optional GitHub Releases updater (never forced).
- Offline detection with cached playlist data.
- Local rotating logs with URL redaction.
- GitHub Actions CI/CD: PR/push pipeline + tag-based release with installer,
  portable ZIP and SHA256SUMS.txt.
- Inno Setup installer (per-user install, Start Menu + optional desktop
  shortcut, uninstall entry).
- Full documentation and third-party license notices.

### Security

- Untrusted-input handling: protocol whitelist, input caps, no command
  execution from playlists, sanitized imports.
- Code signing pipeline ready (documented, activated via secrets).

### Privacy

- No telemetry, analytics, ads or tracking SDKs. Documented network behavior.
