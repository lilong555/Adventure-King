//在基类中提供通用的 DOT（持续伤害） 和 参数同步 默认实现，可以极大减少子类（如中毒、燃烧）的重复代码
#include "Character/Base/StatusEffect.h"
#include "Character/Base/CharacterBase.h"
#include <algorithm>
#include <cmath>

void StatusEffect::onTick(CharacterBase* owner, float dt) {
    if (!owner || owner->isDead() || tickInterval <= 0.0f) return;

    // 1. 累计时间
    tickAccumulator += dt;

    // 2. 计算触发次数（处理可能出现的掉帧或高频 Tick）
    int tickCount = static_cast<int>(std::floor(tickAccumulator / tickInterval));
    if (tickCount <= 0) return;

    tickAccumulator -= tickInterval * static_cast<float>(tickCount);

    // 3. 计算 DOT 伤害逻辑 [参考自 AttributeComponent 原实现]
    const float scale = baseDamageScale + perStackDamageScale * static_cast<float>(stacks);
    const float dmgAmount = std::floor(std::max(0.0f, scale * sourceAttackPower));

    if (dmgAmount > 0.0f) {
        DamageInfo dotDmg;
        dotDmg.amount = dmgAmount;
        dotDmg.attacker = nullptr; // DOT 通常不触发反伤
        dotDmg.isCritical = false;
        dotDmg.causesHitStun = false;

        for (int i = 0; i < tickCount; ++i) {
            owner->takeDamage(dotDmg);
        }
    }
}

void StatusEffect::updateParametersFrom(const StatusEffect* other) {
    if (!other) return;

    // 1. 刷新时间 (如果允许)
    if (other->refreshOnAdd) {
        this->duration = other->duration;
        this->elapsed = 0.0f;
        this->tickAccumulator = 0.0f;
    }

    // 2. 更新逻辑参数：以最新施加的强力 Buff 为准
    this->tickInterval = other->tickInterval;
    this->sourceAttackPower = other->sourceAttackPower;
    this->baseDamageScale = other->baseDamageScale;
    this->perStackDamageScale = other->perStackDamageScale;
    this->attributeBonus = other->attributeBonus;
}
