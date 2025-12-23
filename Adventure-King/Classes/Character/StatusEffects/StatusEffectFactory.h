#pragma once
#include <memory>
#include "Character/Base/StatusEffect.h"

// 只需前向声明，不需要包含具体实现的头文件
class StatusEffect;

class StatusEffectFactory {
public:
    /**
     * @brief 根据装备 ID 创建对应的逻辑效果对象
     */
    static StatusEffect* createEffectByItemId(int itemId, int level);
    static StatusEffect* createEffectByType(StatusEffectType type, float power, float duration);
};
