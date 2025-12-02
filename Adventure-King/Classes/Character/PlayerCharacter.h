#pragma once

#include "Character/CharacterBase.h"
#include <map>
#include <memory>

class PlayerCharacter : public CharacterBase
{
public:
    // 统一用 create(role, spriteFrameName)
    static PlayerCharacter *create(CharacterRole role,
                                   const std::string &spriteFrameName);

    bool init(CharacterRole role,
              const std::string &spriteFrameName);

    void addExperience(int amount);
    void levelUp();

    // 装备管理
    void equip(const std::shared_ptr<Equipment> &item);
    void unequip(EquipmentSlot slot);

    // 技能管理（外层接口，内部转发给 SkillComponent）
    void useSkill(size_t slotIndex);

    // 实现角色基础攻击
    virtual void attack() override;

    // 技能回调：SkillComponent 在技能成功释放时调用
    virtual void onUseActiveSkill(const ActiveSkill &skill) override;

private:
    PlayerCharacter() = default;

    void initAttributesByRole(CharacterRole role);
    void refreshHpMpFromAttributes();

    CharacterRole _role = CharacterRole::WARRIOR;
    int _skillPoints = 0;

    std::map<EquipmentSlot, std::shared_ptr<Equipment>> _equippedItems;
};
