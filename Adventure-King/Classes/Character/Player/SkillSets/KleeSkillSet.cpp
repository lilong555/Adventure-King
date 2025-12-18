#include "Character/Player/SkillSets/KleeSkillSet.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/SkillComponent.h"
#include "Objects/Projectiles/Bomb.h"
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

    constexpr float BURN_DURATION_SECONDS = 5.0f;
    constexpr float BURN_TICK_INTERVAL_SECONDS = 0.5f;
    constexpr float BURN_BASE_DAMAGE_SCALE = 0.1f;
    constexpr float BURN_PER_STACK_DAMAGE_SCALE = 0.1f;
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

    bool ok = player.runActionLocked(
        []()
        { return true; },
        [&player, castPaths](const std::function<void()> &done)
        { player.playOneShotAnimation(castPaths, 0.13f, PlayerCharacter::ACTION_TAG_ATTACK_ANIM, done); },
        [&player, defaultDir]()
        {
            auto bomb = Bomb::create(defaultDir + "/TNT.png");
            if (!bomb)
            {
                return;
            }

            bomb->setScale(0.5f);
            bomb->setPosition(player.getProjectileSpawnPosition(0.35f, 20.0f, 0.15f, 0.0f));
            bomb->setAttacker(&player);
            bomb->setBaseDamage(BOMB_DAMAGE);
            bomb->setExplosionRadius(BOMB_EXPLOSION_RADIUS);
            bomb->setExplosionSpriteVfx(defaultDir + "/BOOM_1.png", 0.8f, 0.2f, 1.2f, 0.3f);
            bomb->setExplodeOnContact(true);

            player.addToCombatLayer(bomb, 4);

            float dirX = player.isFlippedX() ? -1.0f : 1.0f;
            bomb->throwAt(Vec2(dirX * BOMB_THROW_SPEED_X, BOMB_THROW_SPEED_Y));
        },
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
        { player.playOneShotAnimation(castPaths, 0.04f, PlayerCharacter::ACTION_TAG_SKILL_ANIM, done); },
        [&player, skillDir]()
        {
            Bomb::PhysicsConfig physics;
            physics.mass = 0.4f;
            physics.rotationEnabled = false;
            physics.gravityEnabled = false;

            auto rocket = Bomb::create(skillDir + "/spr_vfx_rocket_trail_long_1.png", physics);
            if (!rocket)
            {
                return;
            }

            rocket->setScale(1.10f);
            rocket->setPosition(player.getProjectileSpawnPosition(0.40f, 25.0f, 0.20f, 0.0f));
            rocket->setFlippedX(player.isFlippedX());
            rocket->setAttacker(&player);
            rocket->setBaseDamage(FIREBALL_DAMAGE);
            rocket->setExplosionRadius(FIREBALL_EXPLOSION_RADIUS);
            rocket->setExplodeOnContact(true);

            rocket->setLoopAnimation(
                {
                    skillDir + "/spr_vfx_rocket_trail_long_1.png",
                    skillDir + "/spr_vfx_rocket_trail_long_2.png",
                    skillDir + "/spr_vfx_rocket_trail_long_3.png",
                    skillDir + "/spr_vfx_rocket_trail_long_4.png",
                },
                0.08f);

            rocket->setExplosionFrameVfx(
                {
                    skillDir + "/spr_vfx_explosion_flash_0.png",
                    skillDir + "/spr_vfx_explosion_flash_1.png",
                    skillDir + "/spr_vfx_explosion_flash_2.png",
                    skillDir + "/spr_vfx_explosion_flash_3.png",
                    skillDir + "/spr_vfx_explosion_flash_4.png",
                },
                0.05f,
                0.9f);

            // 命中/爆炸附加燃烧：持续5秒，每0.5秒一次，叠层并刷新
            Bomb::StatusEffectTemplate burn;
            burn.type = StatusEffectType::BURNING;
            burn.duration = BURN_DURATION_SECONDS;
            burn.stacks = 1;
            burn.maxStacks = 0;
            burn.stackable = true;
            burn.refreshOnAdd = true;
            burn.tickInterval = BURN_TICK_INTERVAL_SECONDS;
            burn.baseDamageScale = BURN_BASE_DAMAGE_SCALE;
            burn.perStackDamageScale = BURN_PER_STACK_DAMAGE_SCALE;
            rocket->addOnHitStatusEffect(burn);

            player.addToCombatLayer(rocket, 4);

            float dirX = player.isFlippedX() ? -1.0f : 1.0f;
            rocket->setVelocity(Vec2(dirX * FIREBALL_SPEED_X, 0.0f));
        },
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
