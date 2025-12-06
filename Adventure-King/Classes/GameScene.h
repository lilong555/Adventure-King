/**
 * @file GameScene.h
 * @brief 游戏关卡场景基类
 *
 * 定义了所有游戏关卡场景的通用功能，包括：
 * - 地图按钮（返回地图选择界面）
 * - 场景基础布局
 * - TMX 地图加载与碰撞检测
 */

#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "2d/CCTMXTiledMap.h"
#include "2d/CCTMXObjectGroup.h"

// 前向声明
class PlayerCharacter;
class GameUI;

// 物理碰撞分类掩码
enum GamePhysicsCategory
{
    GAME_CATEGORY_NONE = 0,
    GAME_CATEGORY_PLAYER = 1 << 0,    ///< 玩家
    GAME_CATEGORY_PLATFORM = 1 << 1,  ///< 平台/地面
    GAME_CATEGORY_COLLISION = 1 << 2, ///< 碰撞体（多边形）
    GAME_CATEGORY_ALL = 0xFFFFFFFF
};

class GameScene : public cocos2d::Scene
{
public:
    virtual bool init() override;
    static cocos2d::Scene *createScene();
    // 节点标签枚举
    enum NodeTags
    {
        TAG_MAP_BUTTON = 100,
        TAG_BACKGROUND = 101,
        TAG_TILEMAP = 102,
    };

protected:
    // TMX 瓦片地图
    cocos2d::TMXTiledMap *_tileMap = nullptr;
    // 碰撞对象组
    cocos2d::TMXObjectGroup *_collisionGroup = nullptr;
    // 玩家角色
    PlayerCharacter *_player = nullptr;
    // 地图尺寸（像素）
    cocos2d::Size _mapSizeInPixels;
    // 移动状态
    bool _isMovingLeft = false;
    bool _isMovingRight = false;
    float _moveSpeed = 350.0f;
    bool _isWalkAnimationPlaying = false;
    // 物理状态
    bool _isGrounded = false;
    int _groundContactCount = 0;
    // Gate（传送门）位置列表
    std::vector<cocos2d::Rect> _gateAreas;
    // 游戏 UI
    GameUI *_gameUI = nullptr;
    // 上一帧是否在传送门区域（用于显示/隐藏提示）
    bool _wasAtGate = false;
    // 常量
    static constexpr float JUMP_IMPULSE = 650.0f;
    static constexpr float GATE_INTERACT_DISTANCE = 100.0f; ///< 与传送门交互的距离

    /**
     * @brief 初始化游戏 UI
     */
    void initGameUI();

    /**
     * @brief 返回地图场景
     */
    void returnToMapScene();

    /**
     * @brief 设置场景背景（子类可重写）
     * @param backgroundPath 背景图片路径
     */
    virtual void setupBackground(const std::string &backgroundPath);

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
     * @brief 检查点是否与碰撞对象组发生碰撞
     * @param worldPos 世界坐标
     * @return 是否发生碰撞
     */
    bool checkCollision(const cocos2d::Vec2 &worldPos) const;

    /**
     * @brief 获取关卡名称（子类需重写）
     * @return 关卡名称字符串
     */
    virtual std::string getLevelName() const = 0;

    /**
     * @brief 设置横向重复背景
     * @param backgroundPath 背景图片路径
     * @param mapWidth 地图总宽度
     */
    virtual void setupRepeatingBackground(const std::string &backgroundPath, float mapWidth);

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
     * @brief 从TMX对象组创建多边形碰撞体
     * @param groupName 对象组名称
     */
    virtual void createCollisionBodiesFromTMX(const std::string &groupName);

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

    /**
     * @brief 物理碰撞开始回调
     */
    virtual bool onContactBegin(cocos2d::PhysicsContact &contact);

    /**
     * @brief 物理碰撞分离回调
     */
    virtual void onContactSeparate(cocos2d::PhysicsContact &contact);

    /**
     * @brief 键盘按下回调
     */
    virtual void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);

    /**
     * @brief 键盘释放回调
     */
    virtual void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);

    /**
     * @brief 帧更新
     */
    virtual void update(float dt) override;
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
};

#endif // __GAME_SCENE_H__
