#include "Character/components/AttributeComponent.h"
#include "Character/Base/CharacterBase.h"
#include <algorithm>
#include <cmath>

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
    if (_statusEffects.empty())
        return;

    auto owner = dynamic_cast<CharacterBase*>(getOwner());

    for (auto& effect : _statusEffects)
    {
        float remaining = effect.duration - effect.elapsed;
        float activeDt = std::max(0.0f, std::min(dt, remaining));
        effect.elapsed += activeDt;

        // DOT：按 tickInterval 结算；伤害来源攻击力在施加时已写入 effect.sourceAttackPower
        if (owner && !owner->isDead() && effect.tickInterval > 0.0f && effect.sourceAttackPower > 0.0f)
        {
            effect.tickAccumulator += activeDt;

            int tickCount = static_cast<int>(std::floor(effect.tickAccumulator / effect.tickInterval));
            if (tickCount <= 0)
            {
                continue;
            }

            effect.tickAccumulator -= effect.tickInterval * static_cast<float>(tickCount);

            const int stacks = std::max(1, effect.stacks);
            const float scale = effect.baseDamageScale + effect.perStackDamageScale * static_cast<float>(stacks);
            const float dmgAmount = std::floor(std::max(0.0f, scale * effect.sourceAttackPower));
            if (dmgAmount <= 0.0f)
            {
                continue;
            }

            DamageInfo dmg;
            dmg.amount = dmgAmount;
            dmg.attacker = nullptr;
            dmg.isCritical = false;
            dmg.penetration = 0.0f;
            dmg.causesHitStun = false;

            for (int i = 0; i < tickCount; ++i)
            {
                owner->takeDamage(dmg);
            }
        }
    }

    // 清理过期效果
    auto it = std::remove_if(_statusEffects.begin(), _statusEffects.end(),
                             [](const StatusEffectInstance& eff) { return eff.isExpired(); });
    bool anyRemoved = (it != _statusEffects.end());
    if (anyRemoved)
    {
        _statusEffects.erase(it, _statusEffects.end());
    }

    // 只在状态变化时重算最终属性（主要用于带 attributeBonus 的效果）
    if (anyRemoved)
    {
        _statusBonus.clear();
        for (const auto& eff : _statusEffects)
        {
            _statusBonus += eff.attributeBonus;
        }
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
    auto merged = std::find_if(_statusEffects.begin(), _statusEffects.end(),
                               [&effect](const StatusEffectInstance& existing) {
                                   return existing.type == effect.type;
                               });

    if (merged != _statusEffects.end())
    {
        bool needRecalculate = false;

        if (effect.stackable)
        {
            const bool bonusChanged = (merged->attributeBonus.values != effect.attributeBonus.values);

            int addStacks = std::max(1, effect.stacks);
            int newStacks = merged->stacks + addStacks;

            // 合并策略：以“最新施加”的配置为准（maxStacks/DOT 参数/刷新行为等）
            int maxStacks = effect.maxStacks;
            if (maxStacks > 0)
            {
                newStacks = std::min(newStacks, maxStacks);
            }
            merged->stacks = std::max(1, newStacks);

            if (effect.refreshOnAdd)
            {
                merged->duration = effect.duration;
                merged->elapsed = 0.0f;
                merged->tickAccumulator = 0.0f;
            }

            // DOT/参数以最新施加为准（便于后续不同来源/配置覆盖）
            merged->tickInterval = effect.tickInterval;
            merged->sourceAttackPower = effect.sourceAttackPower;
            merged->baseDamageScale = effect.baseDamageScale;
            merged->perStackDamageScale = effect.perStackDamageScale;
            merged->attributeBonus = effect.attributeBonus;
            merged->stackable = effect.stackable;
            merged->maxStacks = maxStacks;
            merged->refreshOnAdd = effect.refreshOnAdd;

            needRecalculate = bonusChanged;
        }
        else
        {
            const bool bonusChanged = (merged->attributeBonus.values != effect.attributeBonus.values);
            *merged = effect;
            needRecalculate = bonusChanged;
        }

        if (needRecalculate)
        {
            _statusBonus.clear();
            for (const auto& eff : _statusEffects)
            {
                _statusBonus += eff.attributeBonus;
            }
            recalculateFinalAttributes();
        }
        return;
    }

    _statusEffects.push_back(effect);
    if (!effect.attributeBonus.values.empty())
    {
        _statusBonus += effect.attributeBonus;
        recalculateFinalAttributes();
    }
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

bool AttributeComponent::removeStatusEffect(StatusEffectType type)
{
    if (_statusEffects.empty())
    {
        return false;
    }

    const size_t before = _statusEffects.size();
    _statusEffects.erase(std::remove_if(_statusEffects.begin(), _statusEffects.end(),
                                        [type](const StatusEffectInstance &eff) {
                                            return eff.type == type;
                                        }),
                         _statusEffects.end());

    if (_statusEffects.size() == before)
    {
        return false;
    }

    // 状态变化：重算状态加成并刷新最终属性
    _statusBonus.clear();
    for (const auto &eff : _statusEffects)
    {
        _statusBonus += eff.attributeBonus;
    }
    recalculateFinalAttributes();
    return true;
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
