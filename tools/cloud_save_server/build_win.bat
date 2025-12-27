@echo off
setlocal enabledelayedexpansion

REM ============================================================
REM Adventure-King 云端存档服务（Win 一键构建）
REM - 需要已安装 Visual Studio 2022（含 C++ 工具链）
REM - 本脚本会自动调用 VsDevCmd.bat，再用 cl 编译生成 exe
REM ============================================================

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [错误] 未找到 vswhere.exe：%VSWHERE%
  echo 请使用“x64 Native Tools Command Prompt for VS 2022”运行本脚本，或安装 Visual Studio C++ 工具链。
  exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if "%VSINSTALL%"=="" (
  echo [错误] 未找到可用的 Visual Studio C++ 工具链。
  echo 请在 VS Installer 勾选 “Desktop development with C++”。
  exit /b 1
)

set "VSDEVCMD=%VSINSTALL%\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
  echo [错误] 未找到 VsDevCmd.bat：%VSDEVCMD%
  exit /b 1
)

call "%VSDEVCMD%" -arch=x64 >nul
if errorlevel 1 (
  echo [错误] 初始化 VS 编译环境失败。
  exit /b 1
)

echo [信息] 开始编译 ak_cloud_save_server.exe ...

set "INCLUDE_RAPIDJSON=%SCRIPT_DIR%..\..\Adventure-King\cocos2d\external"
set "OUT_EXE=%SCRIPT_DIR%ak_cloud_save_server.exe"

cl /nologo /std:c++17 /EHsc ^
  /I"%INCLUDE_RAPIDJSON%" ^
  "%SCRIPT_DIR%src\main.cpp" ^
  /Fe:"%OUT_EXE%" ^
  ws2_32.lib

if errorlevel 1 (
  echo [错误] 编译失败。
  exit /b 1
)

echo [完成] 已生成：%OUT_EXE%
echo.
echo 运行示例：
echo   %OUT_EXE% --root E:\ak_cloud_data --host 127.0.0.1 --port 5174
echo.
exit /b 0
