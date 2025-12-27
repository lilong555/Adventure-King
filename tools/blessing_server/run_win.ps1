param(
  [string]$ListenHost = "127.0.0.1",
  [int]$Port = 5181,
  [string]$OpenAiBaseUrl = "",
  [string]$BlessingModel = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Find-Python {
  if (Get-Command py -ErrorAction SilentlyContinue) { return @{ Exe = "py"; Args = @("-3") } }
  if (Get-Command python3 -ErrorAction SilentlyContinue) { return @{ Exe = "python3"; Args = @() } }
  if (Get-Command python -ErrorAction SilentlyContinue) { return @{ Exe = "python"; Args = @() } }
  throw "Python 3.10+ not found. Please install Python and add it to PATH."
}

function Ensure-Venv($PyCmd) {
  $venvPython = Join-Path ".venv" "Scripts/python.exe"
  if (Test-Path $venvPython) { return }
  Write-Host "[INFO] Creating venv (.venv) ..."
  & $PyCmd.Exe @($PyCmd.Args) -m venv .venv
}

function Pip-Install([string]$VenvPython) {
  Write-Host "[INFO] Installing dependencies ..."
  & $VenvPython -m pip install -U pip
  & $VenvPython -m pip install -r requirements.txt
}

$pyCmd = Find-Python
Ensure-Venv $pyCmd

$venvPython = (Resolve-Path (Join-Path ".venv" "Scripts/python.exe")).Path
Pip-Install $venvPython

if ($OpenAiBaseUrl -ne "") { $env:AK_OPENAI_BASE_URL = $OpenAiBaseUrl }
if ($BlessingModel -ne "") { $env:AK_BLESSING_MODEL = $BlessingModel }

Write-Host "[OK] Starting Blessing server: http://$ListenHost`:$Port"
Write-Host "[INFO] Stop: close this window or press Ctrl+C"

& $venvPython ak_blessing_server.py serve --host $ListenHost --port $Port
