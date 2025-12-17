#include "Objects/Projectiles/ExplosiveProjectile.h"

#include "Character/components/AttributeComponent.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>

USING_NS_CC;

namespace
{
    constexpr int LOOP_ANIMATION_TAG = 0x1EAF;

    Animation *createAnimationFromPaths(const std::vector<std::string> &paths, float delayPerUnit)
    {
        Vector<SpriteFrame *> frames;
        frames.reserve(paths.size());

        for (const auto &path : paths)
        {
            auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(path);
            if (frame)
            {
                frames.pushBack(frame);
            }
        }

        if (frames.empty())
        {
            return nullptr;
        }

        return Animation::createWithSpriteFrames(frames, delayPerUnit);
    }
} // namespace

void ExplosiveProjectile::setExplosionSpriteVfx(const std::string &spritePath,
                                                float scale,
                                                float scaleUpDuration,
                                                float scaleUpFactor,
                                                float fadeOutDuration)
{
    _explosionFramePaths.clear();

    _explosionSpritePath = spritePath;
    _explosionSpriteScale = scale;
    _explosionSpriteScaleUpDuration = scaleUpDuration;
    _explosionSpriteScaleUpFactor = scaleUpFactor;
    _explosionSpriteFadeOutDuration = fadeOutDuration;
}

void ExplosiveProjectile::setExplosionFrameVfx(const std::vector<std::string> &framePaths,
                                               float frameDelay,
                                               float frameScale)
{
    _explosionSpritePath.clear();

    _explosionFramePaths = framePaths;
    _explosionFrameDelay = frameDelay;
    _explosionFrameScale = frameScale;
}

void ExplosiveProjectile::setLoopAnimation(const std::vector<std::string> &framePaths, float frameDelay)
{
    _loopAnimationPaths = framePaths;
    _loopAnimationDelay = frameDelay;

    stopActionByTag(LOOP_ANIMATION_TAG);

    if (_loopAnimationPaths.empty())
    {
        return;
    }

    auto anim = createAnimationFromPaths(_loopAnimationPaths, _loopAnimationDelay);
    if (!anim)
    {
        return;
    }

    auto action = RepeatForever::create(Animate::create(anim));
    action->setTag(LOOP_ANIMATION_TAG);
    runAction(action);
}

void ExplosiveProjectile::explode()
{
    if (_isExploded)
    {
        return;
    }

    _isExploded = true;

    stopActionByTag(LOOP_ANIMATION_TAG);

    playExplosionVfx();
    applyAoEDamage();

    removeFromParent();
}

void ExplosiveProjectile::playExplosionVfx()
{
    auto parent = getParent();
    if (!parent)
    {
        return;
    }

    const Vec2 pos = getPosition();

    if (!_explosionFramePaths.empty())
    {
        auto animation = createAnimationFromPaths(_explosionFramePaths, _explosionFrameDelay);
        if (!animation)
        {
            return;
        }

        auto firstFrame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(_explosionFramePaths.front());
        if (!firstFrame)
        {
            return;
        }

        auto sprite = Sprite::createWithSpriteFrame(firstFrame);
        if (!sprite)
        {
            return;
        }

        sprite->setPosition(pos);
        sprite->setScale(_explosionFrameScale);
        parent->addChild(sprite, 6);
        sprite->runAction(Sequence::create(Animate::create(animation), RemoveSelf::create(), nullptr));
        return;
    }

    if (!_explosionSpritePath.empty())
    {
        auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(_explosionSpritePath);
        if (!frame)
        {
            return;
        }

        auto sprite = Sprite::createWithSpriteFrame(frame);
        if (!sprite)
        {
            return;
        }

        sprite->setPosition(pos);
        sprite->setScale(_explosionSpriteScale);
        parent->addChild(sprite, 6);

        auto scaleUp = ScaleTo::create(_explosionSpriteScaleUpDuration,
                                       _explosionSpriteScale * _explosionSpriteScaleUpFactor);
        auto fadeOut = FadeOut::create(_explosionSpriteFadeOutDuration);
        auto spawn = Spawn::create(scaleUp, fadeOut, nullptr);
        sprite->runAction(Sequence::create(spawn, RemoveSelf::create(), nullptr));
    }
}

void ExplosiveProjectile::applyAoEDamage()
{
    if (_baseDamage <= 0.0f)
    {
        return;
    }
    if (_explosionRadius <= 0.0f)
    {
        return;
    }

    auto root = getParent();
    if (!root)
    {
        return;
    }

    Vec2 explosionWorld = getPosition();
    if (root)
    {
        explosionWorld = root->convertToWorldSpace(getPosition());
    }

    float damageAmount = _baseDamage;
    bool isCrit = false;

    if (_attacker)
    {
        auto attr = _attacker->getAttributeComponent();
        if (attr)
        {
            float critRate = attr->getAttributeValue(AttributeType::CRITICAL_RATE);
            float strength = attr->getAttributeValue(AttributeType::STRENGTH);
            damageAmount += (strength * 5.0f);

            // MSVC 当前工程可能未启用 C++17，避免使用 std::clamp
            float critChancePercent = critRate * 100.0f;
            critChancePercent = std::max(0.0f, std::min(critChancePercent, 100.0f));
            if ((rand() % 100) < static_cast<int>(critChancePercent))
            {
                isCrit = true;
                damageAmount *= 1.5f;
            }
        }
    }

    DamageInfo dmg;
    dmg.amount = damageAmount;
    dmg.attacker = _attacker;
    dmg.isCritical = isCrit;

    std::vector<CharacterBase *> targets;
    targets.reserve(8);

    std::function<void(Node *)> collectTargets = [&](Node *node)
    {
        if (!node)
        {
            return;
        }

        if (auto character = dynamic_cast<CharacterBase *>(node))
        {
            if (character != _attacker && !character->isDead())
            {
                targets.push_back(character);
            }
        }

        auto children = node->getChildren();
        for (auto child : children)
        {
            collectTargets(child);
        }
    };

    collectTargets(root);

    const float radiusSq = _explosionRadius * _explosionRadius;

    for (auto target : targets)
    {
        if (!target || target->isDead())
        {
            continue;
        }

        Rect hitRectWorld;
        bool hasHitRectWorld = false;

        if (auto body = target->getPhysicsBody())
        {
            auto shape = body->getFirstShape();
            if (shape)
            {
                Size shapeSizeWorld;
                switch (shape->getType())
                {
                case PhysicsShape::Type::BOX:
                    shapeSizeWorld = static_cast<PhysicsShapeBox *>(shape)->getSize();
                    break;
                case PhysicsShape::Type::CIRCLE:
                {
                    float r = static_cast<PhysicsShapeCircle *>(shape)->getRadius();
                    shapeSizeWorld = Size(r * 2.0f, r * 2.0f);
                    break;
                }
                default:
                    break;
                }

                if (shapeSizeWorld.width > 0.0f && shapeSizeWorld.height > 0.0f)
                {
                    Vec2 centerLocal(target->getContentSize().width * 0.5f,
                                     target->getContentSize().height * 0.5f);
                    Vec2 bodyCenterWorld = target->convertToWorldSpace(centerLocal);
                    Vec2 rectCenterWorld = bodyCenterWorld + shape->getCenter();

                    hitRectWorld = Rect(rectCenterWorld.x - shapeSizeWorld.width / 2.0f,
                                        rectCenterWorld.y - shapeSizeWorld.height / 2.0f,
                                        shapeSizeWorld.width,
                                        shapeSizeWorld.height);
                    hasHitRectWorld = true;
                }
            }
        }

        if (!hasHitRectWorld)
        {
            Rect bboxParent = target->getBoundingBox();
            Vec2 originWorld = bboxParent.origin;
            Vec2 topRightWorld = bboxParent.origin + bboxParent.size;
            if (target->getParent())
            {
                originWorld = target->getParent()->convertToWorldSpace(bboxParent.origin);
                topRightWorld = target->getParent()->convertToWorldSpace(bboxParent.origin + bboxParent.size);
            }

            hitRectWorld = Rect(
                std::min(originWorld.x, topRightWorld.x),
                std::min(originWorld.y, topRightWorld.y),
                std::fabs(topRightWorld.x - originWorld.x),
                std::fabs(topRightWorld.y - originWorld.y));
        }

        float dx = 0.0f;
        if (explosionWorld.x < hitRectWorld.getMinX())
            dx = hitRectWorld.getMinX() - explosionWorld.x;
        else if (explosionWorld.x > hitRectWorld.getMaxX())
            dx = explosionWorld.x - hitRectWorld.getMaxX();

        float dy = 0.0f;
        if (explosionWorld.y < hitRectWorld.getMinY())
            dy = hitRectWorld.getMinY() - explosionWorld.y;
        else if (explosionWorld.y > hitRectWorld.getMaxY())
            dy = explosionWorld.y - hitRectWorld.getMaxY();

        if ((dx * dx + dy * dy) <= radiusSq)
        {
            target->takeDamage(dmg);
        }
    }
}
