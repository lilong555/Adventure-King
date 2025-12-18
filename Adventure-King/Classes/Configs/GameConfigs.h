#pragma once
#include "cocos2d.h"
// ============================================================================
// 1. 游戏数值配置 (Game Constants)
// ============================================================================
//namespace中存的都是写死的常量配置，方便全局调用

namespace GameConfig{
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
    namespace Monster
    {
        // 可以在这里放一些所有怪物通用的默认值（如果有的话）

        // --- 哥布林 (Goblin) 专属配置 ---
        namespace Goblin
        {
            // 基础属性
            inline constexpr float MAX_HP = 1000.0f;
            inline constexpr float MAX_MP = 0.0f;       // 哥布林可能没蓝条
            inline constexpr float STRENGTH = 10.0f;
            inline constexpr float DEFENSE = 2.0f;
            inline constexpr float CRITICAL_RATE = 0.05f;

            // 移动与战斗
            inline constexpr float MOVE_SPEED = 200.0f;      // 基础移速
            inline constexpr float ATTACK_INTERVAL = 2.0f;   // 攻击间隔 (秒)
            inline constexpr float ATTACK_RANGE = 150.0f;    // 攻击距离 (像素)

            // 视野/AI相关 (可选，建议也放在这)
            inline constexpr float VISION_RANGE = 400.0f;    // 索敌范围
            inline constexpr float CHASE_RANGE = 600.0f;     // 追击范围
        }

        // --- 以后加 Boss 或者其他怪物 ---
        namespace Slime
        {
            // ...
        }
    }
    // --- 物理材质 (密度, 弹性, 摩擦) ---
    // 可以在代码中直接使用: PhysicsBody::createBox(size, GameConfig::Material::DEFAULT)
    namespace Material
    {
        const cocos2d::PhysicsMaterial DEFAULT(0.1f, 0.5f, 0.5f);
        const cocos2d::PhysicsMaterial COLLISION(1.0f, 0.0f, 0.0f);
        const cocos2d::PhysicsMaterial PLAYER(1.0f, 0.0f, 0.0f); // 玩家通常无摩擦无弹性以免卡住
        const cocos2d::PhysicsMaterial BOMB(0.5f, 0.3f, 0.2f);   // 炸弹带一点弹性
        const cocos2d::PhysicsMaterial GOBLIN(1.0f, 0.0f, 0.0f); // 火球无弹性
    }
}
