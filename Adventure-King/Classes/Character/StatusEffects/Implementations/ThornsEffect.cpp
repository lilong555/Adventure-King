#include "Character/StatusEffects/Implementations/ThornsEffect.h"
#include "Character/Base/CharacterBase.h" // 关键：在这里包含完整定义
#include <algorithm>

ThornsEffect::ThornsEffect(float reflectRate, float cooldown)
    : _reflectRate(reflectRate), _procCooldown(cooldown) {
    this->type = StatusEffectType::THORNS;
    this->isPermanent = true;
}

void ThornsEffect::onModifyReceiveDamage(CharacterBase* owner, CharacterBase* attacker, DamageInfo& info) {
    if (_currentCooldown > 0.0f) return;

    // 此时 CharacterBase 是完整类型，可以进行比较和调用方法
    if (!attacker || attacker == owner || attacker->isDead()) return;

    float reflectAmount = std::max(1.0f, info.amount * _reflectRate);

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
