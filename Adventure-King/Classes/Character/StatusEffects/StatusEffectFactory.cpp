#include "StatusEffectFactory.h"
// 必须包含所有具体的实现头文件
#include "Character/StatusEffects/Implementations/ThornsEffect.h"
#include "Character/StatusEffects/Implementations/BurningEffect.h"
#include "Character/StatusEffects/Implementations/RegenEffect.h"
#include "Character/StatusEffects/Implementations/PoisonEffect.h"
#include "Character/StatusEffects/Implementations/BloodPactLifestealEffect.h"
#include "Character/StatusEffects/Implementations/EmberStaffEffect.h"
#include "Character/StatusEffects/Implementations/EmergencyMaskEffect.h"
#include "Character/StatusEffects/Implementations/HunterBootsEffect.h"
#include "Configs/GameConfig.h"

// =================================================================
// 按装备 ID 创建（用于装备系统）
// =================================================================
StatusEffect* StatusEffectFactory::createEffectByItemId(int itemId, int level) {

    // 荆棘甲逻辑
    if (itemId == GameConfig::Equipment::Armor::THORNS_ARMOR) {
        float rate = GameConfig::EquipmentEffect::ThornsArmor::getReflectRate(level);
        float cd = GameConfig::EquipmentEffect::ThornsArmor::PROC_COOLDOWN;
        // 返回子类实例
        auto effect = new (std::nothrow) ThornsEffect(rate, cd);
        if (effect) {
            effect->autorelease();
            return effect;
        }
    }

    // 血契短剑：吸血（随装备等级成长）
    if (itemId == GameConfig::Equipment::Weapon::BLOOD_PACT_SWORD)
    {
        const float rate = GameConfig::EquipmentEffect::BloodPactSword::getLifestealRate(level);
        if (auto effect = BloodPactLifestealEffect::create(rate))
        {
            return effect;
        }
    }

    // 焰纹法杖：命中概率施加燃烧
    if (itemId == GameConfig::Equipment::Weapon::EMBER_STAFF)
    {
        if (auto effect = EmberStaffEffect::create(GameConfig::EquipmentEffect::EmberStaff::PROC_CHANCE,
                                                   GameConfig::EquipmentEffect::EmberStaff::PROC_COOLDOWN))
        {
            return effect;
        }
    }

    // 急救面罩：低血量救援（带冷却）
    if (itemId == GameConfig::Equipment::Helmet::EMERGENCY_MASK)
    {
        if (auto effect = EmergencyMaskEffect::create(GameConfig::EquipmentEffect::EmergencyMask::TRIGGER_HP_RATIO,
                                                      GameConfig::EquipmentEffect::EmergencyMask::HEAL_TARGET_HP_RATIO,
                                                      GameConfig::EquipmentEffect::EmergencyMask::PROC_COOLDOWN))
        {
            return effect;
        }
    }

    // 追猎之靴：击杀触发亢奋加速
    if (itemId == GameConfig::Equipment::Boots::HUNTER_BOOTS)
    {
        if (auto effect = HunterBootsEffect::create(GameConfig::EquipmentEffect::HunterBoots::BUFF_DURATION_SECONDS,
                                                    GameConfig::EquipmentEffect::HunterBoots::MOVE_SPEED_BONUS))
        {
            return effect;
        }
    }

    // 这里可以映射其他带特殊效果的装备 ID
    // 例如：如果某件火系披风自带燃烧效果...

    return nullptr;
}

bool StatusEffectFactory::tryGetEffectTypeByItemId(int itemId, StatusEffectType& outType)
{
    // 说明：该函数应保持“仅映射、不分配内存”，供装备系统移除特效时查询使用。
    if (itemId == GameConfig::Equipment::Armor::THORNS_ARMOR)
    {
        outType = StatusEffectType::THORNS;
        return true;
    }

    if (itemId == GameConfig::Equipment::Weapon::BLOOD_PACT_SWORD)
    {
        outType = StatusEffectType::EQUIP_BLOOD_PACT_SWORD;
        return true;
    }
    if (itemId == GameConfig::Equipment::Weapon::EMBER_STAFF)
    {
        outType = StatusEffectType::EQUIP_EMBER_STAFF;
        return true;
    }
    if (itemId == GameConfig::Equipment::Helmet::EMERGENCY_MASK)
    {
        outType = StatusEffectType::EQUIP_EMERGENCY_MASK;
        return true;
    }
    if (itemId == GameConfig::Equipment::Boots::HUNTER_BOOTS)
    {
        outType = StatusEffectType::EQUIP_HUNTER_BOOTS;
        return true;
    }

    return false;
}

// =================================================================
// 按类型创建（建议新增此接口，专门用于 DebugScene 或技能系统）
// =================================================================
StatusEffect* StatusEffectFactory::createEffectByType(StatusEffectType type, float power, float duration) {
    StatusEffect* effect = nullptr;

    switch (type) {
    case StatusEffectType::POISONED:
        // 如果你写了 PoisonEffect 子类，这里返回它
        effect = new (std::nothrow) PoisonEffect(power, 1.0f, duration);
        break;
    case StatusEffectType::BURNING:
        effect = new (std::nothrow) BurningEffect(power, 1.0f, duration);
        break;
    case StatusEffectType::REGEN:
        effect = new (std::nothrow) RegenEffect(power, 1.0f, duration);
        break;
    default:
        // 亢奋、眩晕等纯属性加成状态暂时可以使用基类
        effect = new (std::nothrow) StatusEffect();
        if (effect) {
            effect->type = type;
            effect->duration = duration;
        }
        break;
    }

    if (effect) {
        effect->autorelease();
    }
    return effect;
}
