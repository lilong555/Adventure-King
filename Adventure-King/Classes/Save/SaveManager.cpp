#include "SaveManager.h"
#include "JsonSerializer.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/SkillComponent.h"
#include "storage/local-storage/LocalStorage.h"
#include "cocos2d.h"
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

USING_NS_CC;

namespace
{
// 说明：localStorage 在 Android 端只使用文件名（不包含目录），因此文件名需要保持稳定且尽量唯一
static const char *const LOCAL_STORAGE_DB_FILENAME = "adventure_king_save.db";

static const char *const STORAGE_KEY_PREFIX = "ak_";

std::string buildStorageKey(const std::string &suffix)
{
    return std::string(STORAGE_KEY_PREFIX) + suffix;
}
}

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

    // 初始化本地 SQLite（KV）数据库：作为主存档存储
    // 说明：基于 cocos2d::localStorage（底层是 sqlite3），跨平台可用。
    _localStorageDbPath = savesDir + LOCAL_STORAGE_DB_FILENAME;
    localStorageInit(_localStorageDbPath);
    _localStorageReady = true;
    CCLOG("SaveManager - 初始化本地存档数据库: %s", _localStorageDbPath.c_str());
}

SaveManager::~SaveManager()
{
    if (_localStorageReady)
    {
        localStorageFree();
        _localStorageReady = false;
        CCLOG("SaveManager - 已释放本地存档数据库");
    }
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

//================== 运行时关卡进度（不落盘） ==================

void SaveManager::setRuntimeProgressData(const GameProgressSaveData &data)
{
    _runtimeProgressData = data;
    _hasRuntimeProgressData = true;
    CCLOG("SaveManager::setRuntimeProgressData - 已设置运行时关卡进度（scene=%s）",
          _runtimeProgressData.currentSceneName.c_str());
}

void SaveManager::clearRuntimeProgressData()
{
    _hasRuntimeProgressData = false;
    _runtimeProgressData = GameProgressSaveData();
}

//================== 会话角色选择（不落盘） ==================

void SaveManager::setSessionSelectedRole(CharacterRole role)
{
    _sessionSelectedRole = role;
    _hasSessionSelectedRole = true;

    // 主菜单切换职业通常意味着“新开局”：
    // 如果不清理运行时数据，GameScene 进图时会优先 applyRuntimePlayerData，
    // 造成“换了职业但进图仍然是上一局职业”的问题。
    clearRuntimePlayerData();
    clearRuntimePlayerPosition();
    clearRuntimeProgressData();

    CCLOG("SaveManager::setSessionSelectedRole - 已设置会话职业：%d", static_cast<int>(role));
}

void SaveManager::clearSessionSelectedRole()
{
    _hasSessionSelectedRole = false;
    _sessionSelectedRole = CharacterRole::MAGE;
    CCLOG("SaveManager::clearSessionSelectedRole - 已清空会话职业选择");
}

void SaveManager::setRuntimePlayerPosition(const cocos2d::Vec2& pos)
{
    _runtimePlayerPosition = pos;
    _hasRuntimePlayerPosition = true;
    CCLOG("SaveManager::setRuntimePlayerPosition - 已设置运行时玩家位置 (%.1f, %.1f)", pos.x, pos.y);
}

void SaveManager::clearRuntimePlayerPosition()
{
    _hasRuntimePlayerPosition = false;
    _runtimePlayerPosition = Vec2::ZERO;
}

//================== 辅助方法 ==================

std::string SaveManager::getSaveStorageKey(int slotIndex) const
{
    return buildStorageKey("save_slot_" + std::to_string(slotIndex));
}

std::string SaveManager::getSettingsStorageKey() const
{
    return buildStorageKey("settings");
}

bool SaveManager::writeToStorage(const std::string &key, const std::string &content)
{
    if (!_localStorageReady)
    {
        CCLOG("SaveManager::writeToStorage - 本地数据库未初始化");
        return false;
    }

    localStorageSetItem(key, content);
    return true;
}

bool SaveManager::readFromStorage(const std::string &key, std::string &outContent) const
{
    if (!_localStorageReady)
    {
        CCLOG("SaveManager::readFromStorage - 本地数据库未初始化");
        return false;
    }

    return localStorageGetItem(key, &outContent);
}

bool SaveManager::removeFromStorage(const std::string &key)
{
    if (!_localStorageReady)
    {
        CCLOG("SaveManager::removeFromStorage - 本地数据库未初始化");
        return false;
    }

    localStorageRemoveItem(key);
    return true;
}

bool SaveManager::hasStorageKey(const std::string &key) const
{
    if (!_localStorageReady)
    {
        return false;
    }

    std::string tmp;
    return localStorageGetItem(key, &tmp);
}

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
    GameProgressSaveData progressData;
    progressData.currentSceneName = sceneName;
    progressData.playerPosX = playerPos.x;
    progressData.playerPosY = playerPos.y;
    return saveGame(slotIndex, player, progressData);
}

bool SaveManager::saveGame(int slotIndex, PlayerCharacter *player, const GameProgressSaveData &progressData)
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

    // 设置游戏进度（包含刷怪点/竞技场/怪物快照等）
    saveData.progressData = progressData;

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

    // 写入本地数据库（主存档）
    std::string storageKey = getSaveStorageKey(slotIndex);
    if (!writeToStorage(storageKey, json))
    {
        CCLOG("SaveManager::saveGame - 写入本地数据库失败");
        return false;
    }

    // 兼容：仍写入 legacy JSON 文件作为备份（便于手动拷贝/回滚/调试）
    std::string filePath = getSaveFilePath(slotIndex);
    if (!writeToFile(filePath, json))
    {
        CCLOG("SaveManager::saveGame - 备份 JSON 写入失败（不影响主存档）");
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

    std::string json;

    // 优先从本地数据库读取
    std::string storageKey = getSaveStorageKey(slotIndex);
    if (!readFromStorage(storageKey, json))
    {
        // 回退到 legacy JSON 文件（兼容旧存档/也作为备份恢复途径）
        std::string filePath = getSaveFilePath(slotIndex);
        if (!readFromFile(filePath, json))
        {
            CCLOG("SaveManager::loadGame - 读取本地数据库/备份文件均失败");
            return false;
        }

        // 将备份文件写回数据库，完成一次“按需迁移/恢复”
        if (writeToStorage(storageKey, json))
        {
            CCLOG("SaveManager::loadGame - 已从备份 JSON 恢复到本地数据库（slot %d）", slotIndex);
        }
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

    // 删除本地数据库中的槽位
    std::string storageKey = getSaveStorageKey(slotIndex);
    if (_localStorageReady)
    {
        removeFromStorage(storageKey);
    }

    // 同步删除备份文件，避免 UI 显示“幽灵存档”
    std::string filePath = getSaveFilePath(slotIndex);
    if (FileUtils::getInstance()->isFileExist(filePath) && !FileUtils::getInstance()->removeFile(filePath))
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

    // 优先检查本地数据库
    std::string storageKey = getSaveStorageKey(slotIndex);
    if (hasStorageKey(storageKey))
    {
        return true;
    }

    // 兼容：旧存档/备份文件
    return FileUtils::getInstance()->isFileExist(getSaveFilePath(slotIndex));
}

std::vector<SaveSlotData> SaveManager::getAllSaveSlotInfos() const
{
    std::vector<SaveSlotData> infos;

    for (int i = 0; i < MAX_SAVE_SLOTS; ++i)
    {
        if (hasSave(i))
        {
            SaveSlotData data;
            std::string json;

            // 优先从本地数据库读取；若没有再回退到备份文件
            std::string storageKey = getSaveStorageKey(i);
            if (!readFromStorage(storageKey, json))
            {
                std::string filePath = getSaveFilePath(i);
                if (readFromFile(filePath, json))
                {
                    // 注意：此处仅用于“读取槽位信息展示 UI”，保持 const 语义，不在此写回数据库；
                    // 真正的按需迁移/恢复在 loadGame / loadSettings 等非 const 接口中完成。
                }
            }

            if (!json.empty() && JsonSerializer::deserialize(json, data))
            {
                infos.push_back(data);
            }
            else
            {
                // 槽位存在但读取/反序列化失败：仍然返回一个“空槽位”，避免 UI 列表长度异常
                SaveSlotData emptyData;
                emptyData.slotIndex = i;
                emptyData.saveTimestamp = 0;
                infos.push_back(emptyData);
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

    // 写入本地数据库（主存储）
    std::string storageKey = getSettingsStorageKey();
    if (!writeToStorage(storageKey, json))
    {
        CCLOG("SaveManager::saveSettings - 写入本地数据库失败");
        return false;
    }

    // 兼容：仍写入 legacy JSON 文件作为备份（便于手动拷贝/回滚/调试）
    std::string filePath = getSettingsFilePath();
    if (!writeToFile(filePath, json))
    {
        CCLOG("SaveManager::saveSettings - 备份 JSON 写入失败（不影响主设置）");
    }

    CCLOG("SaveManager::saveSettings - 成功保存设置");
    return true;
}

bool SaveManager::loadSettings(SettingsSaveData &outSettings)
{
    std::string json;

    // 优先从本地数据库读取
    std::string storageKey = getSettingsStorageKey();
    if (!readFromStorage(storageKey, json))
    {
        // 回退到 legacy JSON 文件（兼容旧设置/也作为备份恢复途径）
        std::string filePath = getSettingsFilePath();
        if (!readFromFile(filePath, json))
        {
            CCLOG("SaveManager::loadSettings - 读取本地数据库/备份文件均失败，使用默认设置");
            outSettings = SettingsSaveData(); // 使用默认值
            return false;
        }

        // 将备份文件写回数据库，完成一次“按需迁移/恢复”
        if (writeToStorage(storageKey, json))
        {
            CCLOG("SaveManager::loadSettings - 已从备份 JSON 恢复到本地数据库");
        }
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

//================== 云同步预留接口（导入/导出） ==================

bool SaveManager::exportSaveSlotToJsonString(int slotIndex, std::string &outJson) const
{
    outJson.clear();

    if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
    {
        CCLOG("SaveManager::exportSaveSlotToJsonString - 无效的槽位索引: %d", slotIndex);
        return false;
    }

    const std::string storageKey = getSaveStorageKey(slotIndex);
    if (readFromStorage(storageKey, outJson))
    {
        return true;
    }

    // 回退：备份 JSON
    const std::string filePath = getSaveFilePath(slotIndex);
    return readFromFile(filePath, outJson);
}

bool SaveManager::importSaveSlotFromJsonString(int slotIndex, const std::string &json, bool overwriteExisting)
{
    if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
    {
        CCLOG("SaveManager::importSaveSlotFromJsonString - 无效的槽位索引: %d", slotIndex);
        return false;
    }

    if (!overwriteExisting && hasSave(slotIndex))
    {
        CCLOG("SaveManager::importSaveSlotFromJsonString - 槽位已存在且不允许覆盖: %d", slotIndex);
        return false;
    }

    SaveSlotData data;
    if (!JsonSerializer::deserialize(json, data))
    {
        CCLOG("SaveManager::importSaveSlotFromJsonString - JSON 反序列化失败");
        return false;
    }

    // 规范化槽位号（云端可能携带不同 slotIndex）
    data.slotIndex = slotIndex;
    std::string normalized = JsonSerializer::serialize(data);
    if (normalized.empty())
    {
        CCLOG("SaveManager::importSaveSlotFromJsonString - JSON 规范化失败");
        return false;
    }

    const std::string storageKey = getSaveStorageKey(slotIndex);
    if (!writeToStorage(storageKey, normalized))
    {
        CCLOG("SaveManager::importSaveSlotFromJsonString - 写入本地数据库失败");
        return false;
    }

    // 同步写入备份文件
    const std::string filePath = getSaveFilePath(slotIndex);
    if (!writeToFile(filePath, normalized))
    {
        CCLOG("SaveManager::importSaveSlotFromJsonString - 备份 JSON 写入失败（不影响主存档）");
    }

    CCLOG("SaveManager::importSaveSlotFromJsonString - 已导入槽位 %d", slotIndex);
    return true;
}

bool SaveManager::exportSettingsToJsonString(std::string &outJson) const
{
    outJson.clear();

    const std::string storageKey = getSettingsStorageKey();
    if (readFromStorage(storageKey, outJson))
    {
        return true;
    }

    // 回退：备份 JSON
    const std::string filePath = getSettingsFilePath();
    return readFromFile(filePath, outJson);
}

bool SaveManager::importSettingsFromJsonString(const std::string &json)
{
    SettingsSaveData settings;
    if (!JsonSerializer::deserialize(json, settings))
    {
        CCLOG("SaveManager::importSettingsFromJsonString - JSON 反序列化失败");
        return false;
    }

    std::string normalized = JsonSerializer::serialize(settings);
    if (normalized.empty())
    {
        CCLOG("SaveManager::importSettingsFromJsonString - JSON 规范化失败");
        return false;
    }

    const std::string storageKey = getSettingsStorageKey();
    if (!writeToStorage(storageKey, normalized))
    {
        CCLOG("SaveManager::importSettingsFromJsonString - 写入本地数据库失败");
        return false;
    }

    // 同步写入备份文件
    const std::string filePath = getSettingsFilePath();
    if (!writeToFile(filePath, normalized))
    {
        CCLOG("SaveManager::importSettingsFromJsonString - 备份 JSON 写入失败（不影响主设置）");
    }

    CCLOG("SaveManager::importSettingsFromJsonString - 已导入设置");
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
    data.activeSkillPoints = player->getActiveSkillPoints();
    data.passiveSkillPoints = player->getPassiveSkillPoints();
    data.attributePoints = player->getAttributePoints();

    // 当前状态
    data.currentHP = player->getCurrentHP();
    data.currentMP = player->getCurrentMP();
    data.outgoingDamageMultiplier = player->getOutgoingDamageMultiplier();
    data.outgoingDamageMultiplierRemainingSeconds = player->getOutgoingDamageMultiplierRemainingSeconds();

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
    auto buildEquipmentSaveData = [](const std::shared_ptr<Equipment> &equipment) -> EquipmentSaveData
    {
        EquipmentSaveData equipData;
        if (!equipment)
        {
            return equipData;
        }

        equipData.id = equipment->id;
        equipData.name = equipment->name;
        equipData.description = equipment->description;
        equipData.slot = static_cast<int>(equipment->slot);
        equipData.level = std::max(1, equipment->level);
        equipData.spritePath = equipment->spritePath;

        // 属性加成
        for (std::map<AttributeType, float>::const_iterator attrIt = equipment->attributeBonus.values.begin();
             attrIt != equipment->attributeBonus.values.end(); ++attrIt)
        {
            equipData.attributeBonus.values[static_cast<int>(attrIt->first)] = attrIt->second;
        }

        // 检查是否为武器
        std::shared_ptr<Weapon> weapon = std::dynamic_pointer_cast<Weapon>(equipment);
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

        return equipData;
    };

    const std::map<EquipmentSlot, std::shared_ptr<Equipment>> &equippedItems = player->getEquippedItems();
    for (std::map<EquipmentSlot, std::shared_ptr<Equipment>>::const_iterator it = equippedItems.begin();
         it != equippedItems.end(); ++it)
    {
        if (!it->second)
            continue;

        data.equippedItems[static_cast<int>(it->first)] = buildEquipmentSaveData(it->second);
    }

    // 背包（装备/武器）
    const auto &inventoryItems = player->getInventoryItems();
    for (const auto &item : inventoryItems)
    {
        if (!item)
        {
            continue;
        }
        data.inventoryItems.push_back(buildEquipmentSaveData(item));
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
            // 默认：被动技能不需要击破值（写 0，避免存档里出现 -1）
            skillData.breakDamage = 0;

            // 主动技能
            std::shared_ptr<ActiveSkill> activeSkill = std::dynamic_pointer_cast<ActiveSkill>(skill);
            if (activeSkill)
            {
                skillData.cooldown = activeSkill->cooldown;
                skillData.manaCost = activeSkill->manaCost;
                skillData.breakDamage = activeSkill->breakDamage;
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

        // 被动技能槽位
        const std::vector<std::shared_ptr<PassiveSkill>> &passiveSlots = skillComp->getPassiveSlots();
        for (size_t i = 0; i < passiveSlots.size(); ++i)
        {
            const std::shared_ptr<PassiveSkill> &skill = passiveSlots[i];
            if (skill)
            {
                data.passiveSlotSkillIds.push_back(skill->id);
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
    player->setActiveSkillPoints(data.activeSkillPoints);
    player->setPassiveSkillPoints(data.passiveSkillPoints);
    player->setAttributePoints(data.attributePoints);

    // 基础属性
    AttributeComponent *attrComp = player->getAttributeComponent();
    if (attrComp)
    {
        // 兼容：如果存档里没有基础属性（例如“新开局仅写入职业”），则保留玩家创建时按职业初始化的默认属性
        if (!data.baseAttributes.values.empty())
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

    // 清空背包（读档时以存档为准）
    player->clearInventory();

    auto createEquipmentFromSaveData = [](const EquipmentSaveData &equipData) -> std::shared_ptr<Equipment>
    {
        std::shared_ptr<Equipment> equipment;
        if (equipData.isWeapon)
        {
            auto weapon = std::make_shared<Weapon>();
            weapon->id = equipData.id;
            weapon->name = equipData.name;
            weapon->description = equipData.description;
            weapon->slot = static_cast<EquipmentSlot>(equipData.slot);
            weapon->level = std::max(1, equipData.level);
            weapon->spritePath = equipData.spritePath;
            weapon->type = static_cast<WeaponType>(equipData.weaponType);
            weapon->attackDamage = equipData.attackDamage;
            weapon->attackRange = equipData.attackRange;
            weapon->attackSpeed = equipData.attackSpeed;
            weapon->attackAnimationPrefix = equipData.attackAnimationPrefix;
            weapon->attackFrameCount = equipData.attackFrameCount;

            for (std::map<int, float>::const_iterator attrIt = equipData.attributeBonus.values.begin();
                 attrIt != equipData.attributeBonus.values.end(); ++attrIt)
            {
                weapon->attributeBonus.values[static_cast<AttributeType>(attrIt->first)] = attrIt->second;
            }
            equipment = weapon;
        }
        else
        {
            equipment = std::make_shared<Equipment>();
            equipment->id = equipData.id;
            equipment->name = equipData.name;
            equipment->description = equipData.description;
            equipment->slot = static_cast<EquipmentSlot>(equipData.slot);
            equipment->level = std::max(1, equipData.level);
            equipment->spritePath = equipData.spritePath;

            for (std::map<int, float>::const_iterator attrIt = equipData.attributeBonus.values.begin();
                 attrIt != equipData.attributeBonus.values.end(); ++attrIt)
            {
                equipment->attributeBonus.values[static_cast<AttributeType>(attrIt->first)] = attrIt->second;
            }
        }
        return equipment;
    };

    // 先恢复背包，再恢复穿戴：保证同一件装备在 UI 上不会出现两份对象
    std::unordered_map<int, std::shared_ptr<Equipment>> inventoryById;
    for (size_t i = 0; i < data.inventoryItems.size(); ++i)
    {
        const EquipmentSaveData &equipData = data.inventoryItems[i];
        auto equipment = createEquipmentFromSaveData(equipData);
        if (!equipment)
        {
            continue;
        }

        // 兼容：存档背包可能包含重复 id，避免重复加入
        if (inventoryById.find(equipment->id) == inventoryById.end())
        {
            player->addToInventory(equipment);
            inventoryById[equipment->id] = equipment;
        }
    }

    // 装备（穿戴）
    for (std::map<int, EquipmentSaveData>::const_iterator it = data.equippedItems.begin();
         it != data.equippedItems.end(); ++it)
    {
        const EquipmentSaveData &equipData = it->second;
        std::shared_ptr<Equipment> equipment;

        auto found = inventoryById.find(equipData.id);
        if (found != inventoryById.end())
        {
            equipment = found->second;
        }
        else
        {
            // 兼容：旧存档/异常数据可能出现“穿戴列表存在，但背包列表缺失”的情况。
            // 此时需要补建对象并加入背包，保证 UI 与加成结算一致。
            equipment = createEquipmentFromSaveData(equipData);
            if (equipment)
            {
                if (inventoryById.find(equipment->id) == inventoryById.end())
                {
                    player->addToInventory(equipment);
                    inventoryById[equipment->id] = equipment;
                }
                else
                {
                    equipment = inventoryById[equipment->id];
                }
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
        auto resolveActiveSkillBreakDamage = [](int skillId, int savedBreakDamage) -> int
        {
            // 优先使用存档值；若旧存档缺失该字段（-1），则按当前配置补齐。
            if (savedBreakDamage >= 0)
            {
                return savedBreakDamage;
            }

            switch (skillId)
            {
            case GameConfig::Bomb::BOMB_ID:
                // 炸弹属于技能：默认按“技能击破值”累计（如需细分，可为 Bomb 增加独立配置字段）
                return GameConfig::Combat::BREAK_DAMAGE_SKILL;
            case GameConfig::Fireball::FIREBALL_ID:
                return GameConfig::Fireball::BREAK_DAMAGE;
            case GameConfig::Assassin::SlashSkill::SLASH_ID:
                return GameConfig::Assassin::SlashSkill::BREAK_DAMAGE_PER_HIT;
            case GameConfig::Warrior::FireSkill::FIRE_ID:
                return GameConfig::Warrior::FireSkill::BREAK_DAMAGE;
            case GameConfig::Assassin::AllInSkill::ALL_IN_ID:
                return GameConfig::Assassin::AllInSkill::BREAK_DAMAGE;
            default:
                return 0;
            }
        };

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
                skill->breakDamage = std::max(0, resolveActiveSkillBreakDamage(skill->id, skillData.breakDamage));
                skill->currentCooldown = skillData.currentCooldown;

                skillComp->learnSkill(skill);
            }
        }

        // 恢复主动技能槽位
        const size_t requiredActiveSlots = static_cast<size_t>(GameSceneConfig::UI::SKILL_BAR_SLOT_COUNT);
        std::vector<std::shared_ptr<ActiveSkill>> activeSlots;
        activeSlots.reserve(requiredActiveSlots);
        for (size_t i = 0; i < requiredActiveSlots; ++i)
        {
            int skillId = -1;
            if (i < data.activeSlotSkillIds.size())
            {
                skillId = data.activeSlotSkillIds[i];
            }

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

        // 恢复被动技能（无槽位概念：存档里保存“已装备的被动技能 id 列表”）
        std::vector<std::shared_ptr<PassiveSkill>> passiveSlots;
        passiveSlots.reserve(data.passiveSlotSkillIds.size());
        std::unordered_set<int> passiveSeen;
        passiveSeen.reserve(data.passiveSlotSkillIds.size());
        for (size_t i = 0; i < data.passiveSlotSkillIds.size(); ++i)
        {
            int skillId = -1;
            skillId = data.passiveSlotSkillIds[i];
            if (skillId < 0)
            {
                continue; // 兼容旧存档里的 -1 占位
            }

            if (passiveSeen.find(skillId) != passiveSeen.end())
            {
                continue;
            }

            std::shared_ptr<Skill> skill = skillComp->findLearnedSkillById(skillId);
            std::shared_ptr<PassiveSkill> passiveSkill = std::dynamic_pointer_cast<PassiveSkill>(skill);
            if (passiveSkill)
            {
                passiveSeen.insert(skillId);
                passiveSlots.push_back(passiveSkill);
            }
        }
        skillComp->clearAndSetPassiveSlots(passiveSlots);
    }

    // 补齐默认测试物品（不重复加入），便于版本更新后直接验证新装备/被动机制
    player->ensureDefaultInventory();

    // 根据最终上限夹取 HP/MP
    player->setCurrentHP(savedHP);
    player->setCurrentMP(savedMP);

    // 恢复临时战斗状态（例如“孤注一掷”）
    if (data.outgoingDamageMultiplierRemainingSeconds > 0.0f &&
        data.outgoingDamageMultiplier > 0.0f)
    {
        player->activateOutgoingDamageMultiplier(data.outgoingDamageMultiplier,
                                                 data.outgoingDamageMultiplierRemainingSeconds);
    }
    else
    {
        // 兜底：确保存档恢复后不会残留异常倍率/特效
        player->activateOutgoingDamageMultiplier(1.0f, 0.0f);
    }

    CCLOG("SaveManager::applyPlayerData - 成功应用玩家数据");
}
