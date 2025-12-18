#include "Character/components/StateMachineComponent.h"
#include "Character/Base/CharacterBase.h"
#include "cocos2d.h"

USING_NS_CC;

// 定义一个常量 Tag，专门用于标记“状态机播放的动画动作”
// 这样 stopActionByTag 时就不会误伤移动或逻辑动作
static const int ACTION_TAG_STATE_ANIM = 199;

StateMachineComponent::StateMachineComponent()
{
    setName("StateMachineComponent"); // 设置组件名
}

StateMachineComponent::~StateMachineComponent()
{
}

bool StateMachineComponent::init()
{
    if (!Component::init()) return false;
    return true;
}

void StateMachineComponent::onAdd()
{
    // 当组件被 addComponent 到 Character 时，自动获取 Owner 并转换类型
    if (getOwner())
    {
        _cachedOwner = dynamic_cast<CharacterBase*>(getOwner());

        // 开启 update 调度 (让 update 函数每帧被调用)
        getOwner()->scheduleUpdate();
    }
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
        // 0.3 秒后从受击回到待机
        // 优化建议：最好检查一下当前是否在地面，避免空中受击后卡在 IDLE 动作下落
        if (_stateTime > 0.3f)
        {
            changeState(CharacterState::IDLE);
        }
        break;

    case CharacterState::DEAD:
        break;

    default:
        break;
    }
}

void StateMachineComponent::registerStateAnimation(CharacterState state,
    const std::string& animationName)
{
    _stateAnimationNames[state] = animationName;
}

void StateMachineComponent::playAnimationForState(CharacterState state)
{
    // 1. 使用 _cachedOwner (CharacterBase*) 而不是 _owner (Node*)
    if (!_cachedOwner)
        return;

    auto it = _stateAnimationNames.find(state);
    if (it == _stateAnimationNames.end())
        return;

    auto animation = AnimationCache::getInstance()->getAnimation(it->second);
    if (!animation)
    {
        // 只有调试模式下才打印警告，避免刷屏
#if COCOS2D_DEBUG > 0
        CCLOG("Warning: Animation not found for state %d: %s", (int)state, it->second.c_str());
#endif
        return;
    }

    auto animate = Animate::create(animation);

    // 2. 【关键优化】只停止之前的动画 Action，保留移动/闪烁等其他 Action
    _cachedOwner->stopActionByTag(ACTION_TAG_STATE_ANIM);

    Action* finalAction = nullptr;

    if (state == CharacterState::DEAD || state == CharacterState::ATTACKING || state == CharacterState::HURT)
    {
        finalAction = animate; // 播放一次
    }
    else
    {
        finalAction = RepeatForever::create(animate); // 循环播放
    }

    // 3. 设置 Tag，以便下次能精确停止它
    finalAction->setTag(ACTION_TAG_STATE_ANIM);
    _cachedOwner->runAction(finalAction);
}
