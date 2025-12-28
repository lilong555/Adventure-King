param(
  [string]$ListenHost = "127.0.0.1",
  [int]$Port = 5181,
  [string]$OpenAiBaseUrl = "",
  [string]$BlessingModel = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

function Find-Python {
  if (Get-Command py -ErrorAction SilentlyContinue) { return @{ Exe = "py"; Args = @("-3") } }
  if (Get-Command python3 -ErrorAction SilentlyContinue) { return @{ Exe = "python3"; Args = @() } }
  if (Get-Command python -ErrorAction SilentlyContinue) { return @{ Exe = "python"; Args = @() } }
  throw "Python 3.10+ not found. Please install Python and add it to PATH."
}

function Ensure-Venv($PyCmd) {
  $venvPython = Join-Path $PSScriptRoot ".venv\\Scripts\\python.exe"
  if (Test-Path $venvPython) { return }
  Write-Host "[INFO] Creating venv (.venv) ..."
  & $PyCmd.Exe @($PyCmd.Args) -m venv .venv
}

function Pip-Install([string]$VenvPython) {
  Write-Host "[INFO] Installing dependencies ..."
  # Ensure pip is available and not broken (some Python distributions ship without a fully working pip).
  & $VenvPython -m ensurepip --upgrade
  & $VenvPython -m pip install -U pip
  & $VenvPython -m pip install -r (Join-Path $PSScriptRoot "requirements.txt")
}

$pyCmd = Find-Python
Ensure-Venv $pyCmd

try {
  $venvPython = (Resolve-Path (Join-Path $PSScriptRoot ".venv\\Scripts\\python.exe")).Path
} catch {
  throw "Venv python not found. Delete .venv and retry."
}
Pip-Install $venvPython

if ($OpenAiBaseUrl -ne "") { $env:AK_OPENAI_BASE_URL = $OpenAiBaseUrl }
if ($BlessingModel -ne "") { $env:AK_BLESSING_MODEL = $BlessingModel }

Write-Host "[OK] Starting Blessing server: http://$ListenHost`:$Port"
Write-Host "[INFO] Stop: close this window or press Ctrl+C"

& $venvPython (Join-Path $PSScriptRoot "ak_blessing_server.py") serve --host $ListenHost --port $Port
