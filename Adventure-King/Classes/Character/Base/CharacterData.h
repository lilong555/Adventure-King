#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

//================== 基础枚举 ==================

// 职业类型
enum class CharacterRole : uint8_t
{
    WARRIOR,  // 战士
    MAGE,     // 法师
    ASSASSIN, // 刺客
    TANK      // 坦克
};

// 角色核心状态
enum class CharacterState : uint8_t
{
    IDLE,           // 待机
    WALKING,        // 行走
    RUNNING,        // 跑步
    JUMPING,        // 跳跃
    DOUBLE_JUMPING, // 二段跳
    ATTACKING,      // 攻击
    CLIMBING,       // 爬墙
    HURT,           // 受伤
    DEAD,           // 死亡
    STATE_PATROL    // 巡逻
};

// 基础属性类型
enum class AttributeType : uint8_t
{
    STRENGTH,      // 力量
	ATTACKINTERVAL,// 敏捷
    DEFENSE,       // 防御
    CRITICAL_RATE, // 暴击率
    MOVE_SPEED,    // 移动速度
    MAX_HP,        // 最大生命值
    MAX_MP,        // 最大能量值

    //敌人特有属性
    ATTACK_INTERVAL, // 攻击间隔
    ATTACK_RANGE,    // 发动攻击的距离
};

// 武器类型
enum class WeaponType : uint8_t
{
    SWORD, // 剑
    STAFF, // 法杖
    DAGGER // 匕首
};

// 装备槽位
enum class EquipmentSlot : uint8_t
{
    WEAPON, // 武器
    HELMET, // 头盔
    ARMOR,  // 盔甲
    BOOTS   // 靴子
};

// 状态效果类型
enum class StatusEffectType : uint8_t
{
    POISONED, // 中毒
    EXCITED,  // 亢奋
    STUNNED,  // 眩晕
    FULL_HP_CRIT // 满血暴击
};

//================== 属性结构 ==================
// 实现属性的加减操作
struct Attributes
{
    std::map<AttributeType, float> values;

    float get(AttributeType type, float defaultValue = 0.0f) const
    {
        auto it = values.find(type);
        return (it != values.end()) ? it->second : defaultValue;
    }

    void set(AttributeType type, float value)
    {
        values[type] = value;
    }

    void add(AttributeType type, float delta)
    {
        values[type] = get(type) + delta;
    }

    void clear()
    {
        values.clear();
    }

    Attributes &operator+=(const Attributes &other)
    {
        for (const auto &kv : other.values)
        {
            add(kv.first, kv.second);
        }
        return *this;
    }

    Attributes operator+(const Attributes &other) const
    {
        Attributes result = *this;
        result += other;
        return result;
    }
};

//================== 技能结构 ==================

struct Skill
{
    int id = 0;
    std::string name;
    std::string description;
    bool isPassive = false; // 是否为被动技能

    virtual ~Skill() = default;
};

// 主动技能
struct ActiveSkill : public Skill
{
    float cooldown = 0.0f;        // 冷却时间（秒）
    float manaCost = 0.0f;        // 消耗 MP
    float currentCooldown = 0.0f; // 当前冷却剩余时间

    ActiveSkill()
    {
        isPassive = false;
    }
};

// 被动技能
struct PassiveSkill : public Skill
{
    Attributes attributeBonus; // 提供的属性加成

    PassiveSkill()
    {
        isPassive = true;
    }
};

//================== 装备结构 ==================

// 装备基类
struct Equipment
{
    int id = 0;
    std::string name;                           // 装备名称
    std::string description;                    // 装备描述
    EquipmentSlot slot = EquipmentSlot::WEAPON; // 装备槽位
    Attributes attributeBonus;                  // 装备提供的属性加成
    std::string spritePath;                     // 装备对应的角色贴图路径（可选）

    virtual ~Equipment() = default;
};

// 武器装备
struct Weapon : public Equipment
{
    WeaponType type = WeaponType::SWORD; // 武器类型
    float attackDamage = 0.0f;           // 攻击力
    float attackRange = 0.0f;            // 攻击范围
    float attackSpeed = 1.0f;            // 攻击速度（攻击间隔倍率）
    std::string attackAnimationPrefix;   // 攻击动画前缀（如 "spr_klee_attack"）
    int attackFrameCount = 3;            // 攻击动画帧数

    Weapon()
    {
        slot = EquipmentSlot::WEAPON;
    }
};

//================== 状态效果实例 ==================
struct StatusEffectInstance
{
    StatusEffectType type;
    float duration = 0.0f;     // 持续时间（秒）
    float elapsed = 0.0f;      // 已经过的时间（秒）
    Attributes attributeBonus; // 状态对属性的影响（例如 EXCITED 给 MOVE_SPEED +50）

    // 是否过期
    bool isExpired() const { return elapsed >= duration; }
};
