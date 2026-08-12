# Architecture

## Overview

```
                    +---------------------------+
                    |        MainWindow         |
                    +---------------------------+
            +-----------+       |        +--------------+
            |  Sidebar  |       |        |   Top bar    |
            +-----------+       |        +--------------+
                                v
              +----------------------------------------+
              |      QStackedWidget (pages)            |
              |  Home | Live TV | Favorites | History  |
              |  Countries | Categories | Languages    |
              |  Search | Settings | About             |
              +----------------------------------------+
                     |                      |
                     |  ChannelView         | PlayerPanel
                     v                      v
              +------------+        +---------------------+
              | Playback   |        | PlaybackController   |
              | Controller |------->|  IPlaybackEngine     |
              +------------+        |   (libVLC engine)    |
                     |              +---------------------+
                     v
      +-----------------------------+
      | PlaylistManager             |
      |  (registry + channel pool)  |
      +-----------------------------+
```

## Layer rules

- **UI (`src/ui`)** talks only to the `PlaybackController` and the managers —
  never to libVLC directly.
- **Playback (`src/playback`)** exposes `IPlaybackEngine`; the libVLC engine
  is one implementation and can be swapped.
- **Services (`src/services`)** own networking, caching, search, favorites,
  history, EPG and updates.
- **Playlist (`src/playlist`)** owns the parser and the manager.
- **Core (`src/core`)** has no dependencies upward: paths, logging, version.
- **Models (`src/models`)** are plain data types.

## Threading

The UI thread never blocks:

- Playlist fetching: async `QNetworkAccessManager` (HTTP) or `QtConcurrent`
  file reads (local).
- Parsing (M3U, XMLTV, channel caches): `QtConcurrent::run` on worker threads;
  results return via `QFutureWatcher` on the UI thread.
- Channel logos: throttled (max 6 concurrent) async downloads with disk +
  memory cache.
- libVLC: non-blocking calls; events arrive on VLC threads and are forwarded
  through queued Qt signals.

## Channel identity

Favorites and history persist `stableKey()` — an MD5 over
`playlistId + name + url`. Array positions are never used, so refreshes,
reorders and renames don't break saved channels.

## Data flow on refresh

1. `PlaylistManager::refresh(id)` → `IPlaylistSource::fetch` (async).
2. Fetch completes → `M3uParser::parse` runs on a worker thread.
3. Results are stored in memory, cached to
   `cache/channels_<id>.json`, and announced via `channelsChanged`.
4. `MainWindow::refreshAllViews` rebuilds the channel pool and all pages.

## Performance decisions

- Channel lists are `QAbstractListModel` + `QListView` (virtualized) — no
  per-channel widgets even with tens of thousands of channels.
- Batched layout (`Batched` + `BatchSize`) keeps grid rendering smooth.
- Logos are requested only for visible items.
- Parsing caps protect against hostile playlists (100k channels, line caps).

## Extending

- **New playback backend**: implement `IPlaybackEngine`, use it in
  `EngineFactory`.
- **EPG**: `EpgManager` + `XmltvParser` are self-contained; feed per-playlist
  XMLTV URLs and read now/next from `EpgManager`.
- **Localization**: all UI strings use `tr()`; add a `translations/` folder
  and `.ts` files when ready.
