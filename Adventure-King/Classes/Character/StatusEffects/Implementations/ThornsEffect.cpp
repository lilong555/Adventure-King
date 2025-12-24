#include "Character/StatusEffects/Implementations/ThornsEffect.h"
#include "Character/Base/CharacterBase.h" // 关键：在这里包含完整定义
#include <algorithm>

ThornsEffect::ThornsEffect(float reflectRate, float cooldown)
    : _reflectRate(reflectRate), _procCooldown(cooldown) {
    this->type = StatusEffectType::THORNS;
    this->isPermanent = true;
}

void ThornsEffect::onAfterReceiveDamage(CharacterBase* owner,
                                       CharacterBase* attacker,
                                       float finalDamage,
                                       const DamageInfo& /*info*/,
                                       bool /*wouldDieBeforeCallback*/) {
    // 如果拥有者已死亡或无效，则不再触发反伤
    if (!owner || owner->isDead())
    {
        return;
    }
    if (_currentCooldown > 0.0f) return;

    // 此时 CharacterBase 是完整类型，可以进行比较和调用方法
    if (!attacker || attacker == owner || attacker->isDead()) return;

    // 反伤按“实际最终伤害”计算，更贴近玩家直觉
    float reflectAmount = std::max(1.0f, finalDamage * _reflectRate);

    DamageInfo thornDmg;
    thornDmg.amount = reflectAmount;
    thornDmg.attacker = nullptr;
    thornDmg.isCritical = false;
    thornDmg.causesHitStun = false;

    attacker->takeDamage(thornDmg);
    _currentCooldown = _procCooldown;
}

void ThornsEffect::onTick(CharacterBase* owner, float dt) {
    if (_currentCooldown > 0.0f) {
        _currentCooldown -= dt;
    }
}
