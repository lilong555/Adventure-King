#include "Utils/ClipboardHelper.h"

#include "cocos2d.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

namespace
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    std::string utf16ToUtf8(const std::wstring &wstr)
    {
        if (wstr.empty())
        {
            return "";
        }
        const int needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        if (needed <= 0)
        {
            return "";
        }
        std::string out;
        out.resize((size_t)needed);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &out[0], needed, nullptr, nullptr);
        return out;
    }

    std::wstring utf8ToUtf16(const std::string &str)
    {
        if (str.empty())
        {
            return L"";
        }
        const int needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
        if (needed <= 0)
        {
            return L"";
        }
        std::wstring out;
        out.resize((size_t)needed);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &out[0], needed);
        return out;
    }
#endif
} // namespace

namespace ClipboardHelper
{
    std::string getText()
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
        if (!OpenClipboard(nullptr))
        {
            return "";
        }

        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (!hData)
        {
            CloseClipboard();
            return "";
        }

        auto locked = static_cast<const wchar_t *>(GlobalLock(hData));
        if (!locked)
        {
            CloseClipboard();
            return "";
        }

        std::wstring wstr(locked);
        GlobalUnlock(hData);
        CloseClipboard();

        return utf16ToUtf8(wstr);
#else
        return "";
#endif
    }

    void setText(const std::string &text)
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
        if (!OpenClipboard(nullptr))
        {
            return;
        }

        if (!EmptyClipboard())
        {
            CloseClipboard();
            return;
        }

        const std::wstring wstr = utf8ToUtf16(text);
        const size_t bytes = (wstr.size() + 1) * sizeof(wchar_t);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (!hMem)
        {
            CloseClipboard();
            return;
        }

        void *ptr = GlobalLock(hMem);
        if (!ptr)
        {
            GlobalFree(hMem);
            CloseClipboard();
            return;
        }

        memcpy(ptr, wstr.c_str(), bytes);
        GlobalUnlock(hMem);

        // 成功后剪贴板接管内存，无需释放；失败则由我们释放
        if (!SetClipboardData(CF_UNICODETEXT, hMem))
        {
            GlobalFree(hMem);
        }

        CloseClipboard();
#else
        (void)text;
#endif
    }
} // namespace ClipboardHelper
