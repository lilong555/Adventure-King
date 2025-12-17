/**
 * @file GameInputController.h
 * @brief GameScene 输入/交互控制：键盘状态、移动、跳跃、攻击/技能触发
 */

#pragma once

#include "Scenes/GameSceneConfig.h"
#include "cocos2d.h"
#include <functional>

class PlayerCharacter;

class GameInputController
{
public:
    void bindPlayer(PlayerCharacter *player);
    void setPlayerConfig(const PlayerConfig &config) { _config = config; }

    void setPauseToggle(const std::function<void()> &togglePause) { _togglePause = togglePause; }
    void setIsPausedGetter(const std::function<bool()> &isPaused) { _isPaused = isPaused; }

    void setGateQuery(const std::function<bool()> &isAtGate) { _isAtGate = isAtGate; }
    void setGateEnter(const std::function<void()> &enterGate) { _enterGate = enterGate; }

    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode);

    void update(float dt);

    // 由物理碰撞回调转发：仅在“玩家脚下平台接触/分离”时调用
    void onGroundContactBegin(float normalY);
    void onGroundContactEnd(float normalY);

private:
    void handleJump();
    void resumeMoveAnimationIfIdle();

    PlayerCharacter *_player = nullptr;
    PlayerConfig _config;

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
