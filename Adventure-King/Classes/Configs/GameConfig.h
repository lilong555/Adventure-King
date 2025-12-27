// 游戏相关数值配置
#pragma once
#include "cocos2d.h"
#include "Character/Base/CharacterData.h"
#include <algorithm>
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
            inline constexpr int TOUGHNESS = 2001;    // 体魄强化
            inline constexpr int SWIFTNESS = 2002;    // 迅捷步伐
            inline constexpr int FOCUS = 2003;        // 战斗专注
            inline constexpr int BLOODTHIRST = 2004;  // 嗜血（吸血）
            inline constexpr int EMBER_MARK = 2005;   // 余烬印记（命中附加燃烧）
            inline constexpr int FULL_HP_CRIT = 2006; // 满血暴击（条件触发）
            inline constexpr int CRIT_ECHO = 2007;    // 冷却回响（暴击缩短冷却）
            inline constexpr int POISON_TOUCH = 2008; // 淬毒（命中附加中毒）
        }

        namespace PassiveEffect
        {
            // 嗜血：按“实际造成伤害”回复生命（比例）
            inline constexpr float BLOODTHIRST_LIFESTEAL = 0.05f;
            // 吸血上限：避免未来叠加来源过多导致数值失控（当前默认不会触发该上限）
            inline constexpr float LIFESTEAL_TOTAL_MAX = 0.20f;

            // 余烬印记：命中附加燃烧
            inline constexpr float EMBER_MARK_PROC_CHANCE = 0.20f;
            inline constexpr float EMBER_MARK_PROC_COOLDOWN = 0.25f;

            // 满血暴击：满血时暴击率加成
            inline constexpr float FULL_HP_CRIT_BONUS = 0.25f;

            // 冷却回响：暴击时减少所有主动技能冷却（秒）
            inline constexpr float CRIT_ECHO_REDUCE_SECONDS = 0.35f;
            inline constexpr float CRIT_ECHO_PROC_COOLDOWN = 0.25f;

            // 淬毒：命中附加中毒
            inline constexpr float POISON_TOUCH_PROC_CHANCE = 0.18f;
            inline constexpr float POISON_TOUCH_PROC_COOLDOWN = 0.35f;
        }
    }

    // --- 装备 ID（集中管理，便于扩展“特效/机制”）---
    namespace Equipment
    {
        namespace Weapon
        {
            inline constexpr int STARTER_SWORD = 5001;    // 新手剑
            inline constexpr int TRAINING_STAFF = 5002;   // 训练法杖
            inline constexpr int EMBER_STAFF = 5003;      // 焰纹法杖（命中燃烧）
            inline constexpr int BLOOD_PACT_SWORD = 5004; // 血契短剑（吸血）
        }

        namespace Helmet
        {
            inline constexpr int LEATHER_CAP = 5101;    // 皮帽
            inline constexpr int EMERGENCY_MASK = 5104; // 急救面罩（低血量救援）
        }

        namespace Armor
        {
            inline constexpr int LEATHER_ARMOR = 5102; // 皮甲
            inline constexpr int THORNS_ARMOR = 5105;  // 荆棘甲（反伤）
        }

        namespace Boots
        {
            inline constexpr int LIGHT_BOOTS = 5103;  // 轻便靴
            inline constexpr int HUNTER_BOOTS = 5106; // 追猎之靴（击杀加速）
        }
    }

    // --- 装备特效参数（随装备等级可做成长）---
    namespace EquipmentEffect
    {
        namespace ThornsArmor
        {
            inline constexpr float REFLECT_RATE_BASE = 0.15f;      // 基础反伤比例
            inline constexpr float REFLECT_RATE_PER_LEVEL = 0.01f; // 每级额外反伤比例
            inline constexpr float REFLECT_RATE_MAX = 0.35f;       // 反伤上限
            inline constexpr float PROC_COOLDOWN = 0.50f;          // 触发间隔（秒）

            // 反伤比例（随装备等级成长并做夹取）
            inline float getReflectRate(int level)
            {
                level = std::max(1, level);
                float rate = REFLECT_RATE_BASE + REFLECT_RATE_PER_LEVEL * static_cast<float>(level - 1);
                return std::max(0.0f, std::min(rate, REFLECT_RATE_MAX));
            }
        }

        namespace EmergencyMask
        {
            inline constexpr float TRIGGER_HP_RATIO = 0.20f;     // 触发阈值：低于最大生命的 20%
            inline constexpr float HEAL_TARGET_HP_RATIO = 0.35f; // 触发后将生命抬到最大生命的 35%
            inline constexpr float PROC_COOLDOWN = 45.0f;        // 冷却（秒）
        }

        namespace HunterBoots
        {
            inline constexpr float BUFF_DURATION_SECONDS = 2.0f;
            inline constexpr float MOVE_SPEED_BONUS = 60.0f;
        }

        namespace EmberStaff
        {
            inline constexpr float PROC_CHANCE = 0.25f;
            inline constexpr float PROC_COOLDOWN = 0.20f;
        }

        namespace BloodPactSword
        {
            inline constexpr float LIFESTEAL_BASE = 0.03f;
            inline constexpr float LIFESTEAL_PER_LEVEL = 0.002f;
            inline constexpr float LIFESTEAL_MAX = 0.10f;

            // 吸血比例（随装备等级成长并做夹取）
            inline float getLifestealRate(int level)
            {
                level = std::max(1, level);
                float rate = LIFESTEAL_BASE + LIFESTEAL_PER_LEVEL * static_cast<float>(level - 1);
                return std::max(0.0f, std::min(rate, LIFESTEAL_MAX));
            }
        }
    }

    // --- 炸弹属性配置 ---
    namespace Bomb
    {
        const int BOMB_ID = 1001;             // 炸弹技能ID
        const float BOMB_CD = 1.0f;           // 炸弹冷却时间
        const float BOMB_MP = 10.0f;          // 炸弹蓝耗
        const float SPRITE_SCALE = 0.1f;      // 缩放比例
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
        // 击破值：命中/爆炸每次结算对 Boss 击破条的累计值（可按技能单独调参）
        inline constexpr int BREAK_DAMAGE = 3;

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
        inline constexpr float SCALE = 0.4f;
        // 角色素材缩放补偿：
        // 不同职业的原始 PNG 像素尺寸/留白不一致（例如战士素材更“宽大”，刺客素材更“扁平”），
        // 如果统一使用同一个 SCALE，会导致游戏内可视体/物理体/攻击判定范围的“实际大小”不合理。
        // 因此这里以 SCALE 为基准，按职业追加倍率做统一补偿。
        //
        // 注意：这是“素材尺寸补偿”，不是数值设计上的体型差异；后续若替换为统一尺寸素材，可将倍率调回 1.0。
        inline constexpr float WARRIOR_SPRITE_SCALE_MULTIPLIER = 1.0f;
        inline constexpr float ASSASSIN_SPRITE_SCALE_MULTIPLIER = 1.8f;
        // 刺客碰撞盒基础尺寸（未缩放）：
        // 由于刺客素材（700x370）横向留白较大，若按贴图尺寸比例生成碰撞盒会导致碰撞范围过宽。
        // 这里参考 Goblu 的“固定物理体尺寸”思路，给刺客使用固定碰撞盒（再叠加 SCALE 与职业倍率）。
        inline constexpr float ASSASSIN_COLLISION_BOX_WIDTH = 137.0f;
        inline constexpr float ASSASSIN_COLLISION_BOX_HEIGHT = 280.0f;
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
            // 不同职业的成长数值
            inline Attributes getGrowthByRole(CharacterRole role)
            {
                Attributes growth;
                switch (role)
                {
                case CharacterRole::WARRIOR:
                    growth.set(AttributeType::MAX_HP, 15.0f);
                    growth.set(AttributeType::MAX_MP, 2.0f);
                    growth.set(AttributeType::STRENGTH, 3.0f);
                    growth.set(AttributeType::DEFENSE, 1.5f);
                    break;
                case CharacterRole::MAGE:
                    growth.set(AttributeType::MAX_HP, 8.0f);
                    growth.set(AttributeType::MAX_MP, 10.0f);
                    growth.set(AttributeType::STRENGTH, 1.0f);
                    growth.set(AttributeType::DEFENSE, 0.5f);
                    break;
                    // 其他职业...
                default:
                    growth.set(AttributeType::MAX_HP, 10.0f);
                    growth.set(AttributeType::STRENGTH, 2.0f);
                    break;
                }
                return growth;
            }
        }

        // 属性点配置（用于属性页面的单项加点）
        namespace AttributePoint
        {
            inline constexpr int POINTS_PER_LEVEL = 1; // 每次升级获得的属性点

            // 每点属性的成长数值（当前为占位，可根据数值体验再调整）
            inline constexpr float MAX_HP_PER_POINT = 10.0f;        // 生命力
            inline constexpr float STRENGTH_PER_POINT = 2.0f;       // 力量
            inline constexpr float MOVE_SPEED_PER_POINT = 10.0f;    // 敏捷（当前映射为移动速度）
            inline constexpr float DEFENSE_PER_POINT = 1.0f;        // 防御
            inline constexpr float CRITICAL_RATE_PER_POINT = 0.02f; // 暴击率（+2%）
        }

        // 技能点配置（用于学习主动/被动技能）
        namespace SkillPoint
        {
            // 主动/被动技能点拆分：两者都通过升级获得
            inline constexpr int ACTIVE_POINTS_PER_LEVEL = 1;  // 每次升级获得的主动技能点
            inline constexpr int PASSIVE_POINTS_PER_LEVEL = 1; // 每次升级获得的被动技能点
        }
    }

    namespace Klee
    {
        namespace NormalAttack
        {
            // 击破值：普通攻击（TNT）每次命中对 Boss 击破条的累计值
            inline constexpr int BREAK_DAMAGE = 1;
            inline constexpr float ANIM_FRAME_DELAY = 0.13f;
            inline constexpr float BOMB_SCALE = 0.3f;
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
            inline constexpr float FIREBALL_SCALE = 1.10f;
            inline constexpr float SPAWN_OFFSET_X_RATIO = 0.40f;
            inline constexpr float SPAWN_OFFSET_X = 25.0f;
            inline constexpr float SPAWN_OFFSET_Y_RATIO = 0.20f;
            inline constexpr float SPAWN_OFFSET_Y = 0.0f;
            inline constexpr float LOOP_ANIM_DELAY = 0.08f;
            inline constexpr float EXPLOSION_FRAME_DELAY = 0.05f;
            inline constexpr float EXPLOSION_FRAME_SCALE = 0.9f;
        }
    }

    // --- 刺客配置 ---
    namespace Assassin
    {
        // 近战技能：斩击（使用 Sprites/Characters/Player/maaer/slash 下的序列帧）
        namespace SlashSkill
        {
            const int SLASH_ID = 1003;   // 斩击技能ID
            const float SLASH_CD = 0.8f; // 冷却时间
            const float SLASH_MP = 0.0f; // 蓝耗（暂不消耗）
            inline constexpr size_t SKILL_SLOT = 0;
            // 击破值：斩击是 4 段伤害（每帧一次），这里按“每段命中”累计（可按技能单独调参）
            inline constexpr int BREAK_DAMAGE_PER_HIT = 1;

            inline constexpr float CAST_ANIM_FRAME_DELAY = 0.12f;
            inline constexpr float DAMAGE_SCALE = 1.2f; // 基于攻击力的倍率

            // 命中判定框：相对角色包围盒比例（可调参）
            inline constexpr float HITBOX_LIFE_SECONDS = 0.10f;
            inline constexpr float HITBOX_DELAY_SECONDS = 0.05f;
            inline constexpr float HITBOX_WIDTH_RATIO = 0.60f;
            inline constexpr float HITBOX_HEIGHT_RATIO = 0.70f;
            inline constexpr float HITBOX_OFFSET_X_RATIO = 0.35f;
            inline constexpr float HITBOX_OFFSET_Y = 6.0f;
        }

        // 主动技能：孤注一掷
        // 效果：将生命降到 1 点，并进入“高手状态”（增伤）
        namespace AllInSkill
        {
            const int ALL_IN_ID = 1005;     // 孤注一掷技能ID
            const float ALL_IN_DURATION = 15.0f; // 持续时间（秒）
            const float ALL_IN_CD = 15.0f;       // 冷却时间（秒）
            const float ALL_IN_MP = 0.0f;   // 蓝耗（暂不消耗）
            inline constexpr size_t SKILL_SLOT = 1; // 默认放在 Q 槽位
            // 击破值：该技能不造成伤害，因此永远为 0
            inline constexpr int BREAK_DAMAGE = 0;

            // 1000% 增伤：伤害提升 1000%，即最终伤害为原始伤害的 1100%（11 倍）
            inline constexpr float DAMAGE_MULTIPLIER = 11.0f;
            inline constexpr float MIN_HP_AFTER_CAST = 1.0f;

            inline constexpr const char* VFX_PLIST = "Particle/par_nap.plist";
            // 高手状态持续特效：只要出伤倍率仍在生效，就挂在角色身上循环播放
            inline constexpr const char* KEEP_VFX_PLIST = "Particle/par_nap_keep.plist";
        }
    }

    namespace Warrior
    {
        // 普通攻击：挥砍（使用 Sprites/Characters/Player/maaer/slash 下的序列帧）
        // 近战判定：持续时间越短越不容易误伤/重复命中
        inline constexpr float HITBOX_LIFE_SECONDS = 0.10f;
        inline constexpr float HITBOX_DELAY_SECONDS = 0.05f; // 略微延迟，贴近挥砍动作

        // 命中框尺寸（先用相对值占位，后续可按手感调参）
        inline constexpr float HITBOX_WIDTH_RATIO = 0.55f;
        inline constexpr float HITBOX_HEIGHT_RATIO = 0.75f;
        inline constexpr float HITBOX_OFFSET_X_RATIO = 0.55f;
        inline constexpr float HITBOX_OFFSET_Y = 8.0f;

        // 主动技能：Fire（使用 Sprites/Characters/Player/man/fire 下的序列帧）
        namespace FireSkill
        {
            const int FIRE_ID = 1004;    // Fire 技能ID（避免与 Bomb/Fireball/Slash 冲突）
            const float FIRE_CD = 1.0f;  // 冷却时间（秒）
            const float FIRE_MP = 0.0f;  // 蓝耗（暂不消耗）
            inline constexpr size_t SKILL_SLOT = 0; // 默认放在 0 号槽位（E/K）

            inline constexpr float CAST_ANIM_FRAME_DELAY = 0.12f; // fire_1~fire_3 播放速度
            inline constexpr float DAMAGE_SCALE = 1.0f;           // “照抄伤害”：按攻击力等比结算
            // 击破值：Fire 命中一次对 Boss 击破条的累计值（可按技能单独调参）
            inline constexpr int BREAK_DAMAGE = 3;

            // 命中判定框：按战士自身尺寸倍数计算
            inline constexpr float HITBOX_WIDTH_MULTIPLIER = 2.0f;
            inline constexpr float HITBOX_HEIGHT_MULTIPLIER = 2.0f;
            inline constexpr float HITBOX_LIFE_SECONDS = 0.10f;
            inline constexpr int HIT_TRIGGER_FRAME_INDEX = 3; // 第 3 帧开始触发伤害/特效
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
            
            inline constexpr float MAX_HP = 1.0f;//700 注意这只是缩放之前的基础数值 
            inline constexpr float MAX_MP = 0.0f; // 哥布林可能没蓝条
            inline constexpr float STRENGTH = 10.0f;
            inline constexpr float DEFENSE = 2.0f;
            inline constexpr float CRITICAL_RATE = 0.05f;
            inline constexpr int HP_SCALE_BASE = 1.0f;         // 基础缩放倍率
            inline constexpr int HP_SCALE_PER_LEVEL = 0.1f;    // 每级倍率增量
            inline constexpr int HP_SCALE_PER_10_LEVEL = 0; // 每10级额外增量

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
            inline constexpr float HP_BAR_SCALE = 3.0f;

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
            inline constexpr float MAX_HP = 1.0f;  //1000
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
            inline constexpr float REMOTE_HITBOX_VFX_HOLD_SECONDS = 0.35f; // 远程攻击特效展示时间（碰撞禁用后仍保留节点）
            inline constexpr float WALK_ANIM_FRAME_DELAY = 0.18f;

            // 击破机制：满条后倒地 -> 起身（替代传统“受击硬直”）
            inline constexpr int BREAK_MAX = 16;
            inline constexpr float BREAK_FALL_ANIM_FRAME_DELAY = 0.12f;
            inline constexpr float BREAK_DOWN_HOLD_SECONDS = 3.0f; // 倒地后在 fall_3 停留时间（可调）
            inline constexpr float BREAK_RISE_ANIM_FRAME_DELAY = 0.12f;
        }

        // --- 黑暗法师 (Obscur) 普通怪物配置 ---
        namespace Obscur
        {
            // 基础属性（可后续按数值体验再调）
            inline constexpr float MAX_HP = 1.0f;//500
            inline constexpr float MAX_MP = 0.0f;
            inline constexpr float STRENGTH = 18.0f;
            inline constexpr float DEFENSE = 4.0f;
            inline constexpr float CRITICAL_RATE = 0.06f;

            // 经验奖励（按玩家等级缩放）
            inline constexpr int EXP_REWARD_BASE = 35;
            inline constexpr int EXP_REWARD_PER_LEVEL = 4;

            // 移动与战斗
            inline constexpr float MOVE_SPEED = 170.0f;
            inline constexpr float ATTACK_INTERVAL = 2.2f;
            inline constexpr float ATTACK_RANGE = 520.0f;        // 最大攻击距离（用于远程冰）
            inline constexpr float MELEE_TRIGGER_RANGE = 180.0f; // 小于该距离优先近战

            // 视野/AI 相关
            inline constexpr float VISION_RANGE = 850.0f;
            inline constexpr float CHASE_RANGE = 0.0f;
            inline constexpr bool PATROL_ENABLED = true;

            // 体型/碰撞盒：要求像 Goblu 一样自定义“实际碰撞箱”
            inline constexpr float SCALE = (1.5f) * Base::SCALE;
            inline constexpr float HP_BAR_SCALE = 2.0f;
            inline constexpr float PHYSICS_BOX_WIDTH = 235.0f;
            inline constexpr float PHYSICS_BOX_HEIGHT = 449.0f;

            // 动画帧间隔
            inline constexpr float ATTACK_ANIM_FRAME_DELAY = 0.20f; // Obscur_attack_1..4
            inline constexpr float USEICE_ANIM_FRAME_DELAY = 0.12f; // Obscur_useice_1..2（循环）
            inline constexpr float ICE_ANIM_FRAME_DELAY = 0.35f;    // Obscur_ice_1..5

            // 近战命中判定：第 4 帧开始到第 4 帧结束
            inline constexpr int MELEE_HIT_START_FRAME = 4;
            inline constexpr int MELEE_HIT_END_FRAME = 4;
            inline constexpr float MELEE_HITBOX_OFFSET_X = 50.0f;
            inline constexpr float MELEE_HITBOX_OFFSET_Y = 50.0f;
            inline constexpr float MELEE_HITBOX_WIDTH = 50.0f;
            inline constexpr float MELEE_HITBOX_HEIGHT = 135.0f;

            // 远程冰命中判定：冰动画第 3 帧开始到第 4 帧结束
            inline constexpr int REMOTE_HIT_START_FRAME = 3;
            inline constexpr int REMOTE_HIT_END_FRAME = 4;
            inline constexpr float REMOTE_HITBOX_WIDTH = 50.0f;
            inline constexpr float REMOTE_HITBOX_HEIGHT = 200.0f;
        }

        // --- 以后加 Boss 或者其他怪物 ---
        namespace Slime
        {
            // ...
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

        namespace Poisoned
        {
            inline constexpr float DURATION_SECONDS = 6.0f;
            inline constexpr float TICK_INTERVAL_SECONDS = 1.0f;
            inline constexpr float BASE_DAMAGE_SCALE = 0.07f;
            inline constexpr float PER_STACK_DAMAGE_SCALE = 0.05f;
        }

        namespace Excited
        {
            // 亢奋：纯属性 buff（用于击杀加速等机制）
            inline constexpr float DURATION_SECONDS = 2.0f;
            inline constexpr float MOVE_SPEED_BONUS = 60.0f;
        }
    }

    // =============================================================
    // 掉落物（血瓶/蓝瓶）
    // =============================================================
    namespace DropItem
    {
        // 掉落概率：怪物死亡时 30% 掉落
        inline constexpr float DROP_CHANCE = 0.30f;
        // 掉落类型：掉落时一半血瓶、一半蓝瓶
        inline constexpr float HP_DROP_RATIO = 0.50f;

        // 恢复比例：按最大值百分比恢复（避免数值随版本变动时失衡）
        inline constexpr float HP_RESTORE_RATIO = 0.25f;
        inline constexpr float MP_RESTORE_RATIO = 0.25f;

        // 资源路径（注意：当前素材文件名为 Red.png / Bule.png）
        inline constexpr const char* HP_SPRITE_PATH = "Sprites/Item/Red.png";
        inline constexpr const char* MP_SPRITE_PATH = "Sprites/Item/Bule.png";

        // 视觉尺寸：原 PNG 尺寸很大，统一缩放到固定高度
        inline constexpr float VISUAL_HEIGHT = 60.0f;
        // 拾取判定：固定 box 大小，避免受图片尺寸/缩放影响
        inline constexpr float PICKUP_BOX_SIZE = 70.0f;

        // 掉落位置微调（相对怪物锚点/脚底）
        inline constexpr float SPAWN_OFFSET_Y = 10.0f;
        inline constexpr float SPAWN_OFFSET_X_RANGE = 30.0f;
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
        // 击破系统（Boss 的“韧性/击破条”等）：普通攻击与技能对击破条的贡献
        inline constexpr int BREAK_DAMAGE_NORMAL = 1;
        inline constexpr int BREAK_DAMAGE_SKILL = 3;
    }

    // --- AI/NPC 配置（展示阶段：范围写死，便于快速迭代） ---
    namespace AI
    {
        namespace Blessing
        {
            // OpenAI 兼容接口默认模型名（可在 UI 中覆盖）
            inline constexpr const char *DEFAULT_MODEL = "gpt-4o-mini";

            // 赐福一次选择的属性条目数量（由 AI 从下列候选中挑选）
            inline constexpr int PICK_COUNT = 2;

            // 属性范围（由你写死即可，展示阶段不做外部配置）
            inline constexpr float STRENGTH_MIN = 2.0f;
            inline constexpr float STRENGTH_MAX = 10.0f;

            // 攻速：使用 AttributeType::ATTACKINTERVAL（越大越快/越小越快取决于你现有逻辑）
            // 若现有实现是“攻击间隔倍率”请把范围调成更合理的倍率值（例如 0.8~1.2）。
            inline constexpr float ATTACK_INTERVAL_MIN = 0.0f;
            inline constexpr float ATTACK_INTERVAL_MAX = 0.0f;

            inline constexpr float DEFENSE_MIN = 1.0f;
            inline constexpr float DEFENSE_MAX = 6.0f;

            // 暴击率：0.05 = +5%
            inline constexpr float CRIT_RATE_MIN = 0.02f;
            inline constexpr float CRIT_RATE_MAX = 0.10f;

            inline constexpr float MOVE_SPEED_MIN = 10.0f;
            inline constexpr float MOVE_SPEED_MAX = 60.0f;

            inline constexpr float MAX_HP_MIN = 30.0f;
            inline constexpr float MAX_HP_MAX = 150.0f;

            inline constexpr float MAX_MP_MIN = 10.0f;
            inline constexpr float MAX_MP_MAX = 80.0f;
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
        const cocos2d::PhysicsMaterial MONSTER(1.0f, 0.0f, 0.0f);
        const cocos2d::PhysicsMaterial GOBLIN(1.0f, 0.0f, 0.0f); // 火球无弹性
    }
}
