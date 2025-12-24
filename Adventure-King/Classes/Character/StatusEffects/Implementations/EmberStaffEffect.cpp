#include "Character/StatusEffects/Implementations/EmberStaffEffect.h"

#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include "Character/StatusEffects/Implementations/BurningEffect.h"
#include "Configs/GameConfigs.h"
#include "cocos2d.h"

#include <algorithm>

USING_NS_CC;

EmberStaffEffect* EmberStaffEffect::create(float procChance, float procCooldownSeconds)
{
    auto p = new (std::nothrow) EmberStaffEffect(procChance, procCooldownSeconds);
    if (p)
    {
        p->autorelease();
    }
    return p;
}

EmberStaffEffect::EmberStaffEffect(float procChance, float procCooldownSeconds)
    : _procChance(clampf(procChance, 0.0f, 1.0f))
    , _procCooldownSeconds(std::max(0.0f, procCooldownSeconds))
{
    type = StatusEffectType::EQUIP_EMBER_STAFF;
    isPermanent = true;
}

void EmberStaffEffect::onTick(CharacterBase* /*owner*/, float dt)
{
    if (_cooldownRemaining > 0.0f)
    {
        _cooldownRemaining = std::max(0.0f, _cooldownRemaining - dt);
    }
}

void EmberStaffEffect::onAfterDealDamage(CharacterBase* owner,
                                        CharacterBase* target,
                                        float finalDamage,
                                        const DamageInfo& /*info*/,
                                        bool /*targetDied*/)
{
    if (!owner || !target || target == owner)
    {
        return;
    }
    if (finalDamage <= 0.0f)
    {
        return;
    }
    if (_cooldownRemaining > 0.0f)
    {
        return;
    }
    if (_procChance <= 0.0f)
    {
        return;
    }

    if (RandomHelper::random_real(0.0f, 1.0f) >= _procChance)
    {
        return;
    }

    auto targetAttr = target->getAttributeComponent();
    if (!targetAttr)
    {
        return;
    }

    // 施加燃烧（允许叠层）
    auto burning = BurningEffect::create(GameConfig::StatusEffect::Burning::BASE_DAMAGE_SCALE,
                                         GameConfig::StatusEffect::Burning::TICK_INTERVAL_SECONDS,
                                         GameConfig::StatusEffect::Burning::DURATION_SECONDS);
    if (!burning)
    {
        return;
    }

    burning->stacks = 1;
    burning->maxStacks = 0;
    burning->stackable = true;
    burning->refreshOnAdd = true;
    burning->perStackDamageScale = GameConfig::StatusEffect::Burning::PER_STACK_DAMAGE_SCALE;
    burning->sourceAttackPower = owner->getAttackPower();

    targetAttr->addStatusEffect(burning);
    _cooldownRemaining = _procCooldownSeconds;
}

