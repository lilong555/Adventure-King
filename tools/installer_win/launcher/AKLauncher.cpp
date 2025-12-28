#include <windows.h>
#include <shlobj.h>

#include <string>
#include <vector>

static std::wstring getExeDir()
{
    wchar_t buf[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring path(buf, n);
    auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L".";
    return path.substr(0, pos);
}

static std::wstring quote(const std::wstring& s)
{
    std::wstring out;
    out.reserve(s.size() + 2);
    out.push_back(L'"');
    out.append(s);
    out.push_back(L'"');
    return out;
}

static bool fileExists(const std::wstring& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static bool ensureDirExists(const std::wstring& dir)
{
    if (dir.empty()) return false;
    DWORD a = GetFileAttributesW(dir.c_str());
    if (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY)) return true;

    auto pos = dir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        std::wstring parent = dir.substr(0, pos);
        if (!parent.empty() && parent != dir) {
            ensureDirExists(parent);
        }
    }
    return CreateDirectoryW(dir.c_str(), nullptr) == TRUE || GetLastError() == ERROR_ALREADY_EXISTS;
}

static std::wstring getBlessingLogPath()
{
    PWSTR localAppData = nullptr;
    std::wstring base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData)) && localAppData) {
        base = localAppData;
        CoTaskMemFree(localAppData);
    } else {
        base = L".";
    }

    std::wstring logDir = base + L"\\Adventure-King\\logs";
    ensureDirExists(logDir);
    return logDir + L"\\blessing_server.log";
}

static HANDLE openLogFile(const std::wstring& logPath)
{
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE h = CreateFileW(
        logPath.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &sa,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    return h;
}

static HANDLE createKillOnCloseJob()
{
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job) return nullptr;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info, sizeof(info));
    return job;
}

static bool startProcess(const std::wstring& app, const std::wstring& cmdLine, DWORD flags, bool inheritHandles, HANDLE stdOutErr, PROCESS_INFORMATION& pi)
{
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    if (stdOutErr) {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = stdOutErr;
        si.hStdError = stdOutErr;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }

    std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
    mutableCmd.push_back(L'\0');

    ZeroMemory(&pi, sizeof(pi));
    BOOL ok = CreateProcessW(
        app.empty() ? nullptr : app.c_str(),
        mutableCmd.data(),
        nullptr,
        nullptr,
        inheritHandles ? TRUE : FALSE,
        flags,
        nullptr,
        nullptr,
        &si,
        &pi);
    return ok == TRUE;
}

static void showInfoBox(const std::wstring& title, const std::wstring& msg)
{
    MessageBoxW(nullptr, msg.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

static void showErrorBox(const std::wstring& title, const std::wstring& msg)
{
    MessageBoxW(nullptr, msg.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    const std::wstring root = getExeDir();
    const std::wstring gameExe = root + L"\\Adventure-King.exe";
    const std::wstring serverPs1 = root + L"\\tools\\blessing_server\\run_win.ps1";
    const std::wstring logPath = getBlessingLogPath();

    if (!fileExists(gameExe)) {
        showInfoBox(L"Adventure-King", L"Game exe not found:\n" + gameExe);
        return 1;
    }

    HANDLE job = createKillOnCloseJob();

    PROCESS_INFORMATION serverPi = {};
    bool serverStarted = false;
    HANDLE logFile = nullptr;
    if (fileExists(serverPs1)) {
        logFile = openLogFile(logPath);
        std::wstring serverCmd = L"powershell -ExecutionPolicy Bypass -File " + quote(serverPs1) +
                                 L" -ListenHost 127.0.0.1 -Port 5181";
        if (startProcess(L"", serverCmd, CREATE_NO_WINDOW, true, logFile, serverPi)) {
            serverStarted = true;
            if (job) AssignProcessToJobObject(job, serverPi.hProcess);
        }
    }

    if (!serverStarted && fileExists(serverPs1)) {
        std::wstring msg = L"Failed to start Blessing server.\n\nLog:\n" + logPath;
        showErrorBox(L"Adventure-King", msg);
    }

    if (serverStarted) {
        Sleep(2000);

        DWORD code = STILL_ACTIVE;
        if (GetExitCodeProcess(serverPi.hProcess, &code) && code != STILL_ACTIVE) {
            std::wstring msg = L"Blessing server exited unexpectedly.\n\nLog:\n" + logPath;
            showErrorBox(L"Adventure-King", msg);
        }
    }

    PROCESS_INFORMATION gamePi = {};
    std::wstring gameCmd = quote(gameExe);
    if (!startProcess(L"", gameCmd, 0, false, nullptr, gamePi)) {
        if (serverStarted) {
            CloseHandle(serverPi.hThread);
            CloseHandle(serverPi.hProcess);
        }
        if (job) CloseHandle(job);
        if (logFile) CloseHandle(logFile);
        showInfoBox(L"Adventure-King", L"Failed to start game process.");
        return 2;
    }

    WaitForSingleObject(gamePi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(gamePi.hProcess, &exitCode);

    CloseHandle(gamePi.hThread);
    CloseHandle(gamePi.hProcess);

    if (serverStarted) {
        CloseHandle(serverPi.hThread);
        CloseHandle(serverPi.hProcess);
    }
    if (job) {
        CloseHandle(job);
    }
    if (logFile) {
        CloseHandle(logFile);
    }

    return static_cast<int>(exitCode);
}
