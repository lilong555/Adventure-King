/**
 * @file ImeHelper.cpp
 * @brief Win32 下禁用/恢复输入法（IME）实现
 */

#include "Utils/ImeHelper.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <windows.h>
#include <mutex>
#endif

USING_NS_CC;

namespace
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    using HimcHandle = HANDLE;
    using ImmAssociateContextFn = HimcHandle(WINAPI*)(HWND, HimcHandle);

    // 说明：
    // - 使用动态加载 imm32.dll 是为了避免额外的链接依赖。
    // - 这里用 RAII 封装模块句柄，程序退出时自动释放（释放的是本模块的引用计数，不影响系统/其它模块持有的引用）。
    struct ImmModuleHandle
    {
        HMODULE handle = nullptr;

        ImmModuleHandle()
            : handle(::LoadLibraryW(L"imm32.dll"))
        {
        }

        ~ImmModuleHandle()
        {
            if (handle)
            {
                ::FreeLibrary(handle);
                handle = nullptr;
            }
        }
    };

    ImmAssociateContextFn getImmAssociateContextFn()
    {
        static ImmModuleHandle immModule;
        if (!immModule.handle)
        {
            return nullptr;
        }

        static ImmAssociateContextFn sFn = reinterpret_cast<ImmAssociateContextFn>(
            ::GetProcAddress(immModule.handle, "ImmAssociateContext"));
        return sFn;
    }

    HWND getGameWindowHwnd()
    {
        auto view = Director::getInstance() ? Director::getInstance()->getOpenGLView() : nullptr;
        return view ? view->getWin32Window() : nullptr;
    }

    std::mutex sMutex;
    int sDisableCount = 0;
    HimcHandle sPrevHimc = nullptr;
    bool sDisabledByUs = false;
    bool sWarnedPopMismatch = false;
    bool sWarnedNoImmApi = false;
#endif
} // namespace

namespace ImeHelper
{
    void pushDisableIme()
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
        std::lock_guard<std::mutex> lock(sMutex);

        auto hwnd = getGameWindowHwnd();
        if (!hwnd)
        {
            return;
        }

        // 说明：即使禁用失败，也会递增计数，保证 onEnter/onExit 的 push/pop 调用对称。
        // 恢复时会根据 sDisabledByUs 判断是否需要真正恢复输入法。
        if (sDisableCount == 0)
        {
            sPrevHimc = nullptr;
            sDisabledByUs = false;

            if (auto fn = getImmAssociateContextFn())
            {
                sPrevHimc = fn(hwnd, nullptr);
                sDisabledByUs = true;
                CCLOG("ImeHelper: 已禁用输入法（IME）");
            }
            else if (!sWarnedNoImmApi)
            {
                sWarnedNoImmApi = true;
                CCLOG("ImeHelper: 禁用输入法失败，ImmAssociateContext 不可用");
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
        std::lock_guard<std::mutex> lock(sMutex);

        if (sDisableCount <= 0)
        {
            if (!sWarnedPopMismatch)
            {
                sWarnedPopMismatch = true;
                CCLOG("ImeHelper: popDisableIme 调用异常（计数=%d），可能 push/pop 不匹配", sDisableCount);
            }
            sDisableCount = 0;
            sPrevHimc = nullptr;
            sDisabledByUs = false;
            return;
        }

        --sDisableCount;
        if (sDisableCount != 0)
        {
            return;
        }

        // 本轮未成功禁用输入法，则无需恢复。
        if (!sDisabledByUs)
        {
            sPrevHimc = nullptr;
            return;
        }

        auto hwnd = getGameWindowHwnd();
        if (!hwnd)
        {
            CCLOG("ImeHelper: 无法恢复输入法（IME），窗口句柄不可用，丢弃先前的 IME 上下文");
            sPrevHimc = nullptr;
            sDisabledByUs = false;
            return;
        }

        if (auto fn = getImmAssociateContextFn())
        {
            fn(hwnd, sPrevHimc);
            sPrevHimc = nullptr;
            sDisabledByUs = false;
            CCLOG("ImeHelper: 已恢复输入法（IME）");
        }
        else
        {
            CCLOG("ImeHelper: 无法恢复输入法（IME），ImmAssociateContext 不可用，丢弃先前的 IME 上下文");
            sPrevHimc = nullptr;
            sDisabledByUs = false;
        }
#else
        (void)0;
#endif
    }
} // namespace ImeHelper
