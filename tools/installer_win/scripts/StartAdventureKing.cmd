@echo off
setlocal

set "ROOT=%~dp0"
REM 说明：
REM - 启动器会随游戏启动/关闭赐福后端（若后端启动失败不会阻止游戏运行）。

start "Adventure-King" "%ROOT%AKLauncher.exe"
