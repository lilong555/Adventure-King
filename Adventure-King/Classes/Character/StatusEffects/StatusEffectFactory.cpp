#include "StatusEffectFactory.h"
#include "Character/StatusEffects/Implementations/ThornsEffect.h"
//#include "Character/StatusEffects/Implementations/LifestealEffect.h"
#include "Configs/GameConfigs.h"

StatusEffect * StatusEffectFactory::createEffectByItemId(int itemId, int level) {

    // 所有的逻辑效果在这里根据 itemId 分发
    if (itemId == GameConfig::Equipment::Armor::THORNS_ARMOR) {
        // 1. 读取配置数值（暂时硬编码或从 GameConfig 取）
        float rate = 0.2f;
        float cd = 1.0f;

        // 2. 使用 new 创建对象并立即 autorelease
        // 这样它就遵循 Cocos2d-x 的引用计数规则，不需要手动 delete
        auto effect = new (std::nothrow) ThornsEffect(rate, cd);
        if (effect) {
            effect->autorelease();
            return effect;
        }
    }

    // 以后增加更多装备效果只需要在这里加 if/switch 分支
    /*
    if (itemId == GameConfig::Equipment::Weapon::BLOOD_PACT_SWORD) {
        auto effect = new (std::nothrow) LifestealEffect(0.1f);
        if (effect) {
            effect->autorelease();
            return effect;
        }
    }
    */

    return nullptr;
}
