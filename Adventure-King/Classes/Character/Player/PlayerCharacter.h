#pragma once

#include "Character/Base/CharacterBase.h"
#include <functional>
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
    std::shared_ptr<Equipment> getEquipment(EquipmentSlot slot) const;
    std::shared_ptr<Weapon> getEquippedWeapon() const;

    // 技能管理（外层接口，内部转发给 SkillComponent）
    void useSkill(size_t slotIndex);

    // ================== 动作/状态驱动 ==================
    // 由场景输入层调用，用于切换跑动/待机动画状态
    void setMoving(bool moving);

    // 播放一次攻击动画，结束后回调（用于场景侧伤害/状态恢复）
    void attackAnimated(const std::function<void()> &onFinished = nullptr);

    // 播放一次技能施放动画，结束后回调（用于场景侧触发技能效果）
    void castSkillAnimated(const std::function<void()> &onFinished = nullptr);

    // 实现角色基础攻击
    virtual void attack() override;

    // 技能回调：SkillComponent 在技能成功释放时调用
    virtual void onUseActiveSkill(const ActiveSkill &skill) override;

    // 获取当前武器类型
    WeaponType getCurrentWeaponType() const;

    // 获取当前攻击动画前缀
    const std::string &getAttackAnimationPrefix() const { return _attackAnimationPrefix; }
    int getAttackFrameCount() const { return _attackFrameCount; }

    // 装备变更回调（供外部监听装备变化）
    using EquipmentChangeCallback = std::function<void(EquipmentSlot, const std::shared_ptr<Equipment> &)>;
    void setEquipmentChangeCallback(const EquipmentChangeCallback &callback) { _equipmentChangeCallback = callback; }

    //================== 存档系统支持 ==================

    // 获取角色职业
    CharacterRole getRole() const { return _role; }
    void setRole(CharacterRole role) { _role = role; }

    // 技能点管理
    int getSkillPoints() const { return _skillPoints; }
    void setSkillPoints(int points) { _skillPoints = points; }

    // 装备管理（用于存档）
    const std::map<EquipmentSlot, std::shared_ptr<Equipment>> &getEquippedItems() const { return _equippedItems; }
    void setEquippedItems(const std::map<EquipmentSlot, std::shared_ptr<Equipment>> &items) { _equippedItems = items; }

private:
    PlayerCharacter() = default;

    void initAttributesByRole(CharacterRole role);
    void refreshHpMpFromAttributes();
    void onWeaponChanged(const std::shared_ptr<Weapon> &weapon); // 武器变更时调用

    CharacterRole _role = CharacterRole::WARRIOR;
    int _skillPoints = 0;

    std::map<EquipmentSlot, std::shared_ptr<Equipment>> _equippedItems;

    // 当前攻击动画配置
    std::string _attackAnimationPrefix = "default";
    int _attackFrameCount = 3;

    // 装备变更回调
    EquipmentChangeCallback _equipmentChangeCallback = nullptr;
};
