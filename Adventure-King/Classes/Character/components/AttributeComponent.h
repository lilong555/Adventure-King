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

    // 2. 构造函数与析构函数
    AttributeComponent();
    virtual ~AttributeComponent();

    // 3. 覆盖 Component 的生命周期方法
    virtual bool init() override;
    virtual void onAdd() override;
    virtual void update(float dt) override; // 这里会自动每帧被调用

    //------------ 基础属性 ------------
    void setBaseAttributes(const Attributes& attributes);
    const Attributes& getBaseAttributes() const;

    void setBaseAttribute(AttributeType type, float value);
    float getBaseAttribute(AttributeType type) const;

    //------------ 装备加成 ------------
    void addEquipmentBonus(const Attributes& attributes);
    void removeEquipmentBonus(const Attributes& attributes);

    //------------ 被动技能加成 ------------
    void addPassiveSkillBonus(const Attributes& attributes);
    void removePassiveSkillBonus(const Attributes& attributes);

    //------------ 状态效果（中毒、亢奋等）------------
    void addStatusEffect(const StatusEffectInstance& effect);
    void updateStatusEffects(float dt);
    // 注意：updateStatusEffects 现在可以设为 private，或者直接把逻辑移到 update 中
    // void updateStatusEffects(float dt); 

    bool hasStatusEffect(StatusEffectType type) const;
    const std::vector<StatusEffectInstance>& getStatusEffects() const { return _statusEffects; }

    //------------ 最终属性 ------------
    void recalculateFinalAttributes();
    float getAttributeValue(AttributeType type) const;
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
