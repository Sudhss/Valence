; Valence 3.0 — Inno Setup script
;
; Differs from ValenceV2.iss in two ways that matter:
;
;  1. It installs from a staged payload (dist\Valence-3.0) rather than straight
;     out of the build directory. V2 shipped CMakeCache.txt, CMakeFiles\,
;     build.ninja, Valence_autogen\ and Testing\ to every user's machine.
;  2. Paths are relative to this script, so the build is not tied to one
;     person's home directory.

#define AppVersion "3.0"
#define Payload    "dist\Valence-3.0"

[Setup]
AppName=Valence
AppVersion={#AppVersion}
AppPublisher=Sudhanshu Shukla
AppComments=Your personal code editor, but cooler.
; Shown in Apps & Features, and used for the uninstaller entry.
AppId={{8F3C1A64-2E17-4B9D-9C21-2A6B7E5D4F03}
DefaultDirName={autopf}\Valence
DefaultGroupName=Valence
OutputDir=Output
OutputBaseFilename=Valence_V3_Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
SetupIconFile=Assets\favicon.ico
; The app is 64-bit; without this it would land in Program Files (x86).
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
; Per-machine install needs elevation; asking up front beats failing halfway.
PrivilegesRequired=admin
UninstallDisplayIcon={app}\Valence.exe
UninstallDisplayName=Valence {#AppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"
Name: "associatecpp"; Description: "Open .cpp files with Valence"; GroupDescription: "File associations:"; Flags: unchecked

[Files]
Source: "{#Payload}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Valence"; Filename: "{app}\Valence.exe"
Name: "{group}\Uninstall Valence"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Valence"; Filename: "{app}\Valence.exe"; Tasks: desktopicon

[Registry]
; Optional .cpp association, only when the user asks for it.
Root: HKA; Subkey: "Software\Classes\.cpp\OpenWithProgids"; ValueType: string; ValueName: "Valence.cpp"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associatecpp
Root: HKA; Subkey: "Software\Classes\Valence.cpp"; ValueType: string; ValueName: ""; ValueData: "C++ Source"; Flags: uninsdeletekey; Tasks: associatecpp
Root: HKA; Subkey: "Software\Classes\Valence.cpp\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\Valence.exe,0"; Tasks: associatecpp
Root: HKA; Subkey: "Software\Classes\Valence.cpp\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Valence.exe"" ""%1"""; Tasks: associatecpp

[Run]
Filename: "{app}\Valence.exe"; Description: "Launch Valence 3.0"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Qt writes window geometry and preferences under HKCU; the binaries are
; removed by the uninstaller, but leave the user's settings alone.
Type: filesandordirs; Name: "{app}"

[Messages]
WelcomeLabel1=Welcome to Valence 3.0 Setup
WelcomeLabel2=Built by Sudhanshu Shukla — let's code something dangerous.
FinishedLabel=You're all set. Go break things (and fix them).
