#include "Character/StatusEffects/Implementations/EmergencyMaskEffect.h"

#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"

#include <algorithm>

EmergencyMaskEffect* EmergencyMaskEffect::create(float triggerHpRatio, float healTargetHpRatio, float cooldownSeconds)
{
    auto p = new (std::nothrow) EmergencyMaskEffect(triggerHpRatio, healTargetHpRatio, cooldownSeconds);
    if (p)
    {
        p->autorelease();
    }
    return p;
}

EmergencyMaskEffect::EmergencyMaskEffect(float triggerHpRatio, float healTargetHpRatio, float cooldownSeconds)
    : _triggerHpRatio(std::max(0.0f, triggerHpRatio))
    , _healTargetHpRatio(std::max(0.0f, healTargetHpRatio))
    , _cooldownSeconds(std::max(0.0f, cooldownSeconds))
{
    type = StatusEffectType::EQUIP_EMERGENCY_MASK;
    isPermanent = true;
}

void EmergencyMaskEffect::onTick(CharacterBase* /*owner*/, float dt)
{
    if (_cooldownRemaining > 0.0f)
    {
        _cooldownRemaining = std::max(0.0f, _cooldownRemaining - dt);
    }
}

void EmergencyMaskEffect::onAfterReceiveDamage(CharacterBase* owner,
                                              CharacterBase* /*attacker*/,
                                              float /*finalDamage*/,
                                              const DamageInfo& /*info*/,
                                              bool /*wouldDieBeforeCallback*/)
{
    if (!owner)
    {
        return;
    }
    if (_cooldownRemaining > 0.0f)
    {
        return;
    }

    auto attr = owner->getAttributeComponent();
    if (!attr)
    {
        return;
    }

    const float maxHp = std::max(1.0f, attr->getAttributeValue(AttributeType::MAX_HP));
    const float triggerHp = maxHp * _triggerHpRatio;
    const float currentHp = owner->getCurrentHP();
    if (currentHp > triggerHp)
    {
        return;
    }

    const float targetHp = std::min(maxHp, maxHp * _healTargetHpRatio);
    if (targetHp <= currentHp)
    {
        return;
    }

    // 注意：这里允许从 0HP 抬血（发生在死亡判定前），因此不使用 CharacterBase::heal
    owner->setCurrentHP(targetHp);
    _cooldownRemaining = _cooldownSeconds;
}

