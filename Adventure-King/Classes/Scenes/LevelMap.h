/**
 * @file LevelMap.h
 * @brief TMX 关卡地图封装：加载地图/碰撞/门区/敌人生成点
 */

#pragma once

#include "2d/CCTMXObjectGroup.h"
#include "2d/CCTMXTiledMap.h"
#include "cocos2d.h"
#include <functional>
#include <string>
#include <vector>

class PlayerCharacter;
class MonsterBase;

class LevelMap
{
public:
    struct EnemySpawnPoint
    {
        cocos2d::Vec2 position;
        std::string monsterType;
        int count = 1;
        bool hasSpawned = false;
    };

    bool load(cocos2d::Node *gameLayer, const std::string &tmxPath);
    cocos2d::TMXTiledMap *getTileMap() const { return _tileMap; }
    const cocos2d::Size &getMapSizeInPixels() const { return _mapSizeInPixels; }

    void setupRepeatingBackground(cocos2d::Node *gameLayer,
                                  const std::string &backgroundPath,
                                  float mapWidthPixels) const;

    // 使用多张背景图按顺序拼接（用于一次性大背景，如 Origin_Mushroom_0x.png）
    void setupBackgroundSeries(cocos2d::Node *gameLayer,
                               const std::vector<std::string> &backgroundPaths) const;

    void createCollisionBodiesFromTMX(cocos2d::Node *gameLayer,
                                      const std::string &groupName);

    cocos2d::Vec2 getPlayerSpawnPoint(const std::string &bornGroupName = "born") const;

    void loadGateAreas(const std::string &gateGroupName = "gate");
    bool isPointAtGate(const cocos2d::Vec2 &worldPos) const;

    void loadEnemySpawnPoints(const std::string &groupName = "enemy_g");
    void updateEnemySpawns(PlayerCharacter *player,
                           cocos2d::Node *gameLayer,
                           const std::function<MonsterBase *(const std::string &)> &createMonsterByType,
                           float viewDistanceX,
                           float dt);

private:
    bool parseTMXObjectVertices(const cocos2d::ValueMap &dict,
                                double objectX,
                                double objectY,
                                std::vector<cocos2d::Vec2> &outVertices) const;

    void createPolygonCollisionBody(const std::vector<cocos2d::Vec2> &vertices,
                                    const std::string &name,
                                    bool isPolygon);
    void createRectCollisionBody(cocos2d::Node *gameLayer,
                                 const cocos2d::Rect &rect,
                                 const std::string &name) const;

    cocos2d::TMXTiledMap *_tileMap = nullptr;
    cocos2d::Size _mapSizeInPixels;

    std::vector<cocos2d::Rect> _gateAreas;
    std::vector<EnemySpawnPoint> _enemySpawnPoints;

    // 性能：避免每帧全量扫描生成点
    size_t _pendingEnemySpawnPoints = 0;
    float _enemySpawnCheckAccumulator = 0.0f;
};
