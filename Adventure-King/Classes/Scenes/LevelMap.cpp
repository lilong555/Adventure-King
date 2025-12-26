/**
 * @file LevelMap.cpp
 * @brief TMX 关卡地图封装实现
 */

#include "Scenes/LevelMap.h"
#include "Scenes/GameScene.h"
#include "Character/Monster/MonsterBase.h"
#include "Character/Player/PlayerCharacter.h"
#include "Configs/GameConfig.h"
#include "Configs/GamePhysicsCategory.h"
#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"
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
    if (!_tileMap) return;

    // 直接作为 _tileMap 的子节点
    auto collisionNode = Node::create();

    // TMX 对象的 rect.origin 已经是左下角坐标
    Vec2 center(rect.origin.x + rect.size.width / 2, rect.origin.y + rect.size.height / 2);
    collisionNode->setPosition(center);

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

    collisionNode->setPhysicsBody(physicsBody);
    _tileMap->addChild(collisionNode); // 关键：统一加到地图里
   /* CCLOG("  Created rect collision: name='%s', size=(%.0f, %.0f) at (%.0f, %.0f)",
          name.c_str(), rect.size.width, rect.size.height, rectCenterX, rectCenterY);*/
}

Vec2 LevelMap::getPlayerSpawnPoint(const std::string& bornGroupName) const
{
    if (!_tileMap) return DEFAULT_SPAWN_POINT;
    auto bornGroup = _tileMap->getObjectGroup(bornGroupName);
    if (!bornGroup) return DEFAULT_SPAWN_POINT;

    auto objects = bornGroup->getObjects();
    for (const auto& obj : objects)
    {
        auto dict = obj.asValueMap();
        // 只有名字匹配 "PlayerSpawn" 才是真正的出生点
        if (dict["name"].asString() == "PlayerSpawn")
        {
            float x = dict["x"].asFloat();
            float y = dict["y"].asFloat();
            CCLOG("Found PlayerSpawn at: (%f, %f)", x, y);
            return Vec2(x, y);
        }
    }

    CCLOG("Warning: 'PlayerSpawn' not found in group '%s'", bornGroupName.c_str());
    return DEFAULT_SPAWN_POINT;
}

void LevelMap::loadGateAreas(const std::string& gateGroupName)
{
    _gateAreas.clear();
    auto gateGroup = _tileMap->getObjectGroup(gateGroupName);
    if (!gateGroup) return;

    auto objects = gateGroup->getObjects();

    for (const auto& obj : objects)
    {
        auto dict = obj.asValueMap();
        float x = dict["x"].asFloat();
        float y = dict["y"].asFloat();
        float w = dict["width"].asFloat();
        float h = dict["height"].asFloat();

        Rect gateRect(x, y, w, h);
        _gateAreas.push_back(gateRect);

        // --- 添加以下代码进行可视化 ---
#if COCOS2D_DEBUG > 0
        auto debugDraw = DrawNode::create();
        // 画一个半透明的蓝色矩形代表传送门
        debugDraw->drawSolidRect(Vec2(x, y), Vec2(x + w, y + h), Color4F(0, 0, 1, 0.3f));
        _tileMap->addChild(debugDraw, 10);
#endif
    }
}

bool LevelMap::isPointAtGate(const Vec2 &worldPos) const
{
    // 如果关卡未清空，传送门不产生任何交互
    if (!_isLevelCleared) return false;

    if (_gateAreas.empty()) return false;

    for (const auto &gateRect : _gateAreas)
    {
        if (gateRect.containsPoint(worldPos)) return true;
    }

    return false;
}

//触发胜利逻辑
void LevelMap::triggerLevelClear() {
    _isLevelCleared = true;
    // 为所有门区域播放激活特效
    for (auto& rect : _gateAreas) {
        auto portalVfx = ParticleSystemQuad::create("Particle/portal_active.plist");
        portalVfx->setPosition(rect.getMidX(), rect.getMidY());
        _tileMap->addChild(portalVfx, 10);
    }
    // 调用 UI 提示
    showVictoryBanner();
}
void LevelMap::showVictoryBanner() {
    auto visibleSize = Director::getInstance()->getVisibleSize();

    auto banner = Sprite::create("Scene/UI/ClearBanner.png");
    banner->setPosition(visibleSize.width / 2, visibleSize.height / 2);
    banner->setScale(1.0f);
    banner->setOpacity(0);

    // 获取当前场景的 UI 层并添加
    Director::getInstance()->getRunningScene()->addChild(banner, 1000);

    // 播放“丝之歌”风格的弹出效果
    banner->runAction(Sequence::create(
        Spawn::create(
            EaseBackOut::create(ScaleTo::create(0.5f, 1.2f)), // 回弹缩放
            FadeIn::create(0.3f),
            nullptr
        ),
        DelayTime::create(2.0f), // 停留两秒
        FadeOut::create(0.5f),
        RemoveSelf::create(),
        nullptr
    ));
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
    }
}

//下面实现连战逻辑相关代码

void LevelMap::loadArenas(const std::string& layerName, Node* gameLayer) {
    auto group = _tileMap->getObjectGroup(layerName);
    if (!group) return;

    auto& objects = group->getObjects();
    for (auto& obj : objects) {
        ValueMap dict = obj.asValueMap();
        std::string type = dict["type"].asString();
        std::string arenaID = dict["arenaID"].asString();
        if (arenaID.empty()) continue;

        if (_arenas.find(arenaID) == _arenas.end()) {
            _arenas[arenaID] = new ArenaConfig();
            _arenas[arenaID]->arenaID = arenaID;
        }
        auto* arena = _arenas[arenaID];

        if (type == "ArenaTrigger") {
            arena->triggerRect = Rect(dict["x"].asFloat(), dict["y"].asFloat(),
                dict["width"].asFloat(), dict["height"].asFloat());
            parseWaves(arena, dict["waves"].asString());
        }
        else if (type == "ArenaSpawn") {
            arena->spawnPoints.push_back(Vec2(dict["x"].asFloat(), dict["y"].asFloat()));
        }
        else if (type == "ArenaGate") {
            // 创建物理门
            auto gate = Node::create();
            float w = dict["width"].asFloat();
            float h = dict["height"].asFloat();
            gate->setPosition(dict["x"].asFloat() + w / 2, dict["y"].asFloat() + h / 2);

            auto pb = PhysicsBody::createBox(Size(w, h), COLLISION_PHYSICS_MATERIAL);
            pb->setDynamic(false);
            // 设置为碰撞层，阻止玩家和怪穿过
            pb->setCategoryBitmask(ToMask(GamePhysicsCategory::COLLISION));
            pb->setCollisionBitmask(ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::MONSTER));

            gate->setPhysicsBody(pb);
            gate->setVisible(false); // 默认不显示
            pb->setEnabled(false);   // 默认不开启碰撞

            _tileMap->addChild(gate); // 挂在地图上，随地图移动
            arena->gates.push_back(gate);
        }
    }
}

void LevelMap::updateArenas(PlayerCharacter* player, Node* gameLayer,
    const std::function<MonsterBase* (const std::string&)>& createMonsterByType) {
    for (auto& pair : _arenas) {
        auto* arena = pair.second;
        if (arena->isFinished || arena->isActivated) continue;

        // 触发检测
        if (arena->triggerRect.containsPoint(player->getPosition())) {
            arena->isActivated = true;

            // 1. 关门：启用碰撞和显示（可以加特效）
            for (auto gate : arena->gates) {
                gate->setVisible(true);
                gate->getPhysicsBody()->setEnabled(true);
            }

            // 2. 开启第一波
            spawnNextWave(arena, player, gameLayer, createMonsterByType);

            CCLOG("Arena [%s] Activated! Total Waves: %zu", arena->arenaID.c_str(), arena->waves.size());
        }
    }
}

void LevelMap::spawnNextWave(ArenaConfig* arena, PlayerCharacter* player, Node* gameLayer,
    const std::function<MonsterBase* (const std::string&)>& createMonsterByType) {
    // 检查是否所有波次已结束
    if (arena->currentWaveIndex >= arena->waves.size()) {
        arena->isFinished = true;
        arena->isActivated = false;

        // 开门逻辑
        for (auto gate : arena->gates) {
            gate->runAction(Sequence::create(
                FadeOut::create(0.5f),
                CallFunc::create([gate]() {
                    gate->setVisible(false);
                    gate->getPhysicsBody()->setEnabled(false);
                    }),
                nullptr
            ));
        }
        CCLOG("Arena [%s] Cleared!", arena->arenaID.c_str());
        return;
    }

    auto& wave = arena->waves[arena->currentWaveIndex];
    arena->activeMonstersCount = 0;

    CCLOG("Arena [%s] - Starting Wave %d", arena->arenaID.c_str(), arena->currentWaveIndex + 1);

    for (auto const& [type, count] : wave.enemies) {
        for (int i = 0; i < count; ++i) {
            auto monster = createMonsterByType(type);
            if (!monster) continue;

            // 随机选择竞技场内的刷怪点
            Vec2 spawnPos = arena->spawnPoints[random(0, (int)arena->spawnPoints.size() - 1)];
            monster->setPosition(spawnPos);
            monster->setTarget(player);
            monster->setHome(spawnPos);

            // --- 核心优化：使用 CharacterBase 的新接口 ---
            monster->setOnDeathCallback([this, arena, player, gameLayer, createMonsterByType](CharacterBase* deadChar) {
                arena->activeMonstersCount--;

                // 当本波所有怪死亡
                if (arena->activeMonstersCount <= 0) {
                    arena->currentWaveIndex++;

                    // 延迟进入下一波，给玩家喘息和拾取掉落的时间
                    auto delay = DelayTime::create(1.5f);
                    auto next = CallFunc::create([this, arena, player, gameLayer, createMonsterByType]() {
                        this->spawnNextWave(arena, player, gameLayer, createMonsterByType);
                        });
                    gameLayer->runAction(Sequence::create(delay, next, nullptr));
                }
                });

            gameLayer->addChild(monster, DEFAULT_CHARACTER_Z_ORDER);
            arena->activeMonstersCount++;
        }
    }
}

void LevelMap::parseWaves(ArenaConfig* config, const std::string& jsonStr) {
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    if (doc.HasParseError() || !doc.IsArray()) {
        CCLOG("Error: Failed to parse Arena waves JSON: %s", jsonStr.c_str());
        return;
    }

    for (rapidjson::SizeType i = 0; i < doc.Size(); i++) {
        WaveData wave;
        const auto& waveObj = doc[i];
        for (auto it = waveObj.MemberBegin(); it != waveObj.MemberEnd(); ++it) {
            wave.enemies[it->name.GetString()] = it->value.GetInt();
        }
        config->waves.push_back(wave);
    }
}
