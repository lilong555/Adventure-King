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

// 前向声明
class PlayerCharacter;
class GameUI;

// ============================================================
// 物理碰撞类型枚举
// ============================================================

/**
 * @brief 游戏场景物理碰撞分类掩码
 * @note 使用位掩码实现多类型碰撞检测
 *       命名为 GamePhysicsCategory 以避免与 DebugScene 中的定义冲突
 */
enum class GamePhysicsCategory : int
{
    NONE = 0,
    PLAYER = 1 << 0,    ///< 玩家
    PLATFORM = 1 << 1,  ///< 平台/地面
    COLLISION = 1 << 2, ///< 碰撞体（多边形）
    TRIGGER = 1 << 3,   ///< 触发器（不产生物理碰撞）
    ALL = 0xFFFFFFFF
};

// 位运算操作符重载，方便组合使用
inline GamePhysicsCategory operator|(GamePhysicsCategory a, GamePhysicsCategory b)
{
    return static_cast<GamePhysicsCategory>(static_cast<int>(a) | static_cast<int>(b));
}

inline int operator&(int a, GamePhysicsCategory b)
{
    return a & static_cast<int>(b);
}

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
    float scale = 0.5f;                   ///< 玩家缩放比例
    float moveSpeed = 350.0f;             ///< 移动速度
    float jumpImpulse = 650.0f;           ///< 跳跃冲量
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
    bool _isWalkAnimationPlaying = false; ///< 是否正在播放行走动画

    // -------------------------------
    // 物理状态
    // -------------------------------
    bool _isGrounded = false;    ///< 是否站在地面上
    int _groundContactCount = 0; ///< 地面接触计数

    // -------------------------------
    // 传送门系统
    // -------------------------------
    std::vector<cocos2d::Rect> _gateAreas; ///< Gate（传送门）位置列表
    bool _wasAtGate = false;               ///< 上一帧是否在传送门区域

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
    // 场景导航
    // ===================================================================

    /**
     * @brief 返回地图场景
     */
    void returnToMapScene();

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
