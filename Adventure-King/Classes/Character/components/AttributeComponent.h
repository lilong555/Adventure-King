#pragma once

#include "Character/CharacterData.h"
#include <vector>

class AttributeComponent
{
public:
    AttributeComponent();

    //------------ 基础属性 ------------
    void setBaseAttributes(const Attributes &attributes);
    const Attributes &getBaseAttributes() const;

    void setBaseAttribute(AttributeType type, float value);
    float getBaseAttribute(AttributeType type) const;

    //------------ 装备加成 ------------
    void addEquipmentBonus(const Attributes &attributes);
    void removeEquipmentBonus(const Attributes &attributes);

    //------------ 被动技能加成 ------------
    void addPassiveSkillBonus(const Attributes &attributes);
    void removePassiveSkillBonus(const Attributes &attributes);

    //------------ 状态效果（中毒、亢奋等）------------
    void addStatusEffect(const StatusEffectInstance &effect);
    void updateStatusEffects(float dt); // 每帧更新持续时间

    //------------ 最终属性 ------------
    void recalculateFinalAttributes();
    float getAttributeValue(AttributeType type) const;
    const Attributes &getFinalAttributes() const { return _finalAttributes; }

private:
    Attributes _baseAttributes;    // 初始/等级成长
    Attributes _equipmentBonus;    // 装备
    Attributes _passiveSkillBonus; // 被动技能
    Attributes _statusBonus;       // 状态效果
    Attributes _finalAttributes;   // 最终属性 = 上面四者相加

    std::vector<StatusEffectInstance> _statusEffects;
};
