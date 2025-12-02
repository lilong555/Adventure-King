#include "Character/components/StateMachineComponent.h"
#include "Character/CharacterBase.h"
#include "cocos2d.h"

USING_NS_CC;

StateMachineComponent::StateMachineComponent(CharacterBase *owner)
    : _owner(owner)
{
}

void StateMachineComponent::changeState(CharacterState newState)
{
    if (_currentState == newState)
        return;

    _previousState = _currentState;
    _currentState = newState;
    _stateTime = 0.0f;

    playAnimationForState(newState);
}

void StateMachineComponent::update(float dt)
{
    _stateTime += dt;

    switch (_currentState)
    {
    case CharacterState::HURT:
        // 示例：0.3 秒后从受击回到待机
        if (_stateTime > 0.3f)
        {
            changeState(CharacterState::IDLE);
        }
        break;

    case CharacterState::DEAD:
        // 死亡一般不自动跳回其他状态
        break;

    default:
        break;
    }
}

void StateMachineComponent::registerStateAnimation(CharacterState state,
                                                   const std::string &animationName)
{
    _stateAnimationNames[state] = animationName;
}

void StateMachineComponent::playAnimationForState(CharacterState state)
{
    if (!_owner)
        return;

    auto it = _stateAnimationNames.find(state);
    if (it == _stateAnimationNames.end())
        return;

    auto animation = AnimationCache::getInstance()->getAnimation(it->second);
    if (!animation)
        return;

    auto animate = Animate::create(animation);

    _owner->stopAllActions();

    if (state == CharacterState::DEAD || state == CharacterState::ATTACKING)
    {
        _owner->runAction(animate); // 播放一次
    }
    else
    {
        _owner->runAction(RepeatForever::create(animate)); // 循环
    }
}
