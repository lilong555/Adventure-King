#pragma once
//此处存储连战场景相关配置
#include "cocos2d.h"
#include <vector>
#include <string>

struct WaveData {
    // 怪物类型和对应的数量
    std::map<std::string, int> enemies;
};

struct ArenaConfig {
    std::string arenaID;
    cocos2d::Rect triggerRect;
    std::vector<cocos2d::Vec2> spawnPoints;
    std::vector<cocos2d::Node*> gates;
    std::vector<WaveData> waves;

    int currentWaveIndex = 0;
    int activeMonstersCount = 0;
    bool isActivated = false;
    bool isFinished = false;
};
