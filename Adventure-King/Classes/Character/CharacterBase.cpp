#include "Character/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

USING_NS_CC;

CharacterBase::CharacterBase() = default;
CharacterBase::~CharacterBase() = default;
// 子类在 create 中调用，用于初始化贴图和组件
bool CharacterBase::initWithSpriteFrameName(const std::string &spriteFrameName)
{
    // 先检查精灵帧是否存在，避免断言失败
    auto spriteFrame = SpriteFrameCache::getInstance()->getSpriteFrameByName(spriteFrameName);
    if (!spriteFrame)
    {
        CCLOG("CharacterBase::initWithSpriteFrameName - SpriteFrame '%s' not found", spriteFrameName.c_str());
        return false;
    }

    if (!Sprite::initWithSpriteFrame(spriteFrame))
    {
        return false;
    }

    _attributeComponent = std::make_unique<AttributeComponent>();
    _stateMachineComponent = std::make_unique<StateMachineComponent>(this);
    _skillComponent = std::make_unique<SkillComponent>(this);

    _level = 1;
    _experience = 0;
    _currentHP = 0.0f;
    _currentMP = 0.0f;

    scheduleUpdate();
    return true;
}

// 使用普通文件路径初始化
bool CharacterBase::initWithFile(const std::string &filename)
{
    if (!Sprite::initWithFile(filename))
    {
        return false;
    }

    _attributeComponent = std::make_unique<AttributeComponent>();
    _stateMachineComponent = std::make_unique<StateMachineComponent>(this);
    _skillComponent = std::make_unique<SkillComponent>(this);

    _level = 1;
    _experience = 0;
    _currentHP = 0.0f;
    _currentMP = 0.0f;

    scheduleUpdate();
    return true;
}
// 每帧更新
void CharacterBase::update(float dt)
{
    // 这里为什么要调用父类的update？是因为：CharacterBase继承自cocos2d::Sprite，而Sprite类本身"可能"有自己的更新逻辑需要执行。
    // 调用父类的update可以确保这些逻辑被正确执行，避免潜在的问题。
    // 是良好的编程实践，确保类的继承体系中的所有必要逻辑都能被执行。
    Sprite::update(dt); // 调用父类的 update

    if (_stateMachineComponent)
    {
        _stateMachineComponent->update(dt);
    }

    if (_skillComponent)
    {
        _skillComponent->update(dt);
    }

    if (_attributeComponent)
    {
        _attributeComponent->updateStatusEffects(dt);
    }
}
// 受击
void CharacterBase::takeDamage(const DamageInfo &info)
{
    if (isDead())
        return;

    float finalDamage = info.amount;

    // ---------------------------------------------------------
    // 1. 暴击计算 (Critical Hit)
    // ---------------------------------------------------------
    if (info.isCritical)
    {
        finalDamage *= info.critMultiplier;
    }

    // ---------------------------------------------------------
    // 2. 防御计算 (Defense Mitigation - MOBA Style)
    // ---------------------------------------------------------
    if (_attributeComponent)
    {
        float defense = _attributeComponent->getAttributeValue(AttributeType::DEFENSE);

        // 引入"护甲穿透"计算：防御力 = 原始防御 - 穿透值
        float effectiveDefense = std::max(0.0f, defense - info.penetration);

        // 使用乘法公式：防御越高，减伤越高，但永远不会达到 100%
        const float ARMOR_CONST = 100.0f;
        float reductionFactor = ARMOR_CONST / (ARMOR_CONST + effectiveDefense);

        finalDamage *= reductionFactor;
    }

    // ---------------------------------------------------------
    // 3. 随机浮动 (Random Variance)
    // 让数字看起来不那么死板，通常浮动 +/- 5%
    // ---------------------------------------------------------
    float variance = 0.95f + (rand() % 11) / 100.0f; // 0.95 ~ 1.05
    finalDamage *= variance;

    // ---------------------------------------------------------
    // 4. 最终结算
    // ---------------------------------------------------------
    // 确保至少造成 1 点伤害（或者是 0，看游戏规则）
    finalDamage = std::max(1.0f, std::floor(finalDamage));

    _currentHP -= finalDamage;

    // Log 输出调试
    // printf("Received Dmg: %.0f (Raw: %.0f, Def: %.0f)\n", finalDamage, info.amount, defense);

    if (_currentHP <= 0.0f)
    {
        _currentHP = 0.0f;
        die();
    }
    else
    {
        // 只有伤害超过一定阈值才播放受击动作，防止太多伤害导致鬼畜
        if (_stateMachineComponent && finalDamage > (_maxHP * 0.05f))
        {
            _stateMachineComponent->changeState(CharacterState::HURT);
        }
    }
}
void CharacterBase::die()
{
    if (_stateMachineComponent)
    {
        _stateMachineComponent->changeState(CharacterState::DEAD);
    }

    // 根据设置决定是否自动移除
    if (_autoRemoveOnDeath)
    {
        // 播放死亡动画后移除角色
        setCascadeOpacityEnabled(true);
        runAction(Sequence::create(
            FadeOut::create(0.5f),
            CallFunc::create([this]()
                             { this->removeFromParent(); }),
            nullptr));
    }
    else
    {
        // 不自动移除，只播放死亡视觉效果（变红闪烁）
        setCascadeOpacityEnabled(true);
        auto tintRed = TintTo::create(0.2f, 255, 100, 100);
        auto tintBack = TintTo::create(0.2f, 200, 200, 200);
        auto blink = Sequence::create(tintRed, tintBack, nullptr);
        runAction(RepeatForever::create(blink));
    }
}

void CharacterBase::setCurrentHP(float hp)
{
    float maxHp = hp;
    if (_attributeComponent)
    {
        maxHp = _attributeComponent->getAttributeValue(AttributeType::MAX_HP);
    }
    _currentHP = clampf(hp, 0.0f, maxHp);
}

void CharacterBase::setCurrentMP(float mp)
{
    float maxMp = mp;
    if (_attributeComponent)
    {
        maxMp = _attributeComponent->getAttributeValue(AttributeType::MAX_MP);
    }
    _currentMP = clampf(mp, 0.0f, maxMp);
}
