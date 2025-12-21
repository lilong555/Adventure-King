#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

//================== 存档数据结构 ==================

// 属性数据（用于序列化 Attributes）
struct AttributesSaveData
{
    std::map<int, float> values; // AttributeType -> value (使用 int 便于 JSON 序列化)

    // 默认构造：保持空属性集合
    AttributesSaveData() = default;
};

// 装备数据（用于序列化 Equipment/Weapon）
struct EquipmentSaveData
{
    int id = 0;
    std::string name;
    std::string description;
    int slot = 0; // EquipmentSlot (使用 int 便于 JSON 序列化)
    int level = 1; // 装备等级（独立于角色等级）
    AttributesSaveData attributeBonus;
    std::string spritePath;

    // 武器特有属性
    bool isWeapon = false;
    int weaponType = 0;           // WeaponType
    float attackDamage = 0.0f;
    float attackRange = 0.0f;
    float attackSpeed = 1.0f;
    std::string attackAnimationPrefix;
    int attackFrameCount = 3;

    // 默认构造：保持默认装备字段
    EquipmentSaveData() = default;
};

// 技能数据（用于序列化 Skill/ActiveSkill/PassiveSkill）
struct SkillSaveData
{
    int id = 0;
    std::string name;
    std::string description;
    bool isPassive = false;

    // 主动技能属性
    float cooldown = 0.0f;
    float manaCost = 0.0f;
    float currentCooldown = 0.0f;

    // 被动技能属性
    AttributesSaveData attributeBonus;

    // 默认构造：保持默认技能字段
    SkillSaveData() = default;
};

// 玩家数据
struct PlayerSaveData
{
    // 基础信息
    int role = 0;           // CharacterRole
    int level = 1;
    int experience = 0;
    // 技能点拆分：主动/被动各自独立（都通过升级获得）
    int activeSkillPoints = 0;
    int passiveSkillPoints = 0;
    int attributePoints = 0;

    // 当前状态
    float currentHP = 100.0f;
    float currentMP = 100.0f;

    // 属性
    AttributesSaveData baseAttributes;

    // 装备（槽位 -> 装备数据）
    std::map<int, EquipmentSaveData> equippedItems;

    // 背包（仅存装备/武器，图标可后续补充）
    std::vector<EquipmentSaveData> inventoryItems;

    // 技能
    std::vector<SkillSaveData> learnedSkills;
    std::vector<int> activeSlotSkillIds; // 主动技能槽位（最多 4 个）
    std::vector<int> passiveSlotSkillIds; // 已装备的被动技能 id 列表（无槽位限制；兼容旧存档可能包含 -1 占位）

    // 默认构造：使用初始玩家数据
    PlayerSaveData() = default;
};

// 游戏进度数据
struct GameProgressSaveData
{
    // 当前位置
    std::string currentSceneName;
    float playerPosX = 0.0f;
    float playerPosY = 0.0f;

    // 解锁关卡
    std::vector<std::string> unlockedLevels;

    // 游戏时长（秒）
    int64_t playTimeSeconds = 0;

    // 默认构造：保持默认进度
    GameProgressSaveData() = default;
};

// 完整存档槽位数据
struct SaveSlotData
{
    // 元数据
    int slotIndex = 0;
    int64_t saveTimestamp = 0; // Unix 时间戳（毫秒）
    std::string gameVersion = "1.0.0";

    // 玩家数据
    PlayerSaveData playerData;

    // 游戏进度
    GameProgressSaveData progressData;

    // 默认构造：保持默认存档槽位数据
    SaveSlotData() = default;
};

// 设置数据
struct SettingsSaveData
{
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
    bool musicEnabled = true;
    bool sfxEnabled = true;

    // 默认构造：使用默认设置
    SettingsSaveData() = default;
};
