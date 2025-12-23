#include "Character/StatusEffects/Implementations/PoisonEffect.h"
#include "Character/Base/CharacterBase.h" // 实现文件需要调用 owner 的方法，必须包含
#include <algorithm>
#include <cmath>

PoisonEffect* PoisonEffect::create(float dmgScale, float interval, float duration) {
    auto p = new (std::nothrow) PoisonEffect(dmgScale, interval, duration);
    if (p) {
        p->autorelease();
        return p;
    }
    return nullptr;
}

PoisonEffect::PoisonEffect(float dmgScale, float interval, float duration) {
    this->type = StatusEffectType::POISONED;
    this->duration = duration;
    this->tickInterval = interval;
    this->baseDamageScale = dmgScale;

    // 默认配置（也可以从 Config 读）
    this->stackable = true;
    this->refreshOnAdd = true;
}

void PoisonEffect::doEffectAction(CharacterBase* owner) {
    if (!owner || owner->isDead()) return;

    // 移植你之前在基类中的逻辑
    const float scale = baseDamageScale + perStackDamageScale * static_cast<float>(stacks);
    const float dmgAmount = std::floor(std::max(0.0f, scale * sourceAttackPower));

    if (dmgAmount > 0.0f) {
        DamageInfo dotDmg;
        dotDmg.amount = dmgAmount;
        dotDmg.attacker = nullptr;
        dotDmg.isCritical = false;
        dotDmg.causesHitStun = false; // 中毒通常不引起受击硬直

        // 触发实际伤害，此时 MonsterBase 会自动弹出飘字
        owner->takeDamage(dotDmg);
    }
}
