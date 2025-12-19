// SetMenuLayer.h

#pragma once
#ifndef __SET_MENU_LAYER_H__
#define __SET_MENU_LAYER_H__

#include "cocos2d.h"
#include "ui/UISlider.h" // 引入 UI 组件头文件


class SettingMenuLayer : public cocos2d::Layer
{
public:
    /// @brief 创建设置菜单
    static SettingMenuLayer* create();
    /// @brief 初始化设置菜单
    virtual bool init();

private:
    // 初始化组件
    /// @brief 初始化背景
    bool initBackground();
    /// @brief 初始化关闭按钮
    bool initCloseButton();
    /// @brief 初始化音量开关
    bool initMusicToggle(); // 初始化音量开关

    // 布局
    /// @brief 布局 UI 元素
    void layoutUI();

    // 回调
    /// @brief 关闭设置菜单
    void onClose(cocos2d::Ref* sender);
    /// @brief 切换音乐开关
    void onMusicToggle(Ref* sender);

    // 触摸事件 (用于吞噬触摸，保持模态)
    /// @brief 吞噬触摸，保持模态
    virtual bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event) override;

    cocos2d::Sprite* _background;
    cocos2d::MenuItemToggle* _musicToggle;
};

#endif // __SET_MENU_LAYER_H__
