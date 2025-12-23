#include "Character/Player/SkillSets/WarriorSkillSet.h"

#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Configs/GamePhysicsCategory.h"
#include "cocos2d.h"
#include <cmath>

USING_NS_CC;

namespace
{
    // 近战判定：持续时间越短越不容易误伤/重复命中
    constexpr float HITBOX_LIFE_SECONDS = 0.10f;
    constexpr float HITBOX_DELAY_SECONDS = 0.05f; // 略微延迟，贴近挥砍动作

    // 命中框尺寸（先用相对值占位，后续可按手感调参）
    constexpr float HITBOX_WIDTH_RATIO = 0.55f;
    constexpr float HITBOX_HEIGHT_RATIO = 0.75f;
    constexpr float HITBOX_OFFSET_X_RATIO = 0.55f;
    constexpr float HITBOX_OFFSET_Y = 8.0f;

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

void WarriorSkillSet::initSkills(PlayerCharacter& /*player*/)
{
    // 战士当前不默认解锁主动技能
}

bool WarriorSkillSet::tryNormalAttack(PlayerCharacter& player, const std::function<void()>& onFinished)
{
    const bool isCrit = rollCritical(player);

    // 使用 runActionLocked 做统一的动作锁/状态切换，避免并发攻击打断状态机
    bool ok = player.runActionLocked(
        []() { return true; },
        [&player, isCrit](const std::function<void()>& done)
        {
            // 在攻击动画中间生成命中框
            player.scheduleOnce(
                [&player, isCrit](float)
                {
                    if (player.isDead())
                    {
                        return;
                    }

                    const float damage = player.getAttackPower();
                    const Rect box = player.getBoundingBox();
                    const float w = std::max(10.0f, box.size.width * HITBOX_WIDTH_RATIO);
                    const float h = std::max(10.0f, box.size.height * HITBOX_HEIGHT_RATIO);

                    const float dirX = player.isFlippedX() ? -1.0f : 1.0f;
                    const float cx = box.getMidX() + dirX * (box.size.width * HITBOX_OFFSET_X_RATIO);
                    const float cy = box.getMidY() + HITBOX_OFFSET_Y;

                    player.spawnPlayerAttackHitbox(Vec2(cx, cy), Size(w, h), damage, isCrit, HITBOX_LIFE_SECONDS);
                },
                HITBOX_DELAY_SECONDS,
                "warrior_melee_hitbox");

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
        CCLOG("WarriorSkillSet: normal attack started");
    }
    return ok;
}

bool WarriorSkillSet::tryUseSkill(PlayerCharacter& /*player*/, size_t /*slotIndex*/, const std::function<void()>& /*onFinished*/)
{
    // 战士当前不实现主动技能
    return false;
}

