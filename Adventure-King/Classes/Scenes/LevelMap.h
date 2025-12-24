/**
 * @file LevelMap.h
 * @brief TMX 关卡地图封装：加载地图/碰撞/门区/敌人生成点
 */

#pragma once

#include "2d/CCTMXObjectGroup.h"
#include "2d/CCTMXTiledMap.h"
#include "cocos2d.h"
#include "Configs/ArenaConfig.h" // 确保你之前定义的这个头文件在路径中
#include <map>
#include <functional>
#include <string>
#include <vector>

class PlayerCharacter;
class MonsterBase;

class LevelMap
{
public:

    // 析构清理
    ~LevelMap() {
        for (auto& pair : _arenas) {
            delete pair.second;
        }
        _arenas.clear();
    }
    /// @brief 刷怪点数据结构
    struct EnemySpawnPoint
    {
        cocos2d::Vec2 position;
        std::string monsterType;
        int count = 1;
        bool hasSpawned = false;
    };

    /// @brief 加载 TMX 地图与基础层
    bool load(cocos2d::Node *gameLayer, const std::string &tmxPath);
    /// @brief 获取 TMX 地图指针
    cocos2d::TMXTiledMap *getTileMap() const { return _tileMap; }
    /// @brief 获取地图像素尺寸
    const cocos2d::Size &getMapSizeInPixels() const { return _mapSizeInPixels; }

    /// @brief 平铺背景（单图重复）
    void setupRepeatingBackground(cocos2d::Node *gameLayer,
                                  const std::string &backgroundPath,
                                  float mapWidthPixels) const;

    // 使用多张背景图按顺序拼接（用于一次性大背景，如 Origin_Mushroom_0x.png）
    /// @brief 按顺序拼接背景序列
    void setupBackgroundSeries(cocos2d::Node *gameLayer,
                               const std::vector<std::string> &backgroundPaths) const;

    /// @brief 解析碰撞对象组并创建物理体
    void createCollisionBodiesFromTMX(cocos2d::Node *gameLayer,
                                      const std::string &groupName);

    /// @brief 获取出生点（born 对象组）
    cocos2d::Vec2 getPlayerSpawnPoint(const std::string &bornGroupName = "born") const;

    /// @brief 加载门区（gate 对象组）
    void loadGateAreas(const std::string &gateGroupName = "gate");
    /// @brief 判断点是否处于任意门区
    bool isPointAtGate(const cocos2d::Vec2 &worldPos) const;

    /// @brief 读取刷怪点（enemy_g 对象组）
    void loadEnemySpawnPoints(const std::string &groupName = "enemy_g");
    /// @brief 根据视野与计时触发刷怪
    void updateEnemySpawns(PlayerCharacter *player,
                           cocos2d::Node *gameLayer,
                           const std::function<MonsterBase *(const std::string &)> &createMonsterByType,
                           float viewDistanceX,
                           float dt);

    /**
     * @brief 加载竞技场层数据
     * @param layerName Tiled中的对象层名称（如 "ArenaLayer"）
     * @param gameLayer 节点容器
     */
    void loadArenas(const std::string& layerName, cocos2d::Node* gameLayer);

    /**
     * @brief 每帧检测竞技场触发
     */
    void updateArenas(PlayerCharacter* player,
        cocos2d::Node* gameLayer,
        const std::function<MonsterBase* (const std::string&)>& createMonsterByType);

private:
    // 解析 JSON 波次数据
    void parseWaves(ArenaConfig* config, const std::string& jsonStr);
    // 触发下一波怪
    void spawnNextWave(ArenaConfig* arena,
        PlayerCharacter* player,
        cocos2d::Node* gameLayer,
        const std::function<MonsterBase* (const std::string&)>& createMonsterByType);

    // 存储所有竞技场配置，Key 是 Tiled 里的 arenaID
    std::map<std::string, ArenaConfig*> _arenas;

    /// @brief 解析 TMX 多边形/折线顶点
    bool parseTMXObjectVertices(const cocos2d::ValueMap &dict,
                                double objectX,
                                double objectY,
                                std::vector<cocos2d::Vec2> &outVertices) const;

    /// @brief 创建多边形/折线碰撞体
    void createPolygonCollisionBody(const std::vector<cocos2d::Vec2> &vertices,
                                    const std::string &name,
                                    bool isPolygon);
    /// @brief 创建矩形碰撞体
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
