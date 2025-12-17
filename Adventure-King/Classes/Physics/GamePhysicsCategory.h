#pragma once
//存放涉及到物理的对象
//当你需要添加新的物体类型（例如 NPC、水流、梯子）时，只需在 enum class 中继续向左移位即可
#include <type_traits>


// ============================================================================
// 1. 游戏数值配置 (Game Constants)
// ============================================================================
namespace GameConfig
{
    // --- 技能系统配置 ---
    namespace Skill
    {
        const size_t SLOT_BOMB = 0;       // 炸弹技能槽位
        const size_t SLOT_FIREBALL = 1;   // 火球技能槽位       
    }

    // --- 炸弹属性配置 ---
    namespace Bomb
    {
        const int BOMB_ID = 1001;          // 炸弹技能ID
        const float BOMB_CD = 1.0f;       // 炸弹冷却时间
        const float BOMB_MP = 10.0f; // 炸弹蓝耗
        const float SPRITE_SCALE = 0.6f;  // 缩放比例
        const float THROW_SPEED_X = 300.0f; // 水平投掷速度
        const float THROW_SPEED_Y = 350.0f; // 垂直投掷速度
        const float EXPLOSION_RADIUS = 80.0f; // 爆炸半径
        const float BASE_DAMAGE = 150.0f;     // 基础伤害
        const float EXPLOSION_DELAY = 0.0f;   // 碰撞后延迟多久爆炸(秒)
    }
    namespace Fireball
    {
        const int FIREBALL_ID = 1002;          // 火球技能ID
        const float FIREBALL_CD = 1.2f;       // 火球冷却时间
        const float FIREBALL_MP = 15.0f; // 火球蓝耗

        const float SPRITE_SCALE = 0.5f;      // 缩放比例
        const float SPEED_X = 650.0f;         // 水平飞行速度
        const float EXPLOSION_RADIUS = 90.0f; // 爆炸半径
        const float BASE_DAMAGE = 220.0f;     // 基础伤害
    }

    // --- 玩家配置 ---
    namespace Player
    {
        inline constexpr float WALKSPEED = 220.0f;
        inline constexpr float RUNSPEED = 350.0f;              ///< 跑步速度
        inline constexpr float JUMP_FORCE = 400.0f;
        inline constexpr int MAX_JUMP_COUNT = 2;
        inline constexpr float JUMP_IMPULSE = 650.0f;           ///< 跳跃冲量
        inline constexpr float SCALE = 0.25f;
        inline constexpr float COLLISION_BOX_RATIO_W = 0.8f;  ///< 碰撞盒宽度比例
        inline constexpr float COLLISION_BOX_RATIO_H = 0.9f; ///< 碰撞盒高度比例

        
        
    }

    // --- 物理材质 (密度, 弹性, 摩擦) ---
    // 可以在代码中直接使用: PhysicsBody::createBox(size, GameConfig::Material::DEFAULT)
    namespace Material
    {
        const cocos2d::PhysicsMaterial DEFAULT(0.1f, 0.5f, 0.5f);
        const cocos2d::PhysicsMaterial COLLISION(1.0f, 0.0f, 0.0f);
        const cocos2d::PhysicsMaterial PLAYER(1.0f, 0.0f, 0.0f); // 玩家通常无摩擦无弹性以免卡住
        const cocos2d::PhysicsMaterial BOMB(0.5f, 0.3f, 0.2f);   // 炸弹带一点弹性
    }
}
// ============================================================
// 2.物理碰撞分类（强类型位掩码）
// ============================================================

enum class GamePhysicsCategory : int
{
    NONE = 0,            // 无碰撞

    // === 核心实体 ===
    PLAYER = 1 << 0,       // 玩家
    MONSTER = 1 << 1,       // 怪物/敌人
    PLATFORM = 1 << 2,       // 地面/平台

    // === 交互物体 ===
    TRIGGER = 1 << 3,       // 触发器
    ITEM = 1 << 7,       // 掉落道具
    BOMB = 1 << 10,      // 炸弹/投掷物

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


