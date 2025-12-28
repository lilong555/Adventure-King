; Adventure-King Windows 安装包脚本（Inno Setup 6）
; 目标：把 Release.win32 输出目录打包为安装包，并附带赐福后端（Python/LangChain）脚本。
;
; 使用方式（在 Windows PowerShell 执行）：
;   powershell -ExecutionPolicy Bypass -File tools\\installer_win\\build_installer.ps1
;
; 说明：
; - 游戏是 Win32（32 位）构建，安装到 Program Files (x86) 属正常现象。
; - 赐福后端是 Python 脚本，首次启动会创建 .venv 并安装依赖（需要本机有 Python 3.10+）。
; - 游戏使用 MSVC 动态 CRT（/MD）构建，安装器会自动安装 VC++ 2015-2022 运行库（x86）。

#define AppName "Adventure-King"
#define AppPublisher "Adventure-King Team"
#define AppExeName "Adventure-King.exe"
#define LauncherExeName "AKLauncher.exe"

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
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
OutputBaseFilename=Adventure-King-Setup
OutputDir=.
WizardStyle=modern

[Files]
; VC++ 运行库（x86），安装时会从 {tmp} 运行并在结束后删除
Source: "build\\vc_redist.x86.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall ignoreversion

; 游戏主体（Release.win32 输出目录）
Source: "{#GameOutDir}\\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#GameOutDir}\\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#GameOutDir}\\Resources\\*"; DestDir: "{app}\\Resources"; Flags: ignoreversion recursesubdirs createallsubdirs

; 启动器（用于随游戏启动/关闭赐福后端）
Source: "build\\{#LauncherExeName}"; DestDir: "{app}"; Flags: ignoreversion

; 赐福后端（Python/LangChain）- 仅打包脚本与依赖清单，不打包 .venv
Source: "..\\blessing_server\\ak_blessing_server.py"; DestDir: "{app}\\tools\\blessing_server"; Flags: ignoreversion
Source: "..\\blessing_server\\requirements.txt"; DestDir: "{app}\\tools\\blessing_server"; Flags: ignoreversion
Source: "..\\blessing_server\\run_win.ps1"; DestDir: "{app}\\tools\\blessing_server"; Flags: ignoreversion
Source: "..\\blessing_server\\README.md"; DestDir: "{app}\\tools\\blessing_server"; Flags: ignoreversion
; 离线依赖包（由 build_installer.ps1 生成）
Source: "build\\blessing_wheels\\*"; DestDir: "{app}\\tools\\blessing_server\\wheels"; Flags: ignoreversion recursesubdirs createallsubdirs

; 启动脚本（安装后可直接点快捷方式启动）
Source: "scripts\\StartBlessingServer.cmd"; DestDir: "{app}"; Flags: ignoreversion
Source: "scripts\\StartAdventureKing.cmd"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\\Adventure-King"; Filename: "{app}\\{#LauncherExeName}"; WorkingDir: "{app}"
Name: "{group}\\Adventure-King（直启，不启动赐福后端）"; Filename: "{app}\\{#AppExeName}"; WorkingDir: "{app}"
Name: "{group}\\赐福后端（Blessing Server）"; Filename: "{app}\\StartBlessingServer.cmd"; WorkingDir: "{app}"

[Run]
; 若系统已安装则跳过（避免反复安装）
Filename: "{tmp}\\vc_redist.x86.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "正在安装 VC++ 运行库（x86）..."; Flags: waituntilterminated; Check: VCRedistX86NeedsInstall
Filename: "{app}\\StartBlessingServer.cmd"; Description: "安装完成后启动赐福后端（需要 Python 3.10+）"; Flags: postinstall skipifsilent unchecked

[Code]
function VCRedistX86NeedsInstall: Boolean;
var
  Installed: Cardinal;
begin
  Result := True;

  { VC++ 2015-2022 Redistributable（x86）常见注册表位置 }
  if RegQueryDWordValue(HKLM32, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86', 'Installed', Installed) then
  begin
    Result := (Installed <> 1);
  end;
end;
