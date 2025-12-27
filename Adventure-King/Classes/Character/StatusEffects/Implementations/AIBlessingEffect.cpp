#include "Character/StatusEffects/Implementations/AIBlessingEffect.h"

AIBlessingEffect *AIBlessingEffect::create(const Attributes &bonus)
{
    auto effect = new (std::nothrow) AIBlessingEffect();
    if (effect && effect->initWithBonus(bonus))
    {
        effect->autorelease();
        return effect;
    }
    delete effect;
    return nullptr;
}

bool AIBlessingEffect::initWithBonus(const Attributes &bonus)
{
    type = StatusEffectType::AI_BLESSING;
    isPermanent = true;
    stackable = false;
    refreshOnAdd = true;
    maxStacks = 1;
    stacks = 1;
    attributeBonus = bonus;
    return true;
}

