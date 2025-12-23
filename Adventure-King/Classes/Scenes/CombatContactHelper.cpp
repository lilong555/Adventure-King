#include "Scenes/CombatContactHelper.h"

#include "Character/Base/CharacterBase.h"
#include "Character/Player/PlayerCharacter.h"
#include "Configs/GamePhysicsCategory.h"
#include "Scenes/GameInputController.h"

#include <cmath>

USING_NS_CC;

namespace
{
    Vec2 getWorldPos(Node* node)
    {
        if (!node)
        {
            return Vec2::ZERO;
        }
        auto parent = node->getParent();
        return parent ? parent->convertToWorldSpace(node->getPosition()) : node->getPosition();
    }
}

bool CombatContactHelper::handleContactBegin(PhysicsContact& contact,
                                            PlayerCharacter* player,
                                            GameInputController* inputController)
{
    auto bodyA = contact.getShapeA()->getBody();
    auto bodyB = contact.getShapeB()->getBody();
    if (!bodyA || !bodyB)
    {
        return true;
    }

    auto nodeA = bodyA->getNode();
    auto nodeB = bodyB->getNode();
    if (!nodeA || !nodeB)
    {
        return true;
    }

    int categoryA = bodyA->getCategoryBitmask();
    int categoryB = bodyB->getCategoryBitmask();

    // ============================================================
    // 落地：玩家与平台/碰撞体接触（用于跳跃/落地判定）
    // ============================================================
    const bool playerIsA = (categoryA & ToMask(GamePhysicsCategory::PLAYER)) != 0;
    const bool playerIsB = (categoryB & ToMask(GamePhysicsCategory::PLAYER)) != 0;
    const bool platformContact =
        (playerIsA && ((categoryB & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) != 0)) ||
        (playerIsB && ((categoryA & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) != 0));

    if (platformContact && inputController)
    {
        if (auto contactData = contact.getContactData())
        {
            Vec2 normal = contactData->normal;
            if (playerIsB)
            {
                normal = -normal;
            }
            inputController->onGroundContactBegin(normal.y);
        }
    }

    // ============================================================
    // 战斗：怪物攻击 -> 玩家
    // ============================================================
    const bool monsterAttackVsPlayer =
        ((categoryA & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) != 0 && (categoryB & ToMask(GamePhysicsCategory::PLAYER)) != 0) ||
        ((categoryB & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) != 0 && (categoryA & ToMask(GamePhysicsCategory::PLAYER)) != 0);

    if (monsterAttackVsPlayer)
    {
        auto attackBody = ((categoryA & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) != 0) ? bodyA : bodyB;
        auto playerNode = ((categoryA & ToMask(GamePhysicsCategory::PLAYER)) != 0) ? nodeA : nodeB;
        auto victim = dynamic_cast<CharacterBase*>(playerNode);

        if (victim && !victim->isDead())
        {
            float rawDamage = static_cast<float>(attackBody->getTag());
            if (rawDamage <= 0.0f)
            {
                rawDamage = 1.0f;
            }

            DamageInfo dmg{};
            dmg.amount = rawDamage;
            if (auto attackNode = attackBody->getNode())
            {
                dmg.attacker = dynamic_cast<CharacterBase*>(attackNode->getUserObject());
                dmg.hitWorldPos = getWorldPos(attackNode);
                dmg.hasHitWorldPos = true;
            }

            // 延迟到下一帧执行，避免物理回调内改状态导致崩溃
            std::string key = StringUtils::format("defer_monster_dmg_%p_%p",
                                                  static_cast<void*>(attackBody),
                                                  static_cast<void*>(victim));
            victim->scheduleOnce(
                [victim, dmg](float)
                {
                    if (!victim || victim->isDead())
                    {
                        return;
                    }
                    victim->takeDamage(dmg);
                },
                0.0f,
                key);
        }
    }

    // ============================================================
    // 战斗：玩家攻击 -> 怪物（近战/判定框）
    // 注：投掷物/爆炸由投掷物逻辑处理
    // ============================================================
    const bool playerAttackVsMonster =
        ((categoryA & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) != 0 && (categoryB & ToMask(GamePhysicsCategory::MONSTER)) != 0) ||
        ((categoryB & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) != 0 && (categoryA & ToMask(GamePhysicsCategory::MONSTER)) != 0);

    if (playerAttackVsMonster && player)
    {
        auto attackBody = ((categoryA & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) != 0) ? bodyA : bodyB;
        auto monsterNode = ((categoryA & ToMask(GamePhysicsCategory::MONSTER)) != 0) ? nodeA : nodeB;
        auto monster = dynamic_cast<CharacterBase*>(monsterNode);

        if (monster && !monster->isDead())
        {
            float rawDamage = static_cast<float>(attackBody->getTag());
            if (rawDamage != 0.0f)
            {
                const bool isCrit = rawDamage < 0.0f;
                rawDamage = std::fabs(rawDamage);

                DamageInfo dmg{};
                dmg.amount = rawDamage;
                dmg.attacker = player;
                dmg.isCritical = isCrit;
                if (auto attackNode = attackBody->getNode())
                {
                    dmg.hitWorldPos = getWorldPos(attackNode);
                    dmg.hasHitWorldPos = true;
                }

                std::string key = StringUtils::format("defer_player_dmg_%p_%p",
                                                      static_cast<void*>(attackBody),
                                                      static_cast<void*>(monster));
                monster->scheduleOnce(
                    [monster, dmg](float)
                    {
                        if (!monster || monster->isDead())
                        {
                            return;
                        }
                        monster->takeDamage(dmg);
                    },
                    0.0f,
                    key);
            }
        }
    }

    return true;
}

void CombatContactHelper::handleContactSeparate(PhysicsContact& contact,
                                               GameInputController* inputController)
{
    if (!inputController)
    {
        return;
    }

    auto bodyA = contact.getShapeA()->getBody();
    auto bodyB = contact.getShapeB()->getBody();
    if (!bodyA || !bodyB)
    {
        return;
    }

    auto nodeA = bodyA->getNode();
    auto nodeB = bodyB->getNode();
    if (!nodeA || !nodeB)
    {
        return;
    }

    int categoryA = bodyA->getCategoryBitmask();
    int categoryB = bodyB->getCategoryBitmask();

    const bool playerIsA = (categoryA & ToMask(GamePhysicsCategory::PLAYER)) != 0;
    const bool playerIsB = (categoryB & ToMask(GamePhysicsCategory::PLAYER)) != 0;
    const bool platformContact =
        (playerIsA && ((categoryB & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) != 0)) ||
        (playerIsB && ((categoryA & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) != 0));

    if (!platformContact)
    {
        return;
    }

    if (auto contactData = contact.getContactData())
    {
        Vec2 normal = contactData->normal;
        if (playerIsB)
        {
            normal = -normal;
        }
        inputController->onGroundContactEnd(normal.y);
    }
}

bool CombatContactHelper::handleContactPreSolve(PhysicsContact& contact, PhysicsContactPreSolve& solve)
{
    auto bodyA = contact.getShapeA()->getBody();
    auto bodyB = contact.getShapeB()->getBody();
    if (!bodyA || !bodyB)
    {
        return true;
    }

    auto nodeA = bodyA->getNode();
    auto nodeB = bodyB->getNode();
    if (!nodeA || !nodeB)
    {
        return true;
    }

    int categoryA = bodyA->getCategoryBitmask();
    int categoryB = bodyB->getCategoryBitmask();

    const bool playerInvolved = (categoryA & ToMask(GamePhysicsCategory::PLAYER)) || (categoryB & ToMask(GamePhysicsCategory::PLAYER));
    const bool platformInvolved =
        (categoryA & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) ||
        (categoryB & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION));

    if (playerInvolved && platformInvolved)
    {
        solve.setRestitution(0.0f);
        solve.setFriction(0.0f);
    }

    return true;
}
