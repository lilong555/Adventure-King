#pragma once

#include "Character/Base/StatusEffect.h"

// 追猎之靴：击杀触发“亢奋”移速加成
class HunterBootsEffect : public StatusEffect
{
public:
    static HunterBootsEffect* create(float durationSeconds, float moveSpeedBonus);

    HunterBootsEffect(float durationSeconds, float moveSpeedBonus);

    void onAfterDealDamage(CharacterBase* owner,
                           CharacterBase* target,
                           float finalDamage,
                           const DamageInfo& info,
                           bool targetDied) override;

private:
    float _durationSeconds = 0.0f;
    float _moveSpeedBonus = 0.0f;
};

