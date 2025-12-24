#pragma once

#include "Character/Base/StatusEffect.h"

// 焰纹法杖：命中概率施加燃烧（带触发冷却）
class EmberStaffEffect : public StatusEffect
{
public:
    static EmberStaffEffect* create(float procChance, float procCooldownSeconds);

    EmberStaffEffect(float procChance, float procCooldownSeconds);

    void onTick(CharacterBase* owner, float dt) override;
    void onAfterDealDamage(CharacterBase* owner,
                           CharacterBase* target,
                           float finalDamage,
                           const DamageInfo& info,
                           bool targetDied) override;

private:
    float _procChance = 0.0f;
    float _procCooldownSeconds = 0.0f;
    float _cooldownRemaining = 0.0f;
};

