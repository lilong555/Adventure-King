#include "Character/Player/SkillSets/KleeSkillSet.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/SkillComponent.h"
#include "Objects/Projectiles/Bomb.h"
#include "Configs/GameConfig.h"
#include "cocos2d.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

USING_NS_CC;

void KleeSkillSet::initSkills(PlayerCharacter &player)
{
    auto skillComp = player.getSkillComponent();
    if (!skillComp)
    {
        return;
    }

    auto existing = std::dynamic_pointer_cast<ActiveSkill>(
        skillComp->findLearnedSkillById(GameConfig::Fireball::FIREBALL_ID));
    std::shared_ptr<ActiveSkill> fireballSkill = existing;
    if (!fireballSkill)
    {
        fireballSkill = std::make_shared<ActiveSkill>();
        fireballSkill->id = GameConfig::Fireball::FIREBALL_ID;
        fireballSkill->name = "火球";
        fireballSkill->description = "发射火球，命中后爆炸造成范围伤害";
        fireballSkill->manaCost = GameConfig::Fireball::FIREBALL_MP;
        fireballSkill->cooldown = GameConfig::Fireball::FIREBALL_CD;
        fireballSkill->currentCooldown = 0.0f;
        skillComp->learnSkill(fireballSkill);
    }

    skillComp->equipActiveSkill(fireballSkill, GameConfig::Klee::FireballSkill::SKILL_SLOT);
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
        { player.playOneShotAnimation(castPaths, GameConfig::Klee::NormalAttack::ANIM_FRAME_DELAY,
                                      PlayerCharacter::ACTION_TAG_ATTACK_ANIM, done); },
        [&player, defaultDir]()
        {
            auto bomb = Bomb::create(defaultDir + "/TNT.png");
            if (!bomb)
            {
                return;
            }

            bomb->setScale(GameConfig::Klee::NormalAttack::PROJECTILE_SCALE);
            bomb->setPosition(player.getProjectileSpawnPosition(
                GameConfig::Klee::NormalAttack::SPAWN_OFFSET_X_RATIO,
                GameConfig::Klee::NormalAttack::SPAWN_OFFSET_X,
                GameConfig::Klee::NormalAttack::SPAWN_OFFSET_Y_RATIO,
                GameConfig::Klee::NormalAttack::SPAWN_OFFSET_Y));
            bomb->setAttacker(&player);
            bomb->setBaseDamage(0.0f);
            bomb->setAttackPowerDamageScale(GameConfig::Bomb::DAMAGE_SCALE);
            bomb->setExplosionRadius(GameConfig::Bomb::EXPLOSION_RADIUS);
            bomb->setExplosionSpriteVfx(
                defaultDir + "/BOOM_1.png",
                GameConfig::Klee::NormalAttack::EXPLOSION_VFX_SCALE,
                GameConfig::Klee::NormalAttack::EXPLOSION_VFX_SCALE_UP_DURATION,
                GameConfig::Klee::NormalAttack::EXPLOSION_VFX_SCALE_UP_FACTOR,
                GameConfig::Klee::NormalAttack::EXPLOSION_VFX_FADE_OUT_DURATION);
            bomb->setExplodeOnContact(true);

            player.addToCombatLayer(bomb, 4);

            float dirX = player.isFlippedX() ? -1.0f : 1.0f;
            bomb->throwAt(Vec2(dirX * GameConfig::Bomb::THROW_SPEED_X,
                               GameConfig::Bomb::THROW_SPEED_Y));
        },
        [onFinished]()
        {
            if (onFinished)
            {
                onFinished();
            }
        });

    if (ok)
    {
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
    if (skill.id == GameConfig::Bomb::BOMB_ID)
    {
        return tryCastBomb(player, slotIndex, skill, onFinished);
    }
    if (skill.id == GameConfig::Fireball::FIREBALL_ID)
    {
        return tryCastFireball(player, slotIndex, skill, onFinished);
    }

    return false;
}

bool KleeSkillSet::tryCastBomb(PlayerCharacter &player,
                               size_t slotIndex,
                               const ActiveSkill &skill,
                               const std::function<void()> &onFinished)
{
    const std::string &defaultDir = player.getDefaultSpriteDir();
    const std::string &characterKey = player.getCharacterKey();

    // 使用普通攻击的帧资源作为“投掷动作”占位
    std::vector<std::string> castPaths;
    castPaths.reserve(3);
    for (int i = 1; i <= 3; ++i)
    {
        castPaths.push_back(StringUtils::format("%s/spr_%s_attack_%d.png", defaultDir.c_str(), characterKey.c_str(), i));
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
                CCLOG("Bomb skill cast failed - MP insufficient or on cooldown: %s", skill.name.c_str());
                return false;
            }
            return true;
        },
        [&player, castPaths](const std::function<void()> &done)
        {
            player.playOneShotAnimation(castPaths,
                                        GameConfig::Klee::NormalAttack::ANIM_FRAME_DELAY,
                                        PlayerCharacter::ACTION_TAG_SKILL_ANIM,
                                        done);
        },
        [&player, defaultDir]()
        {
            auto bomb = Bomb::create(defaultDir + "/TNT.png");
            if (!bomb)
            {
                return;
            }

            bomb->setScale(GameConfig::Bomb::SPRITE_SCALE);
            bomb->setPosition(player.getProjectileSpawnPosition(
                GameConfig::Klee::NormalAttack::SPAWN_OFFSET_X_RATIO,
                GameConfig::Klee::NormalAttack::SPAWN_OFFSET_X,
                GameConfig::Klee::NormalAttack::SPAWN_OFFSET_Y_RATIO,
                GameConfig::Klee::NormalAttack::SPAWN_OFFSET_Y));
            bomb->setAttacker(&player);

            // 技能炸弹：按攻击力缩放（与普通攻击一致），便于数值统一
            bomb->setBaseDamage(0.0f);
            bomb->setAttackPowerDamageScale(GameConfig::Bomb::DAMAGE_SCALE);
            bomb->setExplosionRadius(GameConfig::Bomb::EXPLOSION_RADIUS);
            bomb->setExplosionSpriteVfx(
                defaultDir + "/BOOM_1.png",
                GameConfig::Klee::NormalAttack::EXPLOSION_VFX_SCALE,
                GameConfig::Klee::NormalAttack::EXPLOSION_VFX_SCALE_UP_DURATION,
                GameConfig::Klee::NormalAttack::EXPLOSION_VFX_SCALE_UP_FACTOR,
                GameConfig::Klee::NormalAttack::EXPLOSION_VFX_FADE_OUT_DURATION);
            bomb->setExplodeOnContact(true);

            player.addToCombatLayer(bomb, 4);

            float dirX = player.isFlippedX() ? -1.0f : 1.0f;
            bomb->throwAt(Vec2(dirX * GameConfig::Bomb::THROW_SPEED_X,
                               GameConfig::Bomb::THROW_SPEED_Y));
        },
        [onFinished]()
        {
            if (onFinished)
            {
                onFinished();
            }
        });

    if (ok)
    {
        CCLOG("Skill started: Throw Bomb");
    }
    return ok;
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
        { player.playOneShotAnimation(castPaths, GameConfig::Klee::FireballSkill::CAST_ANIM_FRAME_DELAY,
                                      PlayerCharacter::ACTION_TAG_SKILL_ANIM, done); },
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

            rocket->setScale(GameConfig::Klee::FireballSkill::PROJECTILE_SCALE);
            rocket->setPosition(player.getProjectileSpawnPosition(
                GameConfig::Klee::FireballSkill::SPAWN_OFFSET_X_RATIO,
                GameConfig::Klee::FireballSkill::SPAWN_OFFSET_X,
                GameConfig::Klee::FireballSkill::SPAWN_OFFSET_Y_RATIO,
                GameConfig::Klee::FireballSkill::SPAWN_OFFSET_Y));
            rocket->setFlippedX(player.isFlippedX());
            rocket->setAttacker(&player);
            rocket->setBaseDamage(0.0f);
            rocket->setAttackPowerDamageScale(GameConfig::Fireball::DAMAGE_SCALE);
            rocket->setExplosionRadius(GameConfig::Fireball::EXPLOSION_RADIUS);
            rocket->setExplodeOnContact(true);

            rocket->setLoopAnimation(
                {
                    skillDir + "/spr_vfx_rocket_trail_long_1.png",
                    skillDir + "/spr_vfx_rocket_trail_long_2.png",
                    skillDir + "/spr_vfx_rocket_trail_long_3.png",
                    skillDir + "/spr_vfx_rocket_trail_long_4.png",
                },
                GameConfig::Klee::FireballSkill::LOOP_ANIM_DELAY);

            rocket->setExplosionFrameVfx(
                {
                    skillDir + "/spr_vfx_explosion_flash_0.png",
                    skillDir + "/spr_vfx_explosion_flash_1.png",
                    skillDir + "/spr_vfx_explosion_flash_2.png",
                    skillDir + "/spr_vfx_explosion_flash_3.png",
                    skillDir + "/spr_vfx_explosion_flash_4.png",
                },
                GameConfig::Klee::FireballSkill::EXPLOSION_FRAME_DELAY,
                GameConfig::Klee::FireballSkill::EXPLOSION_FRAME_SCALE);

            // 命中/爆炸附加燃烧：持续5秒，每0.5秒一次，叠层并刷新
            Bomb::StatusEffectTemplate burn;
            burn.type = StatusEffectType::BURNING;
            burn.duration = GameConfig::StatusEffect::Burning::DURATION_SECONDS;
            burn.stacks = 1;
            burn.maxStacks = 0;
            burn.stackable = true;
            burn.refreshOnAdd = true;
            burn.tickInterval = GameConfig::StatusEffect::Burning::TICK_INTERVAL_SECONDS;
            burn.baseDamageScale = GameConfig::StatusEffect::Burning::BASE_DAMAGE_SCALE;
            burn.perStackDamageScale = GameConfig::StatusEffect::Burning::PER_STACK_DAMAGE_SCALE;
            rocket->addOnHitStatusEffect(burn);

            player.addToCombatLayer(rocket, 4);

            float dirX = player.isFlippedX() ? -1.0f : 1.0f;
            rocket->setVelocity(Vec2(dirX * GameConfig::Fireball::SPEED_X, 0.0f));
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
