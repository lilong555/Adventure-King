param(
  [string]$GameOutDir = "",
  [string]$OutputDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
  $here = Split-Path -Parent $PSCommandPath
  return (Resolve-Path (Join-Path $here "..\\..")).Path
}

function Find-IsccExe {
  $candidates = @(
    "C:\\Program Files (x86)\\Inno Setup 6\\ISCC.exe",
    "C:\\Program Files\\Inno Setup 6\\ISCC.exe",
    "$env:LOCALAPPDATA\\Programs\\Inno Setup 6\\ISCC.exe"
  )

  foreach ($p in $candidates) {
    if (Test-Path $p) { return $p }
  }

  $cmd = Get-Command ISCC -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Path }
  $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Path }

  $regKeys = @(
    "HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Inno Setup 6_is1",
    "HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Inno Setup 6_is1",
    "HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Inno Setup 6_is1"
  )
  foreach ($k in $regKeys) {
    $p = Get-ItemProperty -Path $k -ErrorAction SilentlyContinue
    if ($null -ne $p -and $p.InstallLocation) {
      $iscc = Join-Path $p.InstallLocation "ISCC.exe"
      if (Test-Path $iscc) { return $iscc }
    }
  }

  throw "Inno Setup 6 (ISCC.exe) not found. Please install Inno Setup 6 and ensure ISCC.exe is available."
}

$ErrorActionPreference = "Stop"

function Get-ProgramFilesX86 {
  $pf86 = [System.Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
  if ($pf86) { return $pf86 }
  return [System.Environment]::GetFolderPath([System.Environment+SpecialFolder]::ProgramFiles)
}

function Find-VsWhere {
  $pf86 = Get-ProgramFilesX86
  $pf64 = [System.Environment]::GetFolderPath([System.Environment+SpecialFolder]::ProgramFiles)
  $candidates = @(
    (Join-Path $pf86 "Microsoft Visual Studio\\Installer\\vswhere.exe"),
    (Join-Path $pf64 "Microsoft Visual Studio\\Installer\\vswhere.exe")
  ) | Select-Object -Unique

  foreach ($p in $candidates) {
    if (Test-Path $p) { return $p }
  }
  return $null
}

function Find-VsDevCmd {
  $vswhere = Find-VsWhere
  if (-not $vswhere) { return $null }

  $vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if (-not $vsInstall) { return $null }

  $vsDevCmd = Join-Path $vsInstall "Common7\\Tools\\VsDevCmd.bat"
  if (Test-Path $vsDevCmd) { return $vsDevCmd }
  return $null
}

function Build-Launcher([string]$repoRoot) {
  $src = Join-Path $repoRoot "tools\\installer_win\\launcher\\AKLauncher.cpp"
  if (!(Test-Path $src)) { throw "Launcher source not found: $src" }

  $outDir = Join-Path $repoRoot "tools\\installer_win\\build"
  New-Item -ItemType Directory -Force -Path $outDir | Out-Null
  $outExe = Join-Path $outDir "AKLauncher.exe"

  $vsDevCmd = Find-VsDevCmd
  if (-not $vsDevCmd) {
    throw "Visual Studio C++ toolchain not found (VsDevCmd.bat). Please install 'Desktop development with C++' in VS Installer."
  }

  Write-Host "[INFO] Building AKLauncher.exe ..."

  # Use cmd so VsDevCmd + cl run in the same process environment.
  # Build x64 launcher (works fine with Win32 game). Keep flags minimal.
  $cmd = '"' + $vsDevCmd + '" -arch=x64 >nul && ' +
         'cl /nologo /std:c++17 /EHsc /O2 ' +
         '"' + $src + '" ' +
         '/Fe:"' + $outExe + '" ' +
         'user32.lib shell32.lib ole32.lib'

  & cmd.exe /c $cmd | Out-Host
  if ($LASTEXITCODE -ne 0) {
    throw "AKLauncher build failed (exit=$LASTEXITCODE)"
  }

  if (!(Test-Path $outExe)) {
    throw "AKLauncher.exe not produced: $outExe"
  }

  Write-Host "[OK] Built: $outExe"
  return $outExe
}

function Find-AnyPython {
  $cmd = Get-Command python -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Path }

  $cmd = Get-Command python3 -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Path }

  $cmd = Get-Command py -ErrorAction SilentlyContinue
  if ($cmd) { return "py -3" }

  return $null
}

function Build-BlessingWheelhouse([string]$repoRoot) {
  $py = Find-AnyPython
  if (-not $py) {
    Write-Host "[WARN] Python not found; skip offline wheelhouse."
    return
  }

  $req = Join-Path $repoRoot "tools\\blessing_server\\requirements.txt"
  if (!(Test-Path $req)) {
    Write-Host "[WARN] requirements.txt not found; skip offline wheelhouse."
    return
  }

  $wheelDir = Join-Path $repoRoot "tools\\installer_win\\build\\blessing_wheels"
  New-Item -ItemType Directory -Force -Path $wheelDir | Out-Null

  Write-Host "[INFO] Preparing offline wheelhouse (cp310/cp311/cp312) ..."

  # ensurepip for the host python (best-effort)
  try {
    if ($py -eq "py -3") {
      & py -3 -m ensurepip --upgrade | Out-Host
    } else {
      & $py -m ensurepip --upgrade | Out-Host
    }
  } catch {
    Write-Host "[WARN] ensurepip failed; continue."
  }

  $targets = @(
    @{ pyver = "310"; abi = "cp310" },
    @{ pyver = "311"; abi = "cp311" },
    @{ pyver = "312"; abi = "cp312" }
  )

  foreach ($t in $targets) {
    $args = @(
      "-m", "pip", "download",
      "-r", $req,
      "-d", $wheelDir,
      "--only-binary", ":all:",
      "--platform", "win_amd64",
      "--implementation", "cp",
      "--python-version", $t.pyver,
      "--abi", $t.abi
    )

    try {
      if ($py -eq "py -3") {
        & py -3 @args | Out-Host
      } else {
        & $py @args | Out-Host
      }
    } catch {
      Write-Host "[WARN] Wheel download failed for cp$t($t.pyver) ($($t.abi)); continue."
    }
  }

  Write-Host "[OK] Wheelhouse: $wheelDir"
}

$ErrorActionPreference = "Stop"

function Ensure-VcRedistX86([string]$repoRoot) {
  $outDir = Join-Path $repoRoot "tools\\installer_win\\build"
  New-Item -ItemType Directory -Force -Path $outDir | Out-Null

  $dst = Join-Path $outDir "vc_redist.x86.exe"
  if (Test-Path $dst) {
    Write-Host "[INFO] vc_redist.x86.exe exists: $dst"
    return $dst
  }

  # 官方下载地址（会重定向到具体版本）
  $url = "https://aka.ms/vs/17/release/vc_redist.x86.exe"
  Write-Host "[INFO] Downloading vc_redist.x86.exe ..."
  try {
    # 兼容旧版 Windows PowerShell：确保使用 TLS 1.2
    try {
      [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    } catch {
      # ignore
    }
    Invoke-WebRequest -Uri $url -OutFile $dst -UseBasicParsing | Out-Null
  } catch {
    # 注意：StrictMode 下 catch 里引用未初始化变量会直接抛异常；这里用字面量/可重算的值，避免脚本自身报错。
    $manualUrl = "https://aka.ms/vs/17/release/vc_redist.x86.exe"
    $manualPath = Join-Path $outDir "vc_redist.x86.exe"
    $reason = $_.Exception.Message
    throw ("Failed to download vc_redist.x86.exe. Please download it manually from:`n{0}`nAnd place it at:`n{1}`nReason:`n{2}" -f $manualUrl, $manualPath, $reason)
  }

  if (!(Test-Path $dst)) {
    throw "vc_redist.x86.exe not found after download: $dst"
  }

  Write-Host "[OK] Downloaded: $dst"
  return $dst
}

$repoRoot = Resolve-RepoRoot
$issPath = Join-Path $repoRoot "tools\\installer_win\\AdventureKing.iss"

if ($GameOutDir -eq "") {
  $GameOutDir = Join-Path $repoRoot "Adventure-King\\proj.win32\\Release.win32"
}
if ($OutputDir -eq "") {
  $OutputDir = Join-Path $repoRoot "dist"
}

$gameExe = Join-Path $GameOutDir "Adventure-King.exe"
if (!(Test-Path $gameExe)) {
  throw "Release output not found: $gameExe. Please build Release|Win32 in Visual Studio first."
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$iscc = Find-IsccExe
Write-Host "[INFO] ISCC: $iscc"
Write-Host "[INFO] GameOutDir: $GameOutDir"
Write-Host "[INFO] OutputDir: $OutputDir"

# Ensure launcher exists (installer expects tools/installer_win/build/AKLauncher.exe)
Build-Launcher $repoRoot | Out-Null
Ensure-VcRedistX86 $repoRoot | Out-Null
Build-BlessingWheelhouse $repoRoot

& $iscc "/DGameOutDir=$GameOutDir" "/O$OutputDir" $issPath
Write-Host "[OK] Installer generated to: $OutputDir (default filename: Adventure-King-Setup.exe)"
