#include "Character/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/SkillComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "cocos2d.h"

USING_NS_CC;

PlayerCharacter *PlayerCharacter::create(CharacterRole role,
                                         const std::string &spriteFrameName)
{
    PlayerCharacter *ret = new (std::nothrow) PlayerCharacter();
    if (ret && ret->init(role, spriteFrameName))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool PlayerCharacter::init(CharacterRole role,
                           const std::string &spriteFrameName)
{
    // 先尝试用精灵帧名初始化，如果失败则尝试用文件路径
    bool initSuccess = initWithSpriteFrameName(spriteFrameName);
    if (!initSuccess)
    {
        // 尝试作为普通文件路径初始化
        initSuccess = initWithFile(spriteFrameName);
    }

    if (!initSuccess)
    {
        return false;
    }

    _role = role;

    initAttributesByRole(role);

    if (auto attr = getAttributeComponent())
    {
        attr->recalculateFinalAttributes();
        refreshHpMpFromAttributes();
    }

    // 示例：绑定不同状态的动画名（动画要提前放进 AnimationCache）
    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "hero_idle");
        sm->registerStateAnimation(CharacterState::RUNNING, "hero_run");
        sm->registerStateAnimation(CharacterState::ATTACKING, "hero_attack");
        sm->registerStateAnimation(CharacterState::HURT, "hero_hurt");
        sm->registerStateAnimation(CharacterState::DEAD, "hero_dead");
    }

    return true;
}
// 根据角色职业初始化基础属性
void PlayerCharacter::initAttributesByRole(CharacterRole role)
{
    Attributes attrs;

    switch (role)
    {
    case CharacterRole::WARRIOR:
        attrs.set(AttributeType::STRENGTH, 10);
        attrs.set(AttributeType::DEFENSE, 5);
        attrs.set(AttributeType::CRITICAL_RATE, 0.10f);
        attrs.set(AttributeType::MOVE_SPEED, 200.0f);
        attrs.set(AttributeType::MAX_HP, 100.0f);
        attrs.set(AttributeType::MAX_MP, 30.0f);
        break;
    case CharacterRole::MAGE:
        attrs.set(AttributeType::STRENGTH, 4);
        attrs.set(AttributeType::DEFENSE, 2);
        attrs.set(AttributeType::CRITICAL_RATE, 0.15f);
        attrs.set(AttributeType::MOVE_SPEED, 180.0f);
        attrs.set(AttributeType::MAX_HP, 70.0f);
        attrs.set(AttributeType::MAX_MP, 80.0f);
        break;
    case CharacterRole::ASSASSIN:
        attrs.set(AttributeType::STRENGTH, 7);
        attrs.set(AttributeType::DEFENSE, 3);
        attrs.set(AttributeType::CRITICAL_RATE, 0.25f);
        attrs.set(AttributeType::MOVE_SPEED, 240.0f);
        attrs.set(AttributeType::MAX_HP, 80.0f);
        attrs.set(AttributeType::MAX_MP, 40.0f);
        break;
    case CharacterRole::TANK:
        attrs.set(AttributeType::STRENGTH, 6);
        attrs.set(AttributeType::DEFENSE, 8);
        attrs.set(AttributeType::CRITICAL_RATE, 0.05f);
        attrs.set(AttributeType::MOVE_SPEED, 160.0f);
        attrs.set(AttributeType::MAX_HP, 150.0f);
        attrs.set(AttributeType::MAX_MP, 20.0f);
        break;
    }

    if (auto attr = getAttributeComponent())
    {
        attr->setBaseAttributes(attrs);
    }
}
// 根据属性组件刷新当前 HP 和 MP
void PlayerCharacter::refreshHpMpFromAttributes()
{
    auto attr = getAttributeComponent();
    if (!attr)
        return;

    _currentHP = attr->getAttributeValue(AttributeType::MAX_HP);
    _currentMP = attr->getAttributeValue(AttributeType::MAX_MP);
}

void PlayerCharacter::addExperience(int amount)
{
    _experience += amount;

    // 简单经验表：每 100 * 当前等级 涨一级
    while (_experience >= _level * 100)
    {
        _experience -= _level * 100;
        levelUp();
    }
}

void PlayerCharacter::levelUp()
{
    _level++;
    _skillPoints++;

    if (auto attr = getAttributeComponent())
    {
        auto base = attr->getBaseAttributes();
        base.add(AttributeType::MAX_HP, 10.0f);
        base.add(AttributeType::MAX_MP, 5.0f);
        base.add(AttributeType::STRENGTH, 2.0f);
        base.add(AttributeType::DEFENSE, 1.0f);
        attr->setBaseAttributes(base);
    }

    refreshHpMpFromAttributes();
}

void PlayerCharacter::equip(const std::shared_ptr<Equipment> &item)
{
    if (!item)
        return;
    auto attr = getAttributeComponent();
    if (!attr)
        return;

    auto slot = item->slot;

    // 如果这个槽位原来有装备，先移除它的属性加成
    auto it = _equippedItems.find(slot);
    if (it != _equippedItems.end())
    {
        attr->removeEquipmentBonus(it->second->attributeBonus);
    }

    _equippedItems[slot] = item;
    attr->addEquipmentBonus(item->attributeBonus);

    // 如果是武器，更新攻击动画配置
    if (slot == EquipmentSlot::WEAPON)
    {
        auto weapon = std::dynamic_pointer_cast<Weapon>(item);
        onWeaponChanged(weapon);
    }

    // 确保 HP/MP 不超过新的上限
    setCurrentHP(_currentHP);
    setCurrentMP(_currentMP);

    // 触发装备变更回调
    if (_equipmentChangeCallback)
    {
        _equipmentChangeCallback(slot, item);
    }

    CCLOG("Equipped: %s (slot: %d)", item->name.c_str(), static_cast<int>(slot));
}

void PlayerCharacter::unequip(EquipmentSlot slot)
{
    auto attr = getAttributeComponent();
    if (!attr)
        return;

    auto it = _equippedItems.find(slot);
    if (it == _equippedItems.end())
        return;

    std::string itemName = it->second->name;
    attr->removeEquipmentBonus(it->second->attributeBonus);
    _equippedItems.erase(it);

    // 如果卸下武器，恢复默认攻击配置
    if (slot == EquipmentSlot::WEAPON)
    {
        onWeaponChanged(nullptr);
    }

    setCurrentHP(_currentHP);
    setCurrentMP(_currentMP);

    // 触发装备变更回调
    if (_equipmentChangeCallback)
    {
        _equipmentChangeCallback(slot, nullptr);
    }

    CCLOG("Unequipped: %s (slot: %d)", itemName.c_str(), static_cast<int>(slot));
}

std::shared_ptr<Equipment> PlayerCharacter::getEquipment(EquipmentSlot slot) const
{
    auto it = _equippedItems.find(slot);
    if (it != _equippedItems.end())
    {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Weapon> PlayerCharacter::getEquippedWeapon() const
{
    auto equipment = getEquipment(EquipmentSlot::WEAPON);
    if (equipment)
    {
        return std::dynamic_pointer_cast<Weapon>(equipment);
    }
    return nullptr;
}

WeaponType PlayerCharacter::getCurrentWeaponType() const
{
    auto weapon = getEquippedWeapon();
    if (weapon)
    {
        return weapon->type;
    }
    return WeaponType::SWORD; // 默认剑
}

void PlayerCharacter::onWeaponChanged(const std::shared_ptr<Weapon> &weapon)
{
    if (weapon)
    {
        _attackAnimationPrefix = weapon->attackAnimationPrefix.empty()
                                     ? "default"
                                     : weapon->attackAnimationPrefix;
        _attackFrameCount = weapon->attackFrameCount > 0 ? weapon->attackFrameCount : 3;

        CCLOG("Weapon changed: %s, animation: %s, frames: %d",
              weapon->name.c_str(),
              _attackAnimationPrefix.c_str(),
              _attackFrameCount);
    }
    else
    {
        // 恢复默认配置（无武器/拳头）
        _attackAnimationPrefix = "default";
        _attackFrameCount = 3;
        CCLOG("Weapon unequipped, using default attack");
    }
}

void PlayerCharacter::useSkill(size_t slotIndex)
{
    if (auto skillComp = getSkillComponent())
    {
        skillComp->useActiveSkill(slotIndex);
    }
}

void PlayerCharacter::attack()
{
    // 最基础普通攻击：切换至 ATTACKING 状态
    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::ATTACKING);
    }

    // TODO: 在这里加入普通攻击的伤害判定逻辑
}

void PlayerCharacter::onUseActiveSkill(const ActiveSkill &skill)
{
    // 默认行为：切换到 ATTACKING 状态
    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::ATTACKING);
    }

    // TODO: 根据 skill.id 进行不同的特效/逻辑
    // 例如 id == 1001 -> 释放火球；id == 1002 -> 冲刺斩 等
}
