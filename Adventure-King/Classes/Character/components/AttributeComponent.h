#pragma once

#include "cocos2d.h"
#include "Character/Base/CharacterData.h"
#include "Character/Base/StatusEffect.h"
#include <vector>
#include <memory>
#include <map>

/**
 * @brief 属性组件：负责处理角色所有数值计算、装备加成及状态效果(Buff/Debuff)逻辑
 */
class AttributeComponent : public cocos2d::Component {
public:
    // Cocos2d-x 标准创建宏
    CREATE_FUNC(AttributeComponent);

    AttributeComponent();
    virtual ~AttributeComponent();

    // --- 生命周期 ---
    virtual bool init() override;
    virtual void onAdd() override;
    virtual void update(float dt) override;

    // --- 基础属性接口 (Base) ---
    void setBaseAttributes(const Attributes& attributes);
    const Attributes& getBaseAttributes() const;
    void setBaseAttribute(AttributeType type, float value);
    float getBaseAttribute(AttributeType type) const;

    // --- 装备加成接口 (Equipment) ---
    void addEquipmentBonus(const Attributes& attributes);
    void removeEquipmentBonus(const Attributes& attributes);

    // --- 被动技能加成接口 (Passive) ---
    void addPassiveSkillBonus(const Attributes& attributes);
    void removePassiveSkillBonus(const Attributes& attributes);

    // --- 状态效果管理 (Status Effects) ---
    void addStatusEffect(StatusEffect* newEffect);
    bool removeStatusEffect(StatusEffectType type);
    bool hasStatusEffect(StatusEffectType type) const;
    const cocos2d::Vector<StatusEffect*>& getStatusEffects() const;

    // --- 战斗钩子分发 (Combat Hooks) ---
    void executeReceiveDamageHooks(CharacterBase* attacker, DamageInfo& info);
    void executeDealDamageHooks(CharacterBase* target, DamageInfo& info);
    void executeAfterReceiveDamageHooks(CharacterBase* attacker, float finalDamage, const DamageInfo& info, bool wouldDieBeforeCallback);
    void executeAfterDealDamageHooks(CharacterBase* target, float finalDamage, const DamageInfo& info, bool targetDied);

    // --- 最终数值查询 ---
    
    // 核心计算逻辑：汇总所有维度并更新 _finalAttributes
    void recalculateFinalAttributes();
    float getAttributeValue(AttributeType type) const;
    const Attributes& getFinalAttributes() const;

    // --- 属性点与冷却管理 ---
    void setAttributePoints(int points);
    int getAttributePoints() const;
    void setProcCooldown(AttributeType type, float duration);
    float getProcCooldown(AttributeType type) const;
    
private:
    
    // 属性维度存储
    Attributes _baseAttributes;    // 初始/成长属性
    Attributes _equipmentBonus;   // 装备提供的属性
    Attributes _passiveSkillBonus; // 被动技能提供的属性
    Attributes _statusBonus;       // 状态效果(Buff)提供的属性
    Attributes _finalAttributes;   // 最终汇总结果

    // 状态效果逻辑对象池
    cocos2d::Vector<StatusEffect*> _effects;

    // 其他数据
    int _attributePoints = 0;
    std::map<AttributeType, float> _procCooldowns;
};
