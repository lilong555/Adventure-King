$ErrorActionPreference = "Stop"

# ============================================================
# Adventure-King 云端存档服务（Win 一键构建 / PowerShell）
# - 适用于未安装 CMake 的情况
# - 会自动定位 VS2022 的 C++ 工具链并调用 cl 进行编译
# ============================================================

function Write-Info([string]$msg) { Write-Host "[信息] $msg" -ForegroundColor Cyan }
function Write-Warn([string]$msg) { Write-Host "[警告] $msg" -ForegroundColor Yellow }
function Write-Err([string]$msg)  { Write-Host "[错误] $msg" -ForegroundColor Red }

try {
  $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  Set-Location $scriptDir

  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (-not (Test-Path $vswhere)) {
    throw "未找到 vswhere.exe：$vswhere（请安装 Visual Studio 2022 或 VS Build Tools）"
  }

  $vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if (-not $vsInstall) {
    throw "未找到可用的 Visual Studio C++ 工具链（请在 VS Installer 勾选“Desktop development with C++”）"
  }

  $vsDevCmd = Join-Path $vsInstall "Common7\Tools\VsDevCmd.bat"
  if (-not (Test-Path $vsDevCmd)) {
    throw "未找到 VsDevCmd.bat：$vsDevCmd"
  }

  Write-Info "加载 VS 编译环境（x64）..."

  # 通过 cmd 执行 VsDevCmd，然后把 set 输出的环境变量同步到当前 PowerShell 进程
  $envLines = cmd /c "`"$vsDevCmd`" -arch=x64 >nul && set"
  foreach ($line in $envLines) {
    $idx = $line.IndexOf("=")
    if ($idx -le 0) { continue }
    $name = $line.Substring(0, $idx)
    $value = $line.Substring($idx + 1)
    $env:$name = $value
  }

  $rapidJsonInc = Resolve-Path (Join-Path $scriptDir "..\..\Adventure-King\cocos2d\external")
  $src = Join-Path $scriptDir "src\main.cpp"
  $outExe = Join-Path $scriptDir "ak_cloud_save_server.exe"

  if (-not (Test-Path $src)) {
    throw "找不到源文件：$src"
  }

  Write-Info "开始编译 ak_cloud_save_server.exe ..."

  & cl /nologo /std:c++17 /EHsc `
    /I"$rapidJsonInc" `
    "$src" `
    /Fe:"$outExe" `
    ws2_32.lib | Out-Host

  Write-Info "已生成：$outExe"
  Write-Host ""
  Write-Host "运行示例：" -ForegroundColor Green
  Write-Host "  $outExe --root E:\ak_cloud_data --host 127.0.0.1 --port 5173"
  Write-Host ""
}
catch {
  Write-Err $_.Exception.Message
  exit 1
}

