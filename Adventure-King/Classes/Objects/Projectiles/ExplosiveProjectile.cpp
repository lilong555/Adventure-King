#include "Objects/Projectiles/ExplosiveProjectile.h"

#include "Character/components/AttributeComponent.h"
#include"Character/StatusEffects/StatusEffectFactory.h"
#include "Configs/GameConfig.h"
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

void ExplosiveProjectile::addOnHitStatusEffect(const StatusEffectTemplate &effect)
{
    StatusEffectTemplate validated = effect;
    bool hasInvalid = false;

    if (validated.duration < 0.0f)
    {
        validated.duration = 0.0f;
        hasInvalid = true;
    }

    if (validated.stacks <= 0)
    {
        validated.stacks = 1;
        hasInvalid = true;
    }

    if (validated.maxStacks < 0)
    {
        validated.maxStacks = 0;
        hasInvalid = true;
    }

    if (validated.tickInterval < 0.0f)
    {
        validated.tickInterval = 0.0f;
        hasInvalid = true;
    }

    if (validated.baseDamageScale < 0.0f)
    {
        validated.baseDamageScale = 0.0f;
        hasInvalid = true;
    }

    if (validated.perStackDamageScale < 0.0f)
    {
        validated.perStackDamageScale = 0.0f;
        hasInvalid = true;
    }

#if COCOS2D_DEBUG > 0
    if (hasInvalid)
    {
        CCLOG("ExplosiveProjectile: invalid StatusEffectTemplate corrected (type=%d).", static_cast<int>(validated.type));
    }
#endif

    _onHitStatusEffects.push_back(validated);
}

void ExplosiveProjectile::clearOnHitStatusEffects()
{
    _onHitStatusEffects.clear();
}

void ExplosiveProjectile::explode()
{
    if (_isExploded)
    {
        return;
    }

    // 爆炸只结算一次，避免重复伤害/VFX。
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
    if (_explosionRadius <= 0.0f)
    {
        return;
    }

    auto root = getParent();
    if (!root)
    {
        return;
    }

    // 统一转为世界坐标，避免父节点缩放/偏移影响判定。
    Vec2 explosionWorld = root->convertToWorldSpace(getPosition());

    float sourceAttackPower = 0.0f;
    if (_attacker)
    {
        sourceAttackPower = _attacker->getAttackPower();
    }

    float damageAmount = _baseDamage + sourceAttackPower * _attackPowerDamageScale;

    const bool dealDamage = damageAmount > 0.0f;
    bool isCrit = false;

    if (dealDamage && _attacker)
    {
        auto attr = _attacker->getAttributeComponent();
        if (attr)
        {
            float critRate = attr->getAttributeValue(AttributeType::CRITICAL_RATE);

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

    DamageInfo dmg{};
    dmg.amount = damageAmount;
    dmg.attacker = _attacker;
    dmg.isCritical = isCrit;
    dmg.hitWorldPos = explosionWorld;
    dmg.hasHitWorldPos = true;
    // 投掷物/爆炸的击破值由外部配置（不同技能/武器可能不同）
    dmg.breakDamage = (_breakDamage < 0) ? 0 : _breakDamage;

    std::vector<CharacterBase*> targets;
    targets.reserve(8);

    std::function<void(Node*)> collectTargets = [&](Node* node)
        {
            if (!node)
            {
                return;
            }

            if (auto character = dynamic_cast<CharacterBase*>(node))
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

    // 递归遍历场景树，后续用距离做过滤。
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
                    shapeSizeWorld = static_cast<PhysicsShapeBox*>(shape)->getSize();
                    break;
                case PhysicsShape::Type::CIRCLE:
                {
                    float r = static_cast<PhysicsShapeCircle*>(shape)->getRadius();
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
            // 没有物理形状时，用包围盒作为近似判定。
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

        // 圆-矩形距离判定：用最近点距离比较。
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

        if ((dx * dx + dy * dy) > radiusSq)
        {
            continue;
        }

        if (dealDamage)
        {
            target->takeDamage(dmg);
        }

        if (!_onHitStatusEffects.empty())
        {
            auto attr = target->getAttributeComponent();
            if (attr) // 简化判断
            {
                for (const auto& tmpl : _onHitStatusEffects)
                {
                    auto inst = StatusEffectFactory::createEffectByType(
                        tmpl.type,
                        tmpl.baseDamageScale,
                        tmpl.duration
                    );

                    if (inst)
                    {
                        // 1. 传递关键战斗快照（必须！否则伤害为 0）
                        inst->sourceAttackPower = sourceAttackPower;

                        // 2. 复制模板中的其他配置
                        inst->attributeBonus = tmpl.attributeBonus;
                        inst->stacks = tmpl.stacks;
                        inst->maxStacks = tmpl.maxStacks;
                        inst->stackable = tmpl.stackable;
                        inst->refreshOnAdd = tmpl.refreshOnAdd;
                        inst->tickInterval = tmpl.tickInterval;
                        inst->perStackDamageScale = tmpl.perStackDamageScale;

                        // 3. 挂载到目标身上
                        attr->addStatusEffect(inst);
                    }
                }
            }
        }
    }
}
