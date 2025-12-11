#include "GoblinMonster.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"

USING_NS_CC;

GoblinMonster::GoblinMonster()
{
}

GoblinMonster::~GoblinMonster()
{
}

GoblinMonster* GoblinMonster::create(const std::string& spriteFrameName)
{
    GoblinMonster* ret = new (std::nothrow) GoblinMonster();
    if (ret && ret->init(spriteFrameName))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GoblinMonster::init(const std::string& spriteFrameName)
{
    // === 继承 MonsterBase 的初始化（加载纹理）===
    if (!MonsterBase::init(spriteFrameName))
        return false;

    // === 设置怪物属性 ===
    initAttributes();

    // === 刷新 HP / MP ===
    _currentHP = _maxHP;
    _currentMP = 0;

    // === 注册状态动画（需要动画名已加入 AnimationCache）===
    initStateAnimations();

    return true;
}

#pragma region 属性初始化
void GoblinMonster::initAttributes()
{
    auto attr = getAttributeComponent();
    if (!attr) return;

    Attributes base;

    // ★ 哥布林的基础属性（可自定义）★
    base.set(AttributeType::STRENGTH, 8.0f);
    base.set(AttributeType::DEFENSE, 2.0f);
    base.set(AttributeType::MOVE_SPEED, 160.0f);
    base.set(AttributeType::CRITICAL_RATE, 0.05f);
    base.set(AttributeType::MAX_HP, 60.0f);

    _maxHP = 60.0f;

    attr->setBaseAttributes(base);
    attr->recalculateFinalAttributes();

    // 同步移动速度到基类移动逻辑
    _moveSpeed = attr->getAttributeValue(AttributeType::MOVE_SPEED);
}
#pragma endregion

#pragma region 状态动画
void GoblinMonster::initStateAnimations()
{
    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "goblin_idle");
        sm->registerStateAnimation(CharacterState::WALKING, "goblin_walk");
        sm->registerStateAnimation(CharacterState::ATTACKING, "goblin_attack");
        sm->registerStateAnimation(CharacterState::HURT, "goblin_hurt");
        sm->registerStateAnimation(CharacterState::DEAD, "goblin_dead");
    }
}
#pragma endregion

#pragma region AI

void GoblinMonster::updateAI(float dt)
{
    if (!_target)
    {
        getStateMachineComponent()->changeState(CharacterState::IDLE);
        return;
    }

    float dist = distanceTo(_target);

    // ★ 哥布林的感知范围比史莱姆更远 ★
    if (dist < 220.0f)
    {
        if (dist <= _attackRange)
        {
            getStateMachineComponent()->changeState(CharacterState::ATTACKING);
        }
        else
        {
            getStateMachineComponent()->changeState(CharacterState::WALKING);
        }
    }
    else
    {
        getStateMachineComponent()->changeState(CharacterState::IDLE);
    }
}

#pragma endregion

#pragma region Attack

void GoblinMonster::attack()
{
    CCLOG("GoblinMonster performs attack!");

    auto skillComp = getSkillComponent();

    // ★ 如果哥布林具备技能，优先用技能攻击
    if (skillComp)
    {
        bool success = skillComp->useActiveSkill(0);
        if (success)
            return;
    }

    // === 默认普通攻击逻辑 ===
    DamageInfo dmg;
    dmg.amount = 8.0f;
    dmg.attacker = this;

    // 攻击玩家
    if (_target)
    {
        auto targetChar = dynamic_cast<CharacterBase*>(_target);
        if (targetChar)
        {
            targetChar->takeDamage(dmg);
        }
    }
}

#pragma endregion
