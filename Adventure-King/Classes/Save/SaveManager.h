#pragma once

#include "SaveData.h"
#include "Character/Base/CharacterData.h"
#include "Configs/GameConfig.h"
#include "Configs/GameSceneConfig.h"
#include "cocos2d.h"
#include <chrono>
#include <functional>
#include <string>
#include <vector>

// 前向声明
class PlayerCharacter;

/**
 * 存档管理器（单例）
 * 负责游戏存档的保存、加载、删除等操作
 */
class SaveManager
{
public:
    // 获取全局单例
    static SaveManager *getInstance();
    // 销毁单例（程序退出时调用）
    static void destroyInstance();

    // 存档槽位数量
    static constexpr int MAX_SAVE_SLOTS = GameSceneConfig::Save::MAX_SLOTS;
    static constexpr int AUTO_SAVE_SLOT = MAX_SAVE_SLOTS - 1; // 预留自动存档槽

    //================== 核心存档操作 ==================

    /**
     * 保存游戏到指定槽位
     * @param slotIndex 存档槽位索引 (0-4)
     * @param player 玩家角色指针
     * @param sceneName 当前场景名称
     * @param playerPos 玩家位置
     * @return 成功返回 true，失败返回 false
     */
    bool saveGame(int slotIndex, PlayerCharacter *player,
                  const std::string &sceneName, const cocos2d::Vec2 &playerPos);

    /**
     * 从指定槽位加载游戏
     * @param slotIndex 存档槽位索引 (0-4)
     * @param outData 输出的存档数据
     * @return 成功返回 true，失败返回 false
     */
    bool loadGame(int slotIndex, SaveSlotData &outData);

    /**
     * 删除指定槽位的存档
     * @param slotIndex 存档槽位索引 (0-4)
     * @return 成功返回 true，失败返回 false
     */
    bool deleteSave(int slotIndex);

    /**
     * 检查指定槽位是否有存档
     * @param slotIndex 存档槽位索引 (0-4)
     * @return 有存档返回 true，否则返回 false
     */
    bool hasSave(int slotIndex) const;

    /**
     * 获取所有存档槽位的信息（仅元数据，不包含完整数据）
     * @return 存档槽位信息列表
     */
    std::vector<SaveSlotData> getAllSaveSlotInfos() const;

    //================== 自动存档 ==================

    /**
     * 设置是否启用自动存档
     * @param enabled 是否启用
     */
    void setAutoSaveEnabled(bool enabled);

    /**
     * 设置自动存档间隔（秒）
     * @param seconds 间隔秒数
     */
    void setAutoSaveInterval(float seconds);

    /**
     * 执行自动存档（通常在 GameScene 的 update 中调用）
     * @param player 玩家角色指针
     * @param sceneName 当前场景名称
     * @param playerPos 玩家位置
     */
    void performAutoSave(PlayerCharacter *player,
                         const std::string &sceneName, const cocos2d::Vec2 &playerPos);

    /**
     * 重置自动存档计时器
     */
    void resetAutoSaveTimer();

    //================== 设置管理 ==================

    /**
     * 保存设置
     * @param settings 设置数据
     * @return 成功返回 true，失败返回 false
     */
    bool saveSettings(const SettingsSaveData &settings);

    /**
     * 加载设置
     * @param outSettings 输出的设置数据
     * @return 成功返回 true，失败返回 false
     */
    bool loadSettings(SettingsSaveData &outSettings);

    //================== 数据转换 ==================

    /**
     * 从 PlayerCharacter 提取存档数据
     * @param player 玩家角色指针
     * @return 玩家存档数据
     */
    PlayerSaveData extractPlayerData(PlayerCharacter *player) const;

    /**
     * 将存档数据应用到 PlayerCharacter
     * @param player 玩家角色指针
     * @param data 玩家存档数据
     */
    void applyPlayerData(PlayerCharacter *player, const PlayerSaveData &data) const;

    //================== 运行时进度（不落盘） ==================

    /**
     * 缓存当前玩家数据（用于关卡 <-> 地图切换时保持等级/经验/装备等）
     * @note 不会写入磁盘，仅保存在内存中
     */
    void cacheRuntimePlayerData(PlayerCharacter *player);

    /**
     * 直接设置运行时玩家数据（例如从读档结果写入）
     */
    void setRuntimePlayerData(const PlayerSaveData &data);

    /**
     * 当前是否存在运行时玩家数据
     */
    bool hasRuntimePlayerData() const { return _hasRuntimePlayerData; }

    /**
     * 获取运行时玩家数据（调用前请先判断 hasRuntimePlayerData）
     */
    const PlayerSaveData &getRuntimePlayerData() const { return _runtimePlayerData; }

    /**
     * 清空运行时玩家数据（例如返回主菜单重新开始时使用）
     */
    void clearRuntimePlayerData();

    //================== 会话角色选择（不落盘） ==================

    /**
     * 设置本次会话“新开局”的职业选择（不落盘）
     * @note 与 runtimePlayerData 不同：该字段只用于“创建玩家实例时决定职业/贴图”，不会覆盖现有存档数据。
     */
    void setSessionSelectedRole(CharacterRole role);

    /**
     * 当前是否存在会话职业选择
     */
    bool hasSessionSelectedRole() const { return _hasSessionSelectedRole; }

    /**
     * 获取会话职业选择（调用前请先判断 hasSessionSelectedRole）
     */
    CharacterRole getSessionSelectedRole() const { return _sessionSelectedRole; }

    /**
     * 清空会话职业选择（例如返回主菜单后不再沿用上次选择）
     */
    void clearSessionSelectedRole();

    //================== 运行时位置（不落盘） ==================

    /**
     * 设置运行时玩家位置（用于“读档”后恢复到存档点）
     * @note 不会写入磁盘，仅保存在内存中
     */
    void setRuntimePlayerPosition(const cocos2d::Vec2& pos);

    /**
     * 当前是否存在运行时玩家位置
     */
    bool hasRuntimePlayerPosition() const { return _hasRuntimePlayerPosition; }

    /**
     * 获取运行时玩家位置（调用前请先判断 hasRuntimePlayerPosition）
     */
    cocos2d::Vec2 getRuntimePlayerPosition() const { return _runtimePlayerPosition; }

    /**
     * 清空运行时玩家位置（一般在新场景应用完成后调用）
     */
    void clearRuntimePlayerPosition();

private:
    // 私有构造，外部不可实例化
    SaveManager();
    // 私有析构，由 destroyInstance 释放
    ~SaveManager();

    static SaveManager *_instance;

    // 自动存档相关
    bool _autoSaveEnabled = false;
    float _autoSaveInterval = GameSceneConfig::Save::AUTO_SAVE_INTERVAL_SECONDS; // 默认 5 分钟
    float _autoSaveTimer = 0.0f;
    int _lastAutoSaveSlot = -1; // 上次自动存档的槽位

    // 运行时玩家数据：用于关卡切换时保持进度，不落盘
    bool _hasRuntimePlayerData = false;
    PlayerSaveData _runtimePlayerData;

    // 会话职业选择：用于“新开局”决定创建职业/贴图，不落盘
    bool _hasSessionSelectedRole = false;
    CharacterRole _sessionSelectedRole = CharacterRole::MAGE;

    // 运行时玩家位置：用于读档后恢复落点（只对下一次进入 GameScene 生效）
    bool _hasRuntimePlayerPosition = false;
    cocos2d::Vec2 _runtimePlayerPosition = cocos2d::Vec2::ZERO;

    // 构建存档文件路径
    std::string getSaveFilePath(int slotIndex) const;
    // 构建设置文件路径
    std::string getSettingsFilePath() const;
    // 写入文本到文件
    bool writeToFile(const std::string &filePath, const std::string &content);
    // 从文件读取文本
    bool readFromFile(const std::string &filePath, std::string &outContent) const;

    std::chrono::steady_clock::time_point _sessionStartTime;
};
