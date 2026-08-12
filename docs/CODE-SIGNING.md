# Code signing

Release binaries are built **unsigned** unless the maintainers configure a
code-signing certificate. Nothing is ever faked — unsigned releases are
clearly the default until you enable signing.

## How to enable signing

1. Obtain a Windows code-signing certificate (e.g. from a CA such as
   DigiCert or Sectigo) and export it as a **PFX with a password**.
2. Encode the PFX as base64:

   ```bash
   # PowerShell
   [Convert]::ToBase64String([IO.File]::ReadAllBytes("cert.pfx")) | Set-Content cert.b64
   ```

3. Add two **GitHub Actions secrets** to the repository:

   - `CODE_SIGNING_CERT_BASE64` — the base64 contents of `cert.b64`
   - `CODE_SIGNING_PASSWORD` — the PFX password

4. The release workflow (`release.yml`) detects the secrets and, when present:

   - imports the certificate,
   - signs `STGR-IpTV.exe` and the installer with `signtool /fd SHA256`,
   - timestamps with DigiCert's public timestamp server,
   - **recomputes SHA256SUMS.txt** after signing (since signing changes the
     files).

When the secrets are absent, the workflow logs
"No CODE_SIGNING_CERT_BASE64 secret - building unsigned release" and proceeds
normally.

## Recommended CA / notes

- An **EV certificate** gives the cleanest SmartScreen experience but is not
  required.
- Keep the PFX private: never commit it, never paste it in issues, never
  store it unencrypted.
- Consider a hardware/cloud-signed certificate (Azure Trusted Signing) for
  better key hygiene; adapt the signing step accordingly.

## Verification checklist for a signed release

```powershell
Get-AuthenticodeSignature dist\STGR-IpTV-Setup-*.exe
# Status should be Valid
```
