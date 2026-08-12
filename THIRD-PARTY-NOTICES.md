# Third-Party Notices

STGR IpTV bundles or links the following third-party components. This file
documents their licenses and the obligations that come with redistribution.

## Qt 6 — LGPL-3.0 (with Qt Company commercial exception)

- Component: Qt Core, Qt GUI, Qt Widgets, Qt Network, Qt Svg, Qt Concurrent.
- License: GNU Lesser General Public License v3 (LGPL-3.0).
- Usage: dynamically linked; the Qt runtime DLLs are deployed next to the
  executable via `windeployqt`.
- Obligations: license/notice preservation, dynamic linking (satisfied),
  users must be able to relink the application against modified Qt libraries
  (the license text is shipped below and the full source is available at
  https://code.qt.io).
- Copyright: The Qt Company Ltd.

## libVLC (VLC media player libraries) — LGPL-2.1

- Component: libvlc.dll, libvlccore.dll and the VLC `plugins/` modules,
  obtained from the official VideoLAN prebuilt archives
  (e.g. `https://get.videolan.org/vlc/3.0.21/win64/vlc-3.0.21-win64.7z`).
- License: GNU Lesser General Public License v2.1 (LGPL-2.1) — the libVLC core
  libraries are licensed under LGPL-2.1+.
- Usage: dynamically linked; the DLLs and plugins are copied next to the
  executable at packaging time.
- Obligations: license/notice preservation (below), the application must not
  statically link libVLC (it does not), and users must be able to replace the
  VLC runtime with their own build (possible, as the DLLs are drop-in).
- Source: https://code.videolan.org/videolan/vlc
- Note: some VLC plugin modules may carry GPL or other licenses; the bundled
  runtime is used as-is for playback in accordance with VideoLAN's
  redistribution terms. Do not modify the VLC binaries.

## Inno Setup — Inno Setup License

- Component: used only at build time to produce the Windows installer.
- License: Inno Setup License (free and open source, based on the original
  Inno Setup license by Jordan Russell).
- Website: https://jrsoftware.org/isinfo.php

## iptv-org/iptv playlist data — CC-BY-4.0 (data)

- Component: optional built-in playlist URLs pointing at the public
  `https://iptv-org.github.io/iptv/` playlists.
- License: the playlist data of the iptv-org project is published under
  Creative Commons Attribution 4.0 (CC-BY-4.0). STGR IpTV does not bundle the
  playlist data itself — it only references the live URLs maintained by that
  community project. Attribution: iptv-org contributors.
- Repository: https://github.com/iptv-org/iptv

## Microsoft Visual C++ Runtime

- The installer may rely on the VC++ runtime already present on Windows 10/11.
- License: Microsoft Software License Terms.

## Tools used in CI (not redistributed)

- aqtinstall, CMake, 7-Zip, GitHub Actions runners — build-time only.

## License texts

The full text of the LGPL-2.1 and LGPL-3.0 licenses is available from the Free
Software Foundation:

- https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
- https://www.gnu.org/licenses/lgpl-3.0.html

A copy of the LGPL-2.1 text is also available in the VLC source distribution.

## Updating this file

When adding or replacing a dependency, update this file and re-verify that the
intended distribution model (per-user installer, portable ZIP) is permitted by
the dependency's license.
