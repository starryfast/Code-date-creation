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
OutputBaseFilename=CDC4_Setup_4.5.0_WithGCC
SetupIconFile=code.ico
Compression=lzma2
SolidCompression=yes
DisableReadyMemo=yes

[Files]
; 主程序
Source: "CDC4_Main.exe"; DestDir: "{app}"; Flags: ignoreversion
; 捆绑 mingw64 编译器（请确保将 mingw64 文件夹放在与本脚本相同的目录下）
Source: "mingw64\*"; DestDir: "{app}\mingw64"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{userdesktop}\CDC4 数据生成器"; Filename: "{app}\CDC4_Main.exe"
Name: "{userprograms}\CDC4\CDC4.5 数据生成器"; Filename: "{app}\CDC4_Main.exe"
Name: "{userprograms}\CDC4\卸载 CDC4.5"; Filename: "{uninstallexe}"