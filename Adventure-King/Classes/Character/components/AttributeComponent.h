#pragma once
#include"cocos2d.h"
#include "Character/Base/CharacterData.h"
#include <vector>

// 修改继承关系
class AttributeComponent : public cocos2d::Component
{
public:
    // 1. 添加 Cocos2d-x 标准创建宏
    CREATE_FUNC(AttributeComponent);

    // 初始化组件默认状态
    AttributeComponent();
    // 释放组件资源
    virtual ~AttributeComponent();

    // 初始化组件
    virtual bool init() override;
    // 组件挂载到节点时触发
    virtual void onAdd() override;
    // 每帧更新属性状态
    virtual void update(float dt) override; // 这里会自动每帧被调用

    //------------ 基础属性 ------------
    // 设置基础属性集合
    void setBaseAttributes(const Attributes& attributes);
    // 获取基础属性集合
    const Attributes& getBaseAttributes() const;

    // 设置单项基础属性
    void setBaseAttribute(AttributeType type, float value);
    // 获取单项基础属性
    float getBaseAttribute(AttributeType type) const;

    //------------ 装备加成 ------------
    // 叠加装备属性
    void addEquipmentBonus(const Attributes& attributes);
    // 移除装备属性
    void removeEquipmentBonus(const Attributes& attributes);

    //------------ 被动技能加成 ------------
    // 叠加被动技能属性
    void addPassiveSkillBonus(const Attributes& attributes);
    // 移除被动技能属性
    void removePassiveSkillBonus(const Attributes& attributes);

    //------------ 状态效果（中毒、亢奋等）------------
    // 添加状态效果
    void addStatusEffect(const StatusEffectInstance& effect);
    // 更新状态效果持续逻辑
    void updateStatusEffects(float dt);
    // 注意：updateStatusEffects 现在可以设为 private，或者直接把逻辑移到 update 中
    // void updateStatusEffects(float dt); 

    // 查询是否存在指定状态
    bool hasStatusEffect(StatusEffectType type) const;
    // 获取所有状态效果
    const std::vector<StatusEffectInstance>& getStatusEffects() const { return _statusEffects; }

    //------------ 最终属性 ------------
    // 重新计算最终属性
    void recalculateFinalAttributes();
    // 获取指定属性值
    float getAttributeValue(AttributeType type) const;
    // 获取最终属性集合
    const Attributes& getFinalAttributes() const { return _finalAttributes; }

private:
    // 辅助函数：内部更新状态效果逻辑
    void updateStatusEffectsLogic(float dt);

    Attributes _baseAttributes;    // 初始/等级成长
    Attributes _equipmentBonus;    // 装备
    Attributes _passiveSkillBonus; // 被动技能
    Attributes _statusBonus;       // 状态效果
    Attributes _finalAttributes;   // 最终属性

    std::vector<StatusEffectInstance> _statusEffects;
};
