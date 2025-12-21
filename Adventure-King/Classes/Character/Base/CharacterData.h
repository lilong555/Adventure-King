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
    ATTACKINTERVAL, // 敏捷/攻速
    DEFENSE,       // 防御
    CRITICAL_RATE, // 暴击率
    MOVE_SPEED,    // 移动速度
    MAX_HP,        // 最大生命值
    MAX_MP,        // 最大能量值

    // 敌人特有属性
    ATTACK_INTERVAL, // 攻击间隔（兼容旧配置）
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
    BURNING,  // 燃烧
    EXCITED,  // 亢奋
    STUNNED,  // 眩晕
    FULL_HP_CRIT // 满血暴击
};

//================== 属性结构 ==================
// 实现属性的加减操作
struct Attributes
{
    std::map<AttributeType, float> values;

    // 获取指定属性值（不存在则返回默认值）
    float get(AttributeType type, float defaultValue = 0.0f) const
    {
        auto it = values.find(type);
        return (it != values.end()) ? it->second : defaultValue;
    }

    // 设置指定属性值
    void set(AttributeType type, float value)
    {
        values[type] = value;
    }

    // 增加指定属性值
    void add(AttributeType type, float delta)
    {
        values[type] = get(type) + delta;
    }

    // 清空所有属性
    void clear()
    {
        values.clear();
    }

    // 叠加另一组属性
    Attributes &operator+=(const Attributes &other)
    {
        for (const auto &kv : other.values)
        {
            add(kv.first, kv.second);
        }
        return *this;
    }

    // 生成叠加后的新属性对象
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

    // 虚析构，确保派生类正确释放
    virtual ~Skill() = default;
};

// 主动技能
struct ActiveSkill : public Skill
{
    float cooldown = 0.0f;        // 冷却时间（秒）
    float manaCost = 0.0f;        // 消耗 MP
    float currentCooldown = 0.0f; // 当前冷却剩余时间

    // 初始化为主动技能
    ActiveSkill()
    {
        isPassive = false;
    }
};

// 被动技能
struct PassiveSkill : public Skill
{
    Attributes attributeBonus; // 提供的属性加成

    // 初始化为被动技能
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
    int level = 1;                              // 装备等级（独立于角色等级）
    Attributes attributeBonus;                  // 装备提供的属性加成
    std::string spritePath;                     // 装备对应的角色贴图路径（可选）

    // 虚析构，确保派生类正确释放
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

    // 初始化为武器装备
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
    bool permanent = false;    // 是否为“永久效果”（true 时不会过期，需由代码显式移除）
    Attributes attributeBonus; // 状态对属性的影响（例如 EXCITED 给 MOVE_SPEED +50）

    // -------------------------
    // DOT/叠层效果扩展（可选）
    // -------------------------
    // 叠层：同 type 的效果再次施加时，若 stackable=true，则不会新建实例而是合并到已有实例。
    int stacks = 1;              // 当前叠层数（>=1）
    int maxStacks = 0;           // 最大层数（0 = 不限制）
    bool stackable = false;      // true 时同 type 合并并叠加 stacks
    bool refreshOnAdd = true;    // 再次施加时是否刷新持续时间/计时

    // DOT（Damage Over Time）：tickInterval>0 时生效；伤害结算逻辑在 AttributeComponent 中。
    float tickInterval = 0.0f;       // tick 间隔（秒，<=0 表示不 tick）
    float tickAccumulator = 0.0f;    // tick 累计器（内部使用）
    float sourceAttackPower = 0.0f;  // 伤害来源“攻击力”（施加时写入，用于快照）
    float baseDamageScale = 0.0f;    // 基础比例（例如 0.1）
    float perStackDamageScale = 0.0f; // 每层额外比例（例如 0.1）；总比例 = base + perStack * stacks

    // 是否过期
    bool isExpired() const { return !permanent && elapsed >= duration; }
};
