#include "Character/Player/SkillSets/AssassinSkillSet.h"

#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/SkillComponent.h"
#include "Configs/GameConfigs.h"
#include "cocos2d.h"
#include <cmath>
#include <memory>
#include <string>
#include <vector>

USING_NS_CC;

namespace
{
    // 刺客：判定更窄、更短，减少“站很远也能打到”的违和感
    constexpr float HITBOX_LIFE_SECONDS = 0.08f;
    constexpr float HITBOX_DELAY_SECONDS = 0.03f;

    constexpr float HITBOX_WIDTH_RATIO = 0.45f;
    constexpr float HITBOX_HEIGHT_RATIO = 0.65f;
    constexpr float HITBOX_OFFSET_X_RATIO = 0.50f;
    constexpr float HITBOX_OFFSET_Y = 6.0f;

    bool rollCritical(PlayerCharacter& player)
    {
        auto attr = player.getAttributeComponent();
        if (!attr)
        {
            return false;
        }

        float critRate = attr->getAttributeValue(AttributeType::CRITICAL_RATE);
        float critPercent = std::max(0.0f, std::min(critRate * 100.0f, 100.0f));
        return (rand() % 100) < static_cast<int>(critPercent);
    }
}

void AssassinSkillSet::initSkills(PlayerCharacter& player)
{
    auto skillComp = player.getSkillComponent();
    if (!skillComp)
    {
        return;
    }

    // 斩击：默认解锁并装备到 0 号槽位（E/K）
    auto existing = std::dynamic_pointer_cast<ActiveSkill>(
        skillComp->findLearnedSkillById(GameConfig::Assassin::SlashSkill::SLASH_ID));
    std::shared_ptr<ActiveSkill> slashSkill = existing;
    if (!slashSkill)
    {
        slashSkill = std::make_shared<ActiveSkill>();
        slashSkill->id = GameConfig::Assassin::SlashSkill::SLASH_ID;
        slashSkill->name = "斩击";
        slashSkill->description = "向前挥出一道斩击，对前方敌人造成伤害。";
        slashSkill->manaCost = GameConfig::Assassin::SlashSkill::SLASH_MP;
        slashSkill->cooldown = GameConfig::Assassin::SlashSkill::SLASH_CD;
        slashSkill->currentCooldown = 0.0f;
        skillComp->learnSkill(slashSkill);
    }

    skillComp->equipActiveSkill(slashSkill, GameConfig::Assassin::SlashSkill::SKILL_SLOT);
}

bool AssassinSkillSet::tryNormalAttack(PlayerCharacter& player, const std::function<void()>& onFinished)
{
    const bool isCrit = rollCritical(player);

    bool ok = player.runActionLocked(
        []() { return true; },
        [&player, isCrit](const std::function<void()>& done)
        {
            player.scheduleOnce(
                [&player, isCrit](float)
                {
                    if (player.isDead())
                    {
                        return;
                    }

                    // 刺客伤害略低于战士（占位），后续可基于武器/技能进一步拉开差异
                    const float damage = player.getAttackPower() * 0.9f;
                    const Rect box = player.getBoundingBox();
                    const float w = std::max(10.0f, box.size.width * HITBOX_WIDTH_RATIO);
                    const float h = std::max(10.0f, box.size.height * HITBOX_HEIGHT_RATIO);

                    const float dirX = player.isFlippedX() ? -1.0f : 1.0f;
                    const float cx = box.getMidX() + dirX * (box.size.width * HITBOX_OFFSET_X_RATIO);
                    const float cy = box.getMidY() + HITBOX_OFFSET_Y;

                    player.spawnPlayerAttackHitbox(Vec2(cx, cy), Size(w, h), damage, isCrit, HITBOX_LIFE_SECONDS);
                },
                HITBOX_DELAY_SECONDS,
                "assassin_melee_hitbox");

            player.attackAnimated(done);
        },
        nullptr,
        [onFinished]()
        {
            if (onFinished)
            {
                onFinished();
            }
        });

    if (ok)
    {
        CCLOG("AssassinSkillSet: normal attack started");
    }
    return ok;
}

bool AssassinSkillSet::tryUseSkill(PlayerCharacter& player, size_t slotIndex, const std::function<void()>& onFinished)
{
    auto skillComp = player.getSkillComponent();
    if (!skillComp)
    {
        return false;
    }

    const auto& slots = skillComp->getActiveSlots();
    if (slotIndex >= slots.size() || !slots[slotIndex])
    {
        return false;
    }

    const ActiveSkill& skill = *slots[slotIndex];
    if (skill.id != GameConfig::Assassin::SlashSkill::SLASH_ID)
    {
        return false;
    }

    const std::string& skillDir = player.getSkillSpriteDir();
    const std::string& characterKey = player.getCharacterKey();
    if (skillDir.empty() || characterKey.empty())
    {
        return false;
    }

    std::vector<std::string> castPaths;
    castPaths.reserve(4);
    for (int i = 1; i <= 4; ++i)
    {
        castPaths.push_back(StringUtils::format("%s/spr_%s_slash_%d.png", skillDir.c_str(), characterKey.c_str(), i));
    }

    bool ok = player.runActionLocked(
        [&player, slotIndex]()
        {
            auto sc = player.getSkillComponent();
            if (!sc)
            {
                return false;
            }
            return sc->useActiveSkill(slotIndex);
        },
        [&player, castPaths](const std::function<void()>& done)
        {
            // 在技能动画中间生成命中判定框
            player.scheduleOnce(
                [&player](float)
                {
                    if (player.isDead())
                    {
                        return;
                    }

                    const float damage = player.getAttackPower() * GameConfig::Assassin::SlashSkill::DAMAGE_SCALE;
                    const Rect box = player.getBoundingBox();
                    const float w = std::max(10.0f, box.size.width * GameConfig::Assassin::SlashSkill::HITBOX_WIDTH_RATIO);
                    const float h = std::max(10.0f, box.size.height * GameConfig::Assassin::SlashSkill::HITBOX_HEIGHT_RATIO);

                    const float dirX = player.isFlippedX() ? -1.0f : 1.0f;
                    const float cx = box.getMidX() + dirX * (box.size.width * GameConfig::Assassin::SlashSkill::HITBOX_OFFSET_X_RATIO);
                    const float cy = box.getMidY() + GameConfig::Assassin::SlashSkill::HITBOX_OFFSET_Y;

                    player.spawnPlayerAttackHitbox(Vec2(cx, cy),
                                                   Size(w, h),
                                                   damage,
                                                   false,
                                                   GameConfig::Assassin::SlashSkill::HITBOX_LIFE_SECONDS);
                },
                GameConfig::Assassin::SlashSkill::HITBOX_DELAY_SECONDS,
                "assassin_slash_hitbox");

            player.playOneShotAnimation(castPaths,
                                        GameConfig::Assassin::SlashSkill::CAST_ANIM_FRAME_DELAY,
                                        PlayerCharacter::ACTION_TAG_SKILL_ANIM,
                                        done);
        },
        nullptr,
        [onFinished]()
        {
            if (onFinished)
            {
                onFinished();
            }
        });

    if (ok)
    {
        CCLOG("AssassinSkillSet: slash skill started");
    }
    return ok;
}
