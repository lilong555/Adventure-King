#pragma once
#include "cocos2d.h"
// ============================================================================
// 1. 游戏数值配置 (Game Constants)
// ============================================================================
// namespace中存的都是写死的常量配置，方便全局调用

namespace GameConfig
{
    // --- App/Engine 配置 ---
    namespace App
    {
        // 设计分辨率与适配基准
        const cocos2d::Size DESIGN_RESOLUTION_SIZE(1520, 840);
        const cocos2d::Size SMALL_RESOLUTION_SIZE(480, 320);
        const cocos2d::Size MEDIUM_RESOLUTION_SIZE(1024, 768);
        const cocos2d::Size LARGE_RESOLUTION_SIZE(2048, 1536);
        // 帧率控制
        inline constexpr float DEFAULT_FPS = 144.0f;
        inline constexpr int MAX_FPS = 300;
        inline constexpr bool SHOW_FPS = true;
        const char *const FPS_ENV_NAME = "ADVENTURE_KING_FPS";
    }

    // --- 技能系统配置 ---
    namespace Skill
    {
        inline constexpr size_t SLOT_BOMB = 0;     // 炸弹技能槽位
        inline constexpr size_t SLOT_FIREBALL = 0; // 火球技能槽位（当前 Klee 技能1）

        namespace Passive
        {
            // 被动技能 ID（集中管理，避免散落魔法数字）
            inline constexpr int TOUGHNESS = 2001; // 体魄强化
            inline constexpr int SWIFTNESS = 2002; // 迅捷步伐
            inline constexpr int FOCUS = 2003;     // 战斗专注
        }
    }

    // --- 炸弹属性配置 ---
    namespace Bomb
    {
        const int BOMB_ID = 1001;             // 炸弹技能ID
        const float BOMB_CD = 1.0f;           // 炸弹冷却时间
        const float BOMB_MP = 10.0f;          // 炸弹蓝耗
        const float SPRITE_SCALE = 0.6f;      // 缩放比例
        const float THROW_SPEED_X = 300.0f;   // 水平投掷速度
        const float THROW_SPEED_Y = 350.0f;   // 垂直投掷速度
        const float EXPLOSION_RADIUS = 80.0f; // 爆炸半径
        const float BASE_DAMAGE = 150.0f;     // 基础伤害
        const float EXPLOSION_DELAY = 0.0f;   // 碰撞后延迟多久爆炸(秒)
        inline constexpr float DAMAGE_SCALE = 1.0f;
    }
    namespace Fireball
    {
        const int FIREBALL_ID = 1002;    // 火球技能ID
        const float FIREBALL_CD = 1.2f;  // 火球冷却时间
        const float FIREBALL_MP = 15.0f; // 火球蓝耗

        const float SPRITE_SCALE = 0.5f;      // 缩放比例
        const float SPEED_X = 650.0f;         // 水平飞行速度
        const float EXPLOSION_RADIUS = 90.0f; // 爆炸半径
        const float BASE_DAMAGE = 220.0f;     // 基础伤害
        inline constexpr float DAMAGE_SCALE = 2.5f;
    }

    // --- 玩家配置 ---
    namespace Player
    {
        inline constexpr float WALKSPEED = 220.0f;
        inline constexpr float RUNSPEED = 350.0f; ///< 跑步速度
        inline constexpr float JUMP_FORCE = 400.0f;
        inline constexpr int MAX_JUMP_COUNT = 2;
        inline constexpr float JUMP_IMPULSE = 650.0f; ///< 跳跃冲量
        inline constexpr float SCALE = 0.25f;
        inline constexpr float COLLISION_BOX_RATIO_W = 0.8f; ///< 碰撞盒宽度比例
        inline constexpr float COLLISION_BOX_RATIO_H = 0.9f; ///< 碰撞盒高度比例
        inline constexpr float ANIM_DELAY_RUN = 0.15f;
        inline constexpr float ANIM_DELAY_WALK = 0.25f;
        inline constexpr float HURT_DURATION_SECONDS = 0.3f;
        inline constexpr float DEFAULT_WEAPON_DAMAGE = 5.0f;
        inline constexpr float STRENGTH_DAMAGE_MULTIPLIER = 1.5f;
        inline constexpr float GROUND_VELOCITY_THRESHOLD = 5.0f;
        inline constexpr float GROUND_NORMAL_THRESHOLD = -0.3f;
        inline constexpr float DEFAULT_MAX_MP = 1000.0f;

        // 等级/经验曲线配置（避免逻辑与 UI 里各写一份）
        namespace Leveling
        {
            inline constexpr int REQUIRED_EXP_PER_LEVEL = 100; // 线性：每级 *100

            /// @brief 获取“当前等级 -> 下一等级”所需经验值
            /// @details 等级会被夹取到至少为 1，避免出现 0 或负数导致的异常计算。
            inline int getRequiredExp(int level)
            {
                if (level < 1)
                {
                    level = 1;
                }
                return level * REQUIRED_EXP_PER_LEVEL;
            }
        }

        // 属性点配置（用于属性页面的单项加点）
        namespace AttributePoint
        {
            inline constexpr int POINTS_PER_LEVEL = 1; // 每次升级获得的属性点

            // 每点属性的成长数值（当前为占位，可根据数值体验再调整）
            inline constexpr float MAX_HP_PER_POINT = 10.0f;         // 生命力
            inline constexpr float STRENGTH_PER_POINT = 2.0f;        // 力量
            inline constexpr float MOVE_SPEED_PER_POINT = 10.0f;     // 敏捷（当前映射为移动速度）
            inline constexpr float DEFENSE_PER_POINT = 1.0f;         // 防御
            inline constexpr float CRITICAL_RATE_PER_POINT = 0.02f;  // 暴击率（+2%）
        }
    }
    namespace Monster
    {
        // 可以在这里放一些所有怪物通用的默认值（如果有的话）
        namespace Base
        {
            inline constexpr float SCALE = 0.36f;
            inline constexpr float ANCHOR_X = 0.5f;
            inline constexpr float ANCHOR_Y = 0.0f;
            inline constexpr float PHYSICS_BOX_RATIO_W = 0.35f;
            inline constexpr float PHYSICS_BOX_RATIO_H = 0.9f;
            inline constexpr float ACTIVE_UPDATE_DISTANCE_MULTIPLIER = 1.5f;
            inline constexpr float AI_UPDATE_INTERVAL = 0.1f;
            inline constexpr float AI_INACTIVE_UPDATE_INTERVAL = 0.3f;
            inline constexpr float MOVE_UPDATE_INTERVAL = 0.033f;
            inline constexpr float ATTACK_UPDATE_INTERVAL = 0.05f;
            inline constexpr float HP_BAR_WIDTH = 60.0f;
            inline constexpr float HP_BAR_HEIGHT = 8.0f;
            inline constexpr float HP_BAR_Y_OFFSET = 10.0f;
            inline constexpr float PATROL_REACH_EPSILON = 8.0f;
            inline constexpr float CHASE_DEADZONE_X = 8.0f;
            inline constexpr float FACE_DEADZONE_X = 8.0f;
        }

        // --- 哥布林 (Goblin) 专属配置 ---
        namespace Goblin
        {
            // 基础属性
            inline constexpr float MAX_HP = 1000.0f;
            inline constexpr float MAX_MP = 0.0f; // 哥布林可能没蓝条
            inline constexpr float STRENGTH = 10.0f;
            inline constexpr float DEFENSE = 2.0f;
            inline constexpr float CRITICAL_RATE = 0.05f;

            // 经验奖励（按玩家等级简单缩放）
            inline constexpr int EXP_REWARD_BASE = 20;
            inline constexpr int EXP_REWARD_PER_LEVEL = 3;

            // 移动与战斗
            inline constexpr float MOVE_SPEED = 200.0f;    // 基础移速
            inline constexpr float ATTACK_INTERVAL = 2.0f; // 攻击间隔 (秒)
            inline constexpr float ATTACK_RANGE = 150.0f;  // 攻击距离 (像素)

            // 视野/AI相关 (可选，建议也放在这)
            inline constexpr float VISION_RANGE = 700.0f; // 索敌范围
            inline constexpr float CHASE_RANGE = 0.0f;    // 追击范围 (0=不返回)
            inline constexpr bool PATROL_ENABLED = true;

            // HP 缩放
            inline constexpr int HP_SCALE_BASE = 200;
            inline constexpr int HP_SCALE_PER_LEVEL = 100;
            inline constexpr int HP_SCALE_PER_10_LEVEL = 1000;
            inline constexpr float HP_BAR_SCALE = 2.0f;

            // 攻击动画/判定
            inline constexpr float ATTACK_ANIM_FRAME_DELAY = 0.1f;
            inline constexpr int ATTACK_HIT_FRAME_INDEX = 3;
            inline constexpr float ATTACK_HIT_FALLBACK_TIME = 0.4f;
            inline constexpr float HITBOX_TUNE_SCALE = 0.6f;
            inline constexpr float HITBOX_OFFSET_X = 100.0f;
            inline constexpr float HITBOX_OFFSET_Y = 170.0f;
            inline constexpr float HITBOX_WIDTH = 300.0f;
            inline constexpr float HITBOX_HEIGHT = 20.0f;
            inline constexpr float HITBOX_LIFE_SECONDS = 0.1f;
            inline constexpr float WALK_ANIM_FRAME_DELAY = 0.15f;
        }

        // --- 哥布鲁 (Goblu) Boss 配置 ---
        namespace Goblu
        {
            // 基础属性
            inline constexpr float MAX_HP = 1500.0f;
            inline constexpr float MAX_MP = 0.0f;
            inline constexpr float STRENGTH = 25.0f;
            inline constexpr float DEFENSE = 6.0f;
            inline constexpr float CRITICAL_RATE = 0.08f;

            // 经验奖励（Boss）
            inline constexpr int EXP_REWARD_BASE = 200;
            inline constexpr int EXP_REWARD_PER_LEVEL = 12;

            // 移动与战斗
            inline constexpr float MOVE_SPEED = 160.0f;
            inline constexpr float ATTACK_INTERVAL = 1.5f;
            inline constexpr float ATTACK_RANGE = 220.0f;

            // 视野/AI 相关
            inline constexpr float VISION_RANGE = 900.0f;
            inline constexpr float CHASE_RANGE = 0.0f;
            inline constexpr bool PATROL_ENABLED = true;

            // Boss 体型/血条
            inline constexpr float SCALE = 0.72f;
            inline constexpr float SCALE_MULTIPLIER = 2.0f;
            inline constexpr float HP_BAR_SCALE = 1.5f;
            inline constexpr float PHYSICS_BOX_RATIO_W = 0.135f;

            // 攻击动画/判定
            inline constexpr float ATTACK_ANIM_FRAME_DELAY = 0.3f;
            inline constexpr float ATTACK_NEAR_GAP_THRESHOLD = 30.0f;
            inline constexpr int ATTACK_HIT_FRAME_INDEX = 2;
            inline constexpr float ATTACK_HIT_FALLBACK_TIME = ATTACK_ANIM_FRAME_DELAY * ATTACK_HIT_FRAME_INDEX;
            inline constexpr float HITBOX_TUNE_SCALE = SCALE;
            inline constexpr float HITBOX_OFFSET_X = 160.0f;
            inline constexpr float HITBOX_OFFSET_Y = 170.0f;
            inline constexpr float HITBOX_WIDTH = 380.0f;
            inline constexpr float HITBOX_HEIGHT = 30.0f;
            inline constexpr float HITBOX_LIFE_SECONDS = 0.12f;
            inline constexpr float WALK_ANIM_FRAME_DELAY = 0.18f;
        }

        // --- 以后加 Boss 或者其他怪物 ---
        namespace Slime
        {
            // ...
        }
    }

    namespace Klee
    {
        namespace NormalAttack
        {
            inline constexpr float ANIM_FRAME_DELAY = 0.13f;
            inline constexpr float PROJECTILE_SCALE = 0.5f;
            inline constexpr float SPAWN_OFFSET_X_RATIO = 0.35f;
            inline constexpr float SPAWN_OFFSET_X = 20.0f;
            inline constexpr float SPAWN_OFFSET_Y_RATIO = 0.15f;
            inline constexpr float SPAWN_OFFSET_Y = 0.0f;
            inline constexpr float EXPLOSION_VFX_SCALE = 0.8f;
            inline constexpr float EXPLOSION_VFX_SCALE_UP_DURATION = 0.2f;
            inline constexpr float EXPLOSION_VFX_SCALE_UP_FACTOR = 1.2f;
            inline constexpr float EXPLOSION_VFX_FADE_OUT_DURATION = 0.3f;
        }

        namespace FireballSkill
        {
            inline constexpr size_t SKILL_SLOT = 0;
            inline constexpr float CAST_ANIM_FRAME_DELAY = 0.04f;
            inline constexpr float PROJECTILE_SCALE = 1.10f;
            inline constexpr float SPAWN_OFFSET_X_RATIO = 0.40f;
            inline constexpr float SPAWN_OFFSET_X = 25.0f;
            inline constexpr float SPAWN_OFFSET_Y_RATIO = 0.20f;
            inline constexpr float SPAWN_OFFSET_Y = 0.0f;
            inline constexpr float LOOP_ANIM_DELAY = 0.08f;
            inline constexpr float EXPLOSION_FRAME_DELAY = 0.05f;
            inline constexpr float EXPLOSION_FRAME_SCALE = 0.9f;
        }
    }

    namespace StatusEffect
    {
        namespace Burning
        {
            inline constexpr float DURATION_SECONDS = 5.0f;
            inline constexpr float TICK_INTERVAL_SECONDS = 0.5f;
            inline constexpr float BASE_DAMAGE_SCALE = 0.1f;
            inline constexpr float PER_STACK_DAMAGE_SCALE = 0.1f;
        }
    }

    namespace LevelMap
    {
        const cocos2d::Vec2 DEFAULT_SPAWN_POINT(100.0f, 200.0f);
        const cocos2d::PhysicsMaterial COLLISION_PHYSICS_MATERIAL(1.0f, 0.0f, 0.8f);
        inline constexpr float DEFAULT_GATE_INTERACT_DISTANCE = 100.0f;
        inline constexpr float SPAWN_SPACING_X = 80.0f;
        inline constexpr float SPAWN_INTERVAL_SECONDS = 0.4f;
        inline constexpr int DEFAULT_CHARACTER_Z_ORDER = 5;
        inline constexpr float ENEMY_SPAWN_CHECK_INTERVAL_SECONDS = 0.1f;
    }

    namespace Combat
    {
        inline constexpr float ARMOR_CONST = 100.0f;
    }

    namespace Scene
    {
        const char *const DEFAULT_FONT_PATH = "fonts/ZCOOLKuaiLe-Regular.ttf";
        const char *const DEFAULT_PLAYER_SPRITE = "Sprites/Characters/Player/Klee/defalt/spr_klee_run.png";
        const char *const MAP_LOAD_FAILED_TEXT = " - Map Load Failed";
        // 场景切换
        inline constexpr float TRANSITION_DURATION = 0.5f;
        inline constexpr float MENU_TRANSITION_DURATION = 0.6f;
        inline constexpr float TRANSITION_MESSAGE_DELAY = 1.0f;
        inline constexpr float TRANSITION_FADE_DURATION = 0.8f;
        // 层级
        inline constexpr int BACKGROUND_Z_ORDER = -1;
        inline constexpr int PLAYER_Z_ORDER = 5;
        inline constexpr int COLLISION_DEBUG_Z_ORDER = 100;
    }

    namespace Map
    {
        namespace OriginMushroom
        {
            inline constexpr int BACKGROUND_COUNT = 6;
            const char *const BACKGROUND_PREFIX = "Map/Origin_Mushroom/Origin_Mushroom_";
        }
    }
    namespace UI
    {
        const char *const GATE_INTERACTION_HINT = "Press W to enter gate";
        inline constexpr int Z_ORDER = 100;
        inline constexpr float UPDATE_INTERVAL_SECONDS = 0.05f;
        inline constexpr int SKILL_BAR_SLOT_COUNT = 4;

        inline constexpr float PADDING = 20.0f;
        inline constexpr float STATUS_BAR_OFFSET_X = 50.0f;
        inline constexpr float SKILL_BAR_OFFSET_X = 150.0f;
        inline constexpr float SKILL_BAR_OFFSET_Y = 80.0f;
        inline constexpr float BOSS_BAR_OFFSET_Y = 60.0f;
        inline constexpr float MAP_BUTTON_OFFSET = 40.0f;
        inline constexpr float INTERACTION_HINT_OFFSET_Y = 80.0f;
        inline constexpr float LEVEL_NAME_OFFSET_X = 100.0f;
        inline constexpr float LEVEL_NAME_OFFSET_Y = 100.0f;

        namespace MainMenu
        {
            // 主菜单布局与音乐
            inline constexpr float BUTTON_HORIZONTAL_SPACING = 180.0f;
            inline constexpr float SUB_MENU_Y_MULTIPLIER = 1.2f;
            inline constexpr float MENU_OFFSET_Y_DIVISOR = 20.0f;
            inline constexpr int CONTENT_Z_ORDER = 5;
            inline constexpr int MENU_Z_ORDER = 1;
            inline constexpr float BGM_VOLUME = 0.5f;
            inline constexpr float BGM_DELAY_SECONDS = 0.1f;
        }

        namespace SaveMenu
        {
            // 存档菜单整体缩放
            inline constexpr float TARGET_HEIGHT_RATIO = 0.7f;
        }

        namespace SettingMenu
        {
            // 设置菜单整体缩放
            inline constexpr float TARGET_HEIGHT_RATIO = 0.6f;
        }
    }

    namespace Save
    {
        // 存档槽位与自动存档
        inline constexpr int MAX_SLOTS = 5;
        inline constexpr float AUTO_SAVE_INTERVAL_SECONDS = 300.0f;
    }

    namespace Debug
    {
        // 调试场景常量
        inline constexpr float JUMP_IMPULSE = 350.0f;
        inline constexpr float GROUND_Y = 100.0f;
        inline constexpr size_t MAX_LOG_LINES = 5;
        inline constexpr float DEATH_RESET_DELAY = 2.0f;
    }
    // --- 物理材质 (密度, 弹性, 摩擦) ---
    // 可以在代码中直接使用: PhysicsBody::createBox(size, GameConfig::Material::DEFAULT)
    namespace Material
    {
        const cocos2d::PhysicsMaterial DEFAULT(0.1f, 0.5f, 0.5f);
        const cocos2d::PhysicsMaterial COLLISION(1.0f, 0.0f, 0.0f);
        const cocos2d::PhysicsMaterial PLAYER(1.0f, 0.0f, 0.0f); // 玩家通常无摩擦无弹性以免卡住
        const cocos2d::PhysicsMaterial BOMB(0.5f, 0.3f, 0.2f);   // 炸弹带一点弹性
        const cocos2d::PhysicsMaterial MONSTER(1.0f, 0.0f, 0.0f);
        const cocos2d::PhysicsMaterial GOBLIN(1.0f, 0.0f, 0.0f); // 火球无弹性
    }
}
