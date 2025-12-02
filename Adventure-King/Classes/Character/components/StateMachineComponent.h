#pragma once

#include "Character/CharacterData.h"
#include <map>
#include <string>

class CharacterBase;

class StateMachineComponent
{
public:
    explicit StateMachineComponent(CharacterBase *owner);

    void changeState(CharacterState newState);
    CharacterState getCurrentState() const { return _currentState; }
    CharacterState getPreviousState() const { return _previousState; }

    void update(float dt);

    // 绑定状态到动画名（名字在 AnimationCache 里）
    void registerStateAnimation(CharacterState state, const std::string &animationName);

private:
    CharacterBase *_owner = nullptr;
    CharacterState _currentState = CharacterState::IDLE;
    CharacterState _previousState = CharacterState::IDLE;
    float _stateTime = 0.0f;

    std::map<CharacterState, std::string> _stateAnimationNames;

    void playAnimationForState(CharacterState state);
};
