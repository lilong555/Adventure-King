#pragma once

#include "cocos2d.h"
#include <string>
#include <vector>

struct ProjectileExplosionVfx
{
    // Frame-based explosion (preferred for VFX sequences)
    std::vector<std::string> framePaths;
    float frameDelay = 0.05f;
    float frameScale = 1.0f;

    // Single-sprite explosion (fallback)
    std::string spritePath;
    float spriteScale = 1.0f;
    float spriteScaleUpDuration = 0.2f;
    float spriteScaleUpFactor = 1.2f;
    float spriteFadeOutDuration = 0.3f;
};

enum class ProjectileMoveType
{
    IMPULSE,
    VELOCITY,
};

struct PlayerProjectileConfig
{
    // Visual
    std::string spritePath;
    float spriteScale = 1.0f;
    bool flipXWithFacing = true;

    // Optional looping animation while flying
    std::vector<std::string> loopAnimationPaths;
    float loopAnimationDelay = 0.08f;

    // Physics
    float physicsRadius = 16.0f;
    cocos2d::PhysicsMaterial material = cocos2d::PhysicsMaterial(0.5f, 0.0f, 0.0f);
    float mass = 0.4f;
    bool rotationEnabled = false;
    bool gravityEnabled = true;
    float linearDamping = 0.0f;
    int categoryBitmask = 0;
    int collisionBitmask = 0;
    int contactTestBitmask = 0;

    // Spawn relative to player bounding box center
    float spawnOffsetXRatio = 0.0f;
    float spawnOffsetX = 0.0f;
    float spawnOffsetYRatio = 0.0f;
    float spawnOffsetY = 0.0f;

    // Movement: X will be multiplied by facing direction sign
    ProjectileMoveType moveType = ProjectileMoveType::VELOCITY;
    cocos2d::Vec2 moveVector = cocos2d::Vec2::ZERO;
    bool scaleMoveByMass = false; // for impulse: applyImpulse(moveVector * mass)

    // Damage
    float damage = 0.0f;
    float explosionRadius = 0.0f;
    bool explodeOnContact = true;

    // Explosion VFX
    ProjectileExplosionVfx explosionVfx;
};

