$ErrorActionPreference = "Stop"

# ============================================================
# Adventure-King 云端存档服务（Win 一键构建 / PowerShell）
# - 适用于未安装 CMake 的情况
# - 自动定位 VS2022 的 cl.exe 并编译
# - 说明：为了兼容 Windows PowerShell 5.1，这里尽量避免复杂的转义与特殊语法
# ============================================================

function Write-Info([string]$msg) { Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Write-Err([string]$msg)  { Write-Host "[ERROR] $msg" -ForegroundColor Red }

try {
  $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  Set-Location $scriptDir

  # 兼容性：PowerShell 5.1 对 $env:ProgramFiles(x86) 语法非常敏感，避免直接访问，改用 .NET API
  $pf86 = [System.Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
  if (-not $pf86) { $pf86 = [System.Environment]::GetFolderPath([System.Environment+SpecialFolder]::ProgramFiles) }
  $pf64 = [System.Environment]::GetFolderPath([System.Environment+SpecialFolder]::ProgramFiles)

  $vswhereCandidates = @(
    (Join-Path $pf86 "Microsoft Visual Studio\Installer\vswhere.exe"),
    (Join-Path $pf64 "Microsoft Visual Studio\Installer\vswhere.exe")
  ) | Select-Object -Unique

  $vswhere = $vswhereCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
  if (-not $vswhere) {
    throw ("vswhere.exe not found. Tried: " + ($vswhereCandidates -join "; "))
  }

  $vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if (-not $vsInstall) {
    throw "Visual Studio C++ toolchain not found. Please install 'Desktop development with C++' in VS Installer."
  }

  $vsDevCmd = Join-Path $vsInstall "Common7\Tools\VsDevCmd.bat"
  if (-not (Test-Path $vsDevCmd)) {
    throw "VsDevCmd.bat not found: $vsDevCmd"
  }

  $rapidJsonInc = Resolve-Path (Join-Path $scriptDir "..\..\Adventure-King\cocos2d\external")
  $src = Join-Path $scriptDir "src\main.cpp"
  $outExe = Join-Path $scriptDir "ak_cloud_save_server.exe"

  if (-not (Test-Path $src)) {
    throw "Source file not found: $src"
  }

  Write-Info "Compiling ak_cloud_save_server.exe ..."

  # 关键点：不把 VsDevCmd 的环境“搬进 PowerShell”，而是让 cmd 在同一进程里完成 VsDevCmd + cl
  $cmd = '"' + $vsDevCmd + '" -arch=x64 >nul && ' +
         'cl /nologo /std:c++17 /EHsc ' +
         '/I"' + $rapidJsonInc + '" ' +
         '"' + $src + '" ' +
         '/Fe:"' + $outExe + '" ' +
         'ws2_32.lib'

  & cmd.exe /c $cmd | Out-Host
  if ($LASTEXITCODE -ne 0) {
    throw "Build failed (exit=$LASTEXITCODE)"
  }

  Write-Info "Built: $outExe"
  Write-Host ""
  Write-Host "Run example:" -ForegroundColor Green
  Write-Host "  $outExe --root E:\ak_cloud_data --host 127.0.0.1 --port 5174"
  Write-Host ""
}
catch {
  Write-Err $_.Exception.Message
  exit 1
}
