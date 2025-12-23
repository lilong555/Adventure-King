#pragma once
#include "Character/Base/StatusEffect.h"

// 前向声明，不需要包含 CharacterBase.h
class CharacterBase;

class ThornsEffect : public StatusEffect {
public:
    ThornsEffect(float reflectRate, float cooldown);
    virtual void onModifyReceiveDamage(CharacterBase* owner, CharacterBase* attacker, DamageInfo& info) override;
    virtual void onTick(CharacterBase* owner, float dt) override;

private:
    float _reflectRate;
    float _procCooldown;
    float _currentCooldown = 0.0f;
};
