; Adventure-King Windows 安装包脚本（Inno Setup 6）
; 目标：把 Release.win32 输出目录打包为安装包，并附带赐福后端（Python/LangChain）脚本。
;
; 使用方式（在 Windows PowerShell 执行）：
;   powershell -ExecutionPolicy Bypass -File tools\\installer_win\\build_installer.ps1
;
; 说明：
; - 游戏是 Win32（32 位）构建，安装到 Program Files (x86) 属正常现象。
; - 赐福后端是 Python 脚本，首次启动会创建 .venv 并安装依赖（需要本机有 Python 3.10+）。

#define AppName "Adventure-King"
#define AppPublisher "Adventure-King Team"
#define AppExeName "Adventure-King.exe"

; 必须在 ISCC 编译时传入 /DGameOutDir=... 指向 Release.win32 输出目录
; 例如：ISCC /DGameOutDir="E:\\code\\fansqim\\Adventure-King\\proj.win32\\Release.win32" tools\\installer_win\\AdventureKing.iss
#ifndef GameOutDir
  #define GameOutDir "..\\..\\Adventure-King\\proj.win32\\Release.win32"
#endif

[Setup]
AppId={{D787B3AE-9E9E-4A03-9C0B-8A13B2B7C24A}
AppName={#AppName}
AppPublisher={#AppPublisher}
AppVersion=0.1.0
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableDirPage=no
DisableProgramGroupPage=yes
Compression=lzma2
SolidCompression=yes
OutputBaseFilename=Adventure-King-Setup
OutputDir=.
WizardStyle=modern

[Files]
; 游戏主体（Release.win32 输出目录）
Source: "{#GameOutDir}\\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#GameOutDir}\\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#GameOutDir}\\Resources\\*"; DestDir: "{app}\\Resources"; Flags: ignoreversion recursesubdirs createallsubdirs

; 赐福后端（Python/LangChain）- 仅打包脚本与依赖清单，不打包 .venv
Source: "..\\blessing_server\\ak_blessing_server.py"; DestDir: "{app}\\tools\\blessing_server"; Flags: ignoreversion
Source: "..\\blessing_server\\requirements.txt"; DestDir: "{app}\\tools\\blessing_server"; Flags: ignoreversion
Source: "..\\blessing_server\\run_win.ps1"; DestDir: "{app}\\tools\\blessing_server"; Flags: ignoreversion
Source: "..\\blessing_server\\README.md"; DestDir: "{app}\\tools\\blessing_server"; Flags: ignoreversion

; 启动脚本（安装后可直接点快捷方式启动）
Source: "scripts\\StartBlessingServer.cmd"; DestDir: "{app}"; Flags: ignoreversion
Source: "scripts\\StartAdventureKing.cmd"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\\Adventure-King"; Filename: "{app}\\{#AppExeName}"; WorkingDir: "{app}"
Name: "{group}\\Adventure-King（启动赐福后端）"; Filename: "{app}\\StartAdventureKing.cmd"; WorkingDir: "{app}"
Name: "{group}\\赐福后端（Blessing Server）"; Filename: "{app}\\StartBlessingServer.cmd"; WorkingDir: "{app}"

[Run]
Filename: "{app}\\StartBlessingServer.cmd"; Description: "安装完成后启动赐福后端（需要 Python 3.10+）"; Flags: postinstall skipifsilent unchecked
