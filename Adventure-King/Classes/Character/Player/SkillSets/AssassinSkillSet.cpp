#include "Character/Player/SkillSets/AssassinSkillSet.h"

#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Configs/GamePhysicsCategory.h"
#include "cocos2d.h"
#include <cmath>

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

void AssassinSkillSet::initSkills(PlayerCharacter& /*player*/)
{
    // 刺客当前不默认解锁主动技能
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

bool AssassinSkillSet::tryUseSkill(PlayerCharacter& /*player*/, size_t /*slotIndex*/, const std::function<void()>& /*onFinished*/)
{
    return false;
}

