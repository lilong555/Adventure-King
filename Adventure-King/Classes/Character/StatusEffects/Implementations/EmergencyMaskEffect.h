#pragma once

#include "Character/Base/StatusEffect.h"

// 急救面罩：生命低于阈值时抬血到目标比例（带冷却）
class EmergencyMaskEffect : public StatusEffect
{
public:
    static EmergencyMaskEffect* create(float triggerHpRatio, float healTargetHpRatio, float cooldownSeconds);

    EmergencyMaskEffect(float triggerHpRatio, float healTargetHpRatio, float cooldownSeconds);

    void onTick(CharacterBase* owner, float dt) override;
    void onAfterReceiveDamage(CharacterBase* owner,
                              CharacterBase* attacker,
                              float finalDamage,
                              const DamageInfo& info,
                              bool wouldDieBeforeCallback) override;

private:
    float _triggerHpRatio = 0.0f;
    float _healTargetHpRatio = 0.0f;
    float _cooldownSeconds = 0.0f;
    float _cooldownRemaining = 0.0f;
};

