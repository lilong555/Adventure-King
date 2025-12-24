#include "Character/components/StatusEffectVfxComponent.h"

#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include "Utils/PhysicsBodyLocalInfoHelper.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

namespace
{
    const char* const BURNING_PARTICLE_NAME = "BurningParticle";
    const char* const POISON_PARTICLE_NAME = "PoisonParticle";

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
        float startSizeHeightRatio = StatusEffectVfxComponent::BurningVfxParams::START_SIZE_HEIGHT_RATIO;
        float minStartSize = StatusEffectVfxComponent::BurningVfxParams::MIN_START_SIZE;

        float baseEmission = 55.0f;
        float perStackEmission = 25.0f;
    };

    const BurningParticleConfig kBurningParticleConfig;

    using BodyLocalInfo = PhysicsBodyLocalInfoHelper::BodyLocalInfo;

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

        const float rawSize = bodyInfo.size.height * kBurningParticleConfig.startSizeHeightRatio;
        const float baseSize = std::min(kBurningParticleConfig.maxStartSize,
                                        std::max(kBurningParticleConfig.minStartSize, rawSize));
        particle->setStartSize(baseSize);
        particle->setStartSizeVar(baseSize * 0.5f);
        particle->setEndSize(0.0f);

        stacks = std::max(1, stacks);
        particle->setEmissionRate(kBurningParticleConfig.baseEmission +
                                  kBurningParticleConfig.perStackEmission * static_cast<float>(stacks - 1));
    }
} // namespace

const char* const StatusEffectVfxComponent::BURNING_VFX_NAME = "StatusEffectVfx_Burning";
const char* const StatusEffectVfxComponent::POISON_VFX_NAME = "StatusEffectVfx_Poison";

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
    // 由 CharacterBase 统一调度 update，避免重复 scheduleUpdate 触发引擎 warning。
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
    updatePoisonVfx(owner, attr);
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
    const auto bodyInfo = PhysicsBodyLocalInfoHelper::getBodyLocalInfo(owner);

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

void StatusEffectVfxComponent::updatePoisonVfx(Node* owner, AttributeComponent* attr)
{
    if (!owner || !attr)
    {
        return;
    }

    const bool poisoned = attr->hasStatusEffect(StatusEffectType::POISONED);
    auto existing = owner->getChildByName(POISON_VFX_NAME);

    if (!poisoned)
    {
        if (existing)
        {
            existing->removeFromParent();
        }
        return;
    }

    const int stacks = getStacks(attr, StatusEffectType::POISONED);
    const auto bodyInfo = PhysicsBodyLocalInfoHelper::getBodyLocalInfo(owner);

    if (!existing)
    {
        auto particle = ParticleSystemQuad::create("Particle/par_Poison.plist");
        if (!particle)
        {
            // 创建失败时不创建空容器，避免后续帧留下“空节点”无法自愈
            return;
        }

        existing = Node::create();
        existing->setName(POISON_VFX_NAME);
        owner->addChild(existing, 999);

        particle->setName(POISON_PARTICLE_NAME);
        particle->setPositionType(ParticleSystem::PositionType::GROUPED);
        particle->setPosition(Vec2::ZERO);
        existing->addChild(particle);
    }

    existing->setPosition(bodyInfo.center);

    auto particle = dynamic_cast<ParticleSystemQuad*>(existing->getChildByName(POISON_PARTICLE_NAME));
    if (!particle)
    {
        // 如果粒子子节点丢失，移除容器节点，让后续 update 自动重新创建
        existing->removeFromParent();
        return;
    }

    // 经验参数：让散布范围随物理体缩放，但对超大体型做上限限制，避免 Boss 过宽/过窄
    constexpr float kPoisonPosVarXRatio = 0.15f;
    constexpr float kPoisonPosVarYRatio = 0.10f;
    constexpr float kPoisonPosVarXMax = 60.0f;
    constexpr float kPoisonPosVarYMax = 45.0f;

    // 让中毒特效的散布与角色体型匹配，避免 Boss 过宽/过窄
    const float posVarX = std::min(bodyInfo.size.width * kPoisonPosVarXRatio, kPoisonPosVarXMax);
    const float posVarY = std::min(bodyInfo.size.height * kPoisonPosVarYRatio, kPoisonPosVarYMax);
    particle->setPosVar(Vec2(posVarX, posVarY));

    const int safeStacks = std::max(1, stacks);
    const float baseEmission = 6.0f;
    const float perStackEmission = 3.0f;
    particle->setEmissionRate(baseEmission + perStackEmission * static_cast<float>(safeStacks - 1));
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
        if (eff->type == type && !eff->isExpired())
        {
            return std::max(1, eff->stacks);
        }
    }
    return 0;
}
