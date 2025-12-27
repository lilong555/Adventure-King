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
    // 击破值：每次命中对 Boss 击破条的累计值
    // 说明：该字段属于“技能静态配置”，但为保证读档后行为一致，这里一并持久化；
    //       若旧存档缺失该字段，会以 -1 作为“未知”哨兵并在读档时按当前配置补齐。
    int breakDamage = -1;
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

    // 临时战斗状态（例如“孤注一掷”的高手增伤）
    float outgoingDamageMultiplier = 1.0f;
    float outgoingDamageMultiplierRemainingSeconds = 0.0f;

    // AI/NPC 赐福（覆盖式属性 Buff）：存储当前赐福的属性加成（空表示无赐福）
    AttributesSaveData aiBlessingBonus;

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

    //================== 世界状态（用于“存档/读档恢复刷怪与关卡状态”） ==================

    // 关卡是否已通关（用于恢复传送门可交互状态）
    // @note
    // - 旧版本在读档时会重新锁门，导致“已通关但无法离开”；
    // - 若旧存档缺失该字段，读档时会根据 enemySpawnPoints/arenas/aliveMonsters 自动推断一次。
    bool isLevelCleared = false;

    // 刷怪点状态（enemy_g）：用于避免读档后重复刷出已触发的刷怪点
    struct EnemySpawnPointState
    {
        std::string monsterType;
        float posX = 0.0f;
        float posY = 0.0f;
        int count = 1;
        bool hasSpawned = false;
    };
    std::vector<EnemySpawnPointState> enemySpawnPoints;

    // 竞技场（Arena）状态：用于恢复“是否已触发/进行到第几波/是否已完成”
    struct ArenaState
    {
        std::string arenaID;
        int currentWaveIndex = 0;
        bool isActivated = false;
        bool isFinished = false;
    };
    std::vector<ArenaState> arenas;

    // 已刷新且仍存活的怪物快照（用于读档后恢复“场上还活着的怪”）
    // - 普通刷怪（enemy_g）：arenaID 为空
    // - 竞技场怪物：arenaID 不为空（用于读档后恢复波次中的“剩余怪物”，避免重刷整波）
    struct MonsterState
    {
        std::string monsterType; // createMonsterByType 的输入（建议小写：goblin/goblu/obscur...）
        std::string arenaID; // 若为竞技场怪物，保存所属 arenaID（不为空）；普通刷怪则为空
        float posX = 0.0f;
        float posY = 0.0f;
        float currentHP = 0.0f;
        float currentMP = 0.0f;
        int breakMeter = 0; // Boss 击破条当前值（不支持则为 0）
    };
    std::vector<MonsterState> aliveMonsters;

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
