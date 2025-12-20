#include "SaveManager.h"
#include "JsonSerializer.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/SkillComponent.h"
#include "cocos2d.h"
#include <chrono>

USING_NS_CC;

SaveManager *SaveManager::_instance = nullptr;

SaveManager *SaveManager::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new SaveManager();
    }
    return _instance;
}

void SaveManager::destroyInstance()
{
    if (_instance != nullptr)
    {
        delete _instance;
        _instance = nullptr;
    }
}

SaveManager::SaveManager()
{
    _sessionStartTime = std::chrono::steady_clock::now();

    // 确保存档目录存在
    std::string savesDir = FileUtils::getInstance()->getWritablePath() + "saves/";
    if (!FileUtils::getInstance()->isDirectoryExist(savesDir))
    {
        FileUtils::getInstance()->createDirectory(savesDir);
        CCLOG("SaveManager - 创建存档目录: %s", savesDir.c_str());
    }
}

SaveManager::~SaveManager()
{
}

//================== 运行时进度（不落盘） ==================

void SaveManager::cacheRuntimePlayerData(PlayerCharacter *player)
{
    if (!player)
    {
        CCLOG("SaveManager::cacheRuntimePlayerData - 玩家指针为空");
        return;
    }

    _runtimePlayerData = extractPlayerData(player);
    _hasRuntimePlayerData = true;
    CCLOG("SaveManager::cacheRuntimePlayerData - 已缓存运行时玩家数据（Lv.%d, Exp=%d）",
          _runtimePlayerData.level,
          _runtimePlayerData.experience);
}

void SaveManager::setRuntimePlayerData(const PlayerSaveData &data)
{
    _runtimePlayerData = data;
    _hasRuntimePlayerData = true;
    CCLOG("SaveManager::setRuntimePlayerData - 已设置运行时玩家数据（Lv.%d, Exp=%d）",
          _runtimePlayerData.level,
          _runtimePlayerData.experience);
}

void SaveManager::clearRuntimePlayerData()
{
    _hasRuntimePlayerData = false;
    _runtimePlayerData = PlayerSaveData();
    CCLOG("SaveManager::clearRuntimePlayerData - 已清空运行时玩家数据");
}

//================== 辅助方法 ==================

std::string SaveManager::getSaveFilePath(int slotIndex) const
{
    return FileUtils::getInstance()->getWritablePath() + "saves/save_" + std::to_string(slotIndex) + ".json";
}

std::string SaveManager::getSettingsFilePath() const
{
    return FileUtils::getInstance()->getWritablePath() + "settings.json";
}

bool SaveManager::writeToFile(const std::string &filePath, const std::string &content)
{
    // 先写入临时文件，再原子重命名，避免中途崩溃损坏存档
    std::string tempPath = filePath + ".tmp";

    FILE *file = fopen(tempPath.c_str(), "w");
    if (file == nullptr)
    {
        CCLOG("SaveManager::writeToFile - 无法打开文件: %s", tempPath.c_str());
        return false;
    }

    size_t written = fwrite(content.c_str(), 1, content.size(), file);
    fclose(file);

    if (written != content.size())
    {
        CCLOG("SaveManager::writeToFile - 写入文件失败: %s", tempPath.c_str());
        FileUtils::getInstance()->removeFile(tempPath);
        return false;
    }

    // 覆盖目标前先删除旧文件，再重命名
    FileUtils::getInstance()->removeFile(filePath);
    if (!FileUtils::getInstance()->renameFile(tempPath, filePath))
    {
        CCLOG("SaveManager::writeToFile - 重命名临时文件失败: %s -> %s", tempPath.c_str(), filePath.c_str());
        return false;
    }

    CCLOG("SaveManager::writeToFile - 成功写入文件: %s", filePath.c_str());
    return true;
}

bool SaveManager::readFromFile(const std::string &filePath, std::string &outContent) const
{
    if (!FileUtils::getInstance()->isFileExist(filePath))
    {
        CCLOG("SaveManager::readFromFile - 文件不存在: %s", filePath.c_str());
        return false;
    }

    Data data = FileUtils::getInstance()->getDataFromFile(filePath);
    if (data.isNull())
    {
        CCLOG("SaveManager::readFromFile - 读取文件失败: %s", filePath.c_str());
        return false;
    }

    outContent = std::string(reinterpret_cast<const char *>(data.getBytes()), data.getSize());
    CCLOG("SaveManager::readFromFile - 成功读取文件: %s", filePath.c_str());
    return true;
}

//================== 核心存档操作 ==================

bool SaveManager::saveGame(int slotIndex, PlayerCharacter *player,
                           const std::string &sceneName, const cocos2d::Vec2 &playerPos)
{
    if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
    {
        CCLOG("SaveManager::saveGame - 无效的槽位索引: %d", slotIndex);
        return false;
    }

    if (player == nullptr)
    {
        CCLOG("SaveManager::saveGame - 玩家指针为空");
        return false;
    }

    // 构建存档数据
    SaveSlotData saveData;
    saveData.slotIndex = slotIndex;
    saveData.saveTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
    saveData.gameVersion = "1.0.0";

    // 提取玩家数据
    saveData.playerData = extractPlayerData(player);

    // 设置游戏进度
    saveData.progressData.currentSceneName = sceneName;
    saveData.progressData.playerPosX = playerPos.x;
    saveData.progressData.playerPosY = playerPos.y;

    // 游戏时长（当前会话累计）
    auto now = std::chrono::steady_clock::now();
    saveData.progressData.playTimeSeconds =
        std::chrono::duration_cast<std::chrono::seconds>(now - _sessionStartTime).count();

    // 序列化为 JSON
    std::string json = JsonSerializer::serialize(saveData);
    if (json.empty())
    {
        CCLOG("SaveManager::saveGame - 序列化失败");
        return false;
    }

    // 写入文件
    std::string filePath = getSaveFilePath(slotIndex);
    if (!writeToFile(filePath, json))
    {
        CCLOG("SaveManager::saveGame - 写入文件失败");
        return false;
    }

    CCLOG("SaveManager::saveGame - 成功保存到槽位 %d", slotIndex);
    return true;
}

bool SaveManager::loadGame(int slotIndex, SaveSlotData &outData)
{
    if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
    {
        CCLOG("SaveManager::loadGame - 无效的槽位索引: %d", slotIndex);
        return false;
    }

    std::string filePath = getSaveFilePath(slotIndex);
    std::string json;
    if (!readFromFile(filePath, json))
    {
        CCLOG("SaveManager::loadGame - 读取文件失败");
        return false;
    }

    if (!JsonSerializer::deserialize(json, outData))
    {
        CCLOG("SaveManager::loadGame - 反序列化失败");
        return false;
    }

    CCLOG("SaveManager::loadGame - 成功从槽位 %d 加载", slotIndex);
    return true;
}

bool SaveManager::deleteSave(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
    {
        CCLOG("SaveManager::deleteSave - 无效的槽位索引: %d", slotIndex);
        return false;
    }

    std::string filePath = getSaveFilePath(slotIndex);
    if (!FileUtils::getInstance()->isFileExist(filePath))
    {
        CCLOG("SaveManager::deleteSave - 文件不存在: %s", filePath.c_str());
        return true; // 文件不存在也算成功
    }

    if (!FileUtils::getInstance()->removeFile(filePath))
    {
        CCLOG("SaveManager::deleteSave - 删除文件失败: %s", filePath.c_str());
        return false;
    }

    CCLOG("SaveManager::deleteSave - 成功删除槽位 %d", slotIndex);
    return true;
}

bool SaveManager::hasSave(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
    {
        return false;
    }

    std::string filePath = getSaveFilePath(slotIndex);
    return FileUtils::getInstance()->isFileExist(filePath);
}

std::vector<SaveSlotData> SaveManager::getAllSaveSlotInfos() const
{
    std::vector<SaveSlotData> infos;

    for (int i = 0; i < MAX_SAVE_SLOTS; ++i)
    {
        if (hasSave(i))
        {
            SaveSlotData data;
            std::string filePath = getSaveFilePath(i);
            std::string json;
            if (readFromFile(filePath, json) && JsonSerializer::deserialize(json, data))
            {
                infos.push_back(data);
            }
        }
        else
        {
            // 空槽位
            SaveSlotData emptyData;
            emptyData.slotIndex = i;
            emptyData.saveTimestamp = 0;
            infos.push_back(emptyData);
        }
    }

    return infos;
}

//================== 自动存档 ==================

void SaveManager::setAutoSaveEnabled(bool enabled)
{
    _autoSaveEnabled = enabled;
    CCLOG("SaveManager::setAutoSaveEnabled - 自动存档: %s", enabled ? "启用" : "禁用");
}

void SaveManager::setAutoSaveInterval(float seconds)
{
    _autoSaveInterval = seconds;
    CCLOG("SaveManager::setAutoSaveInterval - 自动存档间隔: %.1f 秒", seconds);
}

void SaveManager::performAutoSave(PlayerCharacter *player,
                                  const std::string &sceneName, const cocos2d::Vec2 &playerPos)
{
    if (!_autoSaveEnabled || player == nullptr)
    {
        return;
    }

    _autoSaveTimer += Director::getInstance()->getDeltaTime();

    if (_autoSaveTimer >= _autoSaveInterval)
    {
        // 预留最后一个槽位用于自动存档，避免覆盖手动存档
        int autoSaveSlot = AUTO_SAVE_SLOT;
        if (saveGame(autoSaveSlot, player, sceneName, playerPos))
        {
            CCLOG("SaveManager::performAutoSave - 自动存档成功 (slot %d)", autoSaveSlot);
            _lastAutoSaveSlot = autoSaveSlot;
        }
        else
        {
            CCLOG("SaveManager::performAutoSave - 自动存档失败");
        }

        _autoSaveTimer = 0.0f;
    }
}

void SaveManager::resetAutoSaveTimer()
{
    _autoSaveTimer = 0.0f;
}

//================== 设置管理 ==================

bool SaveManager::saveSettings(const SettingsSaveData &settings)
{
    std::string json = JsonSerializer::serialize(settings);
    if (json.empty())
    {
        CCLOG("SaveManager::saveSettings - 序列化失败");
        return false;
    }

    std::string filePath = getSettingsFilePath();
    if (!writeToFile(filePath, json))
    {
        CCLOG("SaveManager::saveSettings - 写入文件失败");
        return false;
    }

    CCLOG("SaveManager::saveSettings - 成功保存设置");
    return true;
}

bool SaveManager::loadSettings(SettingsSaveData &outSettings)
{
    std::string filePath = getSettingsFilePath();
    std::string json;
    if (!readFromFile(filePath, json))
    {
        CCLOG("SaveManager::loadSettings - 读取文件失败，使用默认设置");
        outSettings = SettingsSaveData(); // 使用默认值
        return false;
    }

    if (!JsonSerializer::deserialize(json, outSettings))
    {
        CCLOG("SaveManager::loadSettings - 反序列化失败，使用默认设置");
        outSettings = SettingsSaveData(); // 使用默认值
        return false;
    }

    CCLOG("SaveManager::loadSettings - 成功加载设置");
    return true;
}

//================== 数据转换 ==================

PlayerSaveData SaveManager::extractPlayerData(PlayerCharacter *player) const
{
    PlayerSaveData data;

    if (!player)
    {
        CCLOG("SaveManager::extractPlayerData - 玩家指针为空");
        return data;
    }

    // 基础信息
    data.role = static_cast<int>(player->getRole());
    data.level = player->getLevel();
    data.experience = player->getExperience();
    data.skillPoints = player->getSkillPoints();

    // 当前状态
    data.currentHP = player->getCurrentHP();
    data.currentMP = player->getCurrentMP();

    // 基础属性
    auto attrComp = player->getAttributeComponent();
    if (attrComp)
    {
        const Attributes &baseAttrs = attrComp->getBaseAttributes();
        for (std::map<AttributeType, float>::const_iterator it = baseAttrs.values.begin();
             it != baseAttrs.values.end(); ++it)
        {
            data.baseAttributes.values[static_cast<int>(it->first)] = it->second;
        }
    }

    // 装备
    const std::map<EquipmentSlot, std::shared_ptr<Equipment>> &equippedItems = player->getEquippedItems();
    for (std::map<EquipmentSlot, std::shared_ptr<Equipment>>::const_iterator it = equippedItems.begin();
         it != equippedItems.end(); ++it)
    {
        if (!it->second)
            continue;

        EquipmentSaveData equipData;
        equipData.id = it->second->id;
        equipData.name = it->second->name;
        equipData.description = it->second->description;
        equipData.slot = static_cast<int>(it->second->slot);
        equipData.spritePath = it->second->spritePath;

        // 属性加成
        for (std::map<AttributeType, float>::const_iterator attrIt = it->second->attributeBonus.values.begin();
             attrIt != it->second->attributeBonus.values.end(); ++attrIt)
        {
            equipData.attributeBonus.values[static_cast<int>(attrIt->first)] = attrIt->second;
        }

        // 检查是否为武器
        std::shared_ptr<Weapon> weapon = std::dynamic_pointer_cast<Weapon>(it->second);
        if (weapon)
        {
            equipData.isWeapon = true;
            equipData.weaponType = static_cast<int>(weapon->type);
            equipData.attackDamage = weapon->attackDamage;
            equipData.attackRange = weapon->attackRange;
            equipData.attackSpeed = weapon->attackSpeed;
            equipData.attackAnimationPrefix = weapon->attackAnimationPrefix;
            equipData.attackFrameCount = weapon->attackFrameCount;
        }

        data.equippedItems[static_cast<int>(it->first)] = equipData;
    }

    // 技能
    auto skillComp = player->getSkillComponent();
    if (skillComp)
    {
        // 已学习的技能
        const std::vector<std::shared_ptr<Skill>> &learnedSkills = skillComp->getLearnedSkills();
        for (size_t i = 0; i < learnedSkills.size(); ++i)
        {
            const std::shared_ptr<Skill> &skill = learnedSkills[i];
            if (!skill)
                continue;

            SkillSaveData skillData;
            skillData.id = skill->id;
            skillData.name = skill->name;
            skillData.description = skill->description;
            skillData.isPassive = skill->isPassive;

            // 主动技能
            std::shared_ptr<ActiveSkill> activeSkill = std::dynamic_pointer_cast<ActiveSkill>(skill);
            if (activeSkill)
            {
                skillData.cooldown = activeSkill->cooldown;
                skillData.manaCost = activeSkill->manaCost;
                skillData.currentCooldown = activeSkill->currentCooldown;
            }

            // 被动技能
            std::shared_ptr<PassiveSkill> passiveSkill = std::dynamic_pointer_cast<PassiveSkill>(skill);
            if (passiveSkill)
            {
                for (std::map<AttributeType, float>::const_iterator attrIt = passiveSkill->attributeBonus.values.begin();
                     attrIt != passiveSkill->attributeBonus.values.end(); ++attrIt)
                {
                    skillData.attributeBonus.values[static_cast<int>(attrIt->first)] = attrIt->second;
                }
            }

            data.learnedSkills.push_back(skillData);
        }

        // 主动技能槽位
        const std::vector<std::shared_ptr<ActiveSkill>> &activeSlots = skillComp->getActiveSlots();
        for (size_t i = 0; i < activeSlots.size(); ++i)
        {
            const std::shared_ptr<ActiveSkill> &skill = activeSlots[i];
            if (skill)
            {
                data.activeSlotSkillIds.push_back(skill->id);
            }
            else
            {
                data.activeSlotSkillIds.push_back(-1); // 空槽位
            }
        }
    }

    CCLOG("SaveManager::extractPlayerData - 成功提取玩家数据");
    return data;
}

void SaveManager::applyPlayerData(PlayerCharacter *player, const PlayerSaveData &data) const
{
    if (!player)
    {
        CCLOG("SaveManager::applyPlayerData - 玩家指针为空");
        return;
    }

    // 记录要恢复的当前值，等属性/装备完整恢复后再夹取
    float savedHP = data.currentHP;
    float savedMP = data.currentMP;

    // 基础信息
    player->setRole(static_cast<CharacterRole>(data.role));
    player->setLevel(data.level);
    player->setExperience(data.experience);
    player->setSkillPoints(data.skillPoints);

    // 基础属性
    AttributeComponent *attrComp = player->getAttributeComponent();
    if (attrComp)
    {
        Attributes baseAttrs;
        for (std::map<int, float>::const_iterator it = data.baseAttributes.values.begin();
             it != data.baseAttributes.values.end(); ++it)
        {
            baseAttrs.values[static_cast<AttributeType>(it->first)] = it->second;
        }
        attrComp->setBaseAttributes(baseAttrs);
        attrComp->recalculateFinalAttributes();
    }

    // 清空当前装备（移除已有加成）
    std::vector<EquipmentSlot> slotsToClear;
    for (const auto &kv : player->getEquippedItems())
    {
        slotsToClear.push_back(kv.first);
    }
    for (auto slot : slotsToClear)
    {
        player->unequip(slot);
    }

    // 装备
    for (std::map<int, EquipmentSaveData>::const_iterator it = data.equippedItems.begin();
         it != data.equippedItems.end(); ++it)
    {
        const EquipmentSaveData &equipData = it->second;
        std::shared_ptr<Equipment> equipment;

        if (equipData.isWeapon)
        {
            // 创建武器
            auto weapon = std::make_shared<Weapon>();
            weapon->id = equipData.id;
            weapon->name = equipData.name;
            weapon->description = equipData.description;
            weapon->slot = static_cast<EquipmentSlot>(equipData.slot);
            weapon->spritePath = equipData.spritePath;
            weapon->type = static_cast<WeaponType>(equipData.weaponType);
            weapon->attackDamage = equipData.attackDamage;
            weapon->attackRange = equipData.attackRange;
            weapon->attackSpeed = equipData.attackSpeed;
            weapon->attackAnimationPrefix = equipData.attackAnimationPrefix;
            weapon->attackFrameCount = equipData.attackFrameCount;

            // 属性加成
            for (std::map<int, float>::const_iterator attrIt = equipData.attributeBonus.values.begin();
                 attrIt != equipData.attributeBonus.values.end(); ++attrIt)
            {
                weapon->attributeBonus.values[static_cast<AttributeType>(attrIt->first)] = attrIt->second;
            }

            equipment = weapon;
        }
        else
        {
            // 创建普通装备
            equipment = std::make_shared<Equipment>();
            equipment->id = equipData.id;
            equipment->name = equipData.name;
            equipment->description = equipData.description;
            equipment->slot = static_cast<EquipmentSlot>(equipData.slot);
            equipment->spritePath = equipData.spritePath;

            // 属性加成
            for (std::map<int, float>::const_iterator attrIt = equipData.attributeBonus.values.begin();
                 attrIt != equipData.attributeBonus.values.end(); ++attrIt)
            {
                equipment->attributeBonus.values[static_cast<AttributeType>(attrIt->first)] = attrIt->second;
            }
        }

        if (equipment)
        {
            player->equip(equipment);
        }
    }

    // 技能
    SkillComponent *skillComp = player->getSkillComponent();
    if (skillComp)
    {
        // 清空已有技能/槽位，防止重复和冷却污染
        skillComp->resetSkills();

        // 重建已学习的技能
        for (size_t i = 0; i < data.learnedSkills.size(); ++i)
        {
            const SkillSaveData &skillData = data.learnedSkills[i];
            if (skillData.isPassive)
            {
                // 创建被动技能
                std::shared_ptr<PassiveSkill> skill = std::make_shared<PassiveSkill>();
                skill->id = skillData.id;
                skill->name = skillData.name;
                skill->description = skillData.description;
                skill->isPassive = true;

                // 属性加成
                for (std::map<int, float>::const_iterator attrIt = skillData.attributeBonus.values.begin();
                     attrIt != skillData.attributeBonus.values.end(); ++attrIt)
                {
                    skill->attributeBonus.values[static_cast<AttributeType>(attrIt->first)] = attrIt->second;
                }

                skillComp->learnSkill(skill);
            }
            else
            {
                // 创建主动技能
                std::shared_ptr<ActiveSkill> skill = std::make_shared<ActiveSkill>();
                skill->id = skillData.id;
                skill->name = skillData.name;
                skill->description = skillData.description;
                skill->isPassive = false;
                skill->cooldown = skillData.cooldown;
                skill->manaCost = skillData.manaCost;
                skill->currentCooldown = skillData.currentCooldown;

                skillComp->learnSkill(skill);
            }
        }

        // 恢复主动技能槽位
        std::vector<std::shared_ptr<ActiveSkill>> activeSlots;
        for (size_t i = 0; i < data.activeSlotSkillIds.size(); ++i)
        {
            int skillId = data.activeSlotSkillIds[i];
            if (skillId == -1)
            {
                activeSlots.push_back(std::shared_ptr<ActiveSkill>()); // 空槽位
            }
            else
            {
                std::shared_ptr<Skill> skill = skillComp->findLearnedSkillById(skillId);
                std::shared_ptr<ActiveSkill> activeSkill = std::dynamic_pointer_cast<ActiveSkill>(skill);
                activeSlots.push_back(activeSkill);
            }
        }
        skillComp->clearAndSetActiveSlots(activeSlots);
    }

    // 根据最终上限夹取 HP/MP
    player->setCurrentHP(savedHP);
    player->setCurrentMP(savedMP);

    CCLOG("SaveManager::applyPlayerData - 成功应用玩家数据");
}
