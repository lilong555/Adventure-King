#include "Character/components/InventoryComponent.h"

#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include "Character/StatusEffects/StatusEffectFactory.h"
#include "Configs/GameConfig.h"

#include <algorithm>

USING_NS_CC;

namespace
{
void removeEquipmentStatusEffectByItemId(AttributeComponent* attr, int itemId)
{
    if (!attr)
    {
        return;
    }

    StatusEffectType type;
    if (!StatusEffectFactory::tryGetEffectTypeByItemId(itemId, type))
    {
        return;
    }

    // 注意：当前 AttributeComponent 仅支持按 type 移除；
    // 因此装备特效建议使用“只由装备系统控制”的专用类型，避免误删其他来源的同类效果。
    attr->removeStatusEffect(type);
}
}

InventoryComponent::InventoryComponent()
{
    setName("InventoryComponent");
}

InventoryComponent::~InventoryComponent() = default;

bool InventoryComponent::init()
{
    if (!Component::init())
    {
        return false;
    }
    return true;
}

void InventoryComponent::onAdd()
{
    if (getOwner())
    {
        _cachedOwner = dynamic_cast<CharacterBase*>(getOwner());
        if (!_cachedOwner)
        {
            CCLOG("错误：InventoryComponent 只能挂载到 CharacterBase 派生对象上");
        }
    }
}

void InventoryComponent::onRemove()
{
    _cachedOwner = nullptr;
    Component::onRemove();
}

void InventoryComponent::addToInventory(const std::shared_ptr<Equipment>& item)
{
    if (!item)
    {
        return;
    }

    const int itemId = item->id;
    if (_inventoryItemIds.find(itemId) != _inventoryItemIds.end())
    {
        return;
    }

    _inventoryItems.push_back(item);
    _inventoryItemIds.insert(itemId);
}

void InventoryComponent::clearInventory()
{
    _inventoryItems.clear();
    _inventoryItemIds.clear();
}

void InventoryComponent::setInventoryItems(const std::vector<std::shared_ptr<Equipment>>& items)
{
    _inventoryItems.clear();
    _inventoryItemIds.clear();
    for (const auto& item : items)
    {
        addToInventory(item);
    }
}

bool InventoryComponent::equip(const std::shared_ptr<Equipment>& item)
{
    if (!item)
    {
        return false;
    }

    auto owner = getCharacterOwner();
    if (!owner)
    {
        return false;
    }

    auto attr = owner->getAttributeComponent();
    if (!attr)
    {
        return false;
    }

    const EquipmentSlot slot = item->slot;

    // 1) 卸载旧装备
    auto it = _equippedItems.find(slot);
    if (it != _equippedItems.end() && it->second)
    {
        auto oldItem = it->second;
        attr->removeEquipmentBonus(oldItem->attributeBonus);

        // 移除旧装备自带的状态效果（如荆棘）
        removeEquipmentStatusEffectByItemId(attr, oldItem->id);

        // 将旧装备返回到背包，避免“仅在穿戴槽内存在”的物品丢失（addToInventory 内部会按 id 去重）
        addToInventory(oldItem);
    }

    // 2) 挂载新装备并加成属性
    _equippedItems[slot] = item;
    attr->addEquipmentBonus(item->attributeBonus);

    // 3) 装备特效：交给工厂创建（例如荆棘）
    // 说明：对于不带特效的装备，createEffectByItemId 返回 nullptr 是“正常情况”
    if (auto effect = StatusEffectFactory::createEffectByItemId(item->id, std::max(1, item->level)))
    {
        attr->addStatusEffect(effect);
    }

    return true;
}

bool InventoryComponent::unequip(EquipmentSlot slot)
{
    auto owner = getCharacterOwner();
    if (!owner)
    {
        return false;
    }

    auto attr = owner->getAttributeComponent();
    if (!attr)
    {
        return false;
    }

    auto it = _equippedItems.find(slot);
    if (it == _equippedItems.end() || !it->second)
    {
        return false;
    }

    auto item = it->second;
    attr->removeEquipmentBonus(item->attributeBonus);

    // 移除装备自带的状态效果（如荆棘）
    removeEquipmentStatusEffectByItemId(attr, item->id);

    _equippedItems.erase(it);

    // 确保卸下的装备仍然可在背包中找到（按 id 去重）
    addToInventory(item);
    return true;
}

std::shared_ptr<Equipment> InventoryComponent::getEquipment(EquipmentSlot slot) const
{
    auto it = _equippedItems.find(slot);
    return (it != _equippedItems.end()) ? it->second : nullptr;
}

std::shared_ptr<Weapon> InventoryComponent::getEquippedWeapon() const
{
    auto equip = getEquipment(EquipmentSlot::WEAPON);
    return std::dynamic_pointer_cast<Weapon>(equip);
}

void InventoryComponent::setEquippedItems(const std::map<EquipmentSlot, std::shared_ptr<Equipment>>& items)
{
    // 注意：该接口主要用于读档/同步场景，必须确保“属性加成/特效”与装备列表一致。

    // 1) 先卸下现有装备（触发移除加成/特效）
    std::vector<EquipmentSlot> slotsToClear;
    slotsToClear.reserve(_equippedItems.size());
    for (const auto& kv : _equippedItems)
    {
        slotsToClear.push_back(kv.first);
    }
    for (auto slot : slotsToClear)
    {
        unequip(slot);
    }

    // 2) 再逐个穿戴新装备（触发添加加成/特效）
    for (const auto& kv : items)
    {
        if (kv.second)
        {
            equip(kv.second);
        }
    }
}

void InventoryComponent::ensureDefaultInventory()
{
    // 说明：这里放少量“测试/占位物品”，用于背包/装备/被动机制的基本交互验证。
    // 该函数应保持幂等：通过 addToInventory 的去重逻辑，避免重复加入。
    //
    // 注意：目前尚未接入掉落/商店等产出系统，因此通过“默认物品”保证功能可测试。

    // 新手剑（武器）
    {
        auto weapon = std::make_shared<Weapon>();
        weapon->id = GameConfig::Equipment::Weapon::STARTER_SWORD;
        weapon->name = "新手剑";
        weapon->description = "一把趁手的练习用短剑";
        weapon->slot = EquipmentSlot::WEAPON;
        weapon->type = WeaponType::SWORD;
        weapon->attackDamage = GameConfig::Player::DEFAULT_WEAPON_DAMAGE;
        weapon->attackRange = 60.0f;
        weapon->attackSpeed = 1.0f;
        weapon->attackAnimationPrefix = ""; // 为空则沿用角色默认攻击动画
        weapon->attackFrameCount = 3;
        weapon->attributeBonus.add(AttributeType::STRENGTH, 2.0f);
        addToInventory(weapon);
    }

    // 训练法杖（武器）
    {
        auto weapon = std::make_shared<Weapon>();
        weapon->id = GameConfig::Equipment::Weapon::TRAINING_STAFF;
        weapon->name = "训练法杖";
        weapon->description = "木制法杖，适合练习施法";
        weapon->slot = EquipmentSlot::WEAPON;
        weapon->type = WeaponType::STAFF;
        weapon->attackDamage = GameConfig::Player::DEFAULT_WEAPON_DAMAGE;
        weapon->attackRange = 80.0f;
        weapon->attackSpeed = 0.9f;
        weapon->attackAnimationPrefix = "";
        weapon->attackFrameCount = 3;
        weapon->attributeBonus.add(AttributeType::MAX_MP, 20.0f);
        addToInventory(weapon);
    }

    // 焰纹法杖（武器：命中有概率施加燃烧，可叠层）
    {
        auto weapon = std::make_shared<Weapon>();
        weapon->id = GameConfig::Equipment::Weapon::EMBER_STAFF;
        weapon->name = "焰纹法杖";
        weapon->description = "杖身刻着古老火纹。命中时有概率施加燃烧（可叠层），适合持续压制。";
        weapon->slot = EquipmentSlot::WEAPON;
        weapon->type = WeaponType::STAFF;
        weapon->attackDamage = GameConfig::Player::DEFAULT_WEAPON_DAMAGE + 2.0f;
        weapon->attackRange = 90.0f;
        weapon->attackSpeed = 0.95f;
        weapon->attackAnimationPrefix = "";
        weapon->attackFrameCount = 3;
        weapon->attributeBonus.add(AttributeType::MAX_MP, 30.0f);
        addToInventory(weapon);
    }

    // 血契短剑（武器：吸血，随装备等级成长）
    {
        auto weapon = std::make_shared<Weapon>();
        weapon->id = GameConfig::Equipment::Weapon::BLOOD_PACT_SWORD;
        weapon->name = "血契短剑";
        weapon->description = "刀刃渴望鲜血。造成伤害会按比例转化为生命回复（随装备等级成长）。";
        weapon->slot = EquipmentSlot::WEAPON;
        weapon->type = WeaponType::SWORD;
        weapon->attackDamage = GameConfig::Player::DEFAULT_WEAPON_DAMAGE + 3.0f;
        weapon->attackRange = 70.0f;
        weapon->attackSpeed = 1.05f;
        weapon->attackAnimationPrefix = "";
        weapon->attackFrameCount = 3;
        weapon->attributeBonus.add(AttributeType::STRENGTH, 3.0f);
        addToInventory(weapon);
    }

    // 皮帽（头盔）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Helmet::LEATHER_CAP;
        equip->name = "皮帽";
        equip->description = "简单的皮制头盔";
        equip->slot = EquipmentSlot::HELMET;
        equip->attributeBonus.add(AttributeType::MAX_HP, 20.0f);
        addToInventory(equip);
    }

    // 急救面罩（头盔：低血量触发救援，带冷却）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Helmet::EMERGENCY_MASK;
        equip->name = "急救面罩";
        equip->description = "内置应急药剂：生命低于 20% 时将生命抬升到 35%，45 秒冷却。";
        equip->slot = EquipmentSlot::HELMET;
        equip->attributeBonus.add(AttributeType::MAX_HP, 10.0f);
        addToInventory(equip);
    }

    // 皮甲（护甲）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Armor::LEATHER_ARMOR;
        equip->name = "皮甲";
        equip->description = "轻便护甲，提供基础防护";
        equip->slot = EquipmentSlot::ARMOR;
        equip->attributeBonus.add(AttributeType::DEFENSE, 1.0f);
        addToInventory(equip);
    }

    // 荆棘甲（护甲：反弹部分伤害，随装备等级成长）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Armor::THORNS_ARMOR;
        equip->name = "荆棘甲";
        equip->description = "带刺甲片会反弹部分伤害（带冷却，反伤随装备等级成长）。";
        equip->slot = EquipmentSlot::ARMOR;
        equip->attributeBonus.add(AttributeType::DEFENSE, 2.0f);
        addToInventory(equip);
    }

    // 轻便靴（靴子）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Boots::LIGHT_BOOTS;
        equip->name = "轻便靴";
        equip->description = "更轻的鞋子，跑得更快";
        equip->slot = EquipmentSlot::BOOTS;
        equip->attributeBonus.add(AttributeType::MOVE_SPEED, 20.0f);
        addToInventory(equip);
    }

    // 追猎之靴（靴子：击杀后短暂加速）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Boots::HUNTER_BOOTS;
        equip->name = "追猎之靴";
        equip->description = "击杀目标后进入亢奋：短时间内移动速度提升。";
        equip->slot = EquipmentSlot::BOOTS;
        equip->attributeBonus.add(AttributeType::MOVE_SPEED, 10.0f);
        addToInventory(equip);
    }
}
