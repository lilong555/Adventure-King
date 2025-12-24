#pragma once

#include "Character/Base/CharacterData.h"
#include "cocos2d.h"

#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

class CharacterBase;

/**
 * @brief 背包/装备组件：负责管理物品列表与穿戴状态，并在穿脱时同步属性加成与装备特效。
 *
 * 说明：
 * - 该组件只负责“数据与结算”，不负责角色动画/输入/移动等表现逻辑；
 * - 武器动画前缀等表现层更新由 PlayerCharacter 在外部处理（避免组件反向依赖 PlayerCharacter 私有成员）。
 */
class InventoryComponent : public cocos2d::Component
{
public:
    CREATE_FUNC(InventoryComponent);

    InventoryComponent();
    ~InventoryComponent() override;

    bool init() override;
    void onAdd() override;

    // -------------------------
    // 背包（数据存取）
    // -------------------------
    const std::vector<std::shared_ptr<Equipment>>& getInventoryItems() const { return _inventoryItems; }
    void addToInventory(const std::shared_ptr<Equipment>& item);
    void clearInventory();
    void setInventoryItems(const std::vector<std::shared_ptr<Equipment>>& items);

    // -------------------------
    // 装备（穿戴/卸下）
    // -------------------------
    bool equip(const std::shared_ptr<Equipment>& item);
    bool unequip(EquipmentSlot slot);

    std::shared_ptr<Equipment> getEquipment(EquipmentSlot slot) const;
    std::shared_ptr<Weapon> getEquippedWeapon() const;
    const std::map<EquipmentSlot, std::shared_ptr<Equipment>>& getEquippedItems() const { return _equippedItems; }
    void setEquippedItems(const std::map<EquipmentSlot, std::shared_ptr<Equipment>>& items) { _equippedItems = items; }

    // 默认测试物品：用于背包系统初期调试（按 id 去重，不会重复添加）
    void ensureDefaultInventory();

private:
    CharacterBase* getCharacterOwner() const { return _cachedOwner; }

    // 组件挂载后缓存 owner，避免频繁 dynamic_cast
    CharacterBase* _cachedOwner = nullptr;

    std::map<EquipmentSlot, std::shared_ptr<Equipment>> _equippedItems;
    std::vector<std::shared_ptr<Equipment>> _inventoryItems;
    std::unordered_set<int> _inventoryItemIds;
};

