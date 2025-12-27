#pragma once

#include <string>

/**
 * @brief 剪贴板工具（目前主要用于 Win32 下的 Ctrl+C / Ctrl+V 支持）
 *
 * 说明：
 * - Cocos2d-x 的 ui::TextField 属于“引擎渲染输入框”，默认不支持系统级复制/粘贴。
 * - 本工具提供最小能力：读取/写入纯文本剪贴板（UTF-8）。
 * - 其它平台默认返回空/不处理（展示阶段先保证 Win32 可用）。
 */
namespace ClipboardHelper
{
    /// @brief 读取剪贴板文本（UTF-8）；失败返回空字符串
    std::string getText();

    /// @brief 写入剪贴板文本（UTF-8）；失败直接忽略
    void setText(const std::string &text);
} // namespace ClipboardHelper

