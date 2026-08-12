# Contributing to STGR IpTV

Thanks for wanting to contribute! STGR IpTV is open source under the MIT
license. Please read this guide and [SECURITY.md](SECURITY.md) first.

## Ground rules

- **STGR IpTV is a player, not a provider.** Contributions that help users
  pirate, bypass DRM/geo-blocks, or scrape paid services will be rejected.
- **No telemetry.** Never add analytics, crash reporters that upload data,
  or any network call that isn't triggered by the user's explicit action.
- **No fake features.** Every button must do something real.
- **Keep it lightweight.** The app targets low RAM/CPU and fast startup.

## Development setup

1. Install Visual Studio 2022 (C++ workload) and CMake.
2. Install Qt 6.5+ (e.g. via aqtinstall) and the libVLC SDK
   (see [docs/BUILDING.md](docs/BUILDING.md)).
3. Configure:

   ```bash
   cmake --preset windows-dev
   cmake --build --preset windows-dev
   ctest --test-dir build/dev -C Release --output-on-failure
   ```

## Code style

- C++20, Qt 6 idioms, 4-space indent, no tabs.
- Follow the existing structure: `src/core`, `src/models`, `src/playlist`,
  `src/playback`, `src/services`, `src/settings`, `src/ui`, `src/utils`.
- All UI strings go through `tr()` so localization can be added later.
- Never block the UI thread: parsing/IO belong in worker threads.
- Treat all external data (M3U, XMLTV, URLs) as untrusted input.
- Keep `/W4` warnings clean on MSVC (warnings are errors in Release CI).

## Testing

- Unit tests live in `tests/` (Qt Test, no network required).
- The CI gate fails on compile errors, test failures, packaging failures and
  warnings-as-errors.

## Making changes

1. Fork and create a feature branch.
2. Make small, focused commits.
3. Run the tests and the build locally (or let CI do it).
4. Open a pull request against `develop`.
5. CI must be green before merge.

## Committing

```bash
git add -A
git commit -m "feat(playlist): add XYZ"
git push origin your-branch
```

Suggested prefixes: `feat:`, `fix:`, `docs:`, `test:`, `refactor:`, `ci:`.
