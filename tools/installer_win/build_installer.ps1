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

  throw "未找到 Inno Setup 6（ISCC.exe）。请先安装 Inno Setup 6，并确保 ISCC.exe 可用。"
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
  throw "未找到 Release 输出：$gameExe。请先在 VS 用 Release|Win32 构建一次。"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$iscc = Find-IsccExe
Write-Host "[INFO] ISCC: $iscc"
Write-Host "[INFO] GameOutDir: $GameOutDir"
Write-Host "[INFO] OutputDir: $OutputDir"

& $iscc "/DGameOutDir=$GameOutDir" "/O$OutputDir" $issPath
Write-Host "[OK] 已生成安装包到：$OutputDir（默认文件名：Adventure-King-Setup.exe）"
