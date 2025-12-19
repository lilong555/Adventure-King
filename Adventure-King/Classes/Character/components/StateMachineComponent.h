#pragma once

#include "cocos2d.h" // 1. 必需引入
#include "Character/Base/CharacterData.h"
#include <map>
#include <string>

class CharacterBase;

// 2. 继承自 cocos2d::Component
class StateMachineComponent : public cocos2d::Component
{
public:
    // 3. 添加标准创建宏 (生成 create() 函数)
    CREATE_FUNC(StateMachineComponent);

    // 初始化状态机组件
    StateMachineComponent();
    // 释放状态机组件
    virtual ~StateMachineComponent();

    // 4. 覆盖生命周期方法
    // 初始化组件数据
    virtual bool init() override;
    // 每帧推进状态机时间
    virtual void update(float dt) override;
    // 绑定 Owner 并缓存指针
    virtual void onAdd() override; // 当组件被挂载到节点时调用（用于获取 Owner）

    // ---------------- 状态机逻辑 ----------------

    // 切换状态（会自动播放对应动画）
    void changeState(CharacterState newState);

    // 获取当前状态
    CharacterState getCurrentState() const { return _currentState; }

    // 获取前一个状态
    CharacterState getPreviousState() const { return _previousState; }

    // 获取当前状态持续时间 (秒)
    float getStateTime() const { return _stateTime; }

    // 绑定状态到动画名（动画需提前加载到 AnimationCache）
    void registerStateAnimation(CharacterState state, const std::string& animationName);

private:
    // 5. 缓存 Owner 指针，类型为 CharacterBase*
    // Component 自带的 _owner 是 Node* 类型，我们需要强转后存在这里
    CharacterBase* _cachedOwner = nullptr;

    CharacterState _currentState = CharacterState::IDLE;
    CharacterState _previousState = CharacterState::IDLE;
    float _stateTime = 0.0f;

    std::map<CharacterState, std::string> _stateAnimationNames;

    // 内部辅助函数：播放指定状态动画
    void playAnimationForState(CharacterState state);
};
