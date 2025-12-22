/**
 * @file ImeHelper.h
 * @brief Win32 下禁用/恢复输入法（IME）工具
 *
 * 背景：
 * - 部分中文输入法在“中文模式”下会抢占按键（例如 WASD/JK 等），导致游戏键盘输入不稳定。
 * - 本工具通过对游戏窗口解除 IME 关联，保证键盘事件稳定传递到游戏逻辑。
 *
 * 设计：
 * - 仅在 Win32 下生效，其它平台为 no-op。
 * - 使用引用计数，允许多个场景叠加调用（enter/exit 成对）。
 * - 通过动态加载 imm32.dll 获取 ImmAssociateContext，避免额外的链接依赖问题。
 */

#pragma once

#include "cocos2d.h"

namespace ImeHelper
{
    /// @brief 进入“游戏运行态”时调用：禁用 IME（引用计数 +1）
    void pushDisableIme();

    /// @brief 离开“游戏运行态”时调用：恢复 IME（引用计数 -1）
    void popDisableIme();
} // namespace ImeHelper

