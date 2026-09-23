; Installer di Vaultly (Inno Setup 6). Non si compila a mano: lo fa scripts/release.ps1,
; che passa AppVersion, SourceDir (cartella con exe + DLL) e OutputDir.
;
; Installazione per utente (nessun permesso di amministratore), così anche gli aggiornamenti
; automatici non chiedono conferme UAC.
;
; Dati: ogni utente di Windows ha il suo DB in %APPDATA%\Vaultly, creato vuoto al primo avvio.
; L'installazione (e quindi ogni aggiornamento) non lo tocca; la DISINSTALLAZIONE cancella tutto:
; programma, dati, file temporanei degli aggiornamenti e impostazioni nel registro.

#ifndef AppVersion
  #error "Manca AppVersion: usa scripts/release.ps1"
#endif
#ifndef SourceDir
  #error "Manca SourceDir: usa scripts/release.ps1"
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

#define AppName "Vaultly"
#define AppExe "Vaultly.exe"

[Setup]
; AppId identifica l'app tra una versione e l'altra: non cambiarlo mai.
AppId={{A09232E3-9344-4ED0-B942-A999AE35099D}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppName}
VersionInfoVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\{#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=Vaultly-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#AppExe}
SetupIconFile=..\assets\vaultly.ico
UninstallDisplayName={#AppName}
; Se l'app è aperta, l'installer la chiude prima di sostituire i file.
CloseApplications=force
RestartApplications=no

[Languages]
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[InstallDelete]
; Via i file della versione precedente (DLL e plugin Qt possono cambiare tra una versione e l'altra).
; Riguarda solo la cartella del programma, mai i dati in %APPDATA%.
Type: filesandordirs; Name: "{app}\*"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
; Disinstallare = non lasciare nulla. Nessuna domanda: conti, movimenti e password vengono eliminati.
Type: filesandordirs; Name: "{userappdata}\{#AppName}"
Type: filesandordirs; Name: "{%TEMP}\{#AppName}-update"
Type: filesandordirs; Name: "{app}"

[Registry]
; Impostazioni (versione saltata; WinSparkle fino alla 1.0.2): la chiave si elimina alla disinstallazione.
Root: HKCU; Subkey: "Software\{#AppName}"; Flags: uninsdeletekey

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
; Senza "skipifsilent": dopo un aggiornamento automatico (installer in modalità /SILENT) l'app riparte da sola.
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall
