<p align="center">
  <img src="assets/branding/logo-dark.svg" alt="STGR IpTV by STEiGER Dojo" width="100%"/>
</p>

# STGR IpTV

**A lightweight, privacy-first Live TV player for Windows 10/11 (x64) by STEiGER Dojo.**

STGR IpTV is a native Windows application (C++20, Qt 6, libVLC) that reads
M3U/M3U8 playlists and lets you browse and watch live television channels with
a dark, Japanese dojo-inspired interface.

> **Important:** STGR IpTV is a **player and a client, not an IPTV provider**.
> It does not host, proxy, download for redistribution, or supply any
> television content. Streams are played **directly** from playlists that you
> add. The bundled IPTV-org entry is an external, community-maintained public
> playlist source. See [Legal disclaimer](#legal-disclaimer).

---

## Features

- 🖥️ **Native Windows x64 application** — no Electron, no embedded browser,
  no web runtime. Fast startup, low memory footprint.
- 📺 **Live TV playback** powered by **libVLC** — HLS, MPEG-TS, MP4, HTTP(S),
  RTSP/RTMP, hardware decoding, network reconnection.
- 📼 **M3U / M3U8 support** — remote URLs and local files, plus an optional
  built-in **IPTV-org** community playlist (enable/disable any time).
- 🔍 **Instant local search** (`Ctrl+K`) across names, countries, categories,
  languages and networks — no network request per keystroke.
- 🌍 **Dynamic browsing** — Countries, Categories and Languages pages generated
  from your playlists' metadata (never hardcoded lists).
- ⭐ **Favorites & Recently Watched** — persisted across restarts, refreshes
  and playlist reorders via stable channel identities.
- 🎛️ **TV-like experience** — keyboard shortcuts, previous/next channel,
  fullscreen, volume, mute, aspect ratio, mini player.
- 🔁 **Resilient playback** — buffering states, automatic reconnect with
  backoff, friendly errors, never crashes on dead streams.
- 🔒 **Privacy-first** — no telemetry, no analytics, no ads, no tracking SDKs,
  no account. Everything stays on your machine.
- 📦 **Production CI/CD** — GitHub Actions builds, tests, packages the
  installer and publishes GitHub Releases automatically.

## Screenshots

Screenshots will be added to `assets/screenshots/` with the first release.

## Installation

Download the latest installer from the
[Releases](https://github.com/Sensei002/stgr-iptv/releases) page:

```
STGR-IpTV-Setup-vX.Y.Z-x64.exe
```

Run it — no account, no registration, no admin rights required (per-user
install). A portable ZIP is also published for users who prefer not to install.

System requirements: **Windows 10 or 11, 64-bit**.

## Quick start

1. Launch STGR IpTV.
2. On first run, choose **"Use the IPTV-org Public Playlist"** (or add your own).
3. Wait for the playlist to load, then pick a channel and watch.

You can add your own legitimate M3U playlists at any time:
**Settings → Playlists → Add Playlist** (URL or local file).

## Adding M3U playlists

- **Remote URL** — e.g. `https://iptv-org.github.io/iptv/index.m3u`
- **Local file** — `.m3u` / `.m3u8` via **Import M3U** (a copy is kept in your
  user profile)
- **Built-in IPTV-org** — All Channels, News, Sports, Music, Movies, Kids
  (community-maintained; you can delete or disable them)

Each playlist can also have an optional **EPG URL** (XMLTV) for the now/next
guide shown in the player.

## Building from source

See [docs/BUILDING.md](docs/BUILDING.md) for local builds and the
[vcpkg](https://vcpkg.io) developer path.

Production builds run entirely through GitHub Actions:

```bash
git tag v1.0.0
git push origin v1.0.0
```

That produces `STGR-IpTV-Setup-v1.0.0-x64.exe`,
`STGR-IpTV-v1.0.0-x64-portable.zip` and `SHA256SUMS.txt` on the release page.

## Architecture

```
src/
├── core/       app paths, logging, version
├── models/     Channel / Playlist domain types
├── playlist/   robust M3U parser + playlist manager
├── playback/   IPlaybackEngine abstraction + libVLC engine + controller
├── services/   network, logo cache, search, favorites, history, EPG, updater
├── settings/   JSON-backed settings
├── ui/         Qt Widgets UI (dark dojo theme, virtualized channel lists)
└── utils/      URL/string helpers
tests/          Qt Test unit tests (no network required)
installer/      Inno Setup script
.github/        CI/CD workflows
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for details.

## Privacy

STGR IpTV has **no telemetry** of any kind. Network requests happen only when
you ask for them (playlist loading/refresh, stream playback, visible channel
logos, optional EPG, optional update check). All data lives in your user
profile folder:

```
%APPDATA%\steigerdojo\STGR IpTV\
├── settings\   settings.json, favorites.json
├── playlists\  playlist registry + imported files
├── cache\      parsed playlist caches
├── logos\      downloaded channel logos
├── history\    recently watched
└── logs\       local diagnostic logs (never uploaded)
```

See [docs/PRIVACY.md](docs/PRIVACY.md).

## Legal disclaimer

STGR IpTV does **not**:

- host, provide, or redistribute copyrighted television streams,
- bypass DRM, geo-blocking or authentication,
- scrape paid IPTV providers or supply pirated credentials.

The application is a **player/client** for playlists you choose to add. The
bundled [IPTV-org](https://github.com/iptv-org/iptv) playlist is a publicly
maintained community source used purely as an example. You are responsible for
ensuring the playlists you use are legitimate in your jurisdiction.

## Third-party licenses

Qt (LGPL-3.0), libVLC (LGPL-2.1), Inno Setup (Inno Setup License), and the
community-maintained IPTV-org playlist data are used in this project.
See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Security issues: [SECURITY.md](SECURITY.md).

## License

[MIT](LICENSE) © 2026 STEiGER Dojo.
