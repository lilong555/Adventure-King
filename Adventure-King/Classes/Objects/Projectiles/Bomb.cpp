#include "Bomb.h"

#include "Utils/SpriteFrameCacheHelper.h"
#include <algorithm>
#include <string>

USING_NS_CC;

namespace
{
    std::string guessExplosionSpritePath(const std::string &projectileSpritePath)
    {
        if (projectileSpritePath.empty())
        {
            return std::string();
        }

        std::string normalized = projectileSpritePath;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        auto lastSlash = normalized.find_last_of('/');
        if (lastSlash == std::string::npos || lastSlash == 0)
        {
            return std::string();
        }

        return normalized.substr(0, lastSlash) + "/BOOM_1.png";
    }
} // namespace

Bomb* Bomb::create(const std::string& filename)
{
    return Bomb::create(filename, PhysicsConfig{});
}

Bomb *Bomb::create(const std::string &filename, const PhysicsConfig &physicsConfig)
{
    Bomb *bomb = new (std::nothrow) Bomb();
    if (!bomb)
    {
        return nullptr;
    }

    auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(filename);
    if (!frame || !bomb->initWithSpriteFrame(frame))
    {
        CC_SAFE_DELETE(bomb);
        return nullptr;
    }

    bomb->autorelease();
    bomb->initPhysics(physicsConfig);

    bomb->setBaseDamage(GameConfig::Bomb::BASE_DAMAGE);
    bomb->setExplosionRadius(GameConfig::Bomb::EXPLOSION_RADIUS);

    auto explosionPath = guessExplosionSpritePath(filename);
    if (!explosionPath.empty())
    {
        bomb->setExplosionSpriteVfx(explosionPath, 0.8f, 0.2f, 1.2f, 0.3f);
    }

    return bomb;
}

bool Bomb::initPhysics()
{
    return initPhysics(PhysicsConfig{});
}

bool Bomb::initPhysics(const PhysicsConfig &config)
{
    removeComponent(getPhysicsBody());

    auto physicsBody = PhysicsBody::createCircle(config.radius, config.material);
    if (!physicsBody)
    {
        return false;
    }

    physicsBody->setDynamic(true);
    physicsBody->setMass(config.mass);
    physicsBody->setRotationEnable(config.rotationEnabled);
    physicsBody->setGravityEnable(config.gravityEnabled);
    physicsBody->setLinearDamping(config.linearDamping);

    physicsBody->setCategoryBitmask(config.categoryBitmask);
    physicsBody->setCollisionBitmask(config.collisionBitmask);
    physicsBody->setContactTestBitmask(config.contactTestBitmask);

    addComponent(physicsBody);
    return true;
}

void Bomb::throwAt(const Vec2& velocity)
{
    auto body = this->getPhysicsBody();
    if (body)
    {
        // 根据质量应用冲量 F = m * v
        Vec2 impulse(velocity.x * body->getMass(), velocity.y * body->getMass());
        body->applyImpulse(impulse);
    }
}

void Bomb::setVelocity(const Vec2 &velocity)
{
    auto body = getPhysicsBody();
    if (body)
    {
        body->setVelocity(velocity);
    }
}
