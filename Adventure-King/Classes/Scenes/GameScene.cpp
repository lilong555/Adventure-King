/**
 * @file GameScene.cpp
 * @brief 游戏关卡场景实现
 *
 * 包含 GameScene 基类和各个关卡子类的实现
 */

#include "GameScene.h"
#include "MapScene.h"
#include "HelloWorldScene.h"
#include "Character/Base/CharacterBase.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/components/SkillComponent.h"
#include "GameUI.h"
#include "UI/PauseMenu.h"
#include "Scenes/Layers/SaveMenuLayer.h"
#include "Save/SaveManager.h"
#include "Save/SaveData.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <vector>

USING_NS_CC;

// ============================================================
// 常量定义
// ============================================================
namespace
{
    // 资源路径
    const char *const DEFAULT_FONT_PATH = "fonts/ZCOOLKuaiLe-Regular.ttf";
    const char *const DEFAULT_PLAYER_SPRITE = "Sprites/Characters/Player/Klee/defalt/spr_klee_run.png";

    // 默认值
    const Vec2 DEFAULT_SPAWN_POINT(100.0f, 200.0f);

    // 物理材质
    const PhysicsMaterial PLAYER_PHYSICS_MATERIAL(1.0f, 0.0f, 0.0f);    // 密度, 弹性, 摩擦
    const PhysicsMaterial COLLISION_PHYSICS_MATERIAL(1.0f, 0.0f, 0.8f); // 碰撞体材质

    // UI 文本
    const char *const GATE_INTERACTION_HINT = "Press W to enter gate";
    const char *const MAP_LOAD_FAILED_TEXT = " - Map Load Failed";

    // 辅助函数：获取物理类别位掩码
    inline int getCategoryBitmask(GamePhysicsCategory category)
    {
        return static_cast<int>(category);
    }
}

// ============================================================
// GameScene 基类实现
// ============================================================

bool GameScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    // 基类不初始化 UI，由子类调用 initGameUI()
    return true;
}

// ===================================================================
// 初始化方法
// ===================================================================

void GameScene::initGameUI()
{
    _gameUI = GameUI::create();
    if (_gameUI)
    {
        // 设置地图按钮回调
        _gameUI->setMapButtonCallback([this]()
                                      { returnToMapScene(); });

        // 设置关卡名称
        _gameUI->setLevelName(getLevelName());

        // 设置暂停菜单回调
        auto pauseMenu = _gameUI->getPauseMenu();
        if (pauseMenu)
        {
            // 继续游戏
            pauseMenu->setResumeCallback([this]()
                                         {
                _isPaused = false;
                CCLOG("Game resumed from pause menu"); });

            // 返回主菜单
            pauseMenu->setMainMenuCallback([]()
                                           {
                CCLOG("=== Main menu callback START ===");

                auto director = Director::getInstance();
                CCLOG("Step 1: Director obtained");

                // 先清空场景栈到根场景
                director->popToRootScene();
                CCLOG("Step 2: popToRootScene completed");

                // 跳转到主菜单场景
                auto mainMenuScene = HelloWorld::createScene();
                CCLOG("Step 3: MainMenuScene created: %p", mainMenuScene);

                if (mainMenuScene)
                {
                    auto transition = TransitionFade::create(0.5f, mainMenuScene, Color3B::BLACK);
                    CCLOG("Step 4: Transition created, calling replaceScene");
                    director->replaceScene(transition);
                    CCLOG("Step 5: replaceScene completed");
                }
                else
                {
                    CCLOG("Error: Failed to create main menu scene!");
                }

                CCLOG("=== Main menu callback END ==="); });

            // 保存游戏
            pauseMenu->setSaveCallback([this]()
                                       {
                // 隐藏暂停菜单
                _gameUI->hidePauseMenu();
                _isPaused = false;

                // 创建保存模式的存档菜单
                auto saveMenu = SaveMenuLayer::create(
                    SaveMenuLayer::Mode::SAVE,
                    _player,
                    getLevelName(),
                    _player ? _player->getPosition() : Vec2::ZERO
                );
                if (saveMenu)
                {
                    this->addChild(saveMenu, UI_Z_ORDER + 1);
                }
                CCLOG("GameScene - 打开保存游戏菜单"); });

            // 加载游戏
            pauseMenu->setLoadCallback([this]()
                                       {
                // 隐藏暂停菜单
                _gameUI->hidePauseMenu();
                _isPaused = false;

                // 创建加载模式的存档菜单
                auto saveMenu = SaveMenuLayer::create(SaveMenuLayer::Mode::LOAD);
                if (saveMenu)
                {
                    saveMenu->setLoadSuccessCallback([](const SaveSlotData& saveData)
                    {
                        CCLOG("GameScene - 加载存档成功，场景: %s", saveData.progressData.currentSceneName.c_str());

                        // 根据存档的场景名称创建对应的场景
                        Scene* targetScene = nullptr;
                        const std::string& sceneName = saveData.progressData.currentSceneName;

                        if (sceneName == "起源之菇")
                        {
                            targetScene = OriginMushroomScene::createScene();
                        }
                        else if (sceneName == "神秘之森")
                        {
                            targetScene = MysteryForestScene::createScene();
                        }
                        else
                        {
                            CCLOG("GameScene - 未知的场景名称: %s", sceneName.c_str());
                            return;
                        }

                        if (targetScene)
                        {
                            auto gameScene = dynamic_cast<GameScene*>(targetScene);
                            if (gameScene)
                            {
                                auto saveManager = SaveManager::getInstance();
                                auto playerData = saveData.playerData;
                                auto playerPos = Vec2(saveData.progressData.playerPosX, saveData.progressData.playerPosY);

                                gameScene->scheduleOnce([saveManager, playerData, playerPos](float dt) {
                                    auto currentScene = Director::getInstance()->getRunningScene();
                                    auto currentGameScene = dynamic_cast<GameScene*>(currentScene);
                                    if (currentGameScene)
                                    {
                                        auto player = currentGameScene->getPlayer();
                                        if (player)
                                        {
                                            saveManager->applyPlayerData(player, playerData);
                                            player->setPosition(playerPos);
                                            CCLOG("GameScene - 玩家数据已恢复，位置: (%.1f, %.1f)", playerPos.x, playerPos.y);
                                        }
                                    }
                                }, 0.1f, "apply_save_data");
                            }

                            auto transition = TransitionFade::create(0.5f, targetScene, Color3B::BLACK);
                            Director::getInstance()->replaceScene(transition);
                        }
                    });
                    this->addChild(saveMenu, UI_Z_ORDER + 1);
                }
                CCLOG("GameScene - 打开加载游戏菜单"); });
        }

        // 绑定玩家到 UI
        if (_player)
        {
            _gameUI->bindPlayer(_player);
        }

        // 添加到场景（高 z-order 确保显示在最上层）
        this->addChild(_gameUI, UI_Z_ORDER);

        CCLOG("GameUI initialized for level: %s", getLevelName().c_str());
    }
    else
    {
        CCLOG("Warning: Failed to create GameUI");
    }
}

bool GameScene::initWithPhysicsConfig(const LevelConfig &config)
{
    //-------------------------------------------------------------------------
    // 步骤1：物理引擎初始化
    //-------------------------------------------------------------------------
    if (!Scene::initWithPhysics())
    {
        CCLOG("Error: Failed to initialize physics scene");
        return false;
    }

    // 配置物理世界参数
    auto physicsWorld = this->getPhysicsWorld();
    physicsWorld->setGravity(Vec2(0, config.gravity));

    // 开启/关闭物理调试绘制
    if (config.enablePhysicsDebug)
    {
        physicsWorld->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    //-------------------------------------------------------------------------
    // 步骤1.5：创建游戏内容层（用于相机跟随）
    //-------------------------------------------------------------------------
    _gameLayer = Node::create();
    this->addChild(_gameLayer, 0);

    //-------------------------------------------------------------------------
    // 步骤2：加载 TMX 瓦片地图
    //-------------------------------------------------------------------------
    if (!loadTileMap(config.tmxMapPath))
    {
        CCLOG("Warning: Failed to load TMX map: %s", config.tmxMapPath.c_str());
        showMapLoadFailedUI();
        return true; // 返回 true 让场景仍然显示，只是没有地图
    }

    //-------------------------------------------------------------------------
    // 步骤3：设置横向重复背景
    //-------------------------------------------------------------------------
    if (!config.backgroundPath.empty())
    {
        setupRepeatingBackground(config.backgroundPath, _mapSizeInPixels.width);
    }

    //-------------------------------------------------------------------------
    // 步骤4：从碰撞图层创建物理碰撞体
    //-------------------------------------------------------------------------
    createCollisionBodiesFromTMX(config.collisionLayerName);

    //-------------------------------------------------------------------------
    // 步骤5：加载传送门区域
    //-------------------------------------------------------------------------
    loadGateAreas();

    //-------------------------------------------------------------------------
    // 步骤6：初始化玩家角色（从 born 图层获取出生点）
    //-------------------------------------------------------------------------
    Vec2 playerStartPos = getPlayerSpawnPoint();
    initPlayer(playerStartPos);

    //-------------------------------------------------------------------------
    // 步骤6.5：加载敌人生成点（enemy_g）
    //-------------------------------------------------------------------------
    loadEnemySpawnPoints();
    // 处理“出生点一开始就在视野内”的情况
    updateEnemySpawns();

    //-------------------------------------------------------------------------
    // 步骤7：初始化物理碰撞监听和输入
    //-------------------------------------------------------------------------
    initPhysicsContactListener();
    initKeyboardListener();

    //-------------------------------------------------------------------------
    // 步骤8：设置相机跟随
    //-------------------------------------------------------------------------
    initCameraFollow();

    //-------------------------------------------------------------------------
    // 步骤9：启用帧更新和初始化 UI
    //-------------------------------------------------------------------------
    scheduleUpdate();
    initGameUI();

    CCLOG("Scene initialized with physics config: %s", getLevelName().c_str());
    return true;
}

void GameScene::initPlayer(const Vec2 &startPos)
{
    // 创建玩家角色（战士职业）
    auto playerSprite = PlayerCharacter::create(CharacterRole::WARRIOR, DEFAULT_PLAYER_SPRITE);

    if (!playerSprite)
    {
        CCLOG("Error: Failed to create player character");
        return;
    }

    // 获取原始尺寸并设置缩放
    Size originalSize = playerSprite->getContentSize();
    float scale = _playerConfig.scale;
    playerSprite->setScale(scale);

    // 计算缩放后的实际显示尺寸（仅用于摆放位置）
    float scaledHeight = originalSize.height * scale;

    // 锚点设置为中心
    playerSprite->setAnchorPoint(Vec2(0.5f, 0.5f));

    // 设置位置：startPos 是地面位置，需要将玩家中心抬高半个身高
    Vec2 playerPos = startPos + Vec2(0, scaledHeight / 2);
    playerSprite->setPosition(playerPos);

    // 计算碰撞体尺寸（基于配置的比例）
    float boxWidth = originalSize.width * _playerConfig.collisionBoxWidthRatio;
    float boxHeight = originalSize.height * _playerConfig.collisionBoxHeightRatio;

    // 创建物理体
    auto physicsBody = PhysicsBody::createBox(Size(boxWidth, boxHeight), PLAYER_PHYSICS_MATERIAL);
    physicsBody->setDynamic(true);
    physicsBody->setRotationEnable(false);
    physicsBody->setMass(1.0f);
    physicsBody->setLinearDamping(0.0f);

    // 配置碰撞掩码
    physicsBody->setCategoryBitmask(getCategoryBitmask(GamePhysicsCategory::PLAYER));
    // 仅与地形发生物理碰撞；同时需要接收怪物攻击判定框的 Contact 回调用于结算伤害
    physicsBody->setCollisionBitmask(getCategoryBitmask(
        GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER_ATTACK));
    physicsBody->setContactTestBitmask(getCategoryBitmask(
        GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER_ATTACK));

    // 添加物理体到精灵
    playerSprite->addComponent(physicsBody);

    // 保存引用并添加到游戏内容层
    _player = playerSprite;
    _player->setTag(TAG_PLAYER);
    _gameLayer->addChild(_player, PLAYER_Z_ORDER);

    // 设置死亡时不自动移除
    _player->setAutoRemoveOnDeath(false);

    // 初始化地面状态
    _isGrounded = true;
    _groundContactCount = 1;
    _jumpCount = 0;

    // 初始化玩家技能
    initPlayerSkills();

    CCLOG("Player created: pos=(%.0f, %.0f), boxSize=(%.0f, %.0f)",
          playerPos.x, playerPos.y, boxWidth, boxHeight);
}

/**
 * @brief 初始化玩家技能
 *
 * 通过 SkillComponent 创建并装备主动技能。
 * 当前实现了技能1：火球。
 */
void GameScene::initPlayerSkills()
{
    if (!_player)
        return;

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        CCLOG("Failed to get skill component");
        return;
    }

    // 技能1：火球
    auto fireballSkill = std::make_shared<ActiveSkill>();
    fireballSkill->id = FIREBALL_SKILL_ID;
    fireballSkill->name = "火球";
    fireballSkill->description = "发射火球，命中后爆炸造成范围伤害";
    fireballSkill->manaCost = FIREBALL_SKILL_MP_COST;
    fireballSkill->cooldown = FIREBALL_SKILL_COOLDOWN;
    fireballSkill->currentCooldown = 0.0f;

    // 学习并装备技能
    skillComp->learnSkill(fireballSkill);
    skillComp->equipActiveSkill(fireballSkill, FIREBALL_SKILL_SLOT);

    CCLOG("Player skills initialized: Fireball equipped to slot %zu", FIREBALL_SKILL_SLOT);
}

void GameScene::initPhysicsContactListener()
{
    auto contactListener = EventListenerPhysicsContact::create();

    // 碰撞开始回调
    contactListener->onContactBegin = CC_CALLBACK_1(GameScene::onContactBegin, this);

    // 碰撞预处理回调 - 设置碰撞参数
    contactListener->onContactPreSolve = [](PhysicsContact &contact, PhysicsContactPreSolve &solve) -> bool
    {
        auto nodeA = contact.getShapeA()->getBody()->getNode();
        auto nodeB = contact.getShapeB()->getBody()->getNode();

        if (!nodeA || !nodeB)
            return true;

        int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
        int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

        // 玩家与碰撞体碰撞时，禁用弹性和摩擦
        bool playerInvolved = (categoryA & GamePhysicsCategory::PLAYER) || (categoryB & GamePhysicsCategory::PLAYER);
        bool platformInvolved = (categoryA & GamePhysicsCategory::PLATFORM) || (categoryB & GamePhysicsCategory::PLATFORM) ||
                                (categoryA & GamePhysicsCategory::COLLISION) || (categoryB & GamePhysicsCategory::COLLISION);

        if (playerInvolved && platformInvolved)
        {
            solve.setRestitution(0.0f);
            solve.setFriction(0.0f);
        }

        return true;
    };

    // 碰撞分离回调
    contactListener->onContactSeparate = CC_CALLBACK_1(GameScene::onContactSeparate, this);

    _eventDispatcher->addEventListenerWithSceneGraphPriority(contactListener, this);

    CCLOG("Physics contact listener initialized");
}

void GameScene::initKeyboardListener()
{
    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = CC_CALLBACK_2(GameScene::onKeyPressed, this);
    keyboardListener->onKeyReleased = CC_CALLBACK_2(GameScene::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    CCLOG("Keyboard listener initialized");
}

void GameScene::initCameraFollow()
{
    if (!_player || !_gameLayer)
    {
        CCLOG("Warning: Cannot init camera follow - player or gameLayer not created");
        return;
    }

    // 定义地图边界（世界坐标）
    Rect worldBound(0, 0, _mapSizeInPixels.width, _mapSizeInPixels.height);

    // 让游戏内容层跟随玩家移动（而不是整个场景）
    // 这样 UI 层不会受到影响
    auto followAction = Follow::create(_player, worldBound);
    _gameLayer->runAction(followAction);

    CCLOG("Camera follow enabled on gameLayer, world bound: (%.0f, %.0f, %.0f, %.0f)",
          worldBound.origin.x, worldBound.origin.y,
          worldBound.size.width, worldBound.size.height);
}

// ===================================================================
// 场景导航
// ===================================================================

void GameScene::returnToMapScene()
{
    CCLOG("Returning to map scene from: %s", getLevelName().c_str());

    // 创建地图场景并切换
    auto mapScene = MapScene::createScene();
    if (mapScene)
    {
        auto transition = TransitionFade::create(SCENE_TRANSITION_DURATION, mapScene, Color3B::BLACK);
        Director::getInstance()->pushScene(transition);
    }
    else
    {
        CCLOG("Error: Failed to create map scene");
    }
}

void GameScene::togglePauseMenu()
{
    if (!_gameUI)
        return;

    if (_gameUI->isPauseMenuShowing())
    {
        _gameUI->hidePauseMenu();
        _isPaused = false;
        CCLOG("Game resumed");
    }
    else
    {
        _gameUI->showPauseMenu();
        _isPaused = true;
        CCLOG("Game paused");
    }
}

// ===================================================================
// 资源加载方法
// ===================================================================

void GameScene::setupBackground(const std::string &backgroundPath)
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    auto background = Sprite::create(backgroundPath);
    if (background)
    {
        background->setPosition(center);

        // 缩放背景以填满屏幕
        Size textureSize = background->getContentSize();
        float scaleX = visibleSize.width / textureSize.width;
        float scaleY = visibleSize.height / textureSize.height;
        float scaleFactor = std::max(scaleX, scaleY);
        background->setScale(scaleFactor);

        background->setTag(TAG_BACKGROUND);
        // 将背景添加到游戏内容层
        _gameLayer->addChild(background, BACKGROUND_Z_ORDER);

        CCLOG("Background loaded: %s", backgroundPath.c_str());
    }
    else
    {
        CCLOG("Warning: Failed to load background: %s", backgroundPath.c_str());
    }
}

bool GameScene::loadTileMap(const std::string &mapPath)
{
    _tileMap = TMXTiledMap::create(mapPath);
    if (!_tileMap)
    {
        CCLOG("Error: Failed to load TMX map: %s", mapPath.c_str());
        return false;
    }

    auto origin = Director::getInstance()->getVisibleOrigin();

    // 计算地图像素尺寸
    Size mapTileSize = _tileMap->getMapSize();
    Size tileSize = _tileMap->getTileSize();
    _mapSizeInPixels = Size(mapTileSize.width * tileSize.width,
                            mapTileSize.height * tileSize.height);

    CCLOG("Map loaded: %s, tiles=%.0fx%.0f, pixels=%.0fx%.0f",
          mapPath.c_str(), mapTileSize.width, mapTileSize.height,
          _mapSizeInPixels.width, _mapSizeInPixels.height);

    // 设置地图锚点为左下角，与屏幕左下角对齐
    _tileMap->setAnchorPoint(Vec2(0, 0));
    _tileMap->setPosition(Vec2(origin.x, origin.y));

    _tileMap->setTag(TAG_TILEMAP);
    // 将地图添加到游戏内容层
    _gameLayer->addChild(_tileMap, 0);
    return true;
}

bool GameScene::loadCollisionGroup(const std::string &groupName)
{
    if (!_tileMap)
    {
        CCLOG("Error: Cannot load collision group - tilemap not loaded");
        return false;
    }

    _collisionGroup = _tileMap->getObjectGroup(groupName);
    if (!_collisionGroup)
    {
        CCLOG("Warning: Collision group '%s' not found in tilemap", groupName.c_str());
        return false;
    }

    // 打印碰撞对象信息（调试用）
    auto objects = _collisionGroup->getObjects();
    CCLOG("Collision group '%s' contains %zu objects", groupName.c_str(), objects.size());

#if COCOS2D_DEBUG > 0
    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        CCLOG("  Object: name='%s', type='%s', pos=(%.0f,%.0f), size=(%.0f,%.0f)",
              dict["name"].asString().c_str(),
              dict["type"].asString().c_str(),
              dict["x"].asFloat(),
              dict["y"].asFloat(),
              dict["width"].asFloat(),
              dict["height"].asFloat());
    }
#endif

    return true;
}

bool GameScene::checkCollision(const Vec2 &worldPos) const
{
    if (!_collisionGroup)
    {
        return false;
    }

    // 将世界坐标转换为地图坐标
    Vec2 mapPos = worldPos;
    if (_tileMap)
    {
        mapPos = worldPos - _tileMap->getPosition();
    }

    // 遍历碰撞对象进行检测
    auto objects = _collisionGroup->getObjects();
    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        float x = dict["x"].asFloat();
        float y = dict["y"].asFloat();
        float width = dict["width"].asFloat();
        float height = dict["height"].asFloat();

        // 简单矩形碰撞检测（针对矩形对象）
        if (width > 0 && height > 0)
        {
            Rect rect(x, y, width, height);
            if (rect.containsPoint(mapPos))
            {
                return true;
            }
        }
    }

    return false;
}

void GameScene::setupRepeatingBackground(const std::string &backgroundPath, float mapWidth)
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 加载背景图片获取尺寸
    auto tempSprite = Sprite::create(backgroundPath);
    if (!tempSprite)
    {
        CCLOG("Error: Failed to load background image: %s", backgroundPath.c_str());
        return;
    }

    Size bgSize = tempSprite->getContentSize();

    // 计算背景高度缩放（让背景高度填满屏幕）
    float scaleY = visibleSize.height / bgSize.height;

    // 计算需要多少张图片来覆盖地图宽度
    int repeatCount = static_cast<int>(ceil(mapWidth / (bgSize.width * scaleY))) + 1;

    // 创建背景容器节点
    auto bgContainer = Node::create();
    bgContainer->setTag(TAG_BACKGROUND);

    // 横向重复铺设背景
    for (int i = 0; i < repeatCount; i++)
    {
        auto bgSprite = Sprite::create(backgroundPath);
        if (bgSprite)
        {
            bgSprite->setAnchorPoint(Vec2(0, 0));
            bgSprite->setScale(scaleY); // 统一缩放以保持比例
            bgSprite->setPosition(Vec2(i * bgSize.width * scaleY, origin.y));
            bgContainer->addChild(bgSprite);
        }
    }

    // 将背景容器添加到游戏内容层
    _gameLayer->addChild(bgContainer, BACKGROUND_Z_ORDER);
    CCLOG("Created repeating background: %d tiles, mapWidth=%.0f", repeatCount, mapWidth);
}

void GameScene::createCollisionBodiesFromTMX(const std::string &groupName)
{
    if (!_tileMap)
    {
        CCLOG("Error: Cannot create collision bodies - tilemap not loaded");
        return;
    }

    auto objectGroup = _tileMap->getObjectGroup(groupName);
    if (!objectGroup)
    {
        CCLOG("Warning: Collision group '%s' not found in tilemap", groupName.c_str());
        return;
    }

    auto objects = objectGroup->getObjects();
    CCLOG("Creating collision bodies from '%s': %zu objects", groupName.c_str(), objects.size());

    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        std::string name = dict["name"].asString();
        const double x = dict["x"].asDouble();
        const double y = dict["y"].asDouble();
        const double width = dict["width"].asDouble();
        const double height = dict["height"].asDouble();

        // 尝试解析多边形/折线顶点
        std::vector<Vec2> vertices;
        bool isPolygon = parseTMXObjectVertices(dict, x, y, vertices);

        if (!vertices.empty())
        {
            // 多边形或折线碰撞体
            createPolygonCollisionBody(vertices, name, isPolygon);
        }
        else if (width > 0 && height > 0)
        {
            // 矩形碰撞体
            Rect rect(x, y, width, height);
            createRectCollisionBody(rect, name);
        }
    }
}

bool GameScene::parseTMXObjectVertices(const ValueMap &dict, double objectX, double objectY,
                                       std::vector<Vec2> &outVertices)
{
    outVertices.clear();

    // cocos2d-x TMX 解析器将 polygon 存储为 "points"，polyline 存储为 "polylinePoints"
    bool hasPolygon = (dict.find("points") != dict.end());
    bool hasPolyline = (dict.find("polylinePoints") != dict.end());

    if (!hasPolygon && !hasPolyline)
    {
        return false;
    }

    ValueVector points;
    bool isPolygon = hasPolygon;

    if (isPolygon)
    {
        points = dict.at("points").asValueVector();
    }
    else
    {
        points = dict.at("polylinePoints").asValueVector();
    }

    if (points.size() < 2)
    {
        return isPolygon;
    }

    // 转换顶点坐标
    const double scaleFactor = Director::getInstance()->getContentScaleFactor();
    for (const auto &pt : points)
    {
        auto ptDict = pt.asValueMap();
        // TMX 解析器对 polygon/polyline points 不会做像素->点缩放，这里需要补上
        double px = ptDict["x"].asDouble() / scaleFactor;
        double py = ptDict["y"].asDouble() / scaleFactor;

        // 转换到 Cocos2d 坐标系 (相对于 _tileMap)
        // X 轴方向一致：直接相加
        // Y 轴方向相反：原点 Y 减去偏移 Y
        outVertices.push_back(Vec2(objectX + px, objectY - py));
    }

    // 如果是多边形，确保首尾相连以闭合
    if (isPolygon && outVertices.size() >= 3)
    {
        if (outVertices.front() != outVertices.back())
        {
            outVertices.push_back(outVertices.front());
        }
    }

    return isPolygon;
}

void GameScene::createPolygonCollisionBody(const std::vector<Vec2> &vertices,
                                           const std::string &name, bool isPolygon)
{
    if (vertices.size() < 2)
    {
        CCLOG("Warning: Not enough vertices for collision body '%s'", name.c_str());
        return;
    }

    // 创建碰撞节点，直接添加到 _tileMap
    auto collisionNode = Node::create();
    collisionNode->setPosition(Vec2::ZERO);

#if COCOS2D_DEBUG > 0
    // 添加调试绘制
    auto drawNode = DrawNode::create();
    drawNode->setPosition(Vec2::ZERO);
    drawNode->drawPoly(vertices.data(), static_cast<unsigned int>(vertices.size()),
                       true, Color4F(0, 1, 0, 0.5f));
    collisionNode->addChild(drawNode, COLLISION_DEBUG_Z_ORDER);
#endif

    // 使用 EdgeChain 创建静态边缘碰撞体
    auto physicsBody = PhysicsBody::createEdgeChain(vertices.data(),
                                                    static_cast<int>(vertices.size()),
                                                    COLLISION_PHYSICS_MATERIAL);

    if (physicsBody)
    {
        physicsBody->setDynamic(false);
        physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLATFORM));
        physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB));
        physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB));

        collisionNode->addComponent(physicsBody);
        _tileMap->addChild(collisionNode, 1);

        CCLOG("  Created %s collision: name='%s', %zu vertices",
              isPolygon ? "polygon" : "polyline", name.c_str(), vertices.size());
    }
    else
    {
        CCLOG("  Error: Failed to create collision body for '%s'", name.c_str());
    }
}

void GameScene::createRectCollisionBody(const Rect &rect, const std::string &name)
{
    // cocos2d-x 解析器已经将坐标转换为 cocos2d 坐标系
    // x, y 是矩形左下角的坐标
    double rectCenterX = rect.origin.x + rect.size.width / 2;
    double rectCenterY = rect.origin.y + rect.size.height / 2;

    auto collisionNode = Node::create();
    collisionNode->setPosition(Vec2(rectCenterX, rectCenterY) + _tileMap->getPosition());

    auto physicsBody = PhysicsBody::createBox(rect.size, COLLISION_PHYSICS_MATERIAL);
    physicsBody->setDynamic(false);
    physicsBody->setRotationEnable(false);

    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLATFORM));
    physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB));
    physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB));

    collisionNode->addComponent(physicsBody);
    // 将碰撞体添加到游戏内容层，而不是场景
    _gameLayer->addChild(collisionNode, 1);

    CCLOG("  Created rect collision: name='%s', size=(%.0f, %.0f) at (%.0f, %.0f)",
          name.c_str(), rect.size.width, rect.size.height, rectCenterX, rectCenterY);
}

// ===================================================================
// 传送门与出生点
// ===================================================================

Vec2 GameScene::getPlayerSpawnPoint()
{
    if (!_tileMap)
    {
        CCLOG("Warning: Cannot get spawn point - tilemap not loaded");
        return DEFAULT_SPAWN_POINT;
    }

    auto bornGroup = _tileMap->getObjectGroup("born");
    if (!bornGroup)
    {
        CCLOG("Warning: 'born' object group not found, using default spawn");
        return DEFAULT_SPAWN_POINT;
    }

    auto objects = bornGroup->getObjects();
    if (objects.empty())
    {
        CCLOG("Warning: No objects in 'born' group, using default spawn");
        return DEFAULT_SPAWN_POINT;
    }

    // 获取第一个出生点对象
    auto dict = objects[0].asValueMap();
    double x = dict["x"].asDouble();
    double y = dict["y"].asDouble();

    Vec2 spawnPoint(x, y);
    CCLOG("Player spawn point: (%.0f, %.0f)", spawnPoint.x, spawnPoint.y);
    return spawnPoint;
}

void GameScene::loadGateAreas()
{
    _gateAreas.clear();

    if (!_tileMap)
    {
        CCLOG("Warning: Cannot load gate areas - tilemap not loaded");
        return;
    }

    auto gateGroup = _tileMap->getObjectGroup("gate");
    if (!gateGroup)
    {
        CCLOG("Info: 'gate' object group not found in tilemap");
        return;
    }

    auto objects = gateGroup->getObjects();
    CCLOG("Loading gate areas: %zu objects", objects.size());

    auto visibleSize = Director::getInstance()->getVisibleSize();

    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        std::string name = dict["name"].asString();
        double x = dict["x"].asDouble();
        double y = dict["y"].asDouble();
        double width = dict["width"].asDouble();
        double height = dict["height"].asDouble();

        // 如果没有宽高，使用默认大小
        if (width <= 0)
            width = DEFAULT_GATE_INTERACT_DISTANCE * 2;
        if (height <= 0)
            height = visibleSize.height;

        Rect gateRect(x, y, width, height);
        _gateAreas.push_back(gateRect);

        CCLOG("  Gate '%s': rect=(%.0f, %.0f, %.0f, %.0f)",
              name.c_str(), x, y, width, height);
    }
}

bool GameScene::isPlayerAtGate() const
{
    if (!_player || _gateAreas.empty())
    {
        return false;
    }

    Vec2 playerPos = _player->getPosition();

    for (const auto &gateRect : _gateAreas)
    {
        if (gateRect.containsPoint(playerPos))
        {
            return true;
        }
    }

    return false;
}

// ===================================================================
// 敌人生成点（enemy_g）
// ===================================================================

void GameScene::loadEnemySpawnPoints(const std::string &groupName)
{
    _enemySpawnPoints.clear();

    if (!_tileMap)
    {
        CCLOG("Warning: Cannot load enemy spawn points - tilemap not loaded");
        return;
    }

    auto enemyGroup = _tileMap->getObjectGroup(groupName);
    if (!enemyGroup)
    {
        CCLOG("Info: '%s' object group not found in tilemap", groupName.c_str());
        return;
    }

    auto objects = enemyGroup->getObjects();
    CCLOG("Loading enemy spawn points '%s': %zu objects", groupName.c_str(), objects.size());

    _enemySpawnPoints.reserve(objects.size());

    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        double x = dict["x"].asDouble();
        double y = dict["y"].asDouble();
        std::string monsterType = dict["class"].asString();
        if (monsterType.empty())
        {
            monsterType = dict["type"].asString();
        }
        std::string countStr = dict["name"].asString();

        if (monsterType.empty())
        {
            CCLOG("Warning: Enemy spawn point at (%.0f, %.0f) missing type, skipped", x, y);
            continue;
        }

        int count = 1;
        if (!countStr.empty())
        {
            char *endPtr = nullptr;
            long parsed = std::strtol(countStr.c_str(), &endPtr, 10);
            if (endPtr != countStr.c_str() && parsed > 0 && parsed <= INT_MAX)
            {
                count = static_cast<int>(parsed);
            }
        }

        EnemySpawnPoint spawnPoint;
        spawnPoint.position = Vec2(static_cast<float>(x), static_cast<float>(y));
        spawnPoint.monsterType = monsterType;
        spawnPoint.count = count;
        spawnPoint.hasSpawned = false;
        _enemySpawnPoints.push_back(std::move(spawnPoint));

        CCLOG("  SpawnPoint: type='%s', count=%d, pos=(%.0f, %.0f)",
              monsterType.c_str(), count, x, y);
    }

    std::sort(_enemySpawnPoints.begin(), _enemySpawnPoints.end(),
              [](const EnemySpawnPoint &a, const EnemySpawnPoint &b)
              { return a.position.x < b.position.x; });
}

float GameScene::getEnemySpawnViewDistance() const
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    return visibleSize.width * 0.5f;
}

MonsterBase *GameScene::createMonsterByType(const std::string &monsterType) const
{
    std::string key = monsterType;
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });

    if (key == "goblin" || key == "goblinmonster")
    {
        return GoblinMonster::create();
    }

    CCLOG("Warning: Unknown monster type '%s'", monsterType.c_str());
    return nullptr;
}

void GameScene::updateEnemySpawns()
{
    if (!_player || !_gameLayer || _enemySpawnPoints.empty())
        return;

    const float viewDistance = getEnemySpawnViewDistance();
    const float playerX = _player->getPositionX();

    constexpr float SPAWN_SPACING_X = 80.0f;
    constexpr float SPAWN_INTERVAL_SECONDS = 0.4f;
    auto player = _player;

    for (auto &spawnPoint : _enemySpawnPoints)
    {
        if (spawnPoint.hasSpawned)
            continue;

        const float dx = std::fabs(playerX - spawnPoint.position.x);
        if (dx > viewDistance)
            continue;

        const int count = std::max(1, spawnPoint.count);
        const float centerIndex = (static_cast<float>(count) - 1.0f) * 0.5f;

        // 首次进入视野即锁定生成，避免来回触发
        spawnPoint.hasSpawned = true;

        for (int i = 0; i < count; ++i)
        {
            const std::string monsterType = spawnPoint.monsterType;
            const float offsetX = (static_cast<float>(i) - centerIndex) * SPAWN_SPACING_X;
            const Vec2 monsterPos = spawnPoint.position + Vec2(offsetX, 0.0f);

            // 分批生成：大约每 0.4 秒生成一个
            const float delaySeconds = static_cast<float>(i) * SPAWN_INTERVAL_SECONDS;
            _gameLayer->runAction(Sequence::create(
                DelayTime::create(delaySeconds),
                CallFuncN::create([this, player, monsterType, monsterPos](Node *actionTarget) {
                    if (!actionTarget || !player)
                        return;

                    auto monster = createMonsterByType(monsterType);
                    if (!monster)
                        return;

                    monster->setPosition(monsterPos);
                    monster->setTarget(player);
                    monster->setHome(monsterPos);
                    actionTarget->addChild(monster, PLAYER_Z_ORDER);
                }),
                nullptr));
        }

        CCLOG("Enemy spawn triggered: type='%s', count=%d, pos=(%.0f, %.0f)",
              spawnPoint.monsterType.c_str(), count,
              spawnPoint.position.x, spawnPoint.position.y);
    }
}

// ===================================================================
// 碰撞检测回调
// ===================================================================

bool GameScene::onContactBegin(PhysicsContact& contact)
{
    auto nodeA = contact.getShapeA()->getBody()->getNode();
    auto nodeB = contact.getShapeB()->getBody()->getNode();

    // 1. 安全检查：如果节点已经销毁，直接返回
    if (!nodeA || !nodeB) return true;

    auto shapeA = contact.getShapeA();
    auto shapeB = contact.getShapeB();

    // 获取掩码 (int 类型)
    int categoryA = shapeA->getBody()->getCategoryBitmask();
    int categoryB = shapeB->getBody()->getCategoryBitmask();

    // ============================================================
    // 模块 1：炸弹逻辑 (Bomb)
    // ============================================================
    bool isBombA = (categoryA & GamePhysicsCategory::BOMB);
    bool isBombB = (categoryB & GamePhysicsCategory::BOMB);

    if (isBombA || isBombB)
    {
        auto bombNode = isBombA ? nodeA : nodeB;
        int otherCategory = isBombA ? categoryB : categoryA;

        // 转换节点为 Bomb 对象
        if (auto bomb = dynamic_cast<Bomb*>(bombNode))
        {
            // 如果炸弹还没有爆炸，且撞到了：平台、怪物、或通用阻挡
            if (!bomb->isExploded() &&
                (otherCategory & (GamePhysicsCategory::PLATFORM |
                    GamePhysicsCategory::MONSTER |
                    GamePhysicsCategory::COLLISION)))
            {
                // 【重要】必须延迟一帧执行爆炸，因为不能在物理步进中修改场景图
                bomb->scheduleOnce([bomb](float) {
                    bomb->explode();
                    }, 0.0f, "bomb_explode_trigger");
            }
        }
        // 炸弹逻辑处理完毕，不影响后续物理反馈，返回 true
        return true;
    }

    // ============================================================
    // 模块 2：玩家落地检测 (Ground Detection)
    // ============================================================
    // 只有当玩家参与碰撞时才计算
    bool isPlayerA = (categoryA & GamePhysicsCategory::PLAYER);
    bool isPlayerB = (categoryB & GamePhysicsCategory::PLAYER);

    if (isPlayerA || isPlayerB)
    {
        int otherCategory = isPlayerA ? categoryB : categoryA;

        // 如果撞到的是平台或通用碰撞体
        if (otherCategory & (GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION))
        {
            auto contactData = contact.getContactData();
            if (contactData)
            {
                Vec2 normal = contactData->normal;
                // 如果玩家是 B，法向量取反（保证 normal 始终指向某一个方向标准）
                // 假设 normal 默认是从 A 指向 B
                if (isPlayerB) normal = -normal;

                // 判定平台在玩家下方 (脚下的法线通常是向上的)
                // 注意：这里需要根据你的具体物理设置调整阈值方向
                if (normal.y < GROUND_NORMAL_THRESHOLD)
                {
                    _groundContactCount++;
                    _isGrounded = true;
                    _jumpCount = 0; // 重置跳跃
                    // CCLOG("Player grounded, count: %d", _groundContactCount);
                }
            }
        }
    }

    // ============================================================
    // 模块 3：怪物攻击 -> 玩家 (受到近战伤害)
    // ============================================================
    // 使用 ToMask 宏简化代码
    bool monsterHitPlayer =
        ((categoryA & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) && (categoryB & ToMask(GamePhysicsCategory::PLAYER))) ||
        ((categoryB & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) && (categoryA & ToMask(GamePhysicsCategory::PLAYER)));

    if (monsterHitPlayer)
    {
        // 确定谁是攻击框，谁是玩家
        auto attackBody = (categoryA & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) ? shapeA->getBody() : shapeB->getBody();
        auto playerNode = (categoryA & ToMask(GamePhysicsCategory::PLAYER)) ? nodeA : nodeB;

        if (auto player = dynamic_cast<CharacterBase*>(playerNode))
        {
            if (!player->isDead())
            {
                float damageVal = static_cast<float>(attackBody->getTag()); // 从 Tag 获取伤害
                DamageInfo dmg;
                dmg.amount = (damageVal > 0) ? damageVal : 10.0f; // 默认伤害保底
                player->takeDamage(dmg);
            }
        }
    }

    // ============================================================
    // 模块 4：玩家攻击 -> 怪物 (近战挥砍)
    // ============================================================
    bool playerHitMonster =
        ((categoryA & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) && (categoryB & ToMask(GamePhysicsCategory::MONSTER))) ||
        ((categoryB & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) && (categoryA & ToMask(GamePhysicsCategory::MONSTER)));

    if (playerHitMonster)
    {
        auto attackBody = (categoryA & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) ? shapeA->getBody() : shapeB->getBody();
        auto monsterNode = (categoryA & ToMask(GamePhysicsCategory::MONSTER)) ? nodeA : nodeB;

        if (auto monster = dynamic_cast<CharacterBase*>(monsterNode))
        {
            if (!monster->isDead())
            {
                float damageVal = static_cast<float>(attackBody->getTag());
                DamageInfo dmg;
                dmg.amount = (damageVal > 0) ? damageVal : 10.0f;
                dmg.attacker = _player; // 记录攻击者

                monster->takeDamage(dmg);

                // 如果需要打击特效，可以在这里生成
                // createHitEffect(monster->getPosition());
            }
        }
    }

    return true;
}

void GameScene::onContactSeparate(PhysicsContact &contact)
{
    auto nodeA = contact.getShapeA()->getBody()->getNode();
    auto nodeB = contact.getShapeB()->getBody()->getNode();

    if (!nodeA || !nodeB)
        return;

    int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
    int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

    // 检测玩家与平台分离
    bool playerIsA = (categoryA & GamePhysicsCategory::PLAYER);
    bool playerIsB = (categoryB & GamePhysicsCategory::PLAYER);
    bool platformContact =
        (playerIsA && ((categoryB & GamePhysicsCategory::PLATFORM) || (categoryB & GamePhysicsCategory::COLLISION))) ||
        (playerIsB && ((categoryA & GamePhysicsCategory::PLATFORM) || (categoryA & GamePhysicsCategory::COLLISION)));

    if (platformContact && _groundContactCount > 0)
    {
        bool wasGroundContact = false;
        if (auto contactData = contact.getContactData())
        {
            Vec2 normal = contactData->normal;
            if (playerIsB)
            {
                normal = -normal;
            }
            // 只有离开“脚下平台”的接触才减少计数，避免侧面碰撞影响落地判断
            wasGroundContact = (normal.y < GROUND_NORMAL_THRESHOLD);
        }

        if (!wasGroundContact)
        {
            return;
        }

        _groundContactCount--;

        if (_groundContactCount <= 0)
        {
            _groundContactCount = 0;
            // 只要完全离开所有脚下平台就认为离地，避免在空中速度为0时误判为落地
            _isGrounded = false;
            CCLOG("Player left ground, contacts: 0");
        }
    }
}

// ===================================================================
// 输入处理
// ===================================================================

void GameScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event *event)
{
    // ESC 键始终响应（用于暂停/恢复）
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        togglePauseMenu();
        return;
    }

    // 暂停时忽略其他输入
    if (_isPaused || !_player)
        return;

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _isMovingLeft = true;
        _player->setFlippedX(true);
        if (!_isAttacking && !_isCastingSkill)
            _player->setMoving(true, _isRunPressed);
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = true;
        _player->setFlippedX(false);
        if (!_isAttacking && !_isCastingSkill)
            _player->setMoving(true, _isRunPressed);
        break;

    case EventKeyboard::KeyCode::KEY_SHIFT:
    case EventKeyboard::KeyCode::KEY_RIGHT_SHIFT:
        _isRunPressed = true;
        if ((_isMovingLeft || _isMovingRight) && !_isAttacking && !_isCastingSkill)
        {
            _player->setMoving(true, true);
        }
        break;

    case EventKeyboard::KeyCode::KEY_W:
        // 先检查传送门交互
        if (!handleGateInteraction())
        {
            // 没有传送门交互则执行跳跃
            handleJump();
        }
        break;

    case EventKeyboard::KeyCode::KEY_SPACE:
        // 空格键只跳跃，不触发传送门
        handleJump();
        break;

    // 攻击按键
    case EventKeyboard::KeyCode::KEY_J:
    case EventKeyboard::KeyCode::KEY_4:
        throwBomb(); // 普通攻击：扔炸弹
        break;

    // 技能按键
    case EventKeyboard::KeyCode::KEY_E:
    case EventKeyboard::KeyCode::KEY_K: // 临时兼容
        //aaaaaacastFireball(); // 技能1：火球
        break;

    default:
        break;
    }
}

void GameScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_SHIFT:
    case EventKeyboard::KeyCode::KEY_RIGHT_SHIFT:
        _isRunPressed = false;
        if ((_isMovingLeft || _isMovingRight) && !_isAttacking && !_isCastingSkill && _player)
        {
            _player->setMoving(true, false);
        }
        break;

    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _isMovingLeft = false;
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = false;
        break;

    default:
        break;
    }

    // 所有方向键释放时停止动画
    if (!_isMovingLeft && !_isMovingRight && !_isAttacking && !_isCastingSkill)
    {
        _player->setMoving(false);
    }
}

void GameScene::handleJump()
{
    if (!_player || !_player->getPhysicsBody())
        return;

    // 落地时允许重新开始计数
    if (_isGrounded)
    {
        _jumpCount = 0;
    }

    if (_jumpCount >= _playerConfig.maxJumpCount)
        return;

    // 主动跳跃时清空地面接触计数，避免离地瞬间残留接触导致空中误判为落地
    _groundContactCount = 0;

    auto physicsBody = _player->getPhysicsBody();
    Vec2 velocity = physicsBody->getVelocity();
    // 让二段跳更干脆：清掉当前垂直速度
    velocity.y = 0.0f;
    physicsBody->setVelocity(velocity);

    physicsBody->applyImpulse(Vec2(0, _playerConfig.jumpImpulse));
    _isGrounded = false;
    _jumpCount++;
    CCLOG(_jumpCount == 1 ? "Player jumped" : "Player double jumped");
}

bool GameScene::handleGateInteraction()
{
    if (isPlayerAtGate())
    {
        CCLOG("Player at gate, returning to map scene");
        returnToMapScene();
        return true;
    }
    return false;
}

// ===================================================================
// 更新循环
// ===================================================================

void GameScene::update(float dt)
{
    if (!_player || !_player->getPhysicsBody())
        return;

    // 暂停时只更新 UI，不更新游戏逻辑
    if (_isPaused)
    {
        updateUI();

        // 清理已爆炸/移除的投掷物，避免列表无限增长
        _player->cleanupProjectiles();
        return;
    }

    updatePlayerMovement(dt);
    updateGroundedState(_player->getPhysicsBody()->getVelocity());
    updateUI();
    updateEnemySpawns();

    // 清理已爆炸/移除的投掷物，避免列表无限增长
    _player->cleanupProjectiles();
}

void GameScene::updatePlayerMovement(float dt)
{
    auto physicsBody = _player->getPhysicsBody();
    Vec2 velocity = physicsBody->getVelocity();

    // 计算目标水平速度
    float currentSpeed = _isRunPressed ? _playerConfig.runSpeed : _playerConfig.walkSpeed;
    float targetVelocityX = 0.0f;
    if (_isMovingLeft)
    {
        targetVelocityX = -currentSpeed;
    }
    else if (_isMovingRight)
    {
        targetVelocityX = currentSpeed;
    }

    velocity.x = targetVelocityX;
    physicsBody->setVelocity(velocity);
}

void GameScene::updateGroundedState(const Vec2 &velocity)
{
    // 辅助着地检测：如果玩家垂直速度很小且有接触计数，确保着地状态
    if (_groundContactCount > 0 && fabsf(velocity.y) < GROUND_VELOCITY_THRESHOLD)
    {
        _isGrounded = true;
        _jumpCount = 0;
    }
    // 没有任何地面接触时一律视为在空中（哪怕垂直速度为0）
    else if (_groundContactCount <= 0)
    {
        _isGrounded = false;
    }
}

void GameScene::updateUI()
{
    if (!_gameUI)
        return;

    // 检查是否在传送门区域，更新交互提示
    bool atGate = isPlayerAtGate();
    if (atGate && !_wasAtGate)
    {
        _gameUI->showInteractionHint(GATE_INTERACTION_HINT);
    }
    else if (!atGate && _wasAtGate)
    {
        _gameUI->hideInteractionHint();
    }
    _wasAtGate = atGate;

    // 刷新数值显示（HP/MP/技能冷却等）
    _gameUI->updateDisplay();
}

// 辅助方法：显示地图加载失败 UI
void GameScene::showMapLoadFailedUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    std::string errorText = getLevelName() + MAP_LOAD_FAILED_TEXT;
    auto titleLabel = Label::createWithTTF(errorText, DEFAULT_FONT_PATH, 48);
    if (titleLabel)
    {
        titleLabel->setPosition(center);
        titleLabel->setColor(Color3B::RED);
        this->addChild(titleLabel, 1);
    }
}

// ============================================================
// 战斗系统实现
// ============================================================

/**
 * @brief 攻击动画结束回调
 */
void GameScene::onAttackAnimationFinished()
{
    _isAttacking = false;

    if (_player)
    {
        // 根据当前输入恢复跑动/待机状态
        _player->setMoving(_isMovingLeft || _isMovingRight, _isRunPressed);
    }

    CCLOG("Attack animation finished");
}

// ============================================================
// 技能系统实现
// ============================================================

/**
 * @brief 只负责检查条件（MP/CD）、设置状态、播放动作
 */
void GameScene::throwBomb()
{
    // 1. 安全检查
    if (!_player || _player->isDead()) return;

    // 2. 状态检查：防止连点
    if (_isCastingSkill || _isAttacking) {
        return;
    }

    // 3. 技能组件检查 (蓝量/冷却)
    auto skillComp = _player->getSkillComponent();
    if (!skillComp || !skillComp->useActiveSkill(GameConfig::Skill::SLOT_BOMB)) {
        CCLOG("Skill failed: No MP or Cooldown");
        return; // 没蓝或冷却中，直接退出
    }

    // 4. 这里的 castSkillAnimated 删掉！直接进执行逻辑！
    doThrowBomb();
}

/**
 * @brief 负责重置状态、触发生成
 */
void GameScene::onSkillAnimationFinished()
{
    // 1. 解锁状态
    _isCastingSkill = false;
    _isAttacking = false;

    // 2. 恢复移动 (如果需要)
    if (_player) {
        // 根据按键状态恢复移动
        _player->setMoving(_isMovingLeft || _isMovingRight, _isRunPressed);
    }

    CCLOG("Skill Finished: Ready for next command.");
}



/**
 * @brief 实际创建并投掷炸弹
 */
void GameScene::doThrowBomb()
{
    if (!_player || _player->isDead()) return;

    // 1. 锁定状态 (防止其他操作打断)
    _isCastingSkill = true;
    _isAttacking = true;

    CCLOG("Bomb Skill Triggered: Playing Animation...");

    // 2. 播放动画 (这是唯一一次播放)
    _player->castSkillAnimated([this]() {

        // --- 动画回调开始 ---
        if (!this || !_player || !_gameLayer) return;

        // A. 生成炸弹逻辑
        auto bomb = Bomb::create("Sprites/Characters/Player/Klee/TNT.png");
        if (bomb)
        {
            Vec2 playerPos = _player->getPosition();
            bool facingLeft = _player->isFlippedX();
            float throwDirX = facingLeft ? -1.0f : 1.0f;

            bomb->setPosition(playerPos + Vec2(throwDirX * 30.0f, 30.0f));
            bomb->setAttacker(_player);
            _gameLayer->addChild(bomb, 4);

            // 抛出
            bomb->throwAt(Vec2(throwDirX * Bomb::DEFAULT_THROW_SPEED_X, Bomb::DEFAULT_THROW_SPEED_Y));
            CCLOG("Bomb Spawned!");
        }

        // B. 动画结束，去重置状态
        this->onSkillAnimationFinished();
        });
}

// ============================================================
// OriginMushroomScene 实现（起源之菇）
// ============================================================

Scene *OriginMushroomScene::createScene()
{
    return OriginMushroomScene::create();
}

LevelConfig OriginMushroomScene::getLevelConfig() const
{
    LevelConfig config;
    config.tmxMapPath = "Map/Origin_Mushroom/Origin_Mushroom.tmx";
    // 起源之菇地图已自带背景图层，这里不再额外铺设重复背景
    config.backgroundPath = "";
    config.playerSpritePath = DEFAULT_PLAYER_SPRITE;
    config.collisionLayerName = "collisions";
    config.bornLayerName = "born";
    config.gateLayerName = "gate";
    config.gravity = -1000.0f;
    config.enablePhysicsDebug = true; // 开发阶段开启调试
    return config;
}

bool OriginMushroomScene::init()
{
    LevelConfig config = getLevelConfig();
    if (!initWithPhysicsConfig(config))
    {
        return false;
    }
    CCLOG("OriginMushroomScene initialized");
    return true;
}

// ============================================================
// MysteryForestScene 实现（神秘之森）
// ============================================================

Scene *MysteryForestScene::createScene()
{
    return MysteryForestScene::create();
}

LevelConfig MysteryForestScene::getLevelConfig() const
{
    LevelConfig config;
    // TODO: 配置神秘之森的资源路径
    config.tmxMapPath = "";
    config.backgroundPath = "";
    config.gravity = -800.0f;
    config.enablePhysicsDebug = false;
    return config;
}

bool MysteryForestScene::init()
{
    if (!GameScene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // 临时：添加场景标题标签（等待资源配置）
    auto titleLabel = Label::createWithTTF(
        getLevelName(),
        DEFAULT_FONT_PATH,
        72);

    if (titleLabel)
    {
        titleLabel->setPosition(center);
        titleLabel->setColor(Color3B::WHITE);
        this->addChild(titleLabel, 1);
    }

    // 添加提示文字
    auto hintLabel = Label::createWithTTF(
        "Click map button to return",
        DEFAULT_FONT_PATH,
        32);

    if (hintLabel)
    {
        hintLabel->setPosition(Vec2(center.x, center.y - 80));
        hintLabel->setColor(Color3B(200, 200, 200));
        this->addChild(hintLabel, 1);
    }

    // 初始化 UI（虽然没有地图但仍需要返回按钮）
    initGameUI();

    CCLOG("MysteryForestScene initialized (placeholder)");
    return true;
}
