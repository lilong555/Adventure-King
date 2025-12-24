#pragma once

#include "Character/Base/StatusEffect.h"

// 血契短剑：吸血（按造成的最终伤害回复生命）
class BloodPactLifestealEffect : public StatusEffect
{
public:
    static BloodPactLifestealEffect* create(float lifestealRate);

    explicit BloodPactLifestealEffect(float lifestealRate);

    void onAfterDealDamage(CharacterBase* owner,
                           CharacterBase* target,
                           float finalDamage,
                           const DamageInfo& info,
                           bool targetDied) override;

private:
    float _lifestealRate = 0.0f;
};

