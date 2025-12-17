#include "Character/components/AttributeComponent.h"
#include <algorithm>

AttributeComponent::AttributeComponent()
{
    // 设置组件名字，方便通过 getComponent("AttributeComponent") 获取
    setName("AttributeComponent");
}

AttributeComponent::~AttributeComponent()
{
}

// 初始化函数
bool AttributeComponent::init()
{
    if (!Component::init())
    {
        return false;
    }

    // 或者更简单的 Cocos2d-x 写法，Component 默认支持 update，
    // 只要 owner 启用了 scheduleUpdate，组件的 update 就会被调用。
    // 通常组件不需要手动 schedule，只要实现 update 方法并在 init 里 setName 即可。

    return true;
}
void AttributeComponent::onAdd()
{
    // 当组件被挂载到节点上时，Cocos会自动调用这个函数
    if (getOwner())
    {
        // 开启宿主节点的 update调度，这样组件的 update(dt) 也会被自动调用
        getOwner()->scheduleUpdate();
    }
}
// 每帧更新
void AttributeComponent::update(float dt)
{
    // 调用原本的状态更新逻辑
    updateStatusEffectsLogic(dt);
}

void AttributeComponent::updateStatusEffectsLogic(float dt)
{
    // 把你原来的 updateStatusEffects 代码搬到这里
    if (_statusEffects.empty()) return;

    bool needsRecalc = false;
    for (auto it = _statusEffects.begin(); it != _statusEffects.end();)
    {
        it->duration -= dt;
        if (it->duration <= 0)
        {
            it = _statusEffects.erase(it); // 移除过期的
            needsRecalc = true;
        }
        else
        {
            ++it;
        }
    }

    if (needsRecalc)
    {
        recalculateFinalAttributes();
    }
}

//---------------- 基础属性 ----------------

void AttributeComponent::setBaseAttributes(const Attributes &attributes)
{
    _baseAttributes = attributes;
    recalculateFinalAttributes();
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
