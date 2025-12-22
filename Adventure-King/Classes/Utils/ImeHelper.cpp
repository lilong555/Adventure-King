/**
 * @file ImeHelper.cpp
 * @brief Win32 下禁用/恢复输入法（IME）实现
 */

#include "Utils/ImeHelper.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <windows.h>
#endif

USING_NS_CC;

namespace
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    using HimcHandle = HANDLE;
    using ImmAssociateContextFn = HimcHandle(WINAPI*)(HWND, HimcHandle);

    ImmAssociateContextFn getImmAssociateContextFn()
    {
        static HMODULE sImmModule = ::LoadLibraryW(L"imm32.dll");
        if (!sImmModule)
        {
            return nullptr;
        }

        static ImmAssociateContextFn sFn = reinterpret_cast<ImmAssociateContextFn>(
            ::GetProcAddress(sImmModule, "ImmAssociateContext"));
        return sFn;
    }

    HWND getGameWindowHwnd()
    {
        auto view = Director::getInstance() ? Director::getInstance()->getOpenGLView() : nullptr;
        return view ? view->getWin32Window() : nullptr;
    }

    int sDisableCount = 0;
    HimcHandle sPrevHimc = nullptr;
#endif
} // namespace

namespace ImeHelper
{
    void pushDisableIme()
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
        auto hwnd = getGameWindowHwnd();
        if (!hwnd)
        {
            return;
        }

        if (sDisableCount == 0)
        {
            if (auto fn = getImmAssociateContextFn())
            {
                sPrevHimc = fn(hwnd, nullptr);
                CCLOG("ImeHelper: 已禁用输入法（IME）");
            }
        }
        ++sDisableCount;
#else
        (void)0;
#endif
    }

    void popDisableIme()
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
        if (sDisableCount <= 0)
        {
            return;
        }

        --sDisableCount;
        if (sDisableCount != 0)
        {
            return;
        }

        auto hwnd = getGameWindowHwnd();
        if (!hwnd)
        {
            sPrevHimc = nullptr;
            return;
        }

        if (auto fn = getImmAssociateContextFn())
        {
            fn(hwnd, sPrevHimc);
            sPrevHimc = nullptr;
            CCLOG("ImeHelper: 已恢复输入法（IME）");
        }
#else
        (void)0;
#endif
    }
} // namespace ImeHelper
