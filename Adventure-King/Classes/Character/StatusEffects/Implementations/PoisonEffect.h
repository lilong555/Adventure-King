#pragma once
#ifndef __POISON_EFFECT_H__
#define __POISON_EFFECT_H__

#include "Character/Base/StatusEffect.h" // 必须包含基类头文件

// 前向声明：头文件里只用到指针，不需要知道 CharacterBase 的具体实现
class CharacterBase;

class PoisonEffect : public StatusEffect {
public:
    /**
     * @brief 创建中毒效果实例
     * @param dmgScale 基础伤害倍率
     * @param interval 触发间隔
     * @param duration 持续时间
     */
    static PoisonEffect* create(float dmgScale, float interval, float duration);

    PoisonEffect(float dmgScale, float interval, float duration);

    // 重写动作逻辑，实现具体的扣血
    virtual void doEffectAction(CharacterBase* owner) override;
};

#endif
