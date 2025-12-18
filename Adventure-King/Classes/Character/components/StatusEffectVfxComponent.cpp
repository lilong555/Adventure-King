#include "Character/components/StatusEffectVfxComponent.h"

#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include <algorithm>

USING_NS_CC;

namespace
{
    const char* const BURNING_PARTICLE_NAME = "BurningParticle";

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

    Vec2 getBottomCenterLocal(Node* node)
    {
        if (!node)
        {
            return Vec2::ZERO;
        }

        const auto size = node->getContentSize();
        const auto anchor = node->getAnchorPoint();
        return Vec2(size.width * (0.5f - anchor.x), -size.height * anchor.y);
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

        particle->setGravity(Vec2(0.0f, 70.0f));
        particle->setAngle(90.0f);
        particle->setAngleVar(25.0f);
        particle->setSpeed(25.0f);
        particle->setSpeedVar(12.0f);
        particle->setLife(0.55f);
        particle->setLifeVar(0.25f);

        particle->setStartSize(14.0f);
        particle->setStartSizeVar(6.0f);
        particle->setEndSize(0.0f);

        particle->setStartColor(Color4F(1.0f, 0.55f, 0.10f, 0.75f));
        particle->setStartColorVar(Color4F(0.10f, 0.10f, 0.10f, 0.10f));
        particle->setEndColor(Color4F(0.70f, 0.12f, 0.05f, 0.00f));
        particle->setEndColorVar(Color4F(0.10f, 0.10f, 0.10f, 0.00f));

        particle->setPositionType(ParticleSystem::PositionType::GROUPED);
        particle->setPosition(Vec2::ZERO);
    }

    void updateBurningParticleIntensity(ParticleSystemQuad* particle, Node* owner, int stacks)
    {
        if (!particle || !owner)
        {
            return;
        }

        const auto size = owner->getContentSize();
        particle->setPosVar(Vec2(size.width * 0.18f, size.height * 0.12f));

        stacks = std::max(1, stacks);
        const float baseEmission = 55.0f;
        const float perStackEmission = 25.0f;
        particle->setEmissionRate(baseEmission + perStackEmission * static_cast<float>(stacks - 1));
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

        auto particle = ParticleSystemQuad::createWithTotalParticles(120);
        particle->setName(BURNING_PARTICLE_NAME);
        existing->addChild(particle);
        applyBurningParticleStyle(particle);
        updateBurningParticleIntensity(particle, owner, stacks);
    }

    existing->setPosition(getBottomCenterLocal(owner) + Vec2(0.0f, owner->getContentSize().height * 0.15f));

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
