/**
 * @file LevelMap.cpp
 * @brief TMX 关卡地图封装实现
 */

#include "Scenes/LevelMap.h"
#include "Character/Monster/MonsterBase.h"
#include "Character/Player/PlayerCharacter.h"
#include "Configs/GameConfigs.h"
#include "Configs/GamePhysicsCategory.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdlib>

USING_NS_CC;

namespace
{
    const Vec2 DEFAULT_SPAWN_POINT = GameConfig::LevelMap::DEFAULT_SPAWN_POINT;
    const PhysicsMaterial COLLISION_PHYSICS_MATERIAL = GameConfig::LevelMap::COLLISION_PHYSICS_MATERIAL; // 密度, 弹性, 摩擦

    constexpr float DEFAULT_GATE_INTERACT_DISTANCE = GameConfig::LevelMap::DEFAULT_GATE_INTERACT_DISTANCE;
    constexpr float SPAWN_SPACING_X = GameConfig::LevelMap::SPAWN_SPACING_X;
    constexpr float SPAWN_INTERVAL_SECONDS = GameConfig::LevelMap::SPAWN_INTERVAL_SECONDS;
    constexpr int DEFAULT_CHARACTER_Z_ORDER = GameConfig::LevelMap::DEFAULT_CHARACTER_Z_ORDER;

    // 敌人生成点检测频率：不需要每帧检测，降低 CPU 开销
    constexpr float ENEMY_SPAWN_CHECK_INTERVAL_SECONDS = GameConfig::LevelMap::ENEMY_SPAWN_CHECK_INTERVAL_SECONDS;
}

bool LevelMap::load(Node *gameLayer, const std::string &tmxPath)
{
    if (!gameLayer)
    {
        CCLOG("LevelMap::load - gameLayer is null");
        return false;
    }

    _tileMap = TMXTiledMap::create(tmxPath);
    if (!_tileMap)
    {
        CCLOG("LevelMap::load - failed to load TMX: %s", tmxPath.c_str());
        return false;
    }

    auto origin = Director::getInstance()->getVisibleOrigin();

    Size mapTileSize = _tileMap->getMapSize();
    Size tileSize = _tileMap->getTileSize();
    _mapSizeInPixels = Size(mapTileSize.width * tileSize.width,
                            mapTileSize.height * tileSize.height);

    _tileMap->setAnchorPoint(Vec2(0, 0));
    _tileMap->setPosition(Vec2(origin.x, origin.y));
    gameLayer->addChild(_tileMap, 0);

    CCLOG("LevelMap loaded: %s, tiles=%.0fx%.0f, pixels=%.0fx%.0f",
          tmxPath.c_str(), mapTileSize.width, mapTileSize.height,
          _mapSizeInPixels.width, _mapSizeInPixels.height);
    return true;
}

void LevelMap::setupRepeatingBackground(Node *gameLayer,
                                        const std::string &backgroundPath,
                                        float mapWidthPixels) const
{
    if (!gameLayer)
        return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto tempSprite = Sprite::create(backgroundPath);
    if (!tempSprite)
    {
        CCLOG("LevelMap::setupRepeatingBackground - failed to load: %s", backgroundPath.c_str());
        return;
    }

    Size bgSize = tempSprite->getContentSize();
    float scaleY = visibleSize.height / bgSize.height;
    int repeatCount = static_cast<int>(ceil(mapWidthPixels / (bgSize.width * scaleY))) + 1;

    auto bgContainer = Node::create();

    for (int i = 0; i < repeatCount; i++)
    {
        auto bgSprite = Sprite::create(backgroundPath);
        if (!bgSprite)
            continue;

        bgSprite->setAnchorPoint(Vec2(0, 0));
        bgSprite->setScale(scaleY);
        bgSprite->setPosition(Vec2(i * bgSize.width * scaleY, origin.y));
        bgContainer->addChild(bgSprite);
    }

    gameLayer->addChild(bgContainer, -1);
    CCLOG("Created repeating background: %d tiles, mapWidth=%.0f", repeatCount, mapWidthPixels);
}

void LevelMap::setupBackgroundSeries(Node *gameLayer,
                                     const std::vector<std::string> &backgroundPaths) const
{
    if (!gameLayer)
        return;

    if (backgroundPaths.empty())
        return;

    auto origin = Director::getInstance()->getVisibleOrigin();

    std::vector<Sprite *> sprites;
    sprites.reserve(backgroundPaths.size());

    float xOffset = 0.0f;
    for (const auto &path : backgroundPaths)
    {
        auto bgSprite = Sprite::create(path);
        if (!bgSprite)
        {
            CCLOG("LevelMap::setupBackgroundSeries - failed to load: %s", path.c_str());
            return;
        }

        bgSprite->setAnchorPoint(Vec2(0, 0));
        bgSprite->setPosition(Vec2(xOffset, 0.0f));

        xOffset += bgSprite->getContentSize().width;
        sprites.push_back(bgSprite);
    }

    auto bgContainer = Node::create();
    bgContainer->setPosition(origin);
    for (auto *const sprite : sprites)
    {
        bgContainer->addChild(sprite);
    }

    gameLayer->addChild(bgContainer, -1);
    CCLOG("Created background series: %zu tiles", backgroundPaths.size());
}

void LevelMap::createCollisionBodiesFromTMX(Node *gameLayer, const std::string &groupName)
{
    if (!_tileMap)
    {
        CCLOG("LevelMap::createCollisionBodiesFromTMX - tilemap not loaded");
        return;
    }
    if (!gameLayer)
    {
        CCLOG("LevelMap::createCollisionBodiesFromTMX - gameLayer is null");
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

        std::vector<Vec2> vertices;
        bool isPolygon = parseTMXObjectVertices(dict, x, y, vertices);

        if (!vertices.empty())
        {
            createPolygonCollisionBody(vertices, name, isPolygon);
        }
        else if (width > 0 && height > 0)
        {
            createRectCollisionBody(gameLayer, Rect(x, y, width, height), name);
        }
    }
}

bool LevelMap::parseTMXObjectVertices(const ValueMap &dict,
                                     double objectX,
                                     double objectY,
                                     std::vector<Vec2> &outVertices) const
{
    outVertices.clear();

    bool hasPolygon = (dict.find("points") != dict.end());
    bool hasPolyline = (dict.find("polylinePoints") != dict.end());
    if (!hasPolygon && !hasPolyline)
    {
        return false;
    }

    ValueVector points;
    bool isPolygon = hasPolygon;
    points = isPolygon ? dict.at("points").asValueVector()
                       : dict.at("polylinePoints").asValueVector();

    if (points.size() < 2)
    {
        return isPolygon;
    }

    const double scaleFactor = Director::getInstance()->getContentScaleFactor();
    for (const auto &pt : points)
    {
        auto ptDict = pt.asValueMap();
        double px = ptDict["x"].asDouble() / scaleFactor;
        double py = ptDict["y"].asDouble() / scaleFactor;
        outVertices.push_back(Vec2(objectX + px, objectY - py));
    }

    if (isPolygon && outVertices.size() >= 3)
    {
        if (outVertices.front() != outVertices.back())
        {
            outVertices.push_back(outVertices.front());
        }
    }

    return isPolygon;
}

void LevelMap::createPolygonCollisionBody(const std::vector<Vec2> &vertices,
                                         const std::string &name,
                                         bool isPolygon)
{
    if (!_tileMap)
        return;
    if (vertices.size() < 2)
    {
        CCLOG("Warning: Not enough vertices for collision body '%s'", name.c_str());
        return;
    }

    auto collisionNode = Node::create();
    collisionNode->setPosition(Vec2::ZERO);

#if COCOS2D_DEBUG > 0
    auto drawNode = DrawNode::create();
    drawNode->setPosition(Vec2::ZERO);
    drawNode->drawPoly(vertices.data(), static_cast<unsigned int>(vertices.size()),
                       true, Color4F(0, 1, 0, 0.5f));
    collisionNode->addChild(drawNode, 100);
#endif

    auto physicsBody = PhysicsBody::createEdgeChain(vertices.data(),
                                                    static_cast<int>(vertices.size()),
                                                    COLLISION_PHYSICS_MATERIAL);
    if (!physicsBody)
    {
        CCLOG("  Error: Failed to create collision body for '%s'", name.c_str());
        return;
    }

    physicsBody->setDynamic(false);
    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLATFORM));
    physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLAYER |
                                            GamePhysicsCategory::MONSTER |
                                            GamePhysicsCategory::PLAYER_ATTACK |
                                            GamePhysicsCategory::BOMB));
    physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER |
                                              GamePhysicsCategory::MONSTER |
                                              GamePhysicsCategory::PLAYER_ATTACK |
                                              GamePhysicsCategory::BOMB));

    collisionNode->addComponent(physicsBody);
    _tileMap->addChild(collisionNode, 1);

    CCLOG("  Created %s collision: name='%s', %zu vertices",
          isPolygon ? "polygon" : "polyline", name.c_str(), vertices.size());
}

void LevelMap::createRectCollisionBody(Node *gameLayer,
                                      const Rect &rect,
                                      const std::string &name) const
{
    if (!gameLayer || !_tileMap)
        return;

    double rectCenterX = rect.origin.x + rect.size.width / 2;
    double rectCenterY = rect.origin.y + rect.size.height / 2;

    auto collisionNode = Node::create();
    collisionNode->setPosition(Vec2(rectCenterX, rectCenterY) + _tileMap->getPosition());

    auto physicsBody = PhysicsBody::createBox(rect.size, COLLISION_PHYSICS_MATERIAL);
    physicsBody->setDynamic(false);
    physicsBody->setRotationEnable(false);

    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLATFORM));
    physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLAYER |
                                            GamePhysicsCategory::MONSTER |
                                            GamePhysicsCategory::PLAYER_ATTACK |
                                            GamePhysicsCategory::BOMB));
    physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER |
                                              GamePhysicsCategory::MONSTER |
                                              GamePhysicsCategory::PLAYER_ATTACK |
                                              GamePhysicsCategory::BOMB));

    collisionNode->addComponent(physicsBody);
    gameLayer->addChild(collisionNode, 1);

    CCLOG("  Created rect collision: name='%s', size=(%.0f, %.0f) at (%.0f, %.0f)",
          name.c_str(), rect.size.width, rect.size.height, rectCenterX, rectCenterY);
}

Vec2 LevelMap::getPlayerSpawnPoint(const std::string &bornGroupName) const
{
    if (!_tileMap)
    {
        CCLOG("Warning: Cannot get spawn point - tilemap not loaded");
        return DEFAULT_SPAWN_POINT;
    }

    auto bornGroup = _tileMap->getObjectGroup(bornGroupName);
    if (!bornGroup)
    {
        CCLOG("Warning: '%s' object group not found, using default spawn", bornGroupName.c_str());
        return DEFAULT_SPAWN_POINT;
    }

    auto objects = bornGroup->getObjects();
    if (objects.empty())
    {
        CCLOG("Warning: No objects in '%s' group, using default spawn", bornGroupName.c_str());
        return DEFAULT_SPAWN_POINT;
    }

    auto dict = objects[0].asValueMap();
    Vec2 spawnPoint(dict["x"].asDouble(), dict["y"].asDouble());
    CCLOG("Player spawn point: (%.0f, %.0f)", spawnPoint.x, spawnPoint.y);
    return spawnPoint;
}

void LevelMap::loadGateAreas(const std::string &gateGroupName)
{
    _gateAreas.clear();

    if (!_tileMap)
    {
        CCLOG("Warning: Cannot load gate areas - tilemap not loaded");
        return;
    }

    auto gateGroup = _tileMap->getObjectGroup(gateGroupName);
    if (!gateGroup)
    {
        CCLOG("Info: '%s' object group not found in tilemap", gateGroupName.c_str());
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

bool LevelMap::isPointAtGate(const Vec2 &worldPos) const
{
    if (_gateAreas.empty())
    {
        return false;
    }

    for (const auto &gateRect : _gateAreas)
    {
        if (gateRect.containsPoint(worldPos))
        {
            return true;
        }
    }

    return false;
}

void LevelMap::loadEnemySpawnPoints(const std::string &groupName)
{
    _enemySpawnPoints.clear();
    _pendingEnemySpawnPoints = 0;
    _enemySpawnCheckAccumulator = 0.0f;

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

    _pendingEnemySpawnPoints = _enemySpawnPoints.size();
}

void LevelMap::updateEnemySpawns(PlayerCharacter *player,
                                Node *gameLayer,
                                const std::function<MonsterBase *(const std::string &)> &createMonsterByType,
                                float viewDistanceX,
                                float dt)
{
    if (!player || !gameLayer || _enemySpawnPoints.empty())
        return;
    if (!createMonsterByType)
        return;
    if (_pendingEnemySpawnPoints == 0)
        return;

    // 允许外部以 dt<=0 的方式强制立即检测（例如场景初始化时）
    if (dt > 0.0f)
    {
        _enemySpawnCheckAccumulator += dt;
        if (_enemySpawnCheckAccumulator < ENEMY_SPAWN_CHECK_INTERVAL_SECONDS)
        {
            return;
        }
    }
    _enemySpawnCheckAccumulator = 0.0f;

    const float playerX = player->getPositionX();
    const float minX = playerX - viewDistanceX;
    const float maxX = playerX + viewDistanceX;

    auto startIt = std::lower_bound(_enemySpawnPoints.begin(),
                                    _enemySpawnPoints.end(),
                                    minX,
                                    [](const EnemySpawnPoint &spawnPoint, float x)
                                    { return spawnPoint.position.x < x; });
    // 刷怪点已按 X 排序，利用 lower_bound 限定视野内遍历区间。

    for (auto it = startIt; it != _enemySpawnPoints.end(); ++it)
    {
        auto &spawnPoint = *it;

        if (spawnPoint.position.x > maxX)
            break;

        if (spawnPoint.hasSpawned)
            continue;

        const int count = std::max(1, spawnPoint.count);
        const float centerIndex = (static_cast<float>(count) - 1.0f) * 0.5f;

        spawnPoint.hasSpawned = true;
        if (_pendingEnemySpawnPoints > 0)
        {
            _pendingEnemySpawnPoints--;
        }

        for (int i = 0; i < count; ++i)
        {
            const std::string monsterType = spawnPoint.monsterType;
            const float offsetX = (static_cast<float>(i) - centerIndex) * SPAWN_SPACING_X;
            const Vec2 monsterPos = spawnPoint.position + Vec2(offsetX, 0.0f);

            const float delaySeconds = static_cast<float>(i) * SPAWN_INTERVAL_SECONDS;
            // 分批延迟生成，避免同一帧刷出过多怪物造成卡顿。
            gameLayer->runAction(Sequence::create(
                DelayTime::create(delaySeconds),
                CallFunc::create([createMonsterByType, player, gameLayer, monsterType, monsterPos]()
                                 {
                                     if (!player || !gameLayer)
                                         return;

                                     auto monster = createMonsterByType(monsterType);
                                     if (!monster)
                                         return;

                                     monster->setPosition(monsterPos);
                                     monster->setTarget(player);
                                     monster->setHome(monsterPos);
                                     gameLayer->addChild(monster, DEFAULT_CHARACTER_Z_ORDER);
                                 }),
                nullptr));
        }

        CCLOG("Enemy spawn triggered: type='%s', count=%d, pos=(%.0f, %.0f)",
              spawnPoint.monsterType.c_str(), count,
              spawnPoint.position.x, spawnPoint.position.y);
    }
}
