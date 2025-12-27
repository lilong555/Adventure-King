/**
 * @file GameInputController.cpp
 * @brief GameScene 输入/交互控制实现
 */

#include "Scenes/GameInputController.h"
#include "Character/Player/PlayerCharacter.h"
#include "Configs/GameConfig.h"
#include <cmath>

USING_NS_CC;

namespace
{
    constexpr float GROUND_VELOCITY_THRESHOLD = GameConfig::Player::GROUND_VELOCITY_THRESHOLD;
    constexpr float GROUND_NORMAL_THRESHOLD = GameConfig::Player::GROUND_NORMAL_THRESHOLD;
}

void GameInputController::bindPlayer(PlayerCharacter *player)
{
    _player = player;
}

void GameInputController::onKeyPressed(EventKeyboard::KeyCode keyCode)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        if (_togglePause)
        {
            _togglePause();
        }
        return;
    }

    // 背包快捷键：允许在暂停状态下按 B 关闭/打开（与 Esc 一样属于 UI 层行为）
    if (keyCode == EventKeyboard::KeyCode::KEY_B)
    {
        if (_toggleInventory)
        {
            _toggleInventory();
        }
        return;
    }

    if (!_player)
        return;
    if (_isPaused && _isPaused())
        return;

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _movingLeft = true;
        _player->setFlippedX(true);
        if (!_player->isActionLocked())
        {
            _player->setMoving(true, _runPressed);
        }
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _movingRight = true;
        _player->setFlippedX(false);
        if (!_player->isActionLocked())
        {
            _player->setMoving(true, _runPressed);
        }
        break;

    case EventKeyboard::KeyCode::KEY_SHIFT:
    case EventKeyboard::KeyCode::KEY_RIGHT_SHIFT:
        _runPressed = true;
        if ((_movingLeft || _movingRight) && !_player->isActionLocked())
        {
            _player->setMoving(true, true);
        }
        break;

    case EventKeyboard::KeyCode::KEY_W:
        // 优先 NPC 交互，其次门区交互，最后执行跳跃。
        if (_isAtNpc && _isAtNpc())
        {
            if (_interactNpc)
            {
                _interactNpc();
            }
        }
        else if (_isAtGate && _isAtGate())
        {
            if (_enterGate)
            {
                _enterGate();
            }
        }
        else
        {
            handleJump();
        }
        break;

    case EventKeyboard::KeyCode::KEY_SPACE:
        handleJump();
        break;

    case EventKeyboard::KeyCode::KEY_J:
    case EventKeyboard::KeyCode::KEY_4:
        _player->tryNormalAttack([this]()
                              { resumeMoveAnimationIfIdle(); });
        break;

    case EventKeyboard::KeyCode::KEY_E:
    case EventKeyboard::KeyCode::KEY_K:
        _player->tryUseSkill(0, [this]()
                                 { resumeMoveAnimationIfIdle(); });
        break;

    case EventKeyboard::KeyCode::KEY_Q:
        _player->tryUseSkill(1, [this]()
                                 { resumeMoveAnimationIfIdle(); });
        break;

    case EventKeyboard::KeyCode::KEY_R:
        _player->tryUseSkill(2, [this]()
                                 { resumeMoveAnimationIfIdle(); });
        break;

    case EventKeyboard::KeyCode::KEY_F:
        _player->tryUseSkill(3, [this]()
                                 { resumeMoveAnimationIfIdle(); });
        break;

    default:
        break;
    }
}

void GameInputController::onKeyReleased(EventKeyboard::KeyCode keyCode)
{
    if (!_player)
        return;

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_SHIFT:
    case EventKeyboard::KeyCode::KEY_RIGHT_SHIFT:
        _runPressed = false;
        if ((_movingLeft || _movingRight) && !_player->isActionLocked())
        {
            _player->setMoving(true, false);
        }
        break;

    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _movingLeft = false;
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _movingRight = false;
        break;

    default:
        break;
    }

    if (!_movingLeft && !_movingRight && !_player->isActionLocked())
    {
        _player->setMoving(false);
    }
}

void GameInputController::update(float dt)
{
    if (!_player || !_player->getPhysicsBody())
        return;
    if (_isPaused && _isPaused())
        return;

    auto physicsBody = _player->getPhysicsBody();
    Vec2 velocity = physicsBody->getVelocity();

    // 直接控制物理速度，保证移动与重力/碰撞一致。
    float currentSpeed = _runPressed ? GameConfig::Player::RUNSPEED : GameConfig::Player::WALKSPEED;
    float targetVelocityX = 0.0f;
    if (_movingLeft)
    {
        targetVelocityX = -currentSpeed;
    }
    else if (_movingRight)
    {
        targetVelocityX = currentSpeed;
    }

    velocity.x = targetVelocityX;
    physicsBody->setVelocity(velocity);

    // 动画同步：
    // - 受击状态结束后 StateMachine 会回到 IDLE，但玩家可能仍在按住移动键
    //   （此时不会再次触发 onKeyPressed），会出现“人在跑但播放 idle”的错位。
    // - 因此在非动作锁、非受击/死亡时，按当前输入状态持续对齐移动动画。
    if (!_player->isActionLocked())
    {
        if (auto sm = _player->getStateMachineComponent())
        {
            auto state = sm->getCurrentState();
            if (state != CharacterState::HURT && state != CharacterState::DEAD)
            {
                _player->setMoving(_movingLeft || _movingRight, _runPressed);
            }
        }
        else
        {
            _player->setMoving(_movingLeft || _movingRight, _runPressed);
        }
    }

    if (_groundContactCount > 0 && std::fabs(velocity.y) < GROUND_VELOCITY_THRESHOLD)
    {
        _grounded = true;
        _jumpCount = 0;
    }
    else if (_groundContactCount <= 0)
    {
        _grounded = false;
    }
}

void GameInputController::onGroundContactBegin(float normalY)
{
    // normalY 足够小才算“地面”，避免墙面/斜面触发落地。
    if (normalY >= GROUND_NORMAL_THRESHOLD)
    {
        return;
    }

    _groundContactCount++;
    _grounded = true;
    _jumpCount = 0;
    CCLOG("Player grounded (normal.y=%.2f), contacts: %d", normalY, _groundContactCount);
}

void GameInputController::onGroundContactEnd(float normalY)
{
    if (normalY >= GROUND_NORMAL_THRESHOLD)
    {
        return;
    }

    if (_groundContactCount <= 0)
    {
        _groundContactCount = 0;
        return;
    }

    _groundContactCount--;
    if (_groundContactCount <= 0)
    {
        _groundContactCount = 0;
        _grounded = false;
    }
}

void GameInputController::handleJump()
{
    if (!_player || !_player->getPhysicsBody())
        return;

    if (_grounded)
    {
        _jumpCount = 0;
    }

    if (_jumpCount >= GameConfig::Player::MAX_JUMP_COUNT)
        return;

    _groundContactCount = 0;

    auto physicsBody = _player->getPhysicsBody();
    Vec2 velocity = physicsBody->getVelocity();
    velocity.y = 0.0f;
    physicsBody->setVelocity(velocity);

    physicsBody->applyImpulse(Vec2(0, GameConfig::Player::JUMP_IMPULSE));
    _grounded = false;
    _jumpCount++;
}

void GameInputController::resumeMoveAnimationIfIdle()
{
    if (!_player)
        return;

    if (_player->isActionLocked())
        return;

    _player->setMoving(_movingLeft || _movingRight, _runPressed);
}

void GameInputController::resyncMoveAnimation()
{
    resumeMoveAnimationIfIdle();
}
