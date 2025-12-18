/**
 * @file GameSceneConfig.h
 * @brief GameScene 相关配置结构体（关卡/玩家）
 */

#pragma once

#include <string>
#include <vector>

/**
 * @brief 关卡场景配置
 */
struct LevelConfig
{
    std::string tmxMapPath;
    std::string backgroundPath;
    std::vector<std::string> backgroundSeriesPaths;
    std::string playerSpritePath;

    std::string collisionLayerName = "collisions";
    std::string bornLayerName = "born";
    std::string gateLayerName = "gate";

    float gravity = -1000.0f;
    bool enablePhysicsDebug = false;
};

/**
 * @brief 玩家配置参数
 */
struct PlayerConfig
{
    float scale = 0.25f;
    float walkSpeed = 220.0f;
    float runSpeed = 350.0f;
    float jumpImpulse = 650.0f;
    int maxJumpCount = 2;
    float collisionBoxWidthRatio = 0.8f;
    float collisionBoxHeightRatio = 0.9f;
};
