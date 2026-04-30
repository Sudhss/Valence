[Setup]
AppName=Valence
AppVersion=1.0
AppPublisher=Sudhanshu Shukla
AppComments=Your personal code editor, but cooler.
DefaultDirName={pf}\Valence
DefaultGroupName=Valence
OutputDir=Output
OutputBaseFilename=ValenceSetup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
SetupIconFile=C:\Users\shukl\OneDrive\Documents\Projects\Valence\Assets\favicon.ico

[Files]
Source: "C:\Users\shukl\OneDrive\Documents\Projects\Valence\build\Desktop_Qt_6_10_2_MinGW_64_bit-Release\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\Valence"; Filename: "{app}\Valence.exe"
Name: "{commondesktop}\Valence"; Filename: "{app}\Valence.exe"

[Run]
Filename: "{app}\Valence.exe"; Description: "Launch Valence now 😏"; Flags: nowait postinstall skipifsilent

[Messages]
WelcomeLabel1=Welcome to Valence Setup
WelcomeLabel2=Built by Sudhanshu Shukla — let’s code something dangerous.
FinishedLabel=You're all set. Go break things (and fix them).