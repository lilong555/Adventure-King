#include <windows.h>

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

static HANDLE createKillOnCloseJob()
{
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job) return nullptr;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info, sizeof(info));
    return job;
}

static bool startProcess(const std::wstring& app, const std::wstring& cmdLine, DWORD flags, PROCESS_INFORMATION& pi)
{
    STARTUPINFOW si = {};
    si.cb = sizeof(si);

    std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
    mutableCmd.push_back(L'\0');

    ZeroMemory(&pi, sizeof(pi));
    BOOL ok = CreateProcessW(
        app.empty() ? nullptr : app.c_str(),
        mutableCmd.data(),
        nullptr,
        nullptr,
        FALSE,
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

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    const std::wstring root = getExeDir();
    const std::wstring gameExe = root + L"\\Adventure-King.exe";
    const std::wstring serverPs1 = root + L"\\tools\\blessing_server\\run_win.ps1";

    if (!fileExists(gameExe)) {
        showInfoBox(L"Adventure-King", L"Game exe not found:\n" + gameExe);
        return 1;
    }

    HANDLE job = createKillOnCloseJob();

    PROCESS_INFORMATION serverPi = {};
    bool serverStarted = false;
    if (fileExists(serverPs1)) {
        std::wstring serverCmd = L"powershell -ExecutionPolicy Bypass -File " + quote(serverPs1) +
                                 L" -ListenHost 127.0.0.1 -Port 5181";
        if (startProcess(L"", serverCmd, CREATE_NO_WINDOW, serverPi)) {
            serverStarted = true;
            if (job) AssignProcessToJobObject(job, serverPi.hProcess);
        }
    }

    if (serverStarted) {
        Sleep(2000);
    }

    PROCESS_INFORMATION gamePi = {};
    std::wstring gameCmd = quote(gameExe);
    if (!startProcess(L"", gameCmd, 0, gamePi)) {
        if (serverStarted) {
            CloseHandle(serverPi.hThread);
            CloseHandle(serverPi.hProcess);
        }
        if (job) CloseHandle(job);
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

    return static_cast<int>(exitCode);
}

