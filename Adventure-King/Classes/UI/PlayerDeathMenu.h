/**
 * @file PlayerDeathMenu.h
 * @brief 角色死亡菜单（强制暂停）
 *
 * 需求：
 * - 玩家死亡后，游戏暂停（冻结世界，UI 可交互）
 * - 显示两个选项：重新挑战 / 返回地图
 */
 
#pragma once

#include "cocos2d.h"
#include <functional>

class PlayerDeathMenu : public cocos2d::Layer
{
public:
    static PlayerDeathMenu *create();
    bool init() override;

    void show();
    void hide();
    /// @brief 立即隐藏（不播放淡出），用于场景切换前防止 UI 残留
    void hideImmediately();
    bool isShowing() const { return _isShowing; }

    void setRestartCallback(const std::function<void()> &callback) { _restartCallback = callback; }
    void setReturnToMapCallback(const std::function<void()> &callback) { _returnToMapCallback = callback; }

private:
    void createBackground();
    void createTitle();
    void createMenuButtons();
    cocos2d::MenuItemLabel *createButton(const std::string &text, const cocos2d::ccMenuCallback &callback);

    void onRestartClicked(cocos2d::Ref *sender);
    void onReturnToMapClicked(cocos2d::Ref *sender);

private:
    cocos2d::Node *_container = nullptr;
    cocos2d::DrawNode *_background = nullptr;
    cocos2d::Menu *_menu = nullptr;
    cocos2d::Label *_titleLabel = nullptr;
    cocos2d::EventListenerTouchOneByOne *_touchListener = nullptr;

    bool _isShowing = false;
    bool _actionTriggered = false; // 防止快速连点导致重复触发场景切换
    std::function<void()> _restartCallback;
    std::function<void()> _returnToMapCallback;
};
