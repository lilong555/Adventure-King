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
#include <functional>

USING_NS_CC;

// ============================================================
// 常量定义
// ============================================================
namespace
{
    // 资源路径
    const char *const DEFAULT_FONT_PATH = "fonts/ZCOOLKuaiLe-Regular.ttf";
    const char *const DEFAULT_PLAYER_SPRITE = "Sprites/Characters/Player/Klee/spr_klee_run.png";

    // 默认值
    const Vec2 DEFAULT_SPAWN_POINT(100.0f, 200.0f);

    // 物理材质
    const PhysicsMaterial PLAYER_PHYSICS_MATERIAL(1.0f, 0.0f, 0.0f);    // 密度, 弹性, 摩擦
    const PhysicsMaterial COLLISION_PHYSICS_MATERIAL(1.0f, 0.0f, 0.8f); // 碰撞体材质

    // UI 文本
    const char *const GATE_INTERACTION_HINT = "Press W to enter gate";
    const char *const MAP_LOAD_FAILED_TEXT = " - Map Load Failed";

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
    auto world = this->getPhysicsWorld();
    if (world) {
        // 强制开启调试绘制，0xFFFF 表示绘制所有细节
        world->setDebugDrawMask(cocos2d::PhysicsWorld::DEBUGDRAW_ALL);
        // 降低一点步长，提高精度
        world->setSubsteps(4);
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
    playerSprite->setScale(_playerConfig.scale);

    // 计算缩放后的实际显示尺寸
    float scaledWidth = originalSize.width;
    float scaledHeight = originalSize.height;

    // 锚点设置为中心
    playerSprite->setAnchorPoint(Vec2(0.5f, 0.5f));

    // 设置位置：startPos 是地面位置，需要将玩家中心抬高半个身高
    Vec2 playerPos = startPos + Vec2(0, scaledHeight / 2);
    playerSprite->setPosition(playerPos);

    // 计算碰撞体尺寸（基于配置的比例）
    float boxWidth = scaledWidth * _playerConfig.collisionBoxWidthRatio;
    float boxHeight = scaledHeight * _playerConfig.collisionBoxHeightRatio;

    // 创建物理体
    auto physicsBody = PhysicsBody::createBox(Size(boxWidth, boxHeight), PLAYER_PHYSICS_MATERIAL);
    physicsBody->setDynamic(true);
    physicsBody->setRotationEnable(false);
    physicsBody->setMass(1.0f);
    physicsBody->setLinearDamping(0.0f);

    // 配置碰撞掩码
    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLAYER));
    physicsBody->setCollisionBitmask(ToMask((GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)));
    physicsBody->setContactTestBitmask(ToMask((GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)));

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

    // 初始化玩家技能
    initPlayerSkills();

    CCLOG("Player created: pos=(%.0f, %.0f), boxSize=(%.0f, %.0f)",
          playerPos.x, playerPos.y, boxWidth, boxHeight);
}

/**
 * @brief 初始化玩家技能
 *
 * 通过 SkillComponent 创建并装备主动技能。
 * 当前实现了炸弹技能作为示例。
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

    // 创建炸弹技能
    auto bombSkill = std::make_shared<ActiveSkill>();
    bombSkill->id = BOMB_SKILL_ID;
    bombSkill->name = "炸弹";
    bombSkill->description = "丢出一个炸弹，造成范围伤害";
    bombSkill->manaCost = BOMB_SKILL_MP_COST;
    bombSkill->cooldown = BOMB_SKILL_COOLDOWN;
    bombSkill->currentCooldown = 0.0f;

    // 学习并装备技能
    skillComp->learnSkill(bombSkill);
    skillComp->equipActiveSkill(bombSkill, BOMB_SKILL_SLOT);

    CCLOG("Player skills initialized: Bomb skill equipped to slot %zu", BOMB_SKILL_SLOT);
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
        bool playerInvolved = (categoryA & static_cast<int>(GamePhysicsCategory::PLAYER)) || (categoryB & static_cast<int>(GamePhysicsCategory::PLAYER));
        bool platformInvolved = (categoryA & static_cast<int>(GamePhysicsCategory::PLATFORM)) || (categoryB & static_cast<int>(GamePhysicsCategory::PLATFORM)) ||
                                (categoryA & static_cast<int>(GamePhysicsCategory::COLLISION)) || (categoryB & static_cast<int>(GamePhysicsCategory::COLLISION));

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

void GameScene::createPolygonCollisionBody(const std::vector<cocos2d::Vec2>& vertices,
    const std::string& name,
    bool isPolygon)
{
    if (vertices.empty())
        return;

    auto node = Node::create();

    auto body = isPolygon ?
        PhysicsBody::createPolygon(vertices.data(), vertices.size(), PhysicsMaterial(0.0f, 0.0f, 1.0f)) :
        PhysicsBody::createEdgeChain(vertices.data(), vertices.size(), PhysicsMaterial(0.0f, 0.0f, 1.0f));

    body->setDynamic(false);
    body->setRotationEnable(false);

    body->setCategoryBitmask(ToMask(GamePhysicsCategory::PLATFORM));
    body->setCollisionBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER | GamePhysicsCategory::BOMB));
    body->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER | GamePhysicsCategory::BOMB));

    node->addComponent(body);
    _gameLayer->addChild(node);
}


void GameScene::createRectCollisionBody(const cocos2d::Rect& rect, const std::string& name)
{
    auto node = Node::create();
    // 坐标计算是完美的，保持不变
    node->setPosition(rect.origin + Vec2(rect.size.width / 2, rect.size.height / 2));

    // 核心修改：把摩擦力从 1.0 改成 0.0
    // 参数：密度(0), 弹性(0), 摩擦(0)
    auto material = PhysicsMaterial(0.0f, 0.0f, 0.0f);

    auto body = PhysicsBody::createBox(rect.size, material);
    body->setDynamic(false);
    body->setRotationEnable(false);

    body->setCategoryBitmask(ToMask(GamePhysicsCategory::PLATFORM));
    body->setCollisionBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER | GamePhysicsCategory::BOMB));
    body->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER | GamePhysicsCategory::BOMB));

    node->addComponent(body);
    _gameLayer->addChild(node);
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
// 碰撞检测回调
// ===================================================================

bool GameScene::onContactBegin(cocos2d::PhysicsContact& contact)
{
    // 1. 通用安全检查
    auto shapeA = contact.getShapeA();
    auto shapeB = contact.getShapeB();
    auto bodyA = shapeA->getBody();
    auto bodyB = shapeB->getBody();

    if (!bodyA || !bodyB) return true;

    auto nodeA = bodyA->getNode();
    auto nodeB = bodyB->getNode();

    // 防止空指针（节点可能刚被移除）
    if (!nodeA || !nodeB) return true;

    // 2. 获取分类掩码
    int maskA = bodyA->getCategoryBitmask();
    int maskB = bodyB->getCategoryBitmask();

    // 预定义掩码，方便后续判断
    int maskPlayer = static_cast<int>(GamePhysicsCategory::PLAYER);
    int maskPlatform = static_cast<int>(GamePhysicsCategory::PLATFORM);
    int maskAttack = static_cast<int>(GamePhysicsCategory::MONSTER_ATTACK);
    int maskSolid = static_cast<int>(GamePhysicsCategory::COLLISION);

    // ============================================================
    // ⚔️ 第一部分：战斗判定 (怪物攻击判定框 vs 玩家)
    // ============================================================

    // 辅助 Lambda：处理攻击
    auto handleAttack = [&](cocos2d::PhysicsBody* attackerBody, cocos2d::PhysicsBody* victimBody) -> bool
        {
            // 获取受害者（玩家）
            auto player = dynamic_cast<CharacterBase*>(victimBody->getNode());

            if (player)
            {
                // 1. 组装伤害信息
                DamageInfo info;

                // 获取伤害数值 (从 Tag 取)
                info.amount = (float)attackerBody->getTag();
                if (info.amount <= 0) info.amount = 10.0f;

                // ---------------------------------------------------
                // 获取真正的攻击者 怪物的本体
                // ---------------------------------------------------
                auto hitboxNode = attackerBody->getNode();
                if (hitboxNode)
                {
                    // 判定框的父节点才是怪物
                    info.attacker = dynamic_cast<CharacterBase*>(hitboxNode->getParent());
                }
                else
                {
                    info.attacker = nullptr;
                }
                // ---------------------------------------------------

                // 2. 扣血
                player->takeDamage(info);
                CCLOG("COMBAT: Player hit for %.1f damage", info.amount);

                // 3. 销毁判定框 (防止多重判定)
                if (hitboxNode) hitboxNode->removeFromParent();

                // 4. 返回 false (表示 Sensor 碰撞，不产生物理反弹)
                return true;
            }
            return false;
        };

    // 判定组合
    if ((maskA & maskAttack) && (maskB & maskPlayer))
    {
        handleAttack(bodyA, bodyB);
        return false; // 攻击框不需要物理反弹
    }
    else if ((maskB & maskAttack) && (maskA & maskPlayer))
    {
        handleAttack(bodyB, bodyA);
        return false;
    }

    // ============================================================
    // 🦶 第二部分：落地检测
    // ============================================================

    // 只有当涉及到玩家时才计算
    bool playerIsA = (maskA & maskPlayer) != 0;
    bool playerIsB = (maskB & maskPlayer) != 0;

    // 检查是否撞到了平台或墙壁
    bool hitSolid = (playerIsA && ((maskB & maskPlatform) || (maskB & maskSolid))) ||
        (playerIsB && ((maskA & maskPlatform) || (maskA & maskSolid)));

    if (hitSolid)
    {
        // 获取法线
        auto contactData = contact.getContactData();
        if (contactData)
        {
            cocos2d::Vec2 normal = contactData->normal;

            // 统一法线方向：让 normal 始终表示 "从玩家指向物体" 的方向
            // 如果玩家是 B，法线默认是 A->B，所以不需要反转？
            // ⚠️注意：这里取决于你的物理引擎版本逻辑，你的原逻辑是 playerIsB 则反转
            // 我们保留你的原逻辑：假设 normal 默认指向 B
            if (playerIsB)
            {
                normal = -normal;
            }

            // 阈值判断 (假设 GROUND_NORMAL_THRESHOLD 是一个类似 -0.7f 的值)
            // normal.y < 0 说明方向向下（脚底下有东西）
            if (normal.y < -0.7f) // 假设阈值是 -0.7，表示脚下的碰撞
            {
                _groundContactCount++;
                _isGrounded = true;
                CCLOG("Player grounded (normal.y=%.2f), contacts: %d", normal.y, _groundContactCount);
            }
        }
    }

    // 炸弹命中非玩家目标时触发爆炸（平台/碰撞体/敌人等）
    bool bombIsA = (maskA & static_cast<int>(GamePhysicsCategory::BOMB)) != 0;
    bool bombIsB = (maskB & static_cast<int>(GamePhysicsCategory::BOMB)) != 0;

    if (bombIsA || bombIsB)
    {
        Node *bombNode = bombIsA ? nodeA : nodeB;
        int otherMask = bombIsA ? maskB : maskA;
        bool hitPlayer = (otherMask & static_cast<int>(GamePhysicsCategory::PLAYER)) != 0;
        bool hitBomb = (otherMask & static_cast<int>(GamePhysicsCategory::BOMB)) != 0;

        if (!hitPlayer && !hitBomb)
        {
            for (auto &bomb : _bombs)
            {
                if (bomb.sprite == bombNode && !bomb.isExploded)
                {
                    bomb.isExploded = true;
                    explodeBomb(bomb);
                    break;
                }
            }
        }
    }

    // ============================================================
    // 🏁 结束：返回 true 允许物理引擎处理剩下的碰撞 (阻挡、反弹)
    // ============================================================
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
    bool playerIsA = (categoryA & static_cast<int>(GamePhysicsCategory::PLAYER));
    bool playerIsB = (categoryB & static_cast<int>(GamePhysicsCategory::PLAYER));
    bool platformContact =
        (playerIsA && ((categoryB & static_cast<int>(GamePhysicsCategory::PLATFORM)) || (categoryB & static_cast<int>(GamePhysicsCategory::COLLISION))) ||
        (playerIsB && ((categoryA & static_cast<int>(GamePhysicsCategory::PLATFORM)) || (categoryA & static_cast<int>(GamePhysicsCategory::COLLISION)))));

    if (platformContact && _groundContactCount > 0)
    {
        _groundContactCount--;

        if (_groundContactCount <= 0)
        {
            _groundContactCount = 0;

            // 检查玩家速度确定是否真的离地
            if (_player && _player->getPhysicsBody())
            {
                Vec2 velocity = _player->getPhysicsBody()->getVelocity();
                if (fabsf(velocity.y) > 10.0f)
                {
                    _isGrounded = false;
                    CCLOG("Player left ground (velocity.y=%.2f)", velocity.y);
                }
            }
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
            _player->setMoving(true);
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = true;
        _player->setFlippedX(false);
        if (!_isAttacking && !_isCastingSkill)
            _player->setMoving(true);
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
        if (!_isAttacking && !_isCastingSkill && _player)
        {
            _isAttacking = true;
            _player->attackAnimated([this]()
                                    { this->onAttackAnimationFinished(); });
        }
        break;

    // 技能按键
    case EventKeyboard::KeyCode::KEY_E:
    case EventKeyboard::KeyCode::KEY_K:
        throwBomb();
        break;

    default:
        break;
    }
}

void GameScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *event)
{
    switch (keyCode)
    {
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
    if (_isGrounded && _player && _player->getPhysicsBody())
    {
        _player->getPhysicsBody()->applyImpulse(Vec2(0, _playerConfig.jumpImpulse));
        _isGrounded = false;
        CCLOG("Player jumped");
    }
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
        return;
    }

    updatePlayerMovement(dt);
    updateGroundedState(_player->getPhysicsBody()->getVelocity());
    updateUI();
}

void GameScene::updatePlayerMovement(float dt)
{
    auto physicsBody = _player->getPhysicsBody();
    Vec2 velocity = physicsBody->getVelocity();

    // 计算目标水平速度
    float targetVelocityX = 0.0f;
    if (_isMovingLeft)
    {
        targetVelocityX = -_playerConfig.moveSpeed;
    }
    else if (_isMovingRight)
    {
        targetVelocityX = _playerConfig.moveSpeed;
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
    }
    // 如果玩家在下落且速度很大，确保非着地状态
    else if (velocity.y < FALLING_VELOCITY_THRESHOLD && _groundContactCount <= 0)
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
        _player->setMoving(_isMovingLeft || _isMovingRight);
    }

    CCLOG("Attack animation finished");
}

// ============================================================
// 技能系统实现
// ============================================================

/**
 * @brief 播放技能施放动画
 */
/**
 * @brief 技能动画播放完成回调
 */
void GameScene::onSkillAnimationFinished()
{
    _isCastingSkill = false;

    // 动画结束后实际丢出炸弹
    doThrowBomb();

    if (_player)
    {
        _player->setMoving(_isMovingLeft || _isMovingRight);
    }

    CCLOG("Skill animation finished");
}

/**
 * @brief 释放炸弹技能
 */
void GameScene::throwBomb()
{
    if (!_player || _player->isDead())
        return;

    // 如果正在施放技能或攻击中，禁用技能
    if (_isCastingSkill || _isAttacking)
    {
        return;
    }

    // 通过技能组件释放技能（会自动检查 MP、冷却，并扣除 MP）
    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        CCLOG("Skill component not found");
        return;
    }

    // 尝试使用槽位 0 的技能（炸弹技能）
    if (!skillComp->useActiveSkill(BOMB_SKILL_SLOT))
    {
        // 技能释放失败（可能是 MP 不足或冷却中）
        CCLOG("Skill cast failed - MP insufficient or on cooldown");
        return;
    }

    // 技能释放成功，播放技能动画
    _isCastingSkill = true;
    _player->castSkillAnimated([this]()
                               { this->onSkillAnimationFinished(); });
    CCLOG("Skill started: Throw Bomb");
}

/**
 * @brief 实际创建并投掷炸弹
 */
void GameScene::doThrowBomb()
{
    if (!_player || _player->isDead())
        return;

    // 创建炸弹精灵
    auto bombSprite = Sprite::create("Sprites/Characters/Player/Klee/TNT.png");
    if (!bombSprite)
    {
        CCLOG("Failed to create bomb sprite");
        return;
    }

    // 创建炸弹对象
    GameBomb bomb;
    bomb.isExploded = false;
    bomb.sprite = bombSprite;

    // 根据角色朝向决定炸弹方向
    bool facingLeft = _player->isFlippedX();
    float throwDirX = facingLeft ? -1.0f : 1.0f;

    // 设置炸弹初始位置（角色上方）
    Vec2 playerPos = _player->getPosition();
    float offsetX = throwDirX * bomb.sprite->getContentSize().width;
    float offsetY = bomb.sprite->getContentSize().height;
    bombSprite->setPosition(playerPos + Vec2(offsetX, offsetY));
    bombSprite->setScale(0.5f);

    // 创建炸弹物理刚体
    PhysicsMaterial bombMaterial(0.5f, 0.3f, 0.2f);                    // 密度、弹性、摩擦
    auto physicsBody = PhysicsBody::createCircle(15.0f, bombMaterial); // 圆形碰撞体
    physicsBody->setDynamic(true);
    physicsBody->setMass(0.5f);
    physicsBody->setRotationEnable(true); // 允许旋转

    // 设置碰撞掩码
    physicsBody->setCategoryBitmask(static_cast<int>(GamePhysicsCategory::BOMB));
    physicsBody->setCollisionBitmask(static_cast<int>(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION));
    physicsBody->setContactTestBitmask(static_cast<int>(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION));

    bombSprite->addComponent(physicsBody);
    _gameLayer->addChild(bombSprite, 4);

    // 施加初始速度（冲量）
    Vec2 impulse(throwDirX * BOMB_THROW_SPEED_X * physicsBody->getMass(),
                 BOMB_THROW_SPEED_Y * physicsBody->getMass());
    physicsBody->applyImpulse(impulse);

    _bombs.push_back(bomb);

    CCLOG("Bomb thrown with physics!");
}

/**
 * @brief 处理炸弹爆炸
 */
void GameScene::explodeBomb(GameBomb &bomb)
{
    if (!bomb.sprite)
        return;

    Vec2 explodePos = bomb.sprite->getPosition();
    Vec2 explosionWorld = explodePos;
    if (bomb.sprite->getParent())
    {
        explosionWorld = bomb.sprite->getParent()->convertToWorldSpace(explodePos);
    }

    // 移除炸弹精灵
    bomb.sprite->removeFromParent();

    // 对范围内角色造成伤害（排除玩家自身）
    if (_gameLayer)
    {
        DamageInfo dmg;
        dmg.amount = BOMB_DAMAGE;
        dmg.attacker = _player;

        std::function<void(Node *)> applyAoE = [&](Node *node)
        {
            if (!node)
                return;

            if (auto character = dynamic_cast<CharacterBase *>(node))
            {
                if (character != _player)
                {
                    Vec2 worldPos = character->getPosition();
                    if (character->getParent())
                    {
                        worldPos = character->getParent()->convertToWorldSpace(character->getPosition());
                    }

                    if (worldPos.distance(explosionWorld) <= BOMB_EXPLOSION_RADIUS)
                    {
                        character->takeDamage(dmg);
                    }
                }
            }

            const auto &children = node->getChildren();
            for (auto child : children)
            {
                applyAoE(child);
            }
        };

        applyAoE(_gameLayer);
    }

    // 创建爆炸效果
    auto boomSprite = Sprite::create("Sprites/Characters/Player/Klee/BOOM_1.png");
    if (boomSprite)
    {
        boomSprite->setPosition(explodePos);
        boomSprite->setScale(0.8f);
        _gameLayer->addChild(boomSprite, 6);

        // 爆炸动画：放大 + 淡出
        auto scaleUp = ScaleTo::create(0.2f, 1.2f);
        auto fadeOut = FadeOut::create(0.3f);
        auto spawn = Spawn::create(scaleUp, fadeOut, nullptr);
        auto remove = RemoveSelf::create();
        auto sequence = Sequence::create(spawn, remove, nullptr);
        boomSprite->runAction(sequence);
    }

    bomb.sprite = nullptr;

    CCLOG("Bomb exploded at (%.0f, %.0f)", explodePos.x, explodePos.y);
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
    config.backgroundPath = "Map/Origin_Mushroom/jimeng-2025-12-07-8064-Game concept art for a 2D side-scrolling...._0.png";
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
	// 添加一只哥布林怪物作为示例
    auto goblin = GoblinMonster::create();
    if (goblin)
    {
        cocos2d::Vec2 p = getPlayerSpawnPoint();

        goblin->setPosition(p + cocos2d::Vec2(600.0f, 0.0f));
        goblin->setHome(goblin->getPosition());
        if (_player)
        {
            goblin->setTarget(_player);
        }
        _gameLayer->addChild(goblin, PLAYER_Z_ORDER - 1);
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
