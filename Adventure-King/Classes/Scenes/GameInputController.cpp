/**
 * @file GameInputController.cpp
 * @brief GameScene 输入/交互控制实现
 */

#include "Scenes/GameInputController.h"
#include "Character/Player/PlayerCharacter.h"
#include "Configs/GameConfigs.h"
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
        // 优先门区交互，其次执行跳跃。
        if (_isAtGate && _isAtGate())
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
        CCLOG("Player left ground, contacts: 0");
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
    CCLOG(_jumpCount == 1 ? "Player jumped" : "Player double jumped");
}

void GameInputController::resumeMoveAnimationIfIdle()
{
    if (!_player)
        return;

    if (_player->isActionLocked())
        return;

    _player->setMoving(_movingLeft || _movingRight, _runPressed);
}
