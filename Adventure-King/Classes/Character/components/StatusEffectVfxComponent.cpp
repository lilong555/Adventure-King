#include "Character/components/StatusEffectVfxComponent.h"

#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include <algorithm>

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
        float emitterOffsetYRatio = 0.15f;
        float posVarXRatio = 0.18f;
        float posVarYRatio = 0.12f;

        float baseEmission = 55.0f;
        float perStackEmission = 25.0f;
    };

    const BurningParticleConfig kBurningParticleConfig;

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

    Vec2 getBurningEmitterLocalPos(Node* owner)
    {
        if (!owner)
        {
            return Vec2::ZERO;
        }

        const auto size = owner->getContentSize();
        return Vec2(size.width * 0.5f, size.height * kBurningParticleConfig.emitterOffsetYRatio);
    }

    void updateBurningParticleIntensity(ParticleSystemQuad* particle, Node* owner, int stacks)
    {
        if (!particle || !owner)
        {
            return;
        }

        const auto size = owner->getContentSize();
        particle->setPosVar(Vec2(size.width * kBurningParticleConfig.posVarXRatio,
                                 size.height * kBurningParticleConfig.posVarYRatio));

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

    if (!existing)
    {
        existing = Node::create();
        existing->setName(BURNING_VFX_NAME);
        owner->addChild(existing, 999);

        auto particle = ParticleSystemQuad::createWithTotalParticles(kBurningParticleConfig.totalParticles);
        particle->setName(BURNING_PARTICLE_NAME);
        existing->addChild(particle);
        applyBurningParticleStyle(particle);
        updateBurningParticleIntensity(particle, owner, stacks);
    }

    existing->setPosition(getBurningEmitterLocalPos(owner));

    if (auto particle = dynamic_cast<ParticleSystemQuad*>(existing->getChildByName(BURNING_PARTICLE_NAME)))
    {
        updateBurningParticleIntensity(particle, owner, stacks);
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
