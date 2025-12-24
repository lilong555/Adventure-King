#include "Character/StatusEffects/Implementations/BloodPactLifestealEffect.h"

#include "Character/Base/CharacterBase.h"
#include "Configs/GameConfigs.h"

#include <algorithm>

BloodPactLifestealEffect* BloodPactLifestealEffect::create(float lifestealRate)
{
    auto p = new (std::nothrow) BloodPactLifestealEffect(lifestealRate);
    if (p)
    {
        p->autorelease();
    }
    return p;
}

BloodPactLifestealEffect::BloodPactLifestealEffect(float lifestealRate)
    : _lifestealRate(std::max(0.0f, lifestealRate))
{
    type = StatusEffectType::EQUIP_BLOOD_PACT_SWORD;
    isPermanent = true;
}

void BloodPactLifestealEffect::onAfterDealDamage(CharacterBase* owner,
                                                 CharacterBase* target,
                                                 float finalDamage,
                                                 const DamageInfo& /*info*/,
                                                 bool /*targetDied*/)
{
    if (!owner || !target || target == owner)
    {
        return;
    }
    if (finalDamage <= 0.0f)
    {
        return;
    }

    // 安全上限：避免未来叠加来源过多导致数值失控
    const float rate = std::min(_lifestealRate, GameConfig::Skill::PassiveEffect::LIFESTEAL_TOTAL_MAX);
    if (rate <= 0.0f)
    {
        return;
    }

    // 注意：这里不能使用 CharacterBase::heal（如果 HP=0 会被判定为 dead），
    // 但“吸血”只会发生在造成伤害后，正常情况下 owner 不会是 0HP。
    owner->setCurrentHP(owner->getCurrentHP() + finalDamage * rate);
}

