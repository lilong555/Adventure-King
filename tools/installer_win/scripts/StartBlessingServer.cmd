@echo off
setlocal

set "ROOT=%~dp0"
set "SERVER_DIR=%ROOT%tools\\blessing_server"

if not exist "%SERVER_DIR%\\ak_blessing_server.py" (
  echo [ERROR] 未找到赐福后端文件：%SERVER_DIR%\ak_blessing_server.py
  echo         请确认安装目录完整。
  pause
  exit /b 1
)

cd /d "%SERVER_DIR%"

REM 启动赐福后端（第一次运行会创建 .venv 并安装依赖）
powershell -ExecutionPolicy Bypass -File ".\\run_win.ps1" -ListenHost 127.0.0.1 -Port 5181
