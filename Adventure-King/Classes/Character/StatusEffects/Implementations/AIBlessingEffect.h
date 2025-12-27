#pragma once

#include "Character/Base/StatusEffect.h"

/**
 * @brief AI/NPC 赐福：属性加成 Buff（覆盖式）
 *
 * 说明：
 * - 这是一个“纯数值”效果：不做 tick，不做伤害钩子
 * - 表现（粒子/光效等）可以后续在 StatusEffectVfxComponent 里按 type 接入
 * - 重复施加时，建议由调用方先 remove，再 add（确保覆盖生效）
 */
class AIBlessingEffect final : public StatusEffect
{
public:
    static AIBlessingEffect *create(const Attributes &bonus);

private:
    bool initWithBonus(const Attributes &bonus);
};

