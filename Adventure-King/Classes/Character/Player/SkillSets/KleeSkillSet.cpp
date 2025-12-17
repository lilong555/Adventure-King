#include "Character/Player/SkillSets/KleeSkillSet.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/SkillComponent.h"
#include "Character/Player/Projectiles/PlayerProjectileConfig.h"
#include "Physics/GamePhysicsCategory.h"
#include "cocos2d.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

USING_NS_CC;

namespace
{
    constexpr int FIREBALL_SKILL_ID = 1002;
    constexpr size_t FIREBALL_SKILL_SLOT = 0;
    constexpr float FIREBALL_SKILL_MP_COST = 15.0f;
    constexpr float FIREBALL_SKILL_COOLDOWN = 1.2f;

    constexpr float BOMB_THROW_SPEED_X = 300.0f;
    constexpr float BOMB_THROW_SPEED_Y = 350.0f;
    constexpr float BOMB_DAMAGE = 150.0f;
    constexpr float BOMB_EXPLOSION_RADIUS = 80.0f;

    constexpr float FIREBALL_SPEED_X = 650.0f;
    constexpr float FIREBALL_DAMAGE = 220.0f;
    constexpr float FIREBALL_EXPLOSION_RADIUS = 90.0f;
}

void KleeSkillSet::initSkills(PlayerCharacter &player)
{
    auto skillComp = player.getSkillComponent();
    if (!skillComp)
    {
        return;
    }

    auto existing = std::dynamic_pointer_cast<ActiveSkill>(skillComp->findLearnedSkillById(FIREBALL_SKILL_ID));
    std::shared_ptr<ActiveSkill> fireballSkill = existing;
    if (!fireballSkill)
    {
        fireballSkill = std::make_shared<ActiveSkill>();
        fireballSkill->id = FIREBALL_SKILL_ID;
        fireballSkill->name = "火球";
        fireballSkill->description = "发射火球，命中后爆炸造成范围伤害";
        fireballSkill->manaCost = FIREBALL_SKILL_MP_COST;
        fireballSkill->cooldown = FIREBALL_SKILL_COOLDOWN;
        fireballSkill->currentCooldown = 0.0f;
        skillComp->learnSkill(fireballSkill);
    }

    skillComp->equipActiveSkill(fireballSkill, FIREBALL_SKILL_SLOT);
}

bool KleeSkillSet::tryNormalAttack(PlayerCharacter &player, const std::function<void()> &onFinished)
{
    const std::string &defaultDir = player.getDefaultSpriteDir();
    const std::string &characterKey = player.getCharacterKey();

    std::vector<std::string> castPaths;
    castPaths.reserve(3);
    for (int i = 1; i <= 3; ++i)
    {
        castPaths.push_back(StringUtils::format("%s/spr_%s_attack_%d.png", defaultDir.c_str(), characterKey.c_str(), i));
    }

    PlayerProjectileConfig bombConfig;
    bombConfig.spritePath = defaultDir + "/TNT.png";
    bombConfig.spriteScale = 0.5f;
    bombConfig.flipXWithFacing = false;
    bombConfig.physicsRadius = 15.0f;
    bombConfig.material = PhysicsMaterial(0.5f, 0.3f, 0.2f);
    bombConfig.mass = 0.5f;
    bombConfig.rotationEnabled = true;
    bombConfig.gravityEnabled = true;
    bombConfig.linearDamping = 0.0f;
    bombConfig.categoryBitmask = ToMask(GamePhysicsCategory::PLAYER_ATTACK);
    bombConfig.collisionBitmask = ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER);
    bombConfig.contactTestBitmask = ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER);

    bombConfig.spawnOffsetXRatio = 0.35f;
    bombConfig.spawnOffsetX = 20.0f;
    bombConfig.spawnOffsetYRatio = 0.15f;
    bombConfig.spawnOffsetY = 0.0f;

    bombConfig.moveType = ProjectileMoveType::IMPULSE;
    bombConfig.moveVector = Vec2(BOMB_THROW_SPEED_X, BOMB_THROW_SPEED_Y);
    bombConfig.scaleMoveByMass = true;

    bombConfig.damage = BOMB_DAMAGE;
    bombConfig.explosionRadius = BOMB_EXPLOSION_RADIUS;
    bombConfig.explodeOnContact = true;

    bombConfig.explosionVfx.spritePath = defaultDir + "/BOOM_1.png";
    bombConfig.explosionVfx.spriteScale = 0.8f;
    bombConfig.explosionVfx.spriteScaleUpDuration = 0.2f;
    bombConfig.explosionVfx.spriteScaleUpFactor = 1.2f;
    bombConfig.explosionVfx.spriteFadeOutDuration = 0.3f;

    bool ok = player.runActionLocked(
        []()
        { return true; },
        [&player, castPaths](const std::function<void()> &done)
        { player.playOneShotAnimation(castPaths, 0.13f, 1001, done); },
        [&player, bombConfig]()
        { player.spawnProjectile(bombConfig); },
        [onFinished]()
        {
            if (onFinished)
            {
                onFinished();
            }
            CCLOG("Attack animation finished");
        });

    if (ok)
    {
        CCLOG("Normal attack started: Throw Bomb");
    }
    return ok;
}

bool KleeSkillSet::tryUseSkill(PlayerCharacter &player, size_t slotIndex, const std::function<void()> &onFinished)
{
    auto skillComp = player.getSkillComponent();
    if (!skillComp)
    {
        return false;
    }

    const auto &slots = skillComp->getActiveSlots();
    if (slotIndex >= slots.size() || !slots[slotIndex])
    {
        return false;
    }

    const ActiveSkill &skill = *slots[slotIndex];
    if (skill.id == FIREBALL_SKILL_ID)
    {
        return tryCastFireball(player, slotIndex, skill, onFinished);
    }

    return false;
}

bool KleeSkillSet::tryCastFireball(PlayerCharacter &player,
                                   size_t slotIndex,
                                   const ActiveSkill &skill,
                                   const std::function<void()> &onFinished)
{
    const std::string &skillDir = player.getSkillSpriteDir();
    const std::string &characterKey = player.getCharacterKey();

    std::vector<std::pair<int, int>> framePlan = {
        {1, 1},
        {4, 2},
        {5, 3},
        {6, 2},
    };

    std::vector<std::string> castPaths;
    castPaths.reserve(16);
    for (const auto &entry : framePlan)
    {
        int frameIndex = entry.first;
        int repeatCount = entry.second;
        std::string framePath = StringUtils::format("%s/spr_%s_attack_%d.png", skillDir.c_str(), characterKey.c_str(), frameIndex);
        for (int i = 0; i < repeatCount; ++i)
        {
            castPaths.push_back(framePath);
        }
    }

    PlayerProjectileConfig fireballConfig;
    fireballConfig.spritePath = skillDir + "/spr_vfx_rocket_trail_long_1.png";
    fireballConfig.spriteScale = 1.10f;
    fireballConfig.flipXWithFacing = true;
    fireballConfig.loopAnimationPaths = {
        skillDir + "/spr_vfx_rocket_trail_long_1.png",
        skillDir + "/spr_vfx_rocket_trail_long_2.png",
        skillDir + "/spr_vfx_rocket_trail_long_3.png",
        skillDir + "/spr_vfx_rocket_trail_long_4.png",
    };
    fireballConfig.loopAnimationDelay = 0.08f;

    fireballConfig.physicsRadius = 16.0f;
    fireballConfig.material = PhysicsMaterial(0.5f, 0.0f, 0.0f);
    fireballConfig.mass = 0.4f;
    fireballConfig.rotationEnabled = false;
    fireballConfig.gravityEnabled = false;
    fireballConfig.linearDamping = 0.0f;
    fireballConfig.categoryBitmask = ToMask(GamePhysicsCategory::PLAYER_ATTACK);
    fireballConfig.collisionBitmask = 0;
    fireballConfig.contactTestBitmask = ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER);

    fireballConfig.spawnOffsetXRatio = 0.40f;
    fireballConfig.spawnOffsetX = 25.0f;
    fireballConfig.spawnOffsetYRatio = 0.20f;
    fireballConfig.spawnOffsetY = 0.0f;

    fireballConfig.moveType = ProjectileMoveType::VELOCITY;
    fireballConfig.moveVector = Vec2(FIREBALL_SPEED_X, 0.0f);
    fireballConfig.scaleMoveByMass = false;

    fireballConfig.damage = FIREBALL_DAMAGE;
    fireballConfig.explosionRadius = FIREBALL_EXPLOSION_RADIUS;
    fireballConfig.explodeOnContact = true;

    fireballConfig.explosionVfx.framePaths = {
        skillDir + "/spr_vfx_explosion_flash_0.png",
        skillDir + "/spr_vfx_explosion_flash_1.png",
        skillDir + "/spr_vfx_explosion_flash_2.png",
        skillDir + "/spr_vfx_explosion_flash_3.png",
        skillDir + "/spr_vfx_explosion_flash_4.png",
    };
    fireballConfig.explosionVfx.frameDelay = 0.05f;
    fireballConfig.explosionVfx.frameScale = 0.9f;

    bool ok = player.runActionLocked(
        [&player, slotIndex, skill]()
        {
            auto skillComp = player.getSkillComponent();
            if (!skillComp)
            {
                return false;
            }

            if (!skillComp->useActiveSkill(slotIndex))
            {
                CCLOG("Skill cast failed - MP insufficient or on cooldown: %s", skill.name.c_str());
                return false;
            }
            return true;
        },
        [&player, castPaths](const std::function<void()> &done)
        { player.playOneShotAnimation(castPaths, 0.04f, 1002, done); },
        [&player, fireballConfig]()
        { player.spawnProjectile(fireballConfig); },
        [onFinished, skill]()
        {
            if (onFinished)
            {
                onFinished();
            }
            CCLOG("Fireball animation finished");
        });

    if (ok)
    {
        CCLOG("Skill started: %s", skill.name.c_str());
    }
    return ok;
}
