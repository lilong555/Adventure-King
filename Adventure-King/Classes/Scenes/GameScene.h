/**
 * @file GameScene.h
 * @brief 游戏关卡场景基类
 *
 * 定义了所有游戏关卡场景的通用功能，包括：
 * - 地图按钮（返回地图选择界面）
 * - 场景基础布局
 * - TMX 地图加载与碰撞检测
 * - 玩家控制与物理交互
 * - 传送门系统
 */

#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "2d/CCTMXTiledMap.h"
#include "2d/CCTMXObjectGroup.h"
#include "Configs/GamePhysicsCategory.h"
#include "Configs/GameConfig.h"
#include "Configs/GameSceneConfig.h"
#include "Scenes/GameInputController.h"
#include "Scenes/GameUIController.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// 前向声明
class PlayerCharacter;
class MonsterBase;
class LevelMap;

// ============================================================
// GameScene 基类
// ============================================================

class GameScene : public cocos2d::Scene
{
public:
    /**
     * @brief 初始化基础场景（入口）
     */
    virtual bool init() override;
    /// @brief 场景进入：进入游戏时禁用输入法（IME），避免抢输入
    virtual void onEnter() override;
    /// @brief 场景退出：离开游戏时恢复输入法（IME）
    virtual void onExit() override;
    /**
     * @brief 析构：释放场景资源
     */
    virtual ~GameScene();
    /**
     * @brief 创建 GameScene 场景
     */
    static cocos2d::Scene *createScene();

    /**
     * @brief 获取玩家角色指针
     * @return 玩家角色指针，如果不存在则返回 nullptr
     */
    PlayerCharacter *getPlayer() const { return _player; }

    // -------------------------------
    // 节点标签枚举
    // -------------------------------
    enum NodeTags
    {
        TAG_MAP_BUTTON = 100,
        TAG_BACKGROUND = 101,
        TAG_TILEMAP = 102,
        TAG_PLAYER = 103,
        TAG_COLLISION_DEBUG = 104,
    };

protected:
    /**
     * @brief 构造：由派生类 create 调用
     */
    GameScene();

    // -------------------------------
    // 场景结构
    // -------------------------------
    cocos2d::Node *_gameLayer = nullptr; ///< 游戏内容层（受相机跟随影响）

    // -------------------------------
    // 地图系统
    // -------------------------------
    std::unique_ptr<LevelMap> _levelMap; ///< 关卡地图封装

    // -------------------------------
    // 玩家相关
    // -------------------------------
    PlayerCharacter *_player = nullptr; ///< 玩家角色
    //PlayerConfig _playerConfig;         ///< 玩家配置
    MonsterBase *_boss = nullptr; ///< Boss 角色

    // -------------------------------
    // 输入系统
    // -------------------------------
    std::unique_ptr<GameInputController> _inputController;

    // -------------------------------
    // 游戏状态
    // -------------------------------
    bool _isPaused = false; ///< 游戏是否暂停
    bool _cachedPhysicsAutoStep = true; ///< 暂停前物理世界 autoStep 状态
    float _cachedPhysicsSpeed = 1.0f;   ///< 暂停前物理世界 speed 值

    // -------------------------------
    // UI 系统
    // -------------------------------
    std::unique_ptr<GameUIController> _uiController;

    // -------------------------------
    // 常量定义
    // -------------------------------
    static constexpr float SCENE_TRANSITION_DURATION = GameSceneConfig::Scene::TRANSITION_DURATION;        ///< 场景切换时间
    static constexpr int UI_Z_ORDER = GameSceneConfig::UI::Z_ORDER;                                       ///< UI 层级
    static constexpr int BACKGROUND_Z_ORDER = GameSceneConfig::Scene::BACKGROUND_Z_ORDER;                 ///< 背景层级
    static constexpr int PLAYER_Z_ORDER = GameSceneConfig::Scene::PLAYER_Z_ORDER;                         ///< 玩家层级
    static constexpr int COLLISION_DEBUG_Z_ORDER = GameSceneConfig::Scene::COLLISION_DEBUG_Z_ORDER;       ///< 碰撞调试层级

    // ===================================================================
    // 初始化方法
    // ===================================================================

    /**
     * @brief 初始化带物理引擎的场景
     * @param config 关卡配置
     * @return 是否初始化成功
     */
    bool initWithPhysicsConfig(const LevelConfig &config);

    /**
     * @brief 初始化玩家角色
     * @param startPos 玩家起始位置
     * @param playerSpritePath 玩家贴图路径（为空则使用默认）
     */
    void initPlayer(const cocos2d::Vec2& startPos);

    /**
     * @brief 初始化物理碰撞监听器
     */
    virtual void initPhysicsContactListener();

    /**
     * @brief 初始化输入系统
     */
    virtual void initInputController();

    /**
     * @brief 初始化相机跟随
     */
    virtual void initCameraFollow();

    /**
     * @brief 初始化 UI 系统
     */
    virtual void initUIController();

    // ===================================================================
    // 资源加载方法
    // ===================================================================

    /**
     * @brief 创建并加载关卡地图（TMX/碰撞/门区/敌人点）
     */
    virtual bool initLevelMap(const LevelConfig &config);

    /**
     * @brief 物理碰撞开始回调
     */
    virtual bool onContactBegin(cocos2d::PhysicsContact &contact);

    /**
     * @brief 物理碰撞分离回调
     */
    virtual void onContactSeparate(cocos2d::PhysicsContact &contact);

    // ===================================================================
    // 场景导航
    // ===================================================================

    /**
     * @brief 返回地图场景
     */
    void returnToMapScene();

    /**
     * @brief 切换暂停菜单显示状态
     */
    void togglePauseMenu();

    /**
     * @brief 设置游戏暂停/恢复（只冻结世界逻辑，UI 仍可交互）
     */
    void setGamePaused(bool paused);

    // ===================================================================
    // 更新循环
    // ===================================================================

    /**
     * @brief 帧更新
     */
    virtual void update(float dt) override;

    /**
     * @brief 获取生成点触发距离（水平）
     * @note 默认为屏幕半宽，视为“进入视野”
     */
    virtual float getEnemySpawnViewDistance() const;

    /**
     * @brief 根据类型创建怪物实例
     * @param monsterType TMX object type
     * @return 创建成功返回怪物指针，否则返回 nullptr
     */
    virtual MonsterBase *createMonsterByType(const std::string &monsterType);

    // ===================================================================
    // 抽象方法（子类必须实现）
    // ===================================================================

    /**
     * @brief 获取关卡名称（子类需重写）
     * @return 关卡名称字符串
     */
    virtual std::string getLevelName() const = 0;

    /**
     * @brief 获取关卡配置（子类可重写）
     * @return 关卡配置结构体
     */
    virtual LevelConfig getLevelConfig() const { return LevelConfig(); }

private:
    /**
     * @brief 显示地图加载失败的 UI
     */
    void showMapLoadFailedUI();
};

#endif // __GAME_SCENE_H__
