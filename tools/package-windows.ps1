# -----------------------------------------------------------------------------
# package-windows.ps1 - production packaging for STGR IpTV (Windows x64).
#
# Steps: copy the exe -> windeployqt (Qt runtime) -> copy the libVLC runtime
# (libvlc.dll, libvlccore.dll, plugins/) -> portable ZIP -> Inno Setup
# installer -> SHA256SUMS.txt.
#
# Usage:
#   powershell -File tools/package-windows.ps1 `
#       -AppExe build\ci-windows\Release\STGR-IpTV.exe `
#       -QtBin  <qt>\win64_msvc2022_64\bin `
#       -VlcDir <extracted-vlc-archive> `
#       -Version 1.0.0 `
#       -OutDir dist
# -----------------------------------------------------------------------------
param(
    [Parameter(Mandatory = $true)][string]$AppExe,
    [Parameter(Mandatory = $true)][string]$QtBin,
    [Parameter(Mandatory = $true)][string]$VlcDir,
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [Parameter(Mandatory = $false)][string]$InnoPath = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

$Staging = Join-Path $OutDir "staging"
New-Item -ItemType Directory -Force -Path $Staging | Out-Null

Write-Host "[package] staging: $Staging"

# 1. Application executable --------------------------------------------------
Copy-Item $AppExe (Join-Path $Staging "STGR-IpTV.exe") -Force

# 2. Qt runtime --------------------------------------------------------------
$Windeployqt = Join-Path $QtBin "windeployqt.exe"
if (-not (Test-Path $Windeployqt)) { throw "windeployqt.exe not found in $QtBin" }
Write-Host "[package] running windeployqt"
& $Windeployqt --release --no-translations --no-opengl-sw --no-system-d3d-compiler (Join-Path $Staging "STGR-IpTV.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed with exit code $LASTEXITCODE" }

# 3. libVLC runtime -----------------------------------------------------------
if (-not (Test-Path (Join-Path $VlcDir "libvlc.dll"))) { throw "libvlc.dll not found in $VlcDir" }
Write-Host "[package] copying VLC runtime"
Copy-Item (Join-Path $VlcDir "libvlc.dll") $Staging -Force
Copy-Item (Join-Path $VlcDir "libvlccore.dll") $Staging -Force
Copy-Item -Recurse -Force (Join-Path $VlcDir "plugins") $Staging
if (Test-Path (Join-Path $VlcDir "lua")) {
    Copy-Item -Recurse -Force (Join-Path $VlcDir "lua") $Staging
}

# 4. Portable ZIP --------------------------------------------------------------
$ZipName = "STGR-IpTV-v$Version-x64-portable.zip"
$ZipPath = Join-Path $OutDir $ZipName
if (Test-Path $ZipPath) { Remove-Item $ZipPath }
Write-Host "[package] creating $ZipName"
Compress-Archive -Path (Join-Path $Staging "*") -DestinationPath $ZipPath -CompressionLevel Optimal

# 5. Installer ----------------------------------------------------------------
if (-not $InnoPath) {
    $candidates = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
    )
    $InnoPath = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if ($InnoPath) {
    Write-Host "[package] building installer with Inno Setup"
    & $InnoPath "/dAppVersion=$Version" "/dAppSourceDir=$Staging" "/o$OutDir" (Join-Path $repoRoot "installer\STGR-IpTV.iss")
    if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed with exit code $LASTEXITCODE" }
} else {
    Write-Warning "[package] Inno Setup not found - installer skipped"
}

# 6. SHA256 checksums ----------------------------------------------------------
$files = @()
if (Test-Path $ZipPath) { $files += $ZipPath }
$installer = Get-ChildItem $OutDir -Filter "STGR-IpTV-Setup-v$Version-*.exe" | Select-Object -First 1
if ($installer) { $files += $installer.FullName }
$lines = foreach ($f in $files) {
    $hash = (Get-FileHash $f -Algorithm SHA256).Hash.ToLower()
    "{0}  {1}" -f $hash, (Split-Path $f -Leaf)
}
$lines | Set-Content (Join-Path $OutDir "SHA256SUMS.txt") -Encoding ascii

Write-Host ""
Write-Host "[package] done. Artifacts in ${OutDir}:"
Get-ChildItem $OutDir | Select-Object Name, Length
