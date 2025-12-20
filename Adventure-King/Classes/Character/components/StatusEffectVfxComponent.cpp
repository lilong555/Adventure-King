#include "Character/components/StatusEffectVfxComponent.h"

#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include <algorithm>
#include <cmath>
#include <vector>

USING_NS_CC;

namespace
{
    const char* const BURNING_PARTICLE_NAME = "BurningParticle";

    struct BurningParticleConfig
    {
        int totalParticles = 120;

        Vec2 gravity = Vec2(0.0f, 70.0f);
        float angle = 90.0f;
        float angleVar = 25.0f;
        float speed = 25.0f;
        float speedVar = 12.0f;
        float life = 0.55f;
        float lifeVar = 0.25f;

        float startSize = 14.0f;
        float startSizeVar = 6.0f;
        float endSize = 0.0f;

        Color4F startColor = Color4F(1.0f, 0.55f, 0.10f, 0.75f);
        Color4F startColorVar = Color4F(0.10f, 0.10f, 0.10f, 0.10f);
        Color4F endColor = Color4F(0.70f, 0.12f, 0.05f, 0.00f);
        Color4F endColorVar = Color4F(0.10f, 0.10f, 0.10f, 0.00f);

        // 位置：使用 Owner 的本地坐标，避免受 scaleX 翻转影响出现偏移
        float emitterOffsetYRatio = StatusEffectVfxComponent::BurningVfxParams::EMITTER_OFFSET_Y_RATIO;
        float posVarXRatio = StatusEffectVfxComponent::BurningVfxParams::POS_VAR_X_RATIO;
        float posVarYRatio = StatusEffectVfxComponent::BurningVfxParams::POS_VAR_Y_RATIO;
        float posVarXMax = StatusEffectVfxComponent::BurningVfxParams::POS_VAR_X_MAX;
        float posVarYMax = StatusEffectVfxComponent::BurningVfxParams::POS_VAR_Y_MAX;
        float maxStartSize = StatusEffectVfxComponent::BurningVfxParams::MAX_START_SIZE;

        float baseEmission = 55.0f;
        float perStackEmission = 25.0f;
    };

    const BurningParticleConfig kBurningParticleConfig;

    struct BodyLocalInfo
    {
        Size size = Size::ZERO;
        Vec2 center = Vec2::ZERO;
    };

    void updateBounds(const Vec2& point, bool& hasPoint, Vec2& minPoint, Vec2& maxPoint)
    {
        if (!hasPoint)
        {
            minPoint = point;
            maxPoint = point;
            hasPoint = true;
            return;
        }
        minPoint.x = std::min(minPoint.x, point.x);
        minPoint.y = std::min(minPoint.y, point.y);
        maxPoint.x = std::max(maxPoint.x, point.x);
        maxPoint.y = std::max(maxPoint.y, point.y);
    }

    void addLocalPointFromWorld(const Node* owner,
                                const Vec2& worldPoint,
                                bool& hasPoint,
                                Vec2& minPoint,
                                Vec2& maxPoint)
    {
        if (!owner)
        {
            return;
        }
        Vec2 localPoint = owner->convertToNodeSpace(worldPoint);
        updateBounds(localPoint, hasPoint, minPoint, maxPoint);
    }

    void addLocalPointFromBody(const Node* owner,
                               PhysicsBody* body,
                               const Vec2& bodyLocalPoint,
                               bool& hasPoint,
                               Vec2& minPoint,
                               Vec2& maxPoint)
    {
        if (!owner || !body)
        {
            return;
        }
        Vec2 worldPoint = body->local2World(bodyLocalPoint);
        addLocalPointFromWorld(owner, worldPoint, hasPoint, minPoint, maxPoint);
    }

    BodyLocalInfo getBodyLocalInfo(Node* owner)
    {
        BodyLocalInfo info;
        if (!owner)
        {
            return info;
        }

        bool hasPoint = false;
        Vec2 minPoint;
        Vec2 maxPoint;

        if (auto body = owner->getPhysicsBody())
        {
            const auto& shapes = body->getShapes();
            for (auto shape : shapes)
            {
                if (!shape)
                {
                    continue;
                }

                if (auto circle = dynamic_cast<PhysicsShapeCircle*>(shape))
                {
                    const Vec2 center = circle->getCenter();
                    const float radius = circle->getRadius();
                    addLocalPointFromBody(owner, body, center + Vec2(radius, 0.0f), hasPoint, minPoint, maxPoint);
                    addLocalPointFromBody(owner, body, center + Vec2(-radius, 0.0f), hasPoint, minPoint, maxPoint);
                    addLocalPointFromBody(owner, body, center + Vec2(0.0f, radius), hasPoint, minPoint, maxPoint);
                    addLocalPointFromBody(owner, body, center + Vec2(0.0f, -radius), hasPoint, minPoint, maxPoint);
                    continue;
                }

                if (auto poly = dynamic_cast<PhysicsShapePolygon*>(shape))
                {
                    const int count = poly->getPointsCount();
                    if (count <= 0)
                    {
                        continue;
                    }
                    std::vector<Vec2> points(static_cast<size_t>(count));
                    poly->getPoints(points.data());
                    for (const auto& point : points)
                    {
                        addLocalPointFromBody(owner, body, point, hasPoint, minPoint, maxPoint);
                    }
                }
            }
        }

        if (!hasPoint)
        {
            Rect bbox = owner->getBoundingBox();
            Vec2 originWorld = bbox.origin;
            Vec2 topRightWorld = bbox.origin + bbox.size;
            if (auto parent = owner->getParent())
            {
                originWorld = parent->convertToWorldSpace(bbox.origin);
                topRightWorld = parent->convertToWorldSpace(bbox.origin + bbox.size);
            }
            addLocalPointFromWorld(owner, originWorld, hasPoint, minPoint, maxPoint);
            addLocalPointFromWorld(owner, topRightWorld, hasPoint, minPoint, maxPoint);
        }

        if (hasPoint)
        {
            info.size = Size(std::max(0.0f, maxPoint.x - minPoint.x),
                             std::max(0.0f, maxPoint.y - minPoint.y));
            info.center = Vec2((minPoint.x + maxPoint.x) * 0.5f,
                               (minPoint.y + maxPoint.y) * 0.5f);
        }
        return info;
    }

    Texture2D* getWhiteParticleTexture()
    {
        static Texture2D* s_texture = nullptr;
        if (s_texture)
        {
            return s_texture;
        }

        unsigned char rgba[] = {255, 255, 255, 255};
        s_texture = new (std::nothrow) Texture2D();
        if (s_texture && s_texture->initWithData(rgba,
                                                 sizeof(rgba),
                                                 Texture2D::PixelFormat::RGBA8888,
                                                 1,
                                                 1,
                                                 Size(1, 1)))
        {
            s_texture->autorelease();
            s_texture->retain();
        }
        else
        {
            CC_SAFE_DELETE(s_texture);
        }

        return s_texture;
    }

    void applyBurningParticleStyle(ParticleSystemQuad* particle)
    {
        if (!particle)
        {
            return;
        }

        auto texture = getWhiteParticleTexture();
        if (texture)
        {
            particle->setTexture(texture);
        }

        particle->setBlendAdditive(true);
        particle->setDuration(ParticleSystem::DURATION_INFINITY);
        particle->setEmitterMode(ParticleSystem::Mode::GRAVITY);

        particle->setGravity(kBurningParticleConfig.gravity);
        particle->setAngle(kBurningParticleConfig.angle);
        particle->setAngleVar(kBurningParticleConfig.angleVar);
        particle->setSpeed(kBurningParticleConfig.speed);
        particle->setSpeedVar(kBurningParticleConfig.speedVar);
        particle->setLife(kBurningParticleConfig.life);
        particle->setLifeVar(kBurningParticleConfig.lifeVar);

        particle->setStartSize(kBurningParticleConfig.startSize);
        particle->setStartSizeVar(kBurningParticleConfig.startSizeVar);
        particle->setEndSize(kBurningParticleConfig.endSize);

        particle->setStartColor(kBurningParticleConfig.startColor);
        particle->setStartColorVar(kBurningParticleConfig.startColorVar);
        particle->setEndColor(kBurningParticleConfig.endColor);
        particle->setEndColorVar(kBurningParticleConfig.endColorVar);

        particle->setPositionType(ParticleSystem::PositionType::GROUPED);
        particle->setPosition(Vec2::ZERO);
    }

    Vec2 getBurningEmitterLocalPos(const BodyLocalInfo& bodyInfo)
    {
        float bottomY = bodyInfo.center.y - bodyInfo.size.height * 0.5f;
        return Vec2(bodyInfo.center.x,
                    bottomY + bodyInfo.size.height * kBurningParticleConfig.emitterOffsetYRatio);
    }

    void updateBurningParticleIntensity(ParticleSystemQuad* particle,
                                        const BodyLocalInfo& bodyInfo,
                                        int stacks)
    {
        if (!particle)
        {
            return;
        }

        const float posVarX = std::min(bodyInfo.size.width * kBurningParticleConfig.posVarXRatio,
                                       kBurningParticleConfig.posVarXMax);
        const float posVarY = std::min(bodyInfo.size.height * kBurningParticleConfig.posVarYRatio,
                                       kBurningParticleConfig.posVarYMax);
        particle->setPosVar(Vec2(posVarX, posVarY));

        const float rawSize = bodyInfo.size.height * 0.08f;
        const float baseSize = std::min(kBurningParticleConfig.maxStartSize,
                                        std::max(6.0f, rawSize));
        particle->setStartSize(baseSize);
        particle->setStartSizeVar(baseSize * 0.5f);
        particle->setEndSize(0.0f);

        stacks = std::max(1, stacks);
        particle->setEmissionRate(kBurningParticleConfig.baseEmission +
                                  kBurningParticleConfig.perStackEmission * static_cast<float>(stacks - 1));
    }
} // namespace

const char* const StatusEffectVfxComponent::BURNING_VFX_NAME = "StatusEffectVfx_Burning";

StatusEffectVfxComponent::StatusEffectVfxComponent()
{
    setName("StatusEffectVfxComponent");
}

StatusEffectVfxComponent::~StatusEffectVfxComponent() = default;

bool StatusEffectVfxComponent::init()
{
    return Component::init();
}

void StatusEffectVfxComponent::onAdd()
{
    if (getOwner())
    {
        getOwner()->scheduleUpdate();
    }
}

void StatusEffectVfxComponent::update(float dt)
{
    Component::update(dt);

    auto owner = dynamic_cast<CharacterBase*>(getOwner());
    if (!owner || owner->isDead())
    {
        return;
    }

    auto attr = owner->getAttributeComponent();
    if (!attr)
    {
        return;
    }

    updateBurningVfx(owner, attr);
}

void StatusEffectVfxComponent::updateBurningVfx(Node* owner, AttributeComponent* attr)
{
    const bool burning = attr->hasStatusEffect(StatusEffectType::BURNING);
    auto existing = owner->getChildByName(BURNING_VFX_NAME);

    if (!burning)
    {
        if (existing)
        {
            existing->removeFromParent();
        }
        return;
    }

    const int stacks = getStacks(attr, StatusEffectType::BURNING);
    const auto bodyInfo = getBodyLocalInfo(owner);

    if (!existing)
    {
        existing = Node::create();
        existing->setName(BURNING_VFX_NAME);
        owner->addChild(existing, 999);

        auto particle = ParticleSystemQuad::createWithTotalParticles(kBurningParticleConfig.totalParticles);
        particle->setName(BURNING_PARTICLE_NAME);
        existing->addChild(particle);
        applyBurningParticleStyle(particle);
        updateBurningParticleIntensity(particle, bodyInfo, stacks);
    }

    existing->setPosition(getBurningEmitterLocalPos(bodyInfo));

    if (auto particle = dynamic_cast<ParticleSystemQuad*>(existing->getChildByName(BURNING_PARTICLE_NAME)))
    {
        updateBurningParticleIntensity(particle, bodyInfo, stacks);
    }
}

int StatusEffectVfxComponent::getStacks(AttributeComponent* attr, StatusEffectType type) const
{
    if (!attr)
    {
        return 0;
    }

    const auto& effects = attr->getStatusEffects();
    for (const auto& eff : effects)
    {
        if (eff.type == type && !eff.isExpired())
        {
            return std::max(1, eff.stacks);
        }
    }
    return 0;
}
