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
#include "Physics/GamePhysicsCategory.h"
#include <cstddef>
#include <string>
#include <vector>

// 前向声明
class PlayerCharacter;
class GameUI;
class MonsterBase;

// ============================================================
// 游戏对象结构体定义
// ============================================================

/**
 * @brief 炸弹数据结构（游戏场景专用）
 */

// ============================================================
// 场景配置结构体
// ============================================================

/**
 * @brief 关卡场景配置
 * @note 用于子类配置关卡参数，提高可扩展性
 */
struct LevelConfig
{
    // 资源路径
    std::string tmxMapPath;       ///< TMX 地图文件路径
    std::string backgroundPath;   ///< 背景图片路径
    std::string playerSpritePath; ///< 玩家精灵路径

    // 图层名称
    std::string collisionLayerName = "collisions"; ///< 碰撞图层名称
    std::string bornLayerName = "born";            ///< 出生点图层名称
    std::string gateLayerName = "gate";            ///< 传送门图层名称

    // 物理参数
    float gravity = -1000.0f;        ///< 重力加速度
    bool enablePhysicsDebug = false; ///< 是否开启物理调试绘制

    // 默认构造函数
    LevelConfig() = default;
};

/**
 * @brief 玩家配置参数
 */
struct PlayerConfig
{
    float scale = 0.25f;                  ///< 玩家缩放比例
    float walkSpeed = 220.0f;             ///< 行走速度
    float runSpeed = 350.0f;              ///< 跑步速度
    float jumpImpulse = 650.0f;           ///< 跳跃冲量
    int maxJumpCount = 2;                 ///< 最大跳跃次数（1=单跳，2=二段跳）
    float collisionBoxWidthRatio = 0.8f;  ///< 碰撞盒宽度比例
    float collisionBoxHeightRatio = 0.9f; ///< 碰撞盒高度比例
};

// ============================================================
// GameScene 基类
// ============================================================

class GameScene : public cocos2d::Scene
{
public:
    virtual bool init() override;
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
    // -------------------------------
    // 场景结构
    // -------------------------------
    cocos2d::Node *_gameLayer = nullptr; ///< 游戏内容层（受相机跟随影响）

    // -------------------------------
    // 场景资源
    // -------------------------------
    cocos2d::TMXTiledMap *_tileMap = nullptr;           ///< TMX 瓦片地图
    cocos2d::TMXObjectGroup *_collisionGroup = nullptr; ///< 碰撞对象组
    cocos2d::Size _mapSizeInPixels;                     ///< 地图尺寸（像素）

    // -------------------------------
    // 玩家相关
    // -------------------------------
    PlayerCharacter *_player = nullptr; ///< 玩家角色
    PlayerConfig _playerConfig;         ///< 玩家配置

    // -------------------------------
    // 移动状态
    // -------------------------------
    bool _isMovingLeft = false;           ///< 是否正在向左移动
    bool _isMovingRight = false;          ///< 是否正在向右移动
    bool _isRunPressed = false;           ///< 是否按下跑步键（Shift）

    // -------------------------------
    // 物理状态
    // -------------------------------
    bool _isGrounded = false;    ///< 是否站在地面上
    int _groundContactCount = 0; ///< 地面接触计数
    int _jumpCount = 0;          ///< 当前空中已跳次数（落地重置）

    // -------------------------------
    // 战斗状态
    // -------------------------------
    bool _isAttacking = false;    ///< 是否正在执行攻击动画
    bool _isCastingSkill = false; ///< 是否正在施放技能

    // -------------------------------
    // 游戏状态
    // -------------------------------
    bool _isPaused = false; ///< 游戏是否暂停

    // -------------------------------
    // 传送门系统
    // -------------------------------
    std::vector<cocos2d::Rect> _gateAreas; ///< Gate（传送门）位置列表
    bool _wasAtGate = false;               ///< 上一帧是否在传送门区域

    // -------------------------------
    // 敌人生成点系统（TMX objectgroup）
    // -------------------------------
    struct EnemySpawnPoint
    {
        cocos2d::Vec2 position;   ///< 生成点位置（世界坐标）
        std::string monsterType;  ///< 怪物类型（TMX object type）
        int count = 1;            ///< 生成数量（TMX object name）
        bool hasSpawned = false;  ///< 是否已触发生成
    };

    std::vector<EnemySpawnPoint> _enemySpawnPoints; ///< 关卡内所有生成点

    // -------------------------------
    // UI 相关
    // -------------------------------
    GameUI *_gameUI = nullptr; ///< 游戏 UI

    // -------------------------------
    // 常量定义
    // -------------------------------
    static constexpr float DEFAULT_GATE_INTERACT_DISTANCE = 100.0f; ///< 默认传送门交互距离
    static constexpr float GROUND_VELOCITY_THRESHOLD = 5.0f;        ///< 着地判断速度阈值
    static constexpr float FALLING_VELOCITY_THRESHOLD = -100.0f;    ///< 下落速度阈值
    static constexpr float GROUND_NORMAL_THRESHOLD = -0.3f;         ///< 地面法线阈值
    static constexpr float SCENE_TRANSITION_DURATION = 0.5f;        ///< 场景切换时间
    static constexpr int UI_Z_ORDER = 100;                          ///< UI 层级
    static constexpr int BACKGROUND_Z_ORDER = -1;                   ///< 背景层级
    static constexpr int PLAYER_Z_ORDER = 5;                        ///< 玩家层级
    static constexpr int COLLISION_DEBUG_Z_ORDER = 100;             ///< 碰撞调试层级

    // 技能1（火球）参数
    static constexpr size_t FIREBALL_SKILL_SLOT = 0;          ///< 技能1所在槽位索引
    static constexpr int FIREBALL_SKILL_ID = 1002;            ///< 技能1唯一ID
    static constexpr float FIREBALL_SKILL_MP_COST = 15.0f;    ///< 技能1 MP消耗
    static constexpr float FIREBALL_SKILL_COOLDOWN = 1.2f;    ///< 技能1冷却时间（秒）

    // ===================================================================
    // 初始化方法
    // ===================================================================

    /**
     * @brief 初始化游戏 UI
     */
    void initGameUI();

    /**
     * @brief 初始化带物理引擎的场景
     * @param config 关卡配置
     * @return 是否初始化成功
     */
    bool initWithPhysicsConfig(const LevelConfig &config);

    /**
     * @brief 初始化玩家角色
     * @param startPos 玩家起始位置
     */
    virtual void initPlayer(const cocos2d::Vec2 &startPos);

    /**
     * @brief 初始化物理碰撞监听器
     */
    virtual void initPhysicsContactListener();

    /**
     * @brief 初始化键盘输入监听
     */
    virtual void initKeyboardListener();

    /**
     * @brief 初始化相机跟随
     */
    virtual void initCameraFollow();

    /**
     * @brief 初始化玩家技能
     */
    virtual void initPlayerSkills();

    // ===================================================================
    // 资源加载方法
    // ===================================================================

    /**
     * @brief 加载 TMX 瓦片地图
     * @param mapPath TMX 文件路径
     * @return 是否加载成功
     */
    virtual bool loadTileMap(const std::string &mapPath);

    /**
     * @brief 加载碰撞对象组
     * @param groupName 碰撞对象组名称
     * @return 是否加载成功
     */
    virtual bool loadCollisionGroup(const std::string &groupName);

    /**
     * @brief 设置场景背景（子类可重写）
     * @param backgroundPath 背景图片路径
     */
    virtual void setupBackground(const std::string &backgroundPath);

    /**
     * @brief 设置横向重复背景
     * @param backgroundPath 背景图片路径
     * @param mapWidth 地图总宽度
     */
    virtual void setupRepeatingBackground(const std::string &backgroundPath, float mapWidth);

    /**
     * @brief 从TMX对象组创建多边形碰撞体
     * @param groupName 对象组名称
     */
    virtual void createCollisionBodiesFromTMX(const std::string &groupName);

    // ===================================================================
    // 传送门与出生点
    // ===================================================================

    /**
     * @brief 从 born 图层获取玩家出生点
     * @return 出生点坐标，如果找不到则返回默认位置
     */
    virtual cocos2d::Vec2 getPlayerSpawnPoint();

    /**
     * @brief 加载 gate 图层的传送门区域
     */
    virtual void loadGateAreas();

    /**
     * @brief 检查玩家是否在传送门区域内
     * @return 是否在传送门区域
     */
    virtual bool isPlayerAtGate() const;

    // ===================================================================
    // 碰撞检测
    // ===================================================================

    /**
     * @brief 检查点是否与碰撞对象组发生碰撞
     * @param worldPos 世界坐标
     * @return 是否发生碰撞
     */
    bool checkCollision(const cocos2d::Vec2 &worldPos) const;

    /**
     * @brief 物理碰撞开始回调
     */
    virtual bool onContactBegin(cocos2d::PhysicsContact &contact);

    /**
     * @brief 物理碰撞分离回调
     */
    virtual void onContactSeparate(cocos2d::PhysicsContact &contact);

    // ===================================================================
    // 输入处理
    // ===================================================================

    /**
     * @brief 键盘按下回调
     */
    virtual void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);

    /**
     * @brief 键盘释放回调
     */
    virtual void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);

    /**
     * @brief 处理跳跃输入
     */
    virtual void handleJump();

    /**
     * @brief 处理传送门交互
     * @return 是否触发了传送
     */
    virtual bool handleGateInteraction();

    // ===================================================================
    // 战斗系统
    // ===================================================================

    /**
     * @brief 攻击动画结束回调
     */
    virtual void onAttackAnimationFinished();

    /**
     * @brief 普通攻击：扔炸弹（入口）
     */
    virtual void throwBomb();

    // ===================================================================
    // 技能系统
    // ===================================================================

    /**
     * @brief 技能1：释放火球（入口）
     */
    virtual void castFireball();

    /**
     * @brief 技能动画结束回调（火球）
     */
    virtual void onFireballAnimationFinished();

    // 投掷物创建/爆炸逻辑已下沉到 PlayerCharacter

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

    // ===================================================================
    // 更新循环
    // ===================================================================

    /**
     * @brief 帧更新
     */
    virtual void update(float dt) override;

    /**
     * @brief 更新玩家移动
     * @param dt 帧间隔时间
     */
    virtual void updatePlayerMovement(float dt);

    /**
     * @brief 更新着地状态
     * @param velocity 当前速度
     */
    virtual void updateGroundedState(const cocos2d::Vec2 &velocity);

    /**
     * @brief 更新 UI 状态
     */
    virtual void updateUI();

    // ===================================================================
    // 敌人生成点
    // ===================================================================

    /**
     * @brief 从 TMX 对象组加载敌人生成点
     * @param groupName 对象组名称（默认 enemy_g）
     */
    virtual void loadEnemySpawnPoints(const std::string &groupName = "enemy_g");

    /**
     * @brief 每帧检测生成点是否进入可视范围，并触发一次生成
     */
    virtual void updateEnemySpawns();

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
    virtual MonsterBase *createMonsterByType(const std::string &monsterType) const;

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
    // ===================================================================
    // 内部辅助方法
    // ===================================================================

    /**
     * @brief 创建多边形碰撞体
     * @param vertices 顶点数组
     * @param name 碰撞体名称
     * @param isPolygon 是否为闭合多边形
     */
    void createPolygonCollisionBody(const std::vector<cocos2d::Vec2> &vertices,
                                    const std::string &name, bool isPolygon);

    /**
     * @brief 创建矩形碰撞体
     * @param rect 矩形区域
     * @param name 碰撞体名称
     */
    void createRectCollisionBody(const cocos2d::Rect &rect, const std::string &name);

    /**
     * @brief 解析 TMX 对象的顶点数据
     * @param dict 对象属性字典
     * @param objectX 对象 X 坐标
     * @param objectY 对象 Y 坐标
     * @param outVertices 输出顶点数组
     * @return 是否为多边形（false 表示折线）
     */
    bool parseTMXObjectVertices(const cocos2d::ValueMap &dict, double objectX, double objectY,
                                std::vector<cocos2d::Vec2> &outVertices);

    /**
     * @brief 显示地图加载失败的 UI
     */
    void showMapLoadFailedUI();
};

// ============================================================
// 起源之菇场景
// ============================================================

class OriginMushroomScene : public GameScene
{
public:
    static cocos2d::Scene *createScene();
    virtual bool init() override;
    CREATE_FUNC(OriginMushroomScene);

protected:
    virtual std::string getLevelName() const override { return "起源之菇"; }
    virtual LevelConfig getLevelConfig() const override;
};

// ============================================================
// 神秘之森场景
// ============================================================

class MysteryForestScene : public GameScene
{
public:
    static cocos2d::Scene *createScene();
    virtual bool init() override;
    CREATE_FUNC(MysteryForestScene);

protected:
    virtual std::string getLevelName() const override { return "神秘之森"; }
    virtual LevelConfig getLevelConfig() const override;
};

#endif // __GAME_SCENE_H__
