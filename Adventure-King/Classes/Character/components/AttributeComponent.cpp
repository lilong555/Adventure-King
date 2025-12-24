#include "Character/components/AttributeComponent.h"
#include "Character/Base/CharacterBase.h"
#include <algorithm>
#include <cmath>

AttributeComponent::AttributeComponent() {
    setName("AttributeComponent");
}

AttributeComponent::~AttributeComponent() {}

bool AttributeComponent::init() {
    if (!cocos2d::Component::init()) return false;
    return true;
}

void AttributeComponent::onAdd() {
    // 由 CharacterBase 统一调度 update，避免重复 scheduleUpdate 触发引擎 warning。
}

// =================================================================
// 核心生命周期
// =================================================================
void AttributeComponent::update(float dt) {
    auto owner = static_cast<CharacterBase*>(getOwner());
    if (!owner || owner->isDead() || _effects.empty()) return;

    bool needsCleanup = false;

    // 1. 驱动所有效果的 Tick 逻辑
    for (auto& effect : _effects) {
        if (!effect->isPermanent) {
            effect->elapsed += dt;
        }

        // 触发状态的每帧逻辑（如中毒扣血）
        effect->onTick(owner, dt);

        if (effect->isExpired()) {
            needsCleanup = true;
        }
    }

    // 2. 清理过期效果
    if (needsCleanup) {
        auto it = _effects.begin();
        while (it != _effects.end()) {
            if ((*it)->isExpired()) {
                (*it)->onRemove(owner); // 触发移除时的逻辑（如停止特效）
                it = _effects.erase(it);
            }
            else {
                ++it;
            }
        }
        // 状态列表改变，必须重算最终属性
        recalculateFinalAttributes();
    }
}

// =================================================================
// 状态效果管理
// =================================================================

void AttributeComponent::addStatusEffect(StatusEffect* newEffect) {
    if (!newEffect) return;

    auto owner = static_cast<CharacterBase*>(getOwner());

    // 查找同类型效果处理叠层/刷新
    auto it = std::find_if(_effects.begin(), _effects.end(),
        [newEffect](StatusEffect* existing) {
            return existing->type == newEffect->type;
        });

    if (it != _effects.end()) {
        auto existing = *it;
        if (newEffect->stackable) {
            // 叠层逻辑
            int addStacks = std::max(1, newEffect->stacks);
            int newTotal = existing->stacks + addStacks;
            if (newEffect->maxStacks > 0) newTotal = std::min(newTotal, newEffect->maxStacks);

            existing->stacks = newTotal;
            if (newEffect->refreshOnAdd) {
                existing->duration = newEffect->duration;
                existing->elapsed = 0.0f;
            }
            existing->updateParametersFrom(newEffect);
        }
        else {
            // 覆盖逻辑
            existing->onRemove(owner);
            existing = std::move(newEffect);
            existing->onApply(owner);
        }
    }
    else {
        // 新增逻辑
        newEffect->onApply(owner);
        _effects.pushBack(newEffect);
    }

    recalculateFinalAttributes();
}

bool AttributeComponent::removeStatusEffect(StatusEffectType type) {
    auto owner = static_cast<CharacterBase*>(getOwner());
    bool anyRemoved = false;

    // 使用迭代器进行手动遍历
    auto it = _effects.begin();
    while (it != _effects.end())
    {
        // 这里的 *it 是 StatusEffect* 指针
        StatusEffect* e = *it;

        if (e && e->type == type)
        {
            // 1. 在移除前触发逻辑钩子（如停止该状态关联的粒子特效）
            e->onRemove(owner);

            // 2. 从 Vector 中移除
            // cocos2d::Vector::erase 会自动调用该对象的 release()
            // 并返回指向下一个元素的有效迭代器
            it = _effects.erase(it);

            anyRemoved = true;
        }
        else
        {
            ++it;
        }
    }

    // 3. 如果确实有状态被移除，重算属性
    if (anyRemoved) {
        recalculateFinalAttributes();
    }

    return anyRemoved;
}

bool AttributeComponent::hasStatusEffect(StatusEffectType type) const {
    return std::any_of(_effects.begin(), _effects.end(),
        [type](StatusEffect* e) {
            return e->type == type && !e->isExpired();
        });
}

// =================================================================
// 战斗钩子分发
// =================================================================

void AttributeComponent::executeReceiveDamageHooks(CharacterBase* attacker, DamageInfo& info) {
    auto owner = static_cast<CharacterBase*>(getOwner());
    for (auto& effect : _effects) {
        effect->onModifyReceiveDamage(owner, attacker, info);
    }
}

void AttributeComponent::executeDealDamageHooks(CharacterBase* target, DamageInfo& info) {
    auto owner = static_cast<CharacterBase*>(getOwner());
    for (auto& effect : _effects) {
        effect->onModifyDealDamage(owner, target, info);
    }
}

void AttributeComponent::executeAfterReceiveDamageHooks(CharacterBase* attacker, float finalDamage, const DamageInfo& info, bool wouldDieBeforeCallback)
{
    auto owner = static_cast<CharacterBase*>(getOwner());
    if (!owner)
    {
        return;
    }

    // 优先执行“濒死救援类”机制，避免装备顺序导致逻辑不一致（例如急救面罩应先于反伤/其他触发）
    for (auto& effect : _effects)
    {
        if (effect && effect->type == StatusEffectType::EQUIP_EMERGENCY_MASK)
        {
            effect->onAfterReceiveDamage(owner, attacker, finalDamage, info, wouldDieBeforeCallback);
        }
    }

    for (auto& effect : _effects)
    {
        if (!effect || effect->type == StatusEffectType::EQUIP_EMERGENCY_MASK)
        {
            continue;
        }
        effect->onAfterReceiveDamage(owner, attacker, finalDamage, info, wouldDieBeforeCallback);
    }
}

void AttributeComponent::executeAfterDealDamageHooks(CharacterBase* target, float finalDamage, const DamageInfo& info, bool targetDied)
{
    auto owner = static_cast<CharacterBase*>(getOwner());
    for (auto& effect : _effects)
    {
        effect->onAfterDealDamage(owner, target, finalDamage, info, targetDied);
    }
}

// =================================================================
// 属性计算逻辑 (核心)
// =================================================================

void AttributeComponent::recalculateFinalAttributes() {
    _finalAttributes.clear();
    _statusBonus.clear();

    // 1. 汇总当前所有 StatusEffect 提供的属性加成
    for (const auto& effect : _effects) {
        _statusBonus += effect->getAttributeBonus();
    }

    // 2. 叠加所有维度
    _finalAttributes += _baseAttributes;
    _finalAttributes += _equipmentBonus;
    _finalAttributes += _passiveSkillBonus;
    _finalAttributes += _statusBonus;
}

// =================================================================
// 基础属性与加成接口
// =================================================================

void AttributeComponent::setBaseAttributes(const Attributes& attributes) {
    _baseAttributes = attributes;
    recalculateFinalAttributes();
}

const Attributes& AttributeComponent::getBaseAttributes() const {
    return _baseAttributes;
}

void AttributeComponent::setBaseAttribute(AttributeType type, float value) {
    _baseAttributes.set(type, value);
    recalculateFinalAttributes();
}

float AttributeComponent::getBaseAttribute(AttributeType type) const {
    return _baseAttributes.get(type);
}

void AttributeComponent::addEquipmentBonus(const Attributes& attributes) {
    _equipmentBonus += attributes;
    recalculateFinalAttributes();
}

void AttributeComponent::removeEquipmentBonus(const Attributes& attributes) {
    for (const auto& kv : attributes.values) {
        _equipmentBonus.add(kv.first, -kv.second);
    }
    recalculateFinalAttributes();
}

void AttributeComponent::addPassiveSkillBonus(const Attributes& attributes) {
    _passiveSkillBonus += attributes;
    recalculateFinalAttributes();
}

void AttributeComponent::removePassiveSkillBonus(const Attributes& attributes) {
    for (const auto& kv : attributes.values) {
        _passiveSkillBonus.add(kv.first, -kv.second);
    }
    recalculateFinalAttributes();
}

// =================================================================
// 点数、冷却与查询
// =================================================================

void AttributeComponent::setAttributePoints(int points) {
    _attributePoints = std::max(0, points);
}

int AttributeComponent::getAttributePoints() const {
    return _attributePoints;
}

void AttributeComponent::setProcCooldown(AttributeType type, float duration) {
    _procCooldowns[type] = duration;
}

float AttributeComponent::getProcCooldown(AttributeType type) const {
    auto it = _procCooldowns.find(type);
    return (it != _procCooldowns.end()) ? it->second : 0.0f;
}

float AttributeComponent::getAttributeValue(AttributeType type) const {
    return _finalAttributes.get(type);
}

const Attributes& AttributeComponent::getFinalAttributes() const {
    return _finalAttributes;
}

// 在 .cpp 文件中
const cocos2d::Vector<StatusEffect*>& AttributeComponent::getStatusEffects() const {
    return _effects;
}
