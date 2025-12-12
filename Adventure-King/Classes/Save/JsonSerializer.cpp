#include "JsonSerializer.h"
#include "cocos2d.h"
#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

USING_NS_CC;

// 辅助函数：序列化 AttributesSaveData
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
static void deserializeAttributes(const rapidjson::Value &jsonObj, AttributesSaveData &attrs)
{
    attrs.values.clear();
    for (auto it = jsonObj.MemberBegin(); it != jsonObj.MemberEnd(); ++it)
    {
        int key = std::stoi(it->name.GetString());
        float value = it->value.GetFloat();
        attrs.values[key] = value;
    }
}

// 辅助函数：序列化 EquipmentSaveData
static rapidjson::Value serializeEquipment(const EquipmentSaveData &equip,
                                            rapidjson::Document::AllocatorType &allocator)
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("id", equip.id, allocator);
    obj.AddMember("name", rapidjson::Value(equip.name.c_str(), allocator).Move(), allocator);
    obj.AddMember("description", rapidjson::Value(equip.description.c_str(), allocator).Move(), allocator);
    obj.AddMember("slot", equip.slot, allocator);
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
static void deserializeEquipment(const rapidjson::Value &jsonObj, EquipmentSaveData &equip)
{
    equip.id = jsonObj["id"].GetInt();
    equip.name = jsonObj["name"].GetString();
    equip.description = jsonObj["description"].GetString();
    equip.slot = jsonObj["slot"].GetInt();
    equip.spritePath = jsonObj["spritePath"].GetString();
    equip.isWeapon = jsonObj["isWeapon"].GetBool();

    if (jsonObj.HasMember("attributeBonus"))
    {
        deserializeAttributes(jsonObj["attributeBonus"], equip.attributeBonus);
    }

    if (equip.isWeapon)
    {
        equip.weaponType = jsonObj["weaponType"].GetInt();
        equip.attackDamage = jsonObj["attackDamage"].GetFloat();
        equip.attackRange = jsonObj["attackRange"].GetFloat();
        equip.attackSpeed = jsonObj["attackSpeed"].GetFloat();
        equip.attackAnimationPrefix = jsonObj["attackAnimationPrefix"].GetString();
        equip.attackFrameCount = jsonObj["attackFrameCount"].GetInt();
    }
}

// 辅助函数：序列化 SkillSaveData
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
static void deserializeSkill(const rapidjson::Value &jsonObj, SkillSaveData &skill)
{
    skill.id = jsonObj["id"].GetInt();
    skill.name = jsonObj["name"].GetString();
    skill.description = jsonObj["description"].GetString();
    skill.isPassive = jsonObj["isPassive"].GetBool();
    skill.cooldown = jsonObj["cooldown"].GetFloat();
    skill.manaCost = jsonObj["manaCost"].GetFloat();
    skill.currentCooldown = jsonObj["currentCooldown"].GetFloat();

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
    playerObj.AddMember("skillPoints", data.playerData.skillPoints, allocator);
    playerObj.AddMember("currentHP", data.playerData.currentHP, allocator);
    playerObj.AddMember("currentMP", data.playerData.currentMP, allocator);

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
        // 元数据
        if (doc.HasMember("meta"))
        {
            const auto &meta = doc["meta"];
            outData.slotIndex = meta["slotIndex"].GetInt();
            outData.saveTimestamp = meta["saveTimestamp"].GetInt64();
            outData.gameVersion = meta["gameVersion"].GetString();
        }

        // 玩家数据
        if (doc.HasMember("player"))
        {
            const auto &player = doc["player"];
            outData.playerData.role = player["role"].GetInt();
            outData.playerData.level = player["level"].GetInt();
            outData.playerData.experience = player["experience"].GetInt();
            outData.playerData.skillPoints = player["skillPoints"].GetInt();
            outData.playerData.currentHP = player["currentHP"].GetFloat();
            outData.playerData.currentMP = player["currentMP"].GetFloat();

            // 基础属性
            if (player.HasMember("baseAttributes"))
            {
                deserializeAttributes(player["baseAttributes"], outData.playerData.baseAttributes);
            }

            // 装备
            if (player.HasMember("equippedItems"))
            {
                const auto &equippedItems = player["equippedItems"];
                for (auto it = equippedItems.MemberBegin(); it != equippedItems.MemberEnd(); ++it)
                {
                    int slot = std::stoi(it->name.GetString());
                    EquipmentSaveData equip;
                    deserializeEquipment(it->value, equip);
                    outData.playerData.equippedItems[slot] = equip;
                }
            }

            // 技能
            if (player.HasMember("learnedSkills"))
            {
                const auto &learnedSkills = player["learnedSkills"];
                for (rapidjson::SizeType i = 0; i < learnedSkills.Size(); ++i)
                {
                    SkillSaveData skill;
                    deserializeSkill(learnedSkills[i], skill);
                    outData.playerData.learnedSkills.push_back(skill);
                }
            }

            // 主动技能槽位
            if (player.HasMember("activeSlotSkillIds"))
            {
                const auto &activeSlotSkillIds = player["activeSlotSkillIds"];
                for (rapidjson::SizeType i = 0; i < activeSlotSkillIds.Size(); ++i)
                {
                    outData.playerData.activeSlotSkillIds.push_back(activeSlotSkillIds[i].GetInt());
                }
            }
        }

        // 游戏进度
        if (doc.HasMember("progress"))
        {
            const auto &progress = doc["progress"];
            outData.progressData.currentSceneName = progress["currentSceneName"].GetString();
            outData.progressData.playerPosX = progress["playerPosX"].GetFloat();
            outData.progressData.playerPosY = progress["playerPosY"].GetFloat();
            outData.progressData.playTimeSeconds = progress["playTimeSeconds"].GetInt64();

            if (progress.HasMember("unlockedLevels"))
            {
                const auto &unlockedLevels = progress["unlockedLevels"];
                for (rapidjson::SizeType i = 0; i < unlockedLevels.Size(); ++i)
                {
                    outData.progressData.unlockedLevels.push_back(unlockedLevels[i].GetString());
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
