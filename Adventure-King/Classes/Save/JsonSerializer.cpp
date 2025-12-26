#include "JsonSerializer.h"
#include "cocos2d.h"
#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

USING_NS_CC;

// 辅助函数：序列化 AttributesSaveData
// 序列化属性集合
static void serializeAttributes(rapidjson::Value &jsonObj, const AttributesSaveData &attrs,
                                rapidjson::Document::AllocatorType &allocator)
{
    for (const auto &kv : attrs.values)
    {
        std::string key = std::to_string(kv.first);
        jsonObj.AddMember(rapidjson::Value(key.c_str(), allocator).Move(), kv.second, allocator);
    }
}

// 辅助函数：反序列化 AttributesSaveData
// 反序列化属性集合
static void deserializeAttributes(const rapidjson::Value &jsonObj, AttributesSaveData &attrs)
{
    attrs.values.clear();
    if (!jsonObj.IsObject())
        return;

    for (auto it = jsonObj.MemberBegin(); it != jsonObj.MemberEnd(); ++it)
    {
        if (!it->value.IsNumber())
            continue;

        int key = 0;
        try
        {
            key = std::stoi(it->name.GetString());
        }
        catch (...)
        {
            continue;
        }
        float value = static_cast<float>(it->value.GetDouble());
        attrs.values[key] = value;
    }
}

// 辅助函数：序列化 EquipmentSaveData
// 序列化装备数据
static rapidjson::Value serializeEquipment(const EquipmentSaveData &equip,
                                           rapidjson::Document::AllocatorType &allocator)
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("id", equip.id, allocator);
    obj.AddMember("name", rapidjson::Value(equip.name.c_str(), allocator).Move(), allocator);
    obj.AddMember("description", rapidjson::Value(equip.description.c_str(), allocator).Move(), allocator);
    obj.AddMember("slot", equip.slot, allocator);
    obj.AddMember("level", equip.level, allocator);
    obj.AddMember("spritePath", rapidjson::Value(equip.spritePath.c_str(), allocator).Move(), allocator);
    obj.AddMember("isWeapon", equip.isWeapon, allocator);

    rapidjson::Value attrObj(rapidjson::kObjectType);
    serializeAttributes(attrObj, equip.attributeBonus, allocator);
    obj.AddMember("attributeBonus", attrObj, allocator);

    if (equip.isWeapon)
    {
        obj.AddMember("weaponType", equip.weaponType, allocator);
        obj.AddMember("attackDamage", equip.attackDamage, allocator);
        obj.AddMember("attackRange", equip.attackRange, allocator);
        obj.AddMember("attackSpeed", equip.attackSpeed, allocator);
        obj.AddMember("attackAnimationPrefix", rapidjson::Value(equip.attackAnimationPrefix.c_str(), allocator).Move(), allocator);
        obj.AddMember("attackFrameCount", equip.attackFrameCount, allocator);
    }

    return obj;
}

// 辅助函数：反序列化 EquipmentSaveData
// 反序列化装备数据
static void deserializeEquipment(const rapidjson::Value &jsonObj, EquipmentSaveData &equip)
{
    if (!jsonObj.IsObject())
        return;

    auto getInt = [&](const char *key, int def) -> int
    {
        return (jsonObj.HasMember(key) && jsonObj[key].IsInt()) ? jsonObj[key].GetInt() : def;
    };
    auto getFloat = [&](const char *key, float def) -> float
    {
        return (jsonObj.HasMember(key) && jsonObj[key].IsNumber()) ? static_cast<float>(jsonObj[key].GetDouble()) : def;
    };
    auto getString = [&](const char *key, const std::string &def) -> std::string
    {
        return (jsonObj.HasMember(key) && jsonObj[key].IsString()) ? jsonObj[key].GetString() : def;
    };
    auto getBool = [&](const char *key, bool def) -> bool
    {
        return (jsonObj.HasMember(key) && jsonObj[key].IsBool()) ? jsonObj[key].GetBool() : def;
    };

    equip.id = getInt("id", 0);
    equip.name = getString("name", "");
    equip.description = getString("description", "");
    equip.slot = getInt("slot", 0);
    equip.level = getInt("level", 1);
    equip.spritePath = getString("spritePath", "");
    equip.isWeapon = getBool("isWeapon", false);

    if (jsonObj.HasMember("attributeBonus"))
    {
        deserializeAttributes(jsonObj["attributeBonus"], equip.attributeBonus);
    }

    if (equip.isWeapon)
    {
        equip.weaponType = getInt("weaponType", 0);
        equip.attackDamage = getFloat("attackDamage", 0.0f);
        equip.attackRange = getFloat("attackRange", 0.0f);
        equip.attackSpeed = getFloat("attackSpeed", 1.0f);
        equip.attackAnimationPrefix = getString("attackAnimationPrefix", "");
        equip.attackFrameCount = getInt("attackFrameCount", 3);
    }
}

// 辅助函数：序列化 SkillSaveData
// 序列化技能数据
static rapidjson::Value serializeSkill(const SkillSaveData &skill,
                                       rapidjson::Document::AllocatorType &allocator)
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("id", skill.id, allocator);
    obj.AddMember("name", rapidjson::Value(skill.name.c_str(), allocator).Move(), allocator);
    obj.AddMember("description", rapidjson::Value(skill.description.c_str(), allocator).Move(), allocator);
    obj.AddMember("isPassive", skill.isPassive, allocator);
    obj.AddMember("cooldown", skill.cooldown, allocator);
    obj.AddMember("manaCost", skill.manaCost, allocator);
    obj.AddMember("currentCooldown", skill.currentCooldown, allocator);

    rapidjson::Value attrObj(rapidjson::kObjectType);
    serializeAttributes(attrObj, skill.attributeBonus, allocator);
    obj.AddMember("attributeBonus", attrObj, allocator);

    return obj;
}

// 辅助函数：反序列化 SkillSaveData
// 反序列化技能数据
static void deserializeSkill(const rapidjson::Value &jsonObj, SkillSaveData &skill)
{
    if (!jsonObj.IsObject())
        return;

    auto getInt = [&](const char *key, int def) -> int
    {
        return (jsonObj.HasMember(key) && jsonObj[key].IsInt()) ? jsonObj[key].GetInt() : def;
    };
    auto getFloat = [&](const char *key, float def) -> float
    {
        return (jsonObj.HasMember(key) && jsonObj[key].IsNumber()) ? static_cast<float>(jsonObj[key].GetDouble()) : def;
    };
    auto getString = [&](const char *key, const std::string &def) -> std::string
    {
        return (jsonObj.HasMember(key) && jsonObj[key].IsString()) ? jsonObj[key].GetString() : def;
    };
    auto getBool = [&](const char *key, bool def) -> bool
    {
        return (jsonObj.HasMember(key) && jsonObj[key].IsBool()) ? jsonObj[key].GetBool() : def;
    };

    skill.id = getInt("id", 0);
    skill.name = getString("name", "");
    skill.description = getString("description", "");
    skill.isPassive = getBool("isPassive", false);
    skill.cooldown = getFloat("cooldown", 0.0f);
    skill.manaCost = getFloat("manaCost", 0.0f);
    skill.currentCooldown = getFloat("currentCooldown", 0.0f);

    if (jsonObj.HasMember("attributeBonus"))
    {
        deserializeAttributes(jsonObj["attributeBonus"], skill.attributeBonus);
    }
}

// 序列化 SaveSlotData
std::string JsonSerializer::serialize(const SaveSlotData &data)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto &allocator = doc.GetAllocator();

    // 元数据
    rapidjson::Value metaObj(rapidjson::kObjectType);
    metaObj.AddMember("slotIndex", data.slotIndex, allocator);
    metaObj.AddMember("saveTimestamp", data.saveTimestamp, allocator);
    metaObj.AddMember("gameVersion", rapidjson::Value(data.gameVersion.c_str(), allocator).Move(), allocator);
    doc.AddMember("meta", metaObj, allocator);

    // 玩家数据
    rapidjson::Value playerObj(rapidjson::kObjectType);
    playerObj.AddMember("role", data.playerData.role, allocator);
    playerObj.AddMember("level", data.playerData.level, allocator);
    playerObj.AddMember("experience", data.playerData.experience, allocator);
    playerObj.AddMember("activeSkillPoints", data.playerData.activeSkillPoints, allocator);
    playerObj.AddMember("passiveSkillPoints", data.playerData.passiveSkillPoints, allocator);
    playerObj.AddMember("attributePoints", data.playerData.attributePoints, allocator);
    playerObj.AddMember("currentHP", data.playerData.currentHP, allocator);
    playerObj.AddMember("currentMP", data.playerData.currentMP, allocator);
    playerObj.AddMember("outgoingDamageMultiplier", data.playerData.outgoingDamageMultiplier, allocator);
    playerObj.AddMember("outgoingDamageMultiplierRemainingSeconds", data.playerData.outgoingDamageMultiplierRemainingSeconds, allocator);

    // 基础属性
    rapidjson::Value baseAttrObj(rapidjson::kObjectType);
    serializeAttributes(baseAttrObj, data.playerData.baseAttributes, allocator);
    playerObj.AddMember("baseAttributes", baseAttrObj, allocator);

    // 装备
    rapidjson::Value equippedItemsObj(rapidjson::kObjectType);
    for (const auto &kv : data.playerData.equippedItems)
    {
        std::string key = std::to_string(kv.first);
        equippedItemsObj.AddMember(rapidjson::Value(key.c_str(), allocator).Move(),
                                    serializeEquipment(kv.second, allocator), allocator);
    }
    playerObj.AddMember("equippedItems", equippedItemsObj, allocator);

    // 背包（装备/武器）
    rapidjson::Value inventoryArr(rapidjson::kArrayType);
    for (const auto &equip : data.playerData.inventoryItems)
    {
        inventoryArr.PushBack(serializeEquipment(equip, allocator), allocator);
    }
    playerObj.AddMember("inventoryItems", inventoryArr, allocator);

    // 技能
    rapidjson::Value learnedSkillsArr(rapidjson::kArrayType);
    for (const auto &skill : data.playerData.learnedSkills)
    {
        learnedSkillsArr.PushBack(serializeSkill(skill, allocator), allocator);
    }
    playerObj.AddMember("learnedSkills", learnedSkillsArr, allocator);

    // 主动技能槽位
    rapidjson::Value activeSlotSkillIdsArr(rapidjson::kArrayType);
    for (int skillId : data.playerData.activeSlotSkillIds)
    {
        activeSlotSkillIdsArr.PushBack(skillId, allocator);
    }
    playerObj.AddMember("activeSlotSkillIds", activeSlotSkillIdsArr, allocator);

    // 被动技能槽位
    rapidjson::Value passiveSlotSkillIdsArr(rapidjson::kArrayType);
    for (int skillId : data.playerData.passiveSlotSkillIds)
    {
        passiveSlotSkillIdsArr.PushBack(skillId, allocator);
    }
    playerObj.AddMember("passiveSlotSkillIds", passiveSlotSkillIdsArr, allocator);

    doc.AddMember("player", playerObj, allocator);

    // 游戏进度
    rapidjson::Value progressObj(rapidjson::kObjectType);
    progressObj.AddMember("currentSceneName", rapidjson::Value(data.progressData.currentSceneName.c_str(), allocator).Move(), allocator);
    progressObj.AddMember("playerPosX", data.progressData.playerPosX, allocator);
    progressObj.AddMember("playerPosY", data.progressData.playerPosY, allocator);
    progressObj.AddMember("playTimeSeconds", data.progressData.playTimeSeconds, allocator);

    rapidjson::Value unlockedLevelsArr(rapidjson::kArrayType);
    for (const auto &level : data.progressData.unlockedLevels)
    {
        unlockedLevelsArr.PushBack(rapidjson::Value(level.c_str(), allocator).Move(), allocator);
    }
    progressObj.AddMember("unlockedLevels", unlockedLevelsArr, allocator);

    // 刷怪点状态（enemy_g）
    rapidjson::Value enemySpawnPointsArr(rapidjson::kArrayType);
    for (const auto &sp : data.progressData.enemySpawnPoints)
    {
        rapidjson::Value spObj(rapidjson::kObjectType);
        spObj.AddMember("monsterType", rapidjson::Value(sp.monsterType.c_str(), allocator).Move(), allocator);
        spObj.AddMember("posX", sp.posX, allocator);
        spObj.AddMember("posY", sp.posY, allocator);
        spObj.AddMember("count", sp.count, allocator);
        spObj.AddMember("hasSpawned", sp.hasSpawned, allocator);
        enemySpawnPointsArr.PushBack(spObj, allocator);
    }
    progressObj.AddMember("enemySpawnPoints", enemySpawnPointsArr, allocator);

    // 竞技场状态（Arena）
    rapidjson::Value arenasArr(rapidjson::kArrayType);
    for (const auto &arena : data.progressData.arenas)
    {
        rapidjson::Value arenaObj(rapidjson::kObjectType);
        arenaObj.AddMember("arenaID", rapidjson::Value(arena.arenaID.c_str(), allocator).Move(), allocator);
        arenaObj.AddMember("currentWaveIndex", arena.currentWaveIndex, allocator);
        arenaObj.AddMember("isActivated", arena.isActivated, allocator);
        arenaObj.AddMember("isFinished", arena.isFinished, allocator);
        arenasArr.PushBack(arenaObj, allocator);
    }
    progressObj.AddMember("arenas", arenasArr, allocator);

    // 场上存活怪物快照（非竞技场）
    rapidjson::Value aliveMonstersArr(rapidjson::kArrayType);
    for (const auto &m : data.progressData.aliveMonsters)
    {
        rapidjson::Value mObj(rapidjson::kObjectType);
        mObj.AddMember("monsterType", rapidjson::Value(m.monsterType.c_str(), allocator).Move(), allocator);
        mObj.AddMember("posX", m.posX, allocator);
        mObj.AddMember("posY", m.posY, allocator);
        mObj.AddMember("currentHP", m.currentHP, allocator);
        mObj.AddMember("currentMP", m.currentMP, allocator);
        mObj.AddMember("breakMeter", m.breakMeter, allocator);
        aliveMonstersArr.PushBack(mObj, allocator);
    }
    progressObj.AddMember("aliveMonsters", aliveMonstersArr, allocator);

    doc.AddMember("progress", progressObj, allocator);

    // 转换为字符串
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

// 序列化 SettingsSaveData
std::string JsonSerializer::serialize(const SettingsSaveData &data)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto &allocator = doc.GetAllocator();

    doc.AddMember("musicVolume", data.musicVolume, allocator);
    doc.AddMember("sfxVolume", data.sfxVolume, allocator);
    doc.AddMember("musicEnabled", data.musicEnabled, allocator);
    doc.AddMember("sfxEnabled", data.sfxEnabled, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

// 反序列化 SaveSlotData
bool JsonSerializer::deserialize(const std::string &json, SaveSlotData &outData)
{
    rapidjson::Document doc;
    doc.Parse(json.c_str());

    if (doc.HasParseError() || !doc.IsObject())
    {
        CCLOG("JsonSerializer::deserialize(SaveSlotData) - JSON 解析失败");
        return false;
    }

    try
    {
        auto getInt = [&](const rapidjson::Value &obj, const char *key, int def) -> int
        {
            return (obj.HasMember(key) && obj[key].IsInt()) ? obj[key].GetInt() : def;
        };
        auto getInt64 = [&](const rapidjson::Value &obj, const char *key, int64_t def) -> int64_t
        {
            return (obj.HasMember(key) && obj[key].IsInt64()) ? obj[key].GetInt64() : def;
        };
        auto getFloat = [&](const rapidjson::Value &obj, const char *key, float def) -> float
        {
            return (obj.HasMember(key) && obj[key].IsNumber()) ? static_cast<float>(obj[key].GetDouble()) : def;
        };
        auto getString = [&](const rapidjson::Value &obj, const char *key, const std::string &def) -> std::string
        {
            return (obj.HasMember(key) && obj[key].IsString()) ? obj[key].GetString() : def;
        };

        // 元数据
        if (doc.HasMember("meta") && doc["meta"].IsObject())
        {
            const auto &meta = doc["meta"];
            outData.slotIndex = getInt(meta, "slotIndex", 0);
            outData.saveTimestamp = getInt64(meta, "saveTimestamp", 0);
            outData.gameVersion = getString(meta, "gameVersion", outData.gameVersion);
        }

        // 玩家数据
        if (doc.HasMember("player") && doc["player"].IsObject())
        {
            const auto &player = doc["player"];
            outData.playerData.role = getInt(player, "role", outData.playerData.role);
            outData.playerData.level = getInt(player, "level", outData.playerData.level);
            outData.playerData.experience = getInt(player, "experience", outData.playerData.experience);
            outData.playerData.attributePoints = getInt(player, "attributePoints", outData.playerData.attributePoints);
            outData.playerData.currentHP = getFloat(player, "currentHP", outData.playerData.currentHP);
            outData.playerData.currentMP = getFloat(player, "currentMP", outData.playerData.currentMP);
            outData.playerData.outgoingDamageMultiplier = getFloat(player, "outgoingDamageMultiplier", outData.playerData.outgoingDamageMultiplier);
            outData.playerData.outgoingDamageMultiplierRemainingSeconds = getFloat(player, "outgoingDamageMultiplierRemainingSeconds", outData.playerData.outgoingDamageMultiplierRemainingSeconds);

            // 技能点：优先读取新字段；兼容旧存档 skillPoints（按 1:1 平分到主动/被动）
            const bool hasActivePoints = player.HasMember("activeSkillPoints") && player["activeSkillPoints"].IsInt();
            const bool hasPassivePoints = player.HasMember("passiveSkillPoints") && player["passiveSkillPoints"].IsInt();
            if (hasActivePoints)
            {
                outData.playerData.activeSkillPoints = player["activeSkillPoints"].GetInt();
            }
            if (hasPassivePoints)
            {
                outData.playerData.passiveSkillPoints = player["passiveSkillPoints"].GetInt();
            }
            if (!hasActivePoints && !hasPassivePoints)
            {
                const int legacySkillPoints = getInt(player, "skillPoints", 0);
                outData.playerData.activeSkillPoints = (legacySkillPoints + 1) / 2;
                outData.playerData.passiveSkillPoints = legacySkillPoints / 2;
            }

            // 基础属性
            if (player.HasMember("baseAttributes"))
            {
                deserializeAttributes(player["baseAttributes"], outData.playerData.baseAttributes);
            }

            // 装备
            if (player.HasMember("equippedItems") && player["equippedItems"].IsObject())
            {
                const auto &equippedItems = player["equippedItems"];
                for (auto it = equippedItems.MemberBegin(); it != equippedItems.MemberEnd(); ++it)
                {
                    int slot = 0;
                    try
                    {
                        slot = std::stoi(it->name.GetString());
                    }
                    catch (...)
                    {
                        continue;
                    }
                    EquipmentSaveData equip;
                    if (it->value.IsObject())
                    {
                        deserializeEquipment(it->value, equip);
                        outData.playerData.equippedItems[slot] = equip;
                    }
                }
            }

            // 背包（装备/武器）
            if (player.HasMember("inventoryItems") && player["inventoryItems"].IsArray())
            {
                const auto &inventoryItems = player["inventoryItems"];
                for (rapidjson::SizeType i = 0; i < inventoryItems.Size(); ++i)
                {
                    if (!inventoryItems[i].IsObject())
                        continue;
                    EquipmentSaveData equip;
                    deserializeEquipment(inventoryItems[i], equip);
                    outData.playerData.inventoryItems.push_back(equip);
                }
            }

            // 技能
            if (player.HasMember("learnedSkills") && player["learnedSkills"].IsArray())
            {
                const auto &learnedSkills = player["learnedSkills"];
                for (rapidjson::SizeType i = 0; i < learnedSkills.Size(); ++i)
                {
                    if (!learnedSkills[i].IsObject())
                        continue;
                    SkillSaveData skill;
                    deserializeSkill(learnedSkills[i], skill);
                    outData.playerData.learnedSkills.push_back(skill);
                }
            }

            // 主动技能槽位
            if (player.HasMember("activeSlotSkillIds") && player["activeSlotSkillIds"].IsArray())
            {
                const auto &activeSlotSkillIds = player["activeSlotSkillIds"];
                for (rapidjson::SizeType i = 0; i < activeSlotSkillIds.Size(); ++i)
                {
                    if (activeSlotSkillIds[i].IsInt())
                    {
                        outData.playerData.activeSlotSkillIds.push_back(activeSlotSkillIds[i].GetInt());
                    }
                }
            }

            // 被动技能槽位
            if (player.HasMember("passiveSlotSkillIds") && player["passiveSlotSkillIds"].IsArray())
            {
                const auto &passiveSlotSkillIds = player["passiveSlotSkillIds"];
                for (rapidjson::SizeType i = 0; i < passiveSlotSkillIds.Size(); ++i)
                {
                    if (passiveSlotSkillIds[i].IsInt())
                    {
                        outData.playerData.passiveSlotSkillIds.push_back(passiveSlotSkillIds[i].GetInt());
                    }
                }
            }
        }

        // 游戏进度
        if (doc.HasMember("progress") && doc["progress"].IsObject())
        {
            const auto &progress = doc["progress"];
            outData.progressData.currentSceneName = getString(progress, "currentSceneName", outData.progressData.currentSceneName);
            outData.progressData.playerPosX = getFloat(progress, "playerPosX", outData.progressData.playerPosX);
            outData.progressData.playerPosY = getFloat(progress, "playerPosY", outData.progressData.playerPosY);
            outData.progressData.playTimeSeconds = getInt64(progress, "playTimeSeconds", outData.progressData.playTimeSeconds);

            if (progress.HasMember("unlockedLevels") && progress["unlockedLevels"].IsArray())
            {
                const auto &unlockedLevels = progress["unlockedLevels"];
                for (rapidjson::SizeType i = 0; i < unlockedLevels.Size(); ++i)
                {
                    if (unlockedLevels[i].IsString())
                    {
                        outData.progressData.unlockedLevels.push_back(unlockedLevels[i].GetString());
                    }
                }
            }

            // 刷怪点状态（enemy_g）
            if (progress.HasMember("enemySpawnPoints") && progress["enemySpawnPoints"].IsArray())
            {
                const auto &arr = progress["enemySpawnPoints"];
                for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
                {
                    if (!arr[i].IsObject())
                    {
                        continue;
                    }
                    const auto &obj = arr[i];
                    GameProgressSaveData::EnemySpawnPointState sp;
                    sp.monsterType = getString(obj, "monsterType", sp.monsterType);
                    sp.posX = getFloat(obj, "posX", sp.posX);
                    sp.posY = getFloat(obj, "posY", sp.posY);
                    sp.count = getInt(obj, "count", sp.count);
                    sp.hasSpawned = (obj.HasMember("hasSpawned") && obj["hasSpawned"].IsBool()) ? obj["hasSpawned"].GetBool() : sp.hasSpawned;
                    outData.progressData.enemySpawnPoints.push_back(sp);
                }
            }

            // 竞技场状态（Arena）
            if (progress.HasMember("arenas") && progress["arenas"].IsArray())
            {
                const auto &arr = progress["arenas"];
                for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
                {
                    if (!arr[i].IsObject())
                    {
                        continue;
                    }
                    const auto &obj = arr[i];
                    GameProgressSaveData::ArenaState arena;
                    arena.arenaID = getString(obj, "arenaID", arena.arenaID);
                    arena.currentWaveIndex = getInt(obj, "currentWaveIndex", arena.currentWaveIndex);
                    arena.isActivated = (obj.HasMember("isActivated") && obj["isActivated"].IsBool()) ? obj["isActivated"].GetBool() : arena.isActivated;
                    arena.isFinished = (obj.HasMember("isFinished") && obj["isFinished"].IsBool()) ? obj["isFinished"].GetBool() : arena.isFinished;
                    if (!arena.arenaID.empty())
                    {
                        outData.progressData.arenas.push_back(arena);
                    }
                }
            }

            // 场上存活怪物快照（非竞技场）
            if (progress.HasMember("aliveMonsters") && progress["aliveMonsters"].IsArray())
            {
                const auto &arr = progress["aliveMonsters"];
                for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
                {
                    if (!arr[i].IsObject())
                    {
                        continue;
                    }
                    const auto &obj = arr[i];
                    GameProgressSaveData::MonsterState m;
                    m.monsterType = getString(obj, "monsterType", m.monsterType);
                    m.posX = getFloat(obj, "posX", m.posX);
                    m.posY = getFloat(obj, "posY", m.posY);
                    m.currentHP = getFloat(obj, "currentHP", m.currentHP);
                    m.currentMP = getFloat(obj, "currentMP", m.currentMP);
                    m.breakMeter = getInt(obj, "breakMeter", m.breakMeter);
                    if (!m.monsterType.empty())
                    {
                        outData.progressData.aliveMonsters.push_back(m);
                    }
                }
            }
        }

        return true;
    }
    catch (const std::exception &e)
    {
        CCLOG("JsonSerializer::deserialize(SaveSlotData) - 异常: %s", e.what());
        return false;
    }
}

// 反序列化 SettingsSaveData
bool JsonSerializer::deserialize(const std::string &json, SettingsSaveData &outData)
{
    rapidjson::Document doc;
    doc.Parse(json.c_str());

    if (doc.HasParseError() || !doc.IsObject())
    {
        CCLOG("JsonSerializer::deserialize(SettingsSaveData) - JSON 解析失败");
        return false;
    }

    try
    {
        if (doc.HasMember("musicVolume"))
            outData.musicVolume = doc["musicVolume"].GetFloat();
        if (doc.HasMember("sfxVolume"))
            outData.sfxVolume = doc["sfxVolume"].GetFloat();
        if (doc.HasMember("musicEnabled"))
            outData.musicEnabled = doc["musicEnabled"].GetBool();
        if (doc.HasMember("sfxEnabled"))
            outData.sfxEnabled = doc["sfxEnabled"].GetBool();

        return true;
    }
    catch (const std::exception &e)
    {
        CCLOG("JsonSerializer::deserialize(SettingsSaveData) - 异常: %s", e.what());
        return false;
    }
}
