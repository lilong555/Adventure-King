#include "StatusEffectFactory.h"
// 1. 必须包含所有具体的实现头文件
#include "Character/StatusEffects/Implementations/ThornsEffect.h"
#include "Character/StatusEffects/Implementations/BurningEffect.h"
#include "Character/StatusEffects/Implementations/RegenEffect.h"
#include "Character/StatusEffects/Implementations/PoisonEffect.h"
#include "Configs/GameConfigs.h"

// =================================================================
// 按装备 ID 创建（用于装备系统）
// =================================================================
StatusEffect* StatusEffectFactory::createEffectByItemId(int itemId, int level) {

    // 荆棘甲逻辑
    if (itemId == GameConfig::Equipment::Armor::THORNS_ARMOR) {
        float rate = 0.2f;
        float cd = 1.0f;
        // 返回子类实例
        auto effect = new (std::nothrow) ThornsEffect(rate, cd);
        if (effect) {
            effect->autorelease();
            return effect;
        }
    }

    // 这里可以映射其他带特殊效果的装备 ID
    // 例如：如果某件火系披风自带燃烧效果...

    return nullptr;
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
