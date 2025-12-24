#include "Character/StatusEffects/Implementations/HunterBootsEffect.h"

#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"

#include <algorithm>

HunterBootsEffect* HunterBootsEffect::create(float durationSeconds, float moveSpeedBonus)
{
    auto p = new (std::nothrow) HunterBootsEffect(durationSeconds, moveSpeedBonus);
    if (p)
    {
        p->autorelease();
    }
    return p;
}

HunterBootsEffect::HunterBootsEffect(float durationSeconds, float moveSpeedBonus)
    : _durationSeconds(std::max(0.0f, durationSeconds))
    , _moveSpeedBonus(moveSpeedBonus)
{
    type = StatusEffectType::EQUIP_HUNTER_BOOTS;
    isPermanent = true;
}

void HunterBootsEffect::onAfterDealDamage(CharacterBase* owner,
                                         CharacterBase* target,
                                         float /*finalDamage*/,
                                         const DamageInfo& /*info*/,
                                         bool targetDied)
{
    if (!owner || !target || target == owner)
    {
        return;
    }
    if (!targetDied)
    {
        return;
    }

    auto attr = owner->getAttributeComponent();
    if (!attr)
    {
        return;
    }

    // 亢奋：纯属性 buff（同类型重复施加会刷新持续时间）
    auto excited = StatusEffect::create();
    excited->type = StatusEffectType::EXCITED;
    excited->duration = _durationSeconds;
    excited->elapsed = 0.0f;
    excited->isPermanent = (_durationSeconds <= 0.0f);

    Attributes bonus;
    bonus.set(AttributeType::MOVE_SPEED, _moveSpeedBonus);
    excited->setAttributeBonus(bonus);

    attr->addStatusEffect(excited);
}

