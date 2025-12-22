#include "Character/Monster/Monsters/TrainingDummyMonster.h"

#include "Configs/GameConfigs.h"
#include <algorithm>

USING_NS_CC;

TrainingDummyMonster* TrainingDummyMonster::create(const std::string& spriteFrameName)
{
    auto ret = new(std::nothrow) TrainingDummyMonster();
    if (ret && ret->init(spriteFrameName))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool TrainingDummyMonster::init(const std::string& spriteFrameName)
{
    if (!MonsterBase::init(spriteFrameName))
    {
        return false;
    }

    // 木桩不应移动/攻击
    setAIConfig(0.0f, 0.0f, false);

    Attributes base;
    base.set(AttributeType::STRENGTH, 0.0f);
    base.set(AttributeType::DEFENSE, 0.0f);
    base.set(AttributeType::MOVE_SPEED, 0.0f);
    base.set(AttributeType::CRITICAL_RATE, 0.0f);
    base.set(AttributeType::MAX_HP, DUMMY_MAX_HP);
    base.set(AttributeType::MAX_MP, 0.0f);

    // 兼容 MonsterBase 缓存字段
    base.set(AttributeType::ATTACKINTERVAL, 999999.0f);
    base.set(AttributeType::ATTACK_RANGE, 0.0f);

    setupCharacterStats(base);
    setCurrentHP(DUMMY_MAX_HP);
    setCurrentMP(0.0f);
    setAutoRemoveOnDeath(false);

    // 固定站立：改为静态刚体，避免受到重力/碰撞影响
    if (auto body = getPhysicsBody())
    {
        body->setDynamic(false);
        body->setGravityEnable(false);
        body->setRotationEnable(false);
    }

    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::IDLE);
    }

    return true;
}

void TrainingDummyMonster::attack()
{
    // 木桩不攻击
}

void TrainingDummyMonster::updateAI(float /*dt*/)
{
    // 木桩不进行 AI
}

void TrainingDummyMonster::updateMovement(float /*dt*/)
{
    // 木桩不移动
}

void TrainingDummyMonster::updateAttack(float /*dt*/)
{
    // 木桩不攻击
}

