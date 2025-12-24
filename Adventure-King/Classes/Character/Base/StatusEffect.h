#pragma once

#include "Character/Base/CharacterData.h"
#include "cocos2d.h"

class CharacterBase;
struct DamageInfo;

// 继承自 Ref，使用 Cocos2d-x 原生引用计数
class StatusEffect : public cocos2d::Ref
{
public:
    StatusEffect() = default;
    virtual ~StatusEffect() = default;

    // 静态创建方法，方便使用
    static StatusEffect* create()
    {
        auto effect = new StatusEffect();
        effect->autorelease();
        return effect;
    }

    virtual void onApply(CharacterBase* owner) {}
    virtual void onTick(CharacterBase* owner, float dt);
    virtual void doEffectAction(CharacterBase* owner) {}
    virtual void onRemove(CharacterBase* owner) {}
    virtual void onModifyDealDamage(CharacterBase* owner, CharacterBase* target, DamageInfo& info) {}
    virtual void onModifyReceiveDamage(CharacterBase* owner, CharacterBase* attacker, DamageInfo& info) {}

    // 伤害结算后的回调（用于装备特效等“触发型机制”）
    // - onAfterReceiveDamage：在受击者扣血后、死亡判定前触发（可用于濒死救援/反伤等）
    // - onAfterDealDamage：在目标扣血与死亡判定完成后触发（可用于吸血/击杀触发等）
    virtual void onAfterReceiveDamage(CharacterBase* owner,
                                      CharacterBase* attacker,
                                      float finalDamage,
                                      const DamageInfo& info,
                                      bool wouldDieBeforeCallback) {}
    virtual void onAfterDealDamage(CharacterBase* owner,
                                   CharacterBase* target,
                                   float finalDamage,
                                   const DamageInfo& info,
                                   bool targetDied) {}

    virtual bool isExpired() const { return !isPermanent && elapsed >= duration; }
    virtual const Attributes& getAttributeBonus() const { return attributeBonus; }
    virtual void updateParametersFrom(const StatusEffect* other);

    void setAttributeBonus(const Attributes& bonus) { attributeBonus = bonus; }
    Attributes attributeBonus;

    StatusEffectType type;
    float duration = 0.0f;
    float elapsed = 0.0f;
    bool isPermanent = false;
    int stacks = 1;
    int maxStacks = 1;
    bool stackable = false;
    bool refreshOnAdd = true;
    float tickInterval = 0.0f;
    float tickAccumulator = 0.0f;
    float sourceAttackPower = 0.0f;
    float baseDamageScale = 0.0f;
    float perStackDamageScale = 0.0f;
};
