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
    "C:\\Program Files\\Inno Setup 6\\ISCC.exe"
  )

  foreach ($p in $candidates) {
    if (Test-Path $p) { return $p }
  }

  $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Path }

  throw "Inno Setup 6 (ISCC.exe) not found. Please install Inno Setup 6 and ensure ISCC.exe is available."
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

& $iscc "/DGameOutDir=$GameOutDir" "/O$OutputDir" $issPath
Write-Host "[OK] Installer generated to: $OutputDir (default filename: Adventure-King-Setup.exe)"
