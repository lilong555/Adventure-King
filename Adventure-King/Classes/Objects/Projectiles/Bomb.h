#pragma once
//将 GameScene.cpp 中 doThrowBomb 和 explodeBomb 的核心逻辑搬运过来
#ifndef __BOMB_H__
#define __BOMB_H__

#include "Objects/Projectiles/ExplosiveProjectile.h"
#include "Configs/GamePhysicsCategory.h"
#include "Configs/GameConfig.h"

class Bomb : public ExplosiveProjectile
{
public:
    struct PhysicsConfig
    {
        float radius = 15.0f;
        cocos2d::PhysicsMaterial material = GameConfig::Material::BOMB;
        float mass = 0.5f;
        bool rotationEnabled = true;
        bool gravityEnabled = true;
        float linearDamping = 0.0f;

        int categoryBitmask = ToMask(GamePhysicsCategory::BOMB);
        int collisionBitmask = ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER);
        int contactTestBitmask = ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER);
    };

    // 静态创建方法
    static Bomb* create(const std::string& filename);
    static Bomb* create(const std::string& filename, const PhysicsConfig& physicsConfig);

    // 初始化物理属性
    bool initPhysics();
    bool initPhysics(const PhysicsConfig& config);

    // 投掷方法 (由外部调用，给予初始速度)
    void throwAt(const cocos2d::Vec2& velocity);
    // 直接设置线速度
    void setVelocity(const cocos2d::Vec2& velocity);

    static constexpr float DEFAULT_THROW_SPEED_X = GameConfig::Bomb::THROW_SPEED_X;
    static constexpr float DEFAULT_THROW_SPEED_Y = GameConfig::Bomb::THROW_SPEED_Y;
};

#endif // __BOMB_H__
