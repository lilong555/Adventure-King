#include "MonsterBase.h"
#include "cocos2d.h"

USING_NS_CC;

MonsterBase::MonsterBase()
{
}

MonsterBase::~MonsterBase()
{
}

bool MonsterBase::init(const std::string& spriteFrameName)
{
    if (!initWithSpriteFrameName(spriteFrameName))
        return false;

    scheduleUpdate();
    return true;
}

void MonsterBase::update(float dt)
{
    if (isDead()) return;

    _attackTimer += dt;

    // 状态机更新
    if (getStateMachineComponent())
        getStateMachineComponent()->update(dt);

    updateAI(dt);
    updateMovement(dt);
    updateAttack(dt);

    cocos2d::Sprite::update(dt);
}

#pragma region AI

void MonsterBase::updateAI(float dt)
{
    if (!_target) return;

    float dist = distanceTo(_target);

    if (dist <= _attackRange)
    {
        getStateMachineComponent()->changeState(CharacterState::ATTACKING);
    }
    else
    {
        getStateMachineComponent()->changeState(CharacterState::WALKING);
    }
}

#pragma endregion


#pragma region 移动

void MonsterBase::updateMovement(float dt)
{
    if (!_target) return;

    float dist = distanceTo(_target);
    if (dist <= _attackRange) return;

    Vec2 dir = (_target->getPosition() - getPosition()).getNormalized();
    setPosition(getPosition() + dir * _moveSpeed * dt);

    faceTarget(_target);
}

#pragma endregion


#pragma region 攻击

void MonsterBase::updateAttack(float dt)
{
    if (!_target) return;

    float dist = distanceTo(_target);
    if (dist > _attackRange) return;

    if (_attackTimer >= _attackInterval)
    {
        _attackTimer = 0;
        attack();
    }
}

#pragma endregion


#pragma region 基础战斗事件

void MonsterBase::attack()
{
    // 默认普通攻击：使用 SkillComponent 简化为使用技能槽 0
    if (auto skill = getSkillComponent())
        skill->useActiveSkill(0);
}

void MonsterBase::takeDamage(const DamageInfo& info)
{
    float dmg = info.amount;

    float hp = getCurrentHP();
    hp -= dmg;

    setCurrentHP(hp);

    if (hp <= 0)
    {
        die();
        return;
    }

    getStateMachineComponent()->changeState(CharacterState::HURT);
}

void MonsterBase::die()
{
    getStateMachineComponent()->changeState(CharacterState::DEAD);

    if (getAutoRemoveOnDeath())
    {
        runAction(Sequence::create(
            DelayTime::create(0.5f),
            RemoveSelf::create(),
            nullptr
        ));
    }
}

#pragma endregion


#pragma region 工具函数

void MonsterBase::faceTarget(Node* target)
{
    if (!target) return;
    setScaleX(target->getPositionX() < getPositionX() ? -1 : 1);
}

float MonsterBase::distanceTo(Node* target)
{
    if (!target) return 99999.0f;
    return target->getPosition().distance(getPosition());
}

bool MonsterBase::inAttackRange(Node* target)
{
    return distanceTo(target) <= _attackRange;
}

#pragma endregion
