

[Setup]
PrivilegesRequired=admin 
AppName=CDC3 数据生成器
AppVersion=3.0.0
AppPublisher=starryfast
AppPublisherURL=https://space.bilibili.com/3546973651077402
DefaultDirName={autopf}\CDC3
DefaultGroupName=CDC3
UninstallDisplayIcon={app}\CDC3_Main.exe
OutputDir=.\Output
OutputBaseFilename=CDC3_Setup_3.0.0
SetupIconFile=code.ico
Compression=lzma2
SolidCompression=yes
DisableReadyMemo=yes

[Files]
Source: "CDC3_Main.exe"; DestDir: "{app}"; Flags: ignoreversion


[Icons]
Name: "{userdesktop}\CDC3 数据生成器"; Filename: "{app}\CDC3_Main.exe"; WorkingDir: "{app}"
Name: "{userprograms}\CDC3\CDC3 数据生成器"; Filename: "{app}\CDC3_Main.exe"; WorkingDir: "{app}"
Name: "{userprograms}\CDC3\卸载 CDC3"; Filename: "{uninstallexe}"

[Run]
Filename: "netsh.exe"; Parameters: "advfirewall firewall add rule name=""CDC3"" dir=in action=allow program=""{app}\CDC3_Main.exe"" enable=yes"; Flags: runhidden

[UninstallRun]
Filename: "netsh.exe"; Parameters: "advfirewall firewall delete rule name=""CDC3"""; Flags: runhidden