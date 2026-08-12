# Building STGR IpTV

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 (Desktop development with C++)
- CMake 3.21+
- Qt 6.5+ (Core, Gui, Widgets, Network, Svg, Concurrent)
- libVLC SDK (the official prebuilt Windows archive from VideoLAN)

## Getting Qt

Option A — official binaries with aqtinstall (recommended):

```bash
pip install aqtinstall
aqt install-qt windows desktop 6.8.2 win64_msvc2022_64 -m qtsvg -O C:\Qt
# Set QT_ROOT to C:\Qt\6.8.2\msvc2022_64
```

Option B — vcpkg manifest mode (also supported):

```bash
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
# then configure with the windows-vcpkg preset (needs VCPKG_ROOT env var)
```

## Getting the libVLC SDK

Download the official prebuilt archive from VideoLAN (pinned, e.g. 3.0.21):

```
https://get.videolan.org/vlc/3.0.21/win64/vlc-3.0.21-win64.7z
```

Extract it with 7-Zip. The archive root contains `libvlc.dll`,
`libvlccore.dll`, `plugins/` and `sdk/`. Set `VLC_DIR` to that folder.

## Configure & build

```bash
# PowerShell
$env:QT_ROOT = "C:\Qt\6.8.2\msvc2022_64"
$env:VLC_DIR = "C:\vlc-3.0.21"

cmake --preset windows-dev
cmake --build --preset windows-dev --config Release
ctest --test-dir build/dev -C Release --output-on-failure
```

The executable is produced at `build/dev/Release/STGR-IpTV.exe`.

To run it from the build tree you need the Qt runtime and the VLC runtime next
to the exe:

```bash
# PowerShell
& "C:\Qt\6.8.2\msvc2022_64\bin\windeployqt.exe" --release --no-translations build/dev/Release
Copy-Item C:\vlc-3.0.21\libvlc.dll, C:\vlc-3.0.21\libvlccore.dll build/dev/Release
Copy-Item -Recurse C:\vlc-3.0.21\plugins build/dev/Release
```

For a full production package (installer + portable ZIP + checksums) use:

```bash
powershell -File tools/package-windows.ps1 `
  -AppExe build/dev/Release/STGR-IpTV.exe `
  -QtBin C:\Qt\6.8.2\msvc2022_64\bin `
  -VlcDir C:\vlc-3.0.21 `
  -Version 1.0.0 `
  -OutDir dist
```

## CI builds

GitHub Actions builds everything from a clean runner — you only need to push:

- `build-windows.yml` runs on PRs and pushes to `main`/`develop`
  (configure, build, test, package, artifacts).
- `release.yml` runs on `v*` tags and publishes the release with installer,
  portable ZIP and `SHA256SUMS.txt`.

## Troubleshooting

- `libVLC SDK not found` — set `VLC_DIR` (cache or environment variable).
- Playback shows "backend unavailable" — the VLC runtime DLLs/`plugins` are
  missing next to the executable.
- Slow first CI build — vcpkg/Qt caches fill after the first run.
