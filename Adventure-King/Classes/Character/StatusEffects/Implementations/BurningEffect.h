#pragma once
#include "Character/Base/StatusEffect.h"

class BurningEffect : public StatusEffect {
public:
    // 构造函数：dmgScale(伤害倍率), interval(间隔), duration(持续时间)
    static BurningEffect* create(float dmgScale, float interval, float duration);

    BurningEffect(float dmgScale, float interval, float duration);

    // 重写核心逻辑动作
    virtual void doEffectAction(CharacterBase* owner) override;
};
