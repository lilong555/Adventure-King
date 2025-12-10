/**
 * @file PauseMenu.h
 * @brief 暂停菜单组件
 *
 * 游戏暂停时显示的菜单：
 * - 继续游戏
 * - 设置
 * - 返回主菜单
 * - 退出游戏
 */

#pragma once

#include "cocos2d.h"
#include <functional>

class PauseMenu : public cocos2d::Layer
{
public:
    /**
     * @brief 创建暂停菜单
     * @return PauseMenu 指针
     */
    static PauseMenu *create();

    virtual bool init() override;

    /**
     * @brief 显示暂停菜单
     */
    void show();

    /**
     * @brief 隐藏暂停菜单
     */
    void hide();

    /**
     * @brief 是否正在显示
     */
    bool isShowing() const { return _isShowing; }

    /**
     * @brief 更新位置（跟随相机）
     * @param cameraOffset 相机偏移量
     */
    void updatePosition(const cocos2d::Vec2 &cameraOffset);

    // 回调设置
    void setResumeCallback(const std::function<void()> &callback) { _resumeCallback = callback; }
    void setSaveCallback(const std::function<void()> &callback) { _saveCallback = callback; }
    void setLoadCallback(const std::function<void()> &callback) { _loadCallback = callback; }
    void setSettingsCallback(const std::function<void()> &callback) { _settingsCallback = callback; }
    void setMainMenuCallback(const std::function<void()> &callback) { _mainMenuCallback = callback; }
    void setQuitCallback(const std::function<void()> &callback) { _quitCallback = callback; }

protected:
    void createBackground();
    void createTitle();
    void createMenuButtons();

    // 按钮回调
    void onResumeClicked(cocos2d::Ref *sender);
    void onSaveClicked(cocos2d::Ref *sender);
    void onLoadClicked(cocos2d::Ref *sender);
    void onSettingsClicked(cocos2d::Ref *sender);
    void onMainMenuClicked(cocos2d::Ref *sender);
    void onQuitClicked(cocos2d::Ref *sender);

    // 创建菜单按钮
    cocos2d::MenuItemLabel *createButton(const std::string &text,
                                         const cocos2d::ccMenuCallback &callback);

protected:
    cocos2d::Node *_container = nullptr;
    cocos2d::DrawNode *_background = nullptr;
    cocos2d::Label *_titleLabel = nullptr;
    cocos2d::Menu *_menu = nullptr;
    cocos2d::EventListenerTouchOneByOne *_touchListener = nullptr;

    bool _isShowing = false;

    // 回调函数
    std::function<void()> _resumeCallback;
    std::function<void()> _saveCallback;
    std::function<void()> _loadCallback;
    std::function<void()> _settingsCallback;
    std::function<void()> _mainMenuCallback;
    std::function<void()> _quitCallback;
};
