; -----------------------------------------------------------------------------
; STGR IpTV - Inno Setup installer
;
; Built automatically by GitHub Actions (release.yml). Version and source dir
; are injected with /dAppVersion=... and /dAppSourceDir=...
; User data is never touched: it lives in %APPDATA%\steigerdojo\STGR IpTV and
; is preserved across updates automatically.
; -----------------------------------------------------------------------------

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef AppSourceDir
  #define AppSourceDir "..\dist\staging"
#endif

#define MyAppName "STGR IpTV"
#define MyAppPublisher "STEiGER Dojo"
#define MyAppExeName "STGR-IpTV.exe"
#define MyAppId "com.steigerdojo.iptv"

[Setup]
AppId={{9A2B3C4D-5E6F-4A7B-8C9D-0E1F2A3B4C5D}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppVerName={#MyAppName} {#AppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/Sensei002/stgr-iptv
AppSupportURL=https://github.com/Sensei002/stgr-iptv/issues
AppUpdatesURL=https://github.com/Sensei002/stgr-iptv/releases
DefaultDirName={localappdata}\Programs\STGR IpTV
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir={#SourcePath}..\dist
OutputBaseFilename=STGR-IpTV-Setup-v{#AppVersion}-x64
SetupIconFile=..\assets\icons\app-icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
Source: "{#AppSourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
