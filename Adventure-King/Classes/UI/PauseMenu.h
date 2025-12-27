/**
 * @file PauseMenu.h
 * @brief 暂停菜单组件
 *
 * 游戏暂停时显示的菜单：
 * - 继续游戏
 * - 背包/技能
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

    /**
     * @brief 初始化暂停菜单节点
     */
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
    /// @brief 设置“继续游戏”回调
    void setResumeCallback(const std::function<void()> &callback) { _resumeCallback = callback; }
    /// @brief 设置“保存游戏”回调
    void setSaveCallback(const std::function<void()> &callback) { _saveCallback = callback; }
    /// @brief 设置“加载游戏”回调
    void setLoadCallback(const std::function<void()> &callback) { _loadCallback = callback; }
    /// @brief 设置“背包/技能”回调
    void setInventoryCallback(const std::function<void()> &callback) { _inventoryCallback = callback; }
    /// @brief 设置“赐福NPC”回调
    void setBlessingCallback(const std::function<void()> &callback) { _blessingCallback = callback; }
    /// @brief 设置“设置”回调
    void setSettingsCallback(const std::function<void()> &callback) { _settingsCallback = callback; }
    /// @brief 设置“主菜单”回调
    void setMainMenuCallback(const std::function<void()> &callback) { _mainMenuCallback = callback; }
    /// @brief 设置“退出游戏”回调
    void setQuitCallback(const std::function<void()> &callback) { _quitCallback = callback; }

protected:
    /// @brief 创建菜单背景
    void createBackground();
    /// @brief 创建标题文本
    void createTitle();
    /// @brief 创建功能按钮列表
    void createMenuButtons();

    // 按钮回调
    /// @brief 点击“继续”
    void onResumeClicked(cocos2d::Ref *sender);
    /// @brief 点击“保存”
    void onSaveClicked(cocos2d::Ref *sender);
    /// @brief 点击“加载”
    void onLoadClicked(cocos2d::Ref *sender);
    /// @brief 点击“背包/技能”
    void onInventoryClicked(cocos2d::Ref *sender);
    /// @brief 点击“赐福NPC”
    void onBlessingClicked(cocos2d::Ref *sender);
    /// @brief 点击“设置”
    void onSettingsClicked(cocos2d::Ref *sender);
    /// @brief 点击“主菜单”
    void onMainMenuClicked(cocos2d::Ref *sender);
    /// @brief 点击“退出”
    void onQuitClicked(cocos2d::Ref *sender);

    // 创建菜单按钮
    /// @brief 创建一个带回调的菜单按钮
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
    std::function<void()> _inventoryCallback;
    std::function<void()> _blessingCallback;
    std::function<void()> _settingsCallback;
    std::function<void()> _mainMenuCallback;
    std::function<void()> _quitCallback;
};
