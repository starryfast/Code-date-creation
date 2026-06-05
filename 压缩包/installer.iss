[Setup]
PrivilegesRequired=lowest
AppName=CDC4.5 数据生成器
AppVersion=4.5.0
AppPublisher=starryfast
AppPublisherURL=https://space.bilibili.com/3546973651077402
DefaultDirName={userpf}\CDC4.5
DefaultGroupName=CDC4.5
UninstallDisplayIcon={app}\CDC4.5_Main.exe
OutputDir=.\Output
OutputBaseFilename=CDC4_Setup_4.5.0
SetupIconFile=code.ico
Compression=lzma2
SolidCompression=yes
DisableReadyMemo=yes

[Files]
; 主程序
Source: "CDC4.5_Main.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{userdesktop}\CDC4.5 数据生成器"; Filename: "{app}\CDC4.5_Main.exe"
Name: "{userprograms}\CDC4.5\CDC4.5 数据生成器"; Filename: "{app}\CDC4.5_Main.exe"
Name: "{userprograms}\CDC4.5\卸载 CDC4.5"; Filename: "{uninstallexe}"