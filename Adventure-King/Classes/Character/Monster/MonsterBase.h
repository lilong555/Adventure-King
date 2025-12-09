#pragma once
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include "Character/components/AttributeComponent.h"
#include "Character/Base/CharacterBase.h"

class MonsterBase : public CharacterBase
{
public:
    MonsterBase();
    virtual ~MonsterBase();

    virtual bool init(const std::string& spriteFrameName);

    virtual void update(float dt) override;

    // 必须实现的纯虚攻击接口
    virtual void attack() override;

    // 受击重写
    virtual void takeDamage(const DamageInfo& info) override;

    // 死亡重写
    virtual void die() override;

protected:
    // AI 行为
    virtual void updateAI(float dt);
    virtual void updateMovement(float dt);
    virtual void updateAttack(float dt);

    // 工具
    void faceTarget(Node* target);
    float distanceTo(Node* target);
    bool inAttackRange(Node* target);

protected:
    Node* _target = nullptr;     // 目标（通常是主角）
    float _attackTimer = 0.0f;   // 攻击间隔计时

    float _attackInterval = 1.0f;  // 攻速
    float _moveSpeed = 40.0f;      // 移动速度
    float _attackRange = 50.0f;    // 攻击距离
};
