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
    /**
     * @brief 查询装备 ID 对应的状态效果类型（用于移除/判断，不创建对象）
     * @return true 表示存在映射；false 表示该装备不附带状态效果
     */
    static bool tryGetEffectTypeByItemId(int itemId, StatusEffectType& outType);
    static StatusEffect* createEffectByType(StatusEffectType type, float power, float duration);
};
