#pragma once

#include <type_traits> // 必须包含这个，用于支持下面的位运算模板

// ============================================================
// 物理碰撞分类（强类型位掩码）
// ============================================================

enum class GamePhysicsCategory : int
{
    NONE = 0,            // 无碰撞

    // === 核心实体 ===
    PLAYER = 1 << 0,       // 玩家 (对应旧 CATEGORY_PLAYER)
    MONSTER = 1 << 1,       // 怪物/敌人 (对应旧 CATEGORY_ENEMY)
    PLATFORM = 1 << 2,       // 地面/平台 (对应旧 CATEGORY_PLATFORM)

    // === 交互物体 ===
    TRIGGER = 1 << 3,       // 触发器
    ITEM = 1 << 7,       // 掉落道具
    BOMB = 1 << 10,      // [新增] 炸弹/投掷物 (对应旧 CATEGORY_BOMB)

    // === 战斗判定 (Hitbox) ===
    PLAYER_ATTACK = 1 << 4,       // 玩家的攻击判定框
    MONSTER_ATTACK = 1 << 5,       // 怪物的攻击判定框
    BULLET = 1 << 6,       // 子弹

    // === 辅助 ===
    COLLISION = 1 << 8,       // 通用碰撞阻挡
    SENSOR = 1 << 9,       // 传感器（只检测不碰撞）

    ALL = -1    // 所有类别
};

// ============================================================
// 位运算操作符重载（让 enum class 支持 | 和 &）
// ============================================================

inline GamePhysicsCategory operator|(GamePhysicsCategory a, GamePhysicsCategory b)
{
    using T = std::underlying_type_t<GamePhysicsCategory>;
    return static_cast<GamePhysicsCategory>(static_cast<T>(a) | static_cast<T>(b));
}

inline GamePhysicsCategory operator&(GamePhysicsCategory a, GamePhysicsCategory b)
{
    using T = std::underlying_type_t<GamePhysicsCategory>;
    return static_cast<GamePhysicsCategory>(static_cast<T>(a) & static_cast<T>(b));
}

inline GamePhysicsCategory& operator|=(GamePhysicsCategory& a, GamePhysicsCategory b)
{
    a = a | b;
    return a;
}

inline GamePhysicsCategory& operator&=(GamePhysicsCategory& a, GamePhysicsCategory b)
{
    a = a & b;
    return a;
}

// ============================================================
// 辅助函数：将枚举转为 int (Cocos2d-x API 需要 int)
// ============================================================

inline int ToMask(GamePhysicsCategory c)
{
    return static_cast<int>(c);
}

// Allow checking bitmasks returned by Cocos2d-x APIs (int) against categories.
inline int operator&(int a, GamePhysicsCategory b)
{
    return a & ToMask(b);
}
