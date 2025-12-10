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

    AttributesSaveData() = default;
};

// 装备数据（用于序列化 Equipment/Weapon）
struct EquipmentSaveData
{
    int id = 0;
    std::string name;
    std::string description;
    int slot = 0; // EquipmentSlot (使用 int 便于 JSON 序列化)
    AttributesSaveData attributeBonus;
    std::string spritePath;

    // 武���特有属性
    bool isWeapon = false;
    int weaponType = 0;           // WeaponType
    float attackDamage = 0.0f;
    float attackRange = 0.0f;
    float attackSpeed = 1.0f;
    std::string attackAnimationPrefix;
    int attackFrameCount = 3;

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

    SkillSaveData() = default;
};

// 玩家数据
struct PlayerSaveData
{
    // 基础信息
    int role = 0;           // CharacterRole
    int level = 1;
    int experience = 0;
    int skillPoints = 0;

    // 当前状态
    float currentHP = 100.0f;
    float currentMP = 100.0f;

    // 属性
    AttributesSaveData baseAttributes;

    // 装备（槽位 -> 装备数据）
    std::map<int, EquipmentSaveData> equippedItems;

    // 技能
    std::vector<SkillSaveData> learnedSkills;
    std::vector<int> activeSlotSkillIds; // 主动技能槽位（最多 4 个）

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

    SaveSlotData() = default;
};

// 设置数据
struct SettingsSaveData
{
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
    bool musicEnabled = true;
    bool sfxEnabled = true;

    SettingsSaveData() = default;
};
