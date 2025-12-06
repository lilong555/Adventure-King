/**
 * @file GameScene.cpp
 * @brief 游戏关卡场景实现
 */

#include "GameScene.h"
#include "MapScene.h"
#include "Character/PlayerCharacter.h"
#include "GameUI.h"

USING_NS_CC;

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

        // 添加到场景（高 z-order 确保显示在最上层）
        this->addChild(_gameUI, 100);

        CCLOG("GameUI initialized");
    }
}

void GameScene::returnToMapScene()
{
    CCLOG("Returning to map scene");

    // 创建地图场景并切换
    auto mapScene = MapScene::createScene();
    if (mapScene)
    {
        const float TRANSITION_DURATION = 0.5f;
        auto transition = TransitionFade::create(TRANSITION_DURATION, mapScene, Color3B::BLACK);
        Director::getInstance()->pushScene(transition);
    }
}

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
        this->addChild(background, -1); // Ensure background is behind everything
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

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 计算地图像素尺寸
    Size mapTileSize = _tileMap->getMapSize();
    Size tileSize = _tileMap->getTileSize();
    _mapSizeInPixels = Size(mapTileSize.width * tileSize.width,
                            mapTileSize.height * tileSize.height);

    CCLOG("Map loaded: %s, size=%.0fx%.0f tiles, pixel size=%.0fx%.0f",
          mapPath.c_str(), mapTileSize.width, mapTileSize.height,
          _mapSizeInPixels.width, _mapSizeInPixels.height);

    // 设置地图锚点为左下角
    _tileMap->setAnchorPoint(Vec2(0, 0));

    // 地图左下角与屏幕左下角对齐
    _tileMap->setPosition(Vec2(origin.x, origin.y));

    _tileMap->setTag(TAG_TILEMAP);
    this->addChild(_tileMap, 0);
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
    CCLOG("Collision group '%s' contains %d objects", groupName.c_str(), (int)objects.size());

    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        std::string name = dict["name"].asString();
        std::string type = dict["type"].asString();
        float x = dict["x"].asFloat();
        float y = dict["y"].asFloat();
        float width = dict["width"].asFloat();
        float height = dict["height"].asFloat();

        CCLOG("  Object: name='%s', type='%s', pos=(%.0f,%.0f), size=(%.0f,%.0f)",
              name.c_str(), type.c_str(), x, y, width, height);
    }

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

    // 计算需要多少张图片来覆盖地图宽度
    int repeatCount = static_cast<int>(ceil(mapWidth / bgSize.width)) + 1;

    // 计算背景高度缩放（让背景高度填满屏幕）
    float scaleY = visibleSize.height / bgSize.height;

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

    this->addChild(bgContainer, -1);
    CCLOG("Created repeating background with %d tiles, mapWidth=%.0f, bgWidth=%.0f",
          repeatCount, mapWidth, bgSize.width);
}

void GameScene::initPlayer(const Vec2 &startPos)
{
    // 创建玩家角色（战士职业）
    _player = PlayerCharacter::create(CharacterRole::WARRIOR, "Sprites/Characters/Player/Klee/spr_klee_run.png");

    if (!_player)
    {
        CCLOG("Failed to create player with sprite");
        return;
    }

    // 配置玩家位置和锚点
    _player->setPosition(startPos + Vec2(0, _player->getContentSize().height / 2));
    _player->setAnchorPoint(Vec2(0.5f, 0.0f));
    _player->setScale(1.0f);

    // 物理材质：密度=1.0, 弹性=0（不弹跳）, 摩擦=0（水平移动流畅）
    PhysicsMaterial playerMaterial(1.0f, 0.0f, 0.0f);

    // 计算碰撞体尺寸（略小于精灵以获得更好的游戏体验）
    Size playerSize = _player->getContentSize();
    float scale = _player->getScale();
    float boxWidth = playerSize.width * scale * 0.8f;
    float boxHeight = playerSize.height * scale * 0.95f;

    auto physicsBody = PhysicsBody::createBox(Size(boxWidth, boxHeight), playerMaterial);
    physicsBody->setDynamic(true);
    physicsBody->setRotationEnable(false);
    physicsBody->setMass(1.0f);
    physicsBody->setLinearDamping(0.0f);

    // 配置碰撞掩码
    physicsBody->setCategoryBitmask(GAME_CATEGORY_PLAYER);
    physicsBody->setCollisionBitmask(GAME_CATEGORY_PLATFORM | GAME_CATEGORY_COLLISION);
    physicsBody->setContactTestBitmask(GAME_CATEGORY_PLATFORM | GAME_CATEGORY_COLLISION);

    _player->addComponent(physicsBody);
    this->addChild(_player, 5);

    // 设置死亡时不自动移除
    _player->setAutoRemoveOnDeath(false);

    // 初始化地面状态
    _isGrounded = true;
    _groundContactCount = 1;

    CCLOG("Player created with physics at position (%.0f, %.0f)", startPos.x, startPos.y);
}

void GameScene::initPhysicsContactListener()
{
    auto contactListener = EventListenerPhysicsContact::create();

    // 碰撞开始回调
    contactListener->onContactBegin = CC_CALLBACK_1(GameScene::onContactBegin, this);

    // 碰撞预处理回调
    contactListener->onContactPreSolve = [this](PhysicsContact &contact, PhysicsContactPreSolve &solve) -> bool
    {
        auto nodeA = contact.getShapeA()->getBody()->getNode();
        auto nodeB = contact.getShapeB()->getBody()->getNode();

        if (!nodeA || !nodeB)
            return true;

        int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
        int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

        // 玩家与碰撞体碰撞时
        if ((categoryA == GAME_CATEGORY_PLAYER && (categoryB & (GAME_CATEGORY_PLATFORM | GAME_CATEGORY_COLLISION))) ||
            ((categoryA & (GAME_CATEGORY_PLATFORM | GAME_CATEGORY_COLLISION)) && categoryB == GAME_CATEGORY_PLAYER))
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
    CCLOG("Creating collision bodies from '%s', %d objects found", groupName.c_str(), (int)objects.size());

    PhysicsMaterial collisionMaterial(1.0f, 0.0f, 0.8f);

    // 获取地图像素高度（用于 Y 坐标转换）
    Size mapSize = _tileMap->getMapSize();
    Size tileSize = _tileMap->getTileSize();
    float mapHeightInPixels = mapSize.height * tileSize.height;

    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        std::string name = dict["name"].asString();
        float x = dict["x"].asFloat();
        float y = dict["y"].asFloat();
        float width = dict["width"].asFloat();
        float height = dict["height"].asFloat();

        // 调试：打印所有键值对
        CCLOG("  Object '%s': x=%.0f, y=%.0f, w=%.0f, h=%.0f", name.c_str(), x, y, width, height);
        for (const auto &kv : dict)
        {
            CCLOG("    Key: '%s', Type: %d", kv.first.c_str(), (int)kv.second.getType());
        }

        // cocos2d-x TMX 解析器将 polygon 存储为 "points"，polyline 存储为 "polylinePoints"
        bool hasPolygon = (dict.find("points") != dict.end());
        bool hasPolyline = (dict.find("polylinePoints") != dict.end());

        CCLOG("    hasPolygon(points)=%d, hasPolyline(polylinePoints)=%d", hasPolygon, hasPolyline);

        if (hasPolygon || hasPolyline)
        {
            // 解析多边形/折线点
            ValueVector points;
            bool isPolygon = hasPolygon;
            if (isPolygon)
            {
                points = dict["points"].asValueVector();
            }
            else
            {
                points = dict["polylinePoints"].asValueVector();
            }
            CCLOG("    Points count: %zu", points.size());

            if (points.size() >= 2)
            {
                std::vector<Vec2> vertices;
                for (const auto &pt : points)
                {
                    auto ptDict = pt.asValueMap();
                    float px = ptDict["x"].asFloat();
                    // cocos2d-x TMX解析器已经处理了相对坐标，但Y轴仍需翻转（相对于对象原点）
                    float py = -ptDict["y"].asFloat();
                    vertices.push_back(Vec2(px, py));
                    CCLOG("      Vertex: (%.0f, %.0f) -> (%.0f, %.0f)", ptDict["x"].asFloat(), ptDict["y"].asFloat(), px, py);
                }

                auto collisionNode = Node::create();
                // cocos2d-x 解析器已经将对象坐标转换为 cocos2d 坐标系
                // x, y 现在就是 cocos2d 坐标
                collisionNode->setPosition(Vec2(x, y) + _tileMap->getPosition());

                PhysicsBody *physicsBody = nullptr;

                if (isPolygon && vertices.size() >= 3)
                {
                    // 尝试创建多边形物理体
                    physicsBody = PhysicsBody::createPolygon(vertices.data(), (int)vertices.size(), collisionMaterial);

                    if (!physicsBody)
                    {
                        // 如果多边形创建失败（非凸或顺序问题），使用边链
                        CCLOG("    Polygon creation failed, trying edge chain for '%s'", name.c_str());
                        physicsBody = PhysicsBody::createEdgeChain(vertices.data(), (int)vertices.size(), collisionMaterial);
                    }
                }
                else
                {
                    // 折线使用边链
                    physicsBody = PhysicsBody::createEdgeChain(vertices.data(), (int)vertices.size(), collisionMaterial);
                }

                if (physicsBody)
                {
                    physicsBody->setDynamic(false);
                    physicsBody->setCategoryBitmask(GAME_CATEGORY_COLLISION);
                    physicsBody->setCollisionBitmask(GAME_CATEGORY_PLAYER);
                    physicsBody->setContactTestBitmask(GAME_CATEGORY_PLAYER);

                    collisionNode->addComponent(physicsBody);
                    this->addChild(collisionNode, 1);
                    CCLOG("  Created %s collision: name='%s', %zu vertices at (%.0f, %.0f)",
                          isPolygon ? "polygon" : "polyline", name.c_str(), vertices.size(), x, y);
                }
                else
                {
                    CCLOG("  Failed to create collision body for '%s'", name.c_str());
                }
            }
        }
        else if (width > 0 && height > 0)
        {
            // 矩形碰撞体
            // cocos2d-x 解析器已经将坐标转换为 cocos2d 坐标系
            // x, y 现在是矩形左下角的坐标
            float rectCenterX = x + width / 2;
            float rectCenterY = y + height / 2;

            auto collisionNode = Node::create();
            collisionNode->setPosition(Vec2(rectCenterX, rectCenterY) + _tileMap->getPosition());

            auto physicsBody = PhysicsBody::createBox(Size(width, height), collisionMaterial);
            physicsBody->setDynamic(false);
            physicsBody->setCategoryBitmask(GAME_CATEGORY_COLLISION);
            physicsBody->setCollisionBitmask(GAME_CATEGORY_PLAYER);
            physicsBody->setContactTestBitmask(GAME_CATEGORY_PLAYER);

            collisionNode->addComponent(physicsBody);
            this->addChild(collisionNode, 1);
            CCLOG("  Created rect collision: name='%s', size=(%.0f, %.0f) at (%.0f, %.0f)",
                  name.c_str(), width, height, rectCenterX, rectCenterY);
        }
    }
}

Vec2 GameScene::getPlayerSpawnPoint()
{
    if (!_tileMap)
    {
        CCLOG("Warning: Cannot get spawn point - tilemap not loaded");
        return Vec2(100, 200); // 默认出生点
    }

    auto bornGroup = _tileMap->getObjectGroup("born");
    if (!bornGroup)
    {
        CCLOG("Warning: 'born' object group not found in tilemap, using default spawn");
        return Vec2(100, 200);
    }

    auto objects = bornGroup->getObjects();
    if (objects.empty())
    {
        CCLOG("Warning: No objects in 'born' group, using default spawn");
        return Vec2(100, 200);
    }

    // 获取第一个出生点对象
    auto dict = objects[0].asValueMap();
    float x = dict["x"].asFloat();
    float y = dict["y"].asFloat();

    // cocos2d-x TMX 解析器已经将坐标转换为 cocos2d 坐标系
    Vec2 spawnPoint(x, y);

    CCLOG("Player spawn point found: (%.0f, %.0f)", spawnPoint.x, spawnPoint.y);
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
    CCLOG("Loading gate areas: %zu objects found", objects.size());

    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        std::string name = dict["name"].asString();
        float x = dict["x"].asFloat();
        float y = dict["y"].asFloat();
        float width = dict["width"].asFloat();
        float height = dict["height"].asFloat();

        // 如果没有宽高，使用默认大小
        if (width <= 0)
            width = GATE_INTERACT_DISTANCE * 2;
        if (height <= 0)
            height = Director::getInstance()->getVisibleSize().height; // 让传送门区域覆盖屏幕高度
        // 创建 gate 区域矩形
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
            CCLOG("Player is at gate area: (%.0f, %.0f)", gateRect.origin.x, gateRect.origin.y);
            return true;
        }
    }

    return false;
}

bool GameScene::onContactBegin(PhysicsContact &contact)
{
    auto nodeA = contact.getShapeA()->getBody()->getNode();
    auto nodeB = contact.getShapeB()->getBody()->getNode();

    if (!nodeA || !nodeB)
        return true;

    int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
    int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

    // 玩家与碰撞体接触 - 检测是否落地
    bool playerIsA = (categoryA == GAME_CATEGORY_PLAYER);
    bool playerIsB = (categoryB == GAME_CATEGORY_PLAYER);
    bool collisionWithPlatform =
        (playerIsA && (categoryB & (GAME_CATEGORY_PLATFORM | GAME_CATEGORY_COLLISION))) ||
        (playerIsB && (categoryA & (GAME_CATEGORY_PLATFORM | GAME_CATEGORY_COLLISION)));

    if (collisionWithPlatform)
    {
        // 获取碰撞法向量判断是否从上方落下
        auto contactData = contact.getContactData();
        if (contactData)
        {
            Vec2 normal = contactData->normal;

            // 法向量的方向取决于碰撞顺序
            // 如果玩家是 A，法向量指向 B（从 A 到 B）
            // 如果玩家是 B，法向量需要反转
            if (playerIsB)
            {
                normal = -normal;
            }

            // 现在 normal 是从玩家指向平台的向量
            // 如果 normal.y < -0.3（向下），说明平台在玩家下方，玩家站在上面
            if (normal.y < -0.3f)
            {
                _groundContactCount++;
                _isGrounded = true;
                CCLOG("Player grounded (normal.y=%.2f), contact count: %d", normal.y, _groundContactCount);
            }
            else
            {
                CCLOG("Player touched platform but not grounded (normal.y=%.2f)", normal.y);
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

    // 玩家与碰撞体分离
    bool playerIsA = (categoryA == GAME_CATEGORY_PLAYER);
    bool playerIsB = (categoryB == GAME_CATEGORY_PLAYER);
    bool collisionWithPlatform =
        (playerIsA && (categoryB & (GAME_CATEGORY_PLATFORM | GAME_CATEGORY_COLLISION))) ||
        (playerIsB && (categoryA & (GAME_CATEGORY_PLATFORM | GAME_CATEGORY_COLLISION)));

    if (collisionWithPlatform && _groundContactCount > 0)
    {
        // 使用射线检测确认是否真的离开地面
        // 简化处理：只在玩家有向上速度或接触计数为正时才减少计数
        if (_player && _player->getPhysicsBody())
        {
            Vec2 velocity = _player->getPhysicsBody()->getVelocity();

            // 如果玩家正在向上移动（跳跃），减少接触计数
            // 或者如果玩家没有向下的速度，也减少计数
            _groundContactCount--;

            if (_groundContactCount <= 0)
            {
                _groundContactCount = 0;
                // 只有当玩家真的在空中（有向上或向下的速度）时才设置为非着地
                if (fabsf(velocity.y) > 10.0f)
                {
                    _isGrounded = false;
                    CCLOG("Player left ground (velocity.y=%.2f)", velocity.y);
                }
                else
                {
                    // 可能是在平坦表面上移动，保持着地状态
                    CCLOG("Player contact ended but staying grounded (velocity.y=%.2f)", velocity.y);
                }
            }
            else
            {
                CCLOG("Player still has ground contacts: %d", _groundContactCount);
            }
        }
    }
}

void GameScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event *event)
{
    if (!_player)
        return;

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _isMovingLeft = true;
        _player->setFlippedX(true);
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = true;
        _player->setFlippedX(false);
        break;

    case EventKeyboard::KeyCode::KEY_W:
        // 先检查是否在传送门区域，如果是则返回地图
        if (isPlayerAtGate())
        {
            CCLOG("Player at gate, returning to map scene");
            returnToMapScene();
            return;
        }
        // 否则执行跳跃
        if (_isGrounded && _player->getPhysicsBody())
        {
            _player->getPhysicsBody()->applyImpulse(Vec2(0, JUMP_IMPULSE));
            _isGrounded = false;
            CCLOG("Player jumped");
        }
        break;

    case EventKeyboard::KeyCode::KEY_SPACE:
        // 跳跃（空格键只跳跃，不触发传送门）
        if (_isGrounded && _player->getPhysicsBody())
        {
            _player->getPhysicsBody()->applyImpulse(Vec2(0, JUMP_IMPULSE));
            _isGrounded = false;
            CCLOG("Player jumped");
        }
        break;

    case EventKeyboard::KeyCode::KEY_ESCAPE:
        returnToMapScene();
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
}

void GameScene::update(float dt)
{
    if (!_player || !_player->getPhysicsBody())
        return;

    auto physicsBody = _player->getPhysicsBody();
    Vec2 velocity = physicsBody->getVelocity();

    // 水平移动
    float targetVelocityX = 0.0f;
    if (_isMovingLeft)
    {
        targetVelocityX = -_moveSpeed;
    }
    else if (_isMovingRight)
    {
        targetVelocityX = _moveSpeed;
    }

    velocity.x = targetVelocityX;
    physicsBody->setVelocity(velocity);

    // 辅助着地检测：如果玩家垂直速度很小且有接触计数，确保着地状态
    // 这可以修复某些多边形边缘导致的误判
    if (_groundContactCount > 0 && fabsf(velocity.y) < 5.0f)
    {
        _isGrounded = true;
    }
    // 如果玩家在下落且速度很大，确保非着地状态
    else if (velocity.y < -100.0f && _groundContactCount <= 0)
    {
        _isGrounded = false;
    }

    // 更新行走动画状态标记（动画功能可后续实现）
    _isWalkAnimationPlaying = (_isMovingLeft || _isMovingRight);

    // 更新 UI 位置（跟随相机）
    if (_gameUI)
    {
        // 获取场景当前位置（Follow 动作会改变场景位置）
        Vec2 scenePos = this->getPosition();
        // UI 需要反向偏移以保持在屏幕固定位置
        _gameUI->updatePosition(-scenePos);
    }

    // 检查是否在传送门区域，更新交互提示
    bool atGate = isPlayerAtGate();
    if (atGate && !_wasAtGate)
    {
        // 刚进入传送门区域
        if (_gameUI)
        {
            _gameUI->showInteractionHint("按 W 键进入传送门");
        }
    }
    else if (!atGate && _wasAtGate)
    {
        // 刚离开传送门区域
        if (_gameUI)
        {
            _gameUI->hideInteractionHint();
        }
    }
    _wasAtGate = atGate;
}

// ============================================================
// OriginMushroomScene 实现（起源之菇）
// ============================================================

Scene *OriginMushroomScene::createScene()
{
    return OriginMushroomScene::create();
}

bool OriginMushroomScene::init()
{
    //-------------------------------------------------------------------------
    // 步骤1：物理引擎初始化
    //-------------------------------------------------------------------------
    if (!Scene::initWithPhysics())
    {
        return false;
    }

    // 配置物理世界参数
    auto physicsWorld = this->getPhysicsWorld();
    physicsWorld->setGravity(Vec2(0, -800.0f));

    // 开启物理调试绘制（开发时可视化碰撞体，发布时应关闭）
    physicsWorld->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    //-------------------------------------------------------------------------
    // 步骤2：加载 TMX 瓦片地图
    //-------------------------------------------------------------------------
    if (!loadTileMap("Map/Map1.tmx"))
    {
        CCLOG("Warning: Failed to load Map1.tmx, using fallback UI");

        Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);
        auto titleLabel = Label::createWithTTF(
            "起源之菇 - 地图加载失败",
            "fonts/ZCOOLKuaiLe-Regular.ttf",
            48);
        if (titleLabel)
        {
            titleLabel->setPosition(center);
            titleLabel->setColor(Color3B::RED);
            this->addChild(titleLabel, 1);
        }
        return true;
    }

    //-------------------------------------------------------------------------
    // 步骤3：设置横向重复背景
    //-------------------------------------------------------------------------
    setupRepeatingBackground("Map/map_background.png", _mapSizeInPixels.width);

    //-------------------------------------------------------------------------
    // 步骤4：从碰撞图层创建物理碰撞体
    //-------------------------------------------------------------------------
    createCollisionBodiesFromTMX("collisions");

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

    // 键盘输入监听
    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = CC_CALLBACK_2(OriginMushroomScene::onKeyPressed, this);
    keyboardListener->onKeyReleased = CC_CALLBACK_2(OriginMushroomScene::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    //-------------------------------------------------------------------------
    // 步骤8：设置相机跟随
    //-------------------------------------------------------------------------
    if (_player)
    {
        // 定义地图边界（世界坐标）
        Rect worldBound(0, 0, _mapSizeInPixels.width, _mapSizeInPixels.height);

        // 让场景跟随玩家移动
        auto followAction = Follow::create(_player, worldBound);
        this->runAction(followAction);

        CCLOG("Camera follow enabled, world bound: (%.0f, %.0f, %.0f, %.0f)",
              worldBound.origin.x, worldBound.origin.y,
              worldBound.size.width, worldBound.size.height);
    }

    //-------------------------------------------------------------------------
    // 步骤9：启用帧更新
    //-------------------------------------------------------------------------
    scheduleUpdate();

    // 初始化游戏 UI（在相机跟随设置后创建，确保 UI 层级正确）
    initGameUI();

    CCLOG("OriginMushroomScene initialized with Physics Engine and Player");
    return true;
}

// ============================================================
// MysteryForestScene 实现（神秘之森）
// ============================================================

Scene *MysteryForestScene::createScene()
{
    return MysteryForestScene::create();
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

    // TODO: 设置神秘之森场景的背景
    // setupBackground("Scene/Backgrounds/MysteryForest.png");

    // 临时：添加场景标题标签
    auto titleLabel = Label::createWithTTF(
        "神秘之森",
        "fonts/ZCOOLKuaiLe-Regular.ttf",
        72);

    if (titleLabel)
    {
        titleLabel->setPosition(center);
        titleLabel->setColor(Color3B::WHITE);
        this->addChild(titleLabel, 1);
    }

    // 添加提示文字
    auto hintLabel = Label::createWithTTF(
        "点击左上角地图进入选择界面",
        "fonts/ZCOOLKuaiLe-Regular.ttf",
        32);

    if (hintLabel)
    {
        hintLabel->setPosition(Vec2(center.x, center.y - 80));
        hintLabel->setColor(Color3B(200, 200, 200));
        this->addChild(hintLabel, 1);
    }

    CCLOG("MysteryForestScene initialized");
    return true;
}
