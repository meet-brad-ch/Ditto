#define MyAppName               "Ditto"
#define MyAppVersion            GetVersionNumbersString("..\Release64\Ditto.exe")
#define MyAppVerName            MyAppName + " " + MyAppVersion + " (local-only build)"
; local-only fork of sabrogden/Ditto: no network code, no firewall rule, no browser links
#define MyAppPublisher          "Ditto local-only fork (meet-brad-ch)"
#define MyAppAuthor             "Scott Brogden"
#define MyAppSupportURL         "https://github.com/meet-brad-ch/Ditto"
#define MyAppCopyrighEndYear    GetDateTimeString('yyyy','','')
#define MyOutputBaseFilename    "DittoSetup_" + StringChange(MyAppVersion, '.', '_')

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppVerName}

AppPublisher={# MyAppPublisher}

AppPublisherURL={#MyAppSupportURL}
AppSupportURL={#MyAppSupportURL}
AppUpdatesURL={#MyAppSupportURL}

WizardStyle=modern

UninstallDisplayIcon={app}\Ditto.exe
UninstallDisplayName={#MyAppName}

VersionInfoDescription={#MyAppName} installer
VersionInfoVersion={#MyAppVersion}
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}

AppCopyright={#MyAppAuthor} {#MyAppCopyrighEndYear}

OutputBaseFilename={#MyOutputBaseFilename}
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
;per-user install, no administrator rights (owner decision 2026-10-07): {autopf} is the user's
;%LOCALAPPDATA%\Programs, and every registry write goes to the installing user's HKCU
PrivilegesRequired=lowest
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableReadyPage=yes
DirExistsWarning=no
UninstallLogMode=overwrite
ChangesAssociations=yes
CloseApplications=yes
;Windows 10 1607 (build 14393) or later: Ditto calls GetDpiForWindow directly
MinVersion=10.0.14393
SetupLogging=yes

[Languages]
;English only (owner decision 2026-10-07); Ditto's own language files are installed below
Name: English; MessagesFile: compiler:Default.isl

[CustomMessages]
English.RunDittoOnStartup=Run Ditto on Windows startup
English.LaunchDitto=Launch Ditto
English.UninstallDitto=Uninstall Ditto
English.MachineInstallExists=Ditto is already installed for all users in %1.%nUninstall it first (Settings > Apps, as an administrator), then run this setup again.

[Tasks]
Name: RunAtStartup; Description: {cm:RunDittoOnStartup}

[Files]
Source: ..\Release64\Ditto.exe; DestDir: {app}; DestName: Ditto.exe; Flags: ignoreversion
Source: ..\Release64\ICU_Loader.dll; DestDir: {app}; Flags: ignoreversion
Source: ..\Release64\Addins\DittoUtil.dll; DestDir: {app}\Addins; Flags: ignoreversion

; the VC++/MFC runtime ships next to Ditto.exe (app-local), so the install needs no
; administrator rights. The 64-bit DLLs live in System32; a 32-bit compiler (Inno Setup 6)
; sees them only through "sysnative", a 64-bit compiler (Inno Setup 7) only through "System32"
#if FileExists("C:\Windows\sysnative\vcruntime140.dll")
  #define Sys64Dir "C:\Windows\sysnative"
#else
  #define Sys64Dir "C:\Windows\System32"
#endif
Source: {#Sys64Dir}\vcruntime140.dll;  DestDir: {app}; Flags: ignoreversion
Source: {#Sys64Dir}\vcruntime140_1.dll;  DestDir: {app}; Flags: ignoreversion
Source: {#Sys64Dir}\msvcp140.dll;  DestDir: {app}; Flags: ignoreversion
Source: {#Sys64Dir}\mfc140u.dll;  DestDir: {app}; Flags: ignoreversion

Source: ..\Debug\Language\*; DestDir: {app}\Language
Source: ..\Debug\Themes\*; DestDir: {app}\Themes

[Icons]
Name: {group}\Ditto; Filename: {app}\Ditto.exe
Name: {group}\{cm:UninstallDitto}; Filename: {uninstallexe}

[Run]
Filename: {app}\Ditto.exe; Description: {cm:LaunchDitto}; Flags: nowait postinstall

[Registry]
;HKCU\Software\Ditto stays on uninstall: it holds the settings and the database path (DBPath3),
;so a later install opens the same clip history
Root: HKCU; Subkey: SOFTWARE\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Ditto; flags: uninsdeletevalue; ValueData: {app}\Ditto.exe; Tasks: RunAtStartup

Root: HKCU; Subkey: Software\Ditto; ValueType: dword; ValueName: SetFocus_iexplore.exe; ValueData: 00000001

Root: HKCU; Subkey: Software\Ditto\PasteStrings; ValueType: string; ValueName: gvim.exe; ValueData: """{{PLUS}gP"
Root: HKCU; Subkey: Software\Ditto\CopyStrings; ValueType: string; ValueName: gvim.exe; ValueData: """{{PLUS}y"
Root: HKCU; Subkey: Software\Ditto\CutStrings; ValueType: string; ValueName: gvim.exe; ValueData: """{{PLUS}x"

;remove the cmd.exe paste/copy strings that installs on Windows 7 and 8 wrote
Root: HKCU; Subkey: Software\Ditto\PasteStrings; ValueName: cmd.exe; Flags: deletevalue
Root: HKCU; Subkey: Software\Ditto\CopyStrings; ValueName: cmd.exe; Flags: deletevalue

;associate .dto with Ditto, for this user (HKCU\Software\Classes needs no administrator rights)
Root: HKCU; Subkey: Software\Classes\.dto; ValueType: string; ValueName: ; ValueData: Ditto; Flags: uninsdeletevalue
Root: HKCU; Subkey: Software\Classes\Ditto; ValueType: string; ValueName: ; ValueData: Ditto; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Classes\Ditto\DefaultIcon; ValueType: string; ValueName: ; ValueData: {app}\Ditto.exe,0
Root: HKCU; Subkey: Software\Classes\Ditto\shell\open\command; ValueType: string; ValueName: ; ValueData: """{app}\Ditto.exe"" ""%1"""


[Code]
const
  MachineUninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppName}_is1';

// An earlier per-machine (administrator) install is not seen by this per-user setup, which would
// install a second Ditto beside it: stop and say what to do instead.
function InitializeSetup(): Boolean;
var
  MachineInstallDir: String;
begin
  Result := True;
  if RegQueryStringValue(HKLM64, MachineUninstallKey, 'InstallLocation', MachineInstallDir) or
     RegQueryStringValue(HKLM32, MachineUninstallKey, 'InstallLocation', MachineInstallDir) then
  begin
    SuppressibleMsgBox(FmtMessage(CustomMessage('MachineInstallExists'), [MachineInstallDir]), mbCriticalError, MB_OK, IDOK);
    Result := False;
  end;
end;
