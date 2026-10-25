#define MyAppName "Emergency Response Simulator"
#define MyAppVersion "1.0.0"
#define MyAppExe "ers_gui.exe"

[Setup]
AppId={{A41DB6F1-BD7C-496B-ADA6-5D356FDA73C8}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\EmergencyResponseSimulator
DefaultGroupName={#MyAppName}
OutputDir=dist
OutputBaseFilename=ERS-Setup-{#MyAppVersion}
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Files]
Source: "dist\ERS\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExe}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
