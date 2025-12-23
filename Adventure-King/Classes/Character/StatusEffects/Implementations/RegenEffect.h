#pragma once
#include "Character/Base/StatusEffect.h"

class RegenEffect : public StatusEffect {
public:
    // healAmount 为每跳恢复的固定值
    static RegenEffect* create(float healAmount, float interval, float duration);

    RegenEffect(float healAmount, float interval, float duration);

    virtual void doEffectAction(CharacterBase* owner) override;
};
