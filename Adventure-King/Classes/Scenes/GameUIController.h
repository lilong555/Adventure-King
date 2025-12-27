/**
 * @file GameUIController.h
 * @brief GameScene 的 UI 编排：初始化 GameUI、暂停菜单、交互提示刷新
 */

#pragma once

#include "cocos2d.h"
#include <functional>
#include <string>

class PlayerCharacter;
class GameUI;
struct SaveSlotData;

class GameUIController
{
public:
    using RequestSaveCallback = std::function<bool(std::string &outMessage)>;

    /// @brief 初始化 UI 管理器与回调
    bool init(cocos2d::Scene *scene,
              PlayerCharacter *player,
              const std::string &levelName,
              const std::function<void()> &onReturnToMap,
              const std::function<void(bool paused)> &onPauseChanged,
              const RequestSaveCallback &onRequestSave,
              const std::function<bool()> &isPlayerAtGate,
              const std::function<void(const SaveSlotData &)> &onLoadSuccess);

    /// @brief 每帧刷新 UI（带节流）
    void update(float dt);

    /// @brief 切换暂停菜单显示
    void togglePauseMenu();
    /// @brief 切换背包显示（B 键 / HUD 背包按钮）
    void toggleInventory();
    /// @brief 当前是否暂停
    bool isPaused() const { return _paused; }

    /// @brief 设置 NPC 交互提示检测（例如：HomeScene 赐福入口）
    void setNpcHintQuery(const std::function<bool()> &isPlayerAtNpc) { _isPlayerAtNpc = isPlayerAtNpc; }

    /// @brief 显示角色死亡菜单（强制暂停）
    void showDeathMenu();
    /// @brief 死亡菜单是否显示中
    bool isDeathMenuShowing() const;

    /// @brief 获取 GameUI 对象
    GameUI *getGameUI() const { return _gameUI; }

    /// @brief 显示一条短暂提示（弹幕/Toast）
    void showToast(const std::string &text, const cocos2d::Color3B &color = cocos2d::Color3B::WHITE);

private:
    /// @brief 背包关闭后的统一收敛：回到暂停菜单或回到游戏
    void applyPostInventoryCloseState();

    cocos2d::Scene *_scene = nullptr;
    PlayerCharacter *_player = nullptr;
    GameUI *_gameUI = nullptr;

    bool _paused = false;
    // 当前背包关闭时是否回到暂停菜单（从暂停菜单进入背包时为 true）
    bool _inventoryReturnToPauseOnClose = false;

    enum class InteractionHintSource
    {
        NONE = 0,
        GATE = 1,
        BLESSING_NPC = 2,
    };
    InteractionHintSource _hintSource = InteractionHintSource::NONE;

    float _updateAccumulator = 0.0f;

    std::function<void()> _onReturnToMap;
    std::function<void(bool)> _onPauseChanged;
    RequestSaveCallback _onRequestSave;
    std::function<bool()> _isPlayerAtGate;
    std::function<bool()> _isPlayerAtNpc;
    std::function<void(const SaveSlotData &)> _onLoadSuccess;
};
