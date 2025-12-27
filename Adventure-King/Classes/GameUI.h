/**
 * @file GameUI.h
 * @brief 游戏内 UI 层
 *
 * 管理游戏过程中的所有 UI 元素，包括：
 * - 玩家状态栏（HP/MP/经验）
 * - 技能栏
 * - Boss血条
 * - 地图按钮
 * - 交互提示
 * - 暂停菜单
 *
 * UI 层不受相机跟随影响，始终显示在屏幕固定位置
 */

#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "cocos2d.h"

// 前向声明
class PlayerCharacter;
class CharacterBase;
class PlayerStatusBar;
class SkillBar;
class BossHealthBar;
class PauseMenu;
class PlayerDeathMenu;
class InventoryLayer;
class BlessingNpcLayer;

class GameUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建 GameUI 实例
     * @return GameUI 指针
     */
    static GameUI *create();

    /**
     * @brief 初始化
     */
    virtual bool init() override;

    //=========================================================================
    // 玩家相关
    //=========================================================================

    /**
     * @brief 绑定玩家角色
     * @param player 玩家角色指针
     */
    void bindPlayer(PlayerCharacter *player);

    /**
     * @brief 获取玩家状态栏
     */
    PlayerStatusBar *getPlayerStatusBar() const { return _playerStatusBar; }

    /**
     * @brief 获取技能栏
     */
    SkillBar *getSkillBar() const { return _skillBar; }

    //=========================================================================
    // Boss相关
    //=========================================================================

    /**
     * @brief 绑定Boss角色
     * @param boss Boss角色指针
     * @param bossName Boss名称
     * @param phaseCount 阶段数量
     */
    void bindBoss(CharacterBase *boss, const std::string &bossName, int phaseCount = 1);

    /**
     * @brief 解绑Boss
     */
    void unbindBoss();

    /**
     * @brief 获取Boss血条
     */
    BossHealthBar *getBossHealthBar() const { return _bossHealthBar; }

    //=========================================================================
    // 暂停菜单
    //=========================================================================

    /**
     * @brief 显示暂停菜单
     */
    void showPauseMenu();

    /**
     * @brief 隐藏暂停菜单
     */
    void hidePauseMenu();

    /**
     * @brief 暂停菜单是否显示中
     */
    bool isPauseMenuShowing() const;

    /**
     * @brief 获取暂停菜单
     */
    PauseMenu *getPauseMenu() const { return _pauseMenu; }

    //=========================================================================
    // 角色死亡菜单（强制暂停）
    //=========================================================================

    /// @brief 显示死亡菜单
    void showDeathMenu();
    /// @brief 隐藏死亡菜单
    void hideDeathMenu();
    /// @brief 死亡菜单是否显示中
    bool isDeathMenuShowing() const;
    /// @brief 获取死亡菜单
    PlayerDeathMenu *getDeathMenu() const { return _deathMenu; }

    //=========================================================================
    // 背包/技能
    //=========================================================================

    /// @brief 显示背包/技能界面
    void showInventory();
    /// @brief 隐藏背包/技能界面
    void hideInventory();
    /// @brief 背包/技能界面是否显示中
    bool isInventoryShowing() const;
    /// @brief 获取背包界面
    InventoryLayer *getInventoryLayer() const { return _inventoryLayer; }

    //=========================================================================
    // 赐福 NPC（展示接口）
    //=========================================================================

    /// @brief 显示赐福 NPC 弹窗
    void showBlessingNpc();
    /// @brief 隐藏赐福 NPC 弹窗
    void hideBlessingNpc();
    /// @brief 赐福 NPC 弹窗是否显示中
    bool isBlessingNpcShowing() const;
    /// @brief 获取赐福 NPC 弹窗
    BlessingNpcLayer *getBlessingNpcLayer() const { return _blessingNpcLayer; }

    //=========================================================================
    // 原有功能
    //=========================================================================

    /**
     * @brief 设置地图按钮回调
     * @param callback 点击回调函数
     */
    void setMapButtonCallback(const std::function<void()> &callback);

    /**
     * @brief 显示交互提示（如 "按 W 进入传送门"）
     * @param message 提示信息
     */
    void showInteractionHint(const std::string &message);

    /**
     * @brief 隐藏交互提示
     */
    void hideInteractionHint();

    /**
     * @brief 设置关卡名称
     * @param name 关卡名称
     */
    void setLevelName(const std::string &name);

    /**
     * @brief 更新 UI 位置（跟随相机）
     * @param cameraOffset 相机偏移量（场景位置的负值）
     */
    void updatePosition(const cocos2d::Vec2 &cameraOffset);

    /**
     * @brief 更新所有UI显示
     * 应在每帧调用
     */
    void updateDisplay();

protected:
    // 创建玩家状态栏 UI
    void createPlayerStatusBar();
    // 创建技能栏 UI
    void createSkillBar();
    // 创建 Boss 血条 UI
    void createBossHealthBar();
    // 创建暂停菜单 UI
    void createPauseMenu();
    // 创建角色死亡菜单 UI
    void createDeathMenu();
    // 创建背包/技能 UI
    void createInventoryLayer();
    // 创建赐福 NPC 弹窗
    void createBlessingNpcLayer();
    // 创建地图按钮 UI
    void createMapButton();
    // 创建交互提示 UI
    void createInteractionHint();
    // 创建关卡名称 UI
    void createLevelNameLabel();

    // 地图按钮点击回调
    void onMapButtonClicked(cocos2d::Ref *sender);

protected:
    // 玩家相关UI
    PlayerStatusBar *_playerStatusBar = nullptr;
    SkillBar *_skillBar = nullptr;
    PlayerCharacter *_player = nullptr;

    // Boss相关UI
    BossHealthBar *_bossHealthBar = nullptr;

    // 暂停菜单
    PauseMenu *_pauseMenu = nullptr;
    // 角色死亡菜单（强制暂停）
    PlayerDeathMenu *_deathMenu = nullptr;
    // 背包/技能界面
    InventoryLayer *_inventoryLayer = nullptr;
    // 赐福 NPC 弹窗
    BlessingNpcLayer *_blessingNpcLayer = nullptr;

    // 地图按钮
    cocos2d::MenuItemImage *_mapButton = nullptr;
    cocos2d::Menu *_mapMenu = nullptr;

    // 交互提示标签
    cocos2d::Label *_interactionHint = nullptr;

    // 关卡名称标签
    cocos2d::Label *_levelNameLabel = nullptr;

    // 地图按钮回调
    std::function<void()> _mapButtonCallback;

    // UI 元素的相对位置（相对于屏幕）
    cocos2d::Vec2 _mapButtonPos;
    cocos2d::Vec2 _interactionHintPos;
    cocos2d::Vec2 _levelNamePos;
    cocos2d::Vec2 _statusBarPos;
    cocos2d::Vec2 _skillBarPos;
    cocos2d::Vec2 _bossHealthBarPos;
};

#endif // __GAME_UI_H__
