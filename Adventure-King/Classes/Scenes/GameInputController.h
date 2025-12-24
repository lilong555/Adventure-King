/**
 * @file GameInputController.h
 * @brief GameScene 输入/交互控制：键盘状态、移动、跳跃、攻击/技能触发
 */

#pragma once

#include "Configs/GameSceneConfig.h"
#include"Configs/GameConfig.h"
#include "cocos2d.h"
#include <functional>

class PlayerCharacter;

class GameInputController
{
public:
    /// @brief 绑定玩家对象
    void bindPlayer(PlayerCharacter *player);

    /// @brief 设置暂停切换回调
    void setPauseToggle(const std::function<void()> &togglePause) { _togglePause = togglePause; }
    /// @brief 设置暂停状态查询
    void setIsPausedGetter(const std::function<bool()> &isPaused) { _isPaused = isPaused; }

    /// @brief 设置门区检测回调
    void setGateQuery(const std::function<bool()> &isAtGate) { _isAtGate = isAtGate; }
    /// @brief 设置门区进入回调
    void setGateEnter(const std::function<void()> &enterGate) { _enterGate = enterGate; }

    /// @brief 按键按下事件
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode);
    /// @brief 按键释放事件
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode);

    /// @brief 每帧更新输入与移动
    void update(float dt);

    // 由物理碰撞回调转发：仅在“玩家脚下平台接触/分离”时调用
    void onGroundContactBegin(float normalY);
    void onGroundContactEnd(float normalY);

    /// @brief 强制同步移动动画（用于非键盘触发的攻击/技能回调）
    void resyncMoveAnimation();

private:
    /// @brief 处理跳跃逻辑
    void handleJump();
    /// @brief 若未移动则恢复待机/跑动动画
    void resumeMoveAnimationIfIdle();

    PlayerCharacter *_player = nullptr;

    bool _movingLeft = false;
    bool _movingRight = false;
    bool _runPressed = false;

    bool _grounded = false;
    int _groundContactCount = 0;
    int _jumpCount = 0;

    std::function<void()> _togglePause;
    std::function<bool()> _isPaused;
    std::function<bool()> _isAtGate;
    std::function<void()> _enterGate;
};
