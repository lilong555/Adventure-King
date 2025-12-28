@echo off
setlocal

REM 说明：
REM - 本脚本会先尝试启动赐福后端（会开一个窗口），再启动游戏。
REM - 赐福后端需要本机安装 Python 3.10+（并在 PATH 可用）。

set "ROOT=%~dp0"

start "AK Blessing Server" cmd /c "%ROOT%StartBlessingServer.cmd"

REM 给后端一点启动时间（避免游戏内立刻点赐福时 404/连接失败）
timeout /t 2 >nul

start "Adventure-King" "%ROOT%Adventure-King.exe"
