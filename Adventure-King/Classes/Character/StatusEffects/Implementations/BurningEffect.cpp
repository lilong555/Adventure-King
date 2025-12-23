#include "Character/StatusEffects/Implementations/BurningEffect.h"
#include "Character/Base/CharacterBase.h"
#include <cmath>

BurningEffect* BurningEffect::create(float dmgScale, float interval, float duration) {
    auto p = new BurningEffect(dmgScale, interval, duration);
    p->autorelease();
    return p;
}

BurningEffect::BurningEffect(float dmgScale, float interval, float duration) {
    this->type = StatusEffectType::BURNING;
    this->duration = duration;
    this->tickInterval = interval;
    this->baseDamageScale = dmgScale;
    this->stackable = true; // 燃烧通常允许叠层
}

void BurningEffect::doEffectAction(CharacterBase* owner) {
    // 使用你之前的公式：(基础倍率 + 每层额外倍率 * 层数) * 初始攻击力
    const float scale = baseDamageScale + perStackDamageScale * static_cast<float>(stacks);
    const float dmgAmount = std::floor(std::max(0.0f, scale * sourceAttackPower));

    if (dmgAmount > 0.0f) {
        DamageInfo dotDmg;
        dotDmg.amount = dmgAmount;
        dotDmg.attacker = nullptr;
        dotDmg.isCritical = false;
        dotDmg.causesHitStun = false; // 持续伤害不触发硬直

        owner->takeDamage(dotDmg);
        // CCLOG("Burning Tick: %f damage to %s", dmgAmount, owner->getName().c_str());
    }
}
