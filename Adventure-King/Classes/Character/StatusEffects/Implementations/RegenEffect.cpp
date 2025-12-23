#include "Character/StatusEffects/Implementations/RegenEffect.h"
#include "Character/Base/CharacterBase.h"

RegenEffect* RegenEffect::create(float healAmount, float interval, float duration) {
    auto p = new RegenEffect(healAmount, interval, duration);
    p->autorelease();
    return p;
}

RegenEffect::RegenEffect(float healAmount, float interval, float duration) {
    this->type = StatusEffectType::REGEN; // 需在 StatusEffectType 枚举中定义
    this->duration = duration;
    this->tickInterval = interval;
    this->baseDamageScale = healAmount; // 借用此字段存储恢复量
}

void RegenEffect::doEffectAction(CharacterBase* owner) {
    if (!owner) return;

    // 假设 baseDamageScale 存储的是每秒恢复量
    float healValue = baseDamageScale;

    // 简单的一行代码即可完成数值、UI 和逻辑结算
    owner->heal(healValue);
}
