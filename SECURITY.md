# Security Policy

## Reporting a vulnerability

Please **do not open a public issue** for security problems. Report them
privately by emailing the maintainers (address TBD) or by using GitHub's
private vulnerability reporting on the repository page.

Include:

- the affected version,
- a description of the issue,
- reproduction steps,
- impact assessment.

We aim to acknowledge reports within 5 business days and to release fixes
promptly.

## Security posture

STGR IpTV treats all external input as untrusted:

- **M3U/M3U8 playlists** — parsed with a defensive parser: length caps,
  never-throw semantics, malformed-line skipping.
- **Stream URLs** — only a whitelist of streaming protocols
  (`http`, `https`, `rtsp`, `rtmps`, `rtmp`, `udp`, ...) is accepted; other
  schemes (`javascript:`, `data:`, `file:` for remote playlists, ...) are
  rejected. No commands are ever executed from playlist content.
- **Filenames** — imported files are copied into the user's data folder with
  sanitized names; no path traversal is possible.
- **Network** — HTTPS is preferred; HTTP is only allowed for playback of
  public streams that require it (treated as insecure transport). Credentials
  are never sent to unknown servers.
- **Updates** — the optional updater only talks to the official GitHub
  repository over HTTPS and never downloads or executes anything automatically.
- **Secrets** — no API keys, certificates or passwords are stored in the
  source tree; signing material is provided via GitHub Actions secrets.

## Supported versions

| Version | Supported          |
| ------- | ------------------ |
| latest release | ✅ |
| older releases | ❌ (update advised) |

## Code signing

Release binaries are signed when the maintainers have configured a
certificate; unsigned builds are clearly labeled. See
[docs/CODE-SIGNING.md](docs/CODE-SIGNING.md).
