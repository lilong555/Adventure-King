// SetMenuLayer.h

#pragma once
#ifndef __SET_MENU_LAYER_H__
#define __SET_MENU_LAYER_H__

#include "cocos2d.h"
#include "ui/UISlider.h" // 引入 UI 组件头文件


class SettingMenuLayer : public cocos2d::Layer
{
public:
    static SettingMenuLayer* create();
    virtual bool init();

private:
    const float TARGET_HEIGHT_RATIO = 0.6f;//缩放的比例

    // 初始化组件
    bool initBackground();
    bool initCloseButton();
    bool initMusicToggle(); // 初始化音量开关

    // 布局
    void layoutUI();

    // 回调
    void onClose(cocos2d::Ref* sender);
    void onMusicToggle(Ref* sender);

    // 触摸事件 (用于吞噬触摸，保持模态)
    virtual bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event) override;

    cocos2d::Sprite* _background;
    cocos2d::MenuItemToggle* _musicToggle;
};

#endif // __SET_MENU_LAYER_H__
