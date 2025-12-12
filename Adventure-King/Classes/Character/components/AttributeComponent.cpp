#include "Character/components/AttributeComponent.h"

AttributeComponent::AttributeComponent() = default;

//---------------- 基础属性 ----------------

void AttributeComponent::setBaseAttributes(const Attributes &attributes)
{
    _baseAttributes = attributes;
    recalculateFinalAttributes();
	addEquipmentBonus(_equipmentBonus);
}

const Attributes &AttributeComponent::getBaseAttributes() const
{
    return _baseAttributes; 
}

void AttributeComponent::setBaseAttribute(AttributeType type, float value)
{
    _baseAttributes.set(type, value);
    recalculateFinalAttributes();
}

float AttributeComponent::getBaseAttribute(AttributeType type) const
{
    return _baseAttributes.get(type);
}

//---------------- 装备加成 ----------------

void AttributeComponent::addEquipmentBonus(const Attributes &attributes)
{
    _equipmentBonus += attributes;
    recalculateFinalAttributes();
}

void AttributeComponent::removeEquipmentBonus(const Attributes &attributes)
{
    for (const auto &kv : attributes.values)
    {
        _equipmentBonus.add(kv.first, -kv.second);
    }
    recalculateFinalAttributes();
}

//---------------- 被动技能加成 ----------------

void AttributeComponent::addPassiveSkillBonus(const Attributes &attributes)
{
    _passiveSkillBonus += attributes;
    recalculateFinalAttributes();
}

void AttributeComponent::removePassiveSkillBonus(const Attributes &attributes)
{
    for (const auto &kv : attributes.values)
    {
        _passiveSkillBonus.add(kv.first, -kv.second);
    }
    recalculateFinalAttributes();
}

//---------------- 状态效果 ----------------

void AttributeComponent::addStatusEffect(const StatusEffectInstance &effect)
{
    _statusEffects.push_back(effect);
    _statusBonus += effect.attributeBonus;
    recalculateFinalAttributes();
}

void AttributeComponent::updateStatusEffects(float dt)
{
    bool anyRemoved = false;

    for (auto &effect : _statusEffects)
    {
        effect.elapsed += dt;
    }

    auto it = std::remove_if(_statusEffects.begin(), _statusEffects.end(),
                             [](const StatusEffectInstance &eff)
                             { return eff.isExpired(); });
    if (it != _statusEffects.end())
    {
        _statusEffects.erase(it, _statusEffects.end());
        anyRemoved = true;
    }

    if (anyRemoved)
    {
        _statusBonus.clear();
        for (const auto &eff : _statusEffects)
        {
            _statusBonus += eff.attributeBonus;
        }
        recalculateFinalAttributes();
    }
}

bool AttributeComponent::hasStatusEffect(StatusEffectType type) const
{
    for (const auto &effect : _statusEffects)
    {
        if (effect.type == type && !effect.isExpired())
        {
            return true;
        }
    }
    return false;
}

//---------------- 最终属性 ----------------

void AttributeComponent::recalculateFinalAttributes()
{
    _finalAttributes.clear();
    _finalAttributes += _baseAttributes;
    _finalAttributes += _equipmentBonus;
    _finalAttributes += _passiveSkillBonus;
    _finalAttributes += _statusBonus;
}

float AttributeComponent::getAttributeValue(AttributeType type) const
{
    return _finalAttributes.get(type);
}
