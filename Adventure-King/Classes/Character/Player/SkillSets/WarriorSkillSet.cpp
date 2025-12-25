#include "Character/Player/SkillSets/WarriorSkillSet.h"

#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/SkillComponent.h"
#include "Configs/GameConfig.h"
#include "Configs/GamePhysicsCategory.h"
#include "Utils/ParticleVfxHelper.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include "cocos2d.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <unordered_set>
#include <vector>

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

    Rect toWorldAabb(Node* owner, const Rect& localRect)
    {
        if (!owner)
        {
            return localRect;
        }

        const Vec2 bl = owner->convertToWorldSpace(localRect.origin);
        const Vec2 br = owner->convertToWorldSpace(localRect.origin + Vec2(localRect.size.width, 0.0f));
        const Vec2 tl = owner->convertToWorldSpace(localRect.origin + Vec2(0.0f, localRect.size.height));
        const Vec2 tr = owner->convertToWorldSpace(localRect.origin + Vec2(localRect.size.width, localRect.size.height));

        const float minX = std::min(std::min(bl.x, br.x), std::min(tl.x, tr.x));
        const float minY = std::min(std::min(bl.y, br.y), std::min(tl.y, tr.y));
        const float maxX = std::max(std::max(bl.x, br.x), std::max(tl.x, tr.x));
        const float maxY = std::max(std::max(bl.y, br.y), std::max(tl.y, tr.y));
        return Rect(minX, minY, std::max(0.0f, maxX - minX), std::max(0.0f, maxY - minY));
    }

    Rect getNodeAabbInLayer(Node* node, Node* layer)
    {
        if (!node)
        {
            return Rect::ZERO;
        }

        // getBoundingBox() 在“父节点坐标系”下返回 AABB
        const Rect parentAabb = node->getBoundingBox();
        auto parent = node->getParent();
        if (!parent || !layer || parent == layer)
        {
            return parentAabb;
        }

        // 转为世界坐标，再转回 layer 坐标系，确保在相机跟随/层级嵌套下也能正确计算
        const Vec2 blW = parent->convertToWorldSpace(parentAabb.origin);
        const Vec2 brW = parent->convertToWorldSpace(parentAabb.origin + Vec2(parentAabb.size.width, 0.0f));
        const Vec2 tlW = parent->convertToWorldSpace(parentAabb.origin + Vec2(0.0f, parentAabb.size.height));
        const Vec2 trW = parent->convertToWorldSpace(parentAabb.origin + Vec2(parentAabb.size.width, parentAabb.size.height));

        const Vec2 bl = layer->convertToNodeSpace(blW);
        const Vec2 br = layer->convertToNodeSpace(brW);
        const Vec2 tl = layer->convertToNodeSpace(tlW);
        const Vec2 tr = layer->convertToNodeSpace(trW);

        const float minX = std::min(std::min(bl.x, br.x), std::min(tl.x, tr.x));
        const float minY = std::min(std::min(bl.y, br.y), std::min(tl.y, tr.y));
        const float maxX = std::max(std::max(bl.x, br.x), std::max(tl.x, tr.x));
        const float maxY = std::max(std::max(bl.y, br.y), std::max(tl.y, tr.y));

        return Rect(minX, minY, std::max(0.0f, maxX - minX), std::max(0.0f, maxY - minY));
    }

    // Fire 技能：计算“覆盖敌人最多”的命中框中心点（在玩家 combatLayer 坐标系中）
    // 算法思路（高效且足够稳定）：
    // 1) 先用 PhysicsWorld::queryRect 在玩家附近做一次空间查询，只拿到“附近怪物”的包围盒（避免遍历整张地图所有节点）
    // 2) 对固定大小的 AABB（命中框），最优解一定可以通过“让命中框的边缘对齐某个怪物包围盒边缘”得到
    //    因此只需要枚举有限的候选位置（O(n^2) 个），逐个统计覆盖数量即可
    static Vec2 computeBestFireHitboxCenter(PlayerCharacter& player, Node* combatLayer, const Size& hitboxSize)
    {
        if (!combatLayer)
        {
            const Rect box = player.getBoundingBox();
            return Vec2(box.getMidX(), box.getMidY());
        }

        auto scene = player.getScene();
        auto world = scene ? scene->getPhysicsWorld() : nullptr;
        if (!world)
        {
            const Rect box = getNodeAabbInLayer(&player, combatLayer);
            return Vec2(box.getMidX(), box.getMidY());
        }

        const Rect playerBox = getNodeAabbInLayer(&player, combatLayer);
        const Vec2 playerCenter(playerBox.getMidX(), playerBox.getMidY());

        // 搜索范围：以命中框为基准扩大 3 倍，仅扫描“可能覆盖到的附近怪物”
        const float searchW = std::max(200.0f, hitboxSize.width * 3.0f);
        const float searchH = std::max(200.0f, hitboxSize.height * 3.0f);
        const Rect localSearch(playerCenter.x - searchW * 0.5f, playerCenter.y - searchH * 0.5f, searchW, searchH);
        const Rect worldSearch = toWorldAabb(combatLayer, localSearch);

        std::vector<Rect> enemyBoxes;
        enemyBoxes.reserve(16);
        std::vector<Vec2> enemyCenters;
        enemyCenters.reserve(16);
        std::unordered_set<Node*> visited;
        visited.reserve(32);

        world->queryRect([&](PhysicsWorld&, PhysicsShape& shape, void*) {
            auto body = shape.getBody();
            if (!body)
            {
                return true;
            }
            if ((body->getCategoryBitmask() & ToMask(GamePhysicsCategory::MONSTER)) == 0)
            {
                return true;
            }
            auto node = body->getNode();
            if (!node)
            {
                return true;
            }
            if (!visited.insert(node).second)
            {
                return true;
            }
            const Rect enemyBox = getNodeAabbInLayer(node, combatLayer);
            enemyBoxes.push_back(enemyBox);
            enemyCenters.emplace_back(enemyBox.getMidX(), enemyBox.getMidY());
            return true;
        }, worldSearch, nullptr);

        if (enemyBoxes.empty())
        {
            return playerCenter;
        }

        std::vector<float> candidateLeftX;
        std::vector<float> candidateLeftY;
        candidateLeftX.reserve(enemyBoxes.size() * 2 + 1);
        candidateLeftY.reserve(enemyBoxes.size() * 2 + 1);

        // 兜底候选：以玩家为中心
        candidateLeftX.push_back(playerCenter.x - hitboxSize.width * 0.5f);
        candidateLeftY.push_back(playerCenter.y - hitboxSize.height * 0.5f);

        // 候选生成：命中框左/下边缘对齐怪物包围盒边缘（或右/上边缘对齐）
        for (const auto& box : enemyBoxes)
        {
            candidateLeftX.push_back(box.getMinX());
            candidateLeftX.push_back(box.getMaxX() - hitboxSize.width);
            candidateLeftY.push_back(box.getMinY());
            candidateLeftY.push_back(box.getMaxY() - hitboxSize.height);
        }

        // 同覆盖数量时，优先让中心点落在“某个怪物的位置”（怪物包围盒中心）
        // 将怪物中心作为候选中心：left = center - hitbox/2，这样 center 会精确等于怪物中心点
        for (const auto& c : enemyCenters)
        {
            candidateLeftX.push_back(c.x - hitboxSize.width * 0.5f);
            candidateLeftY.push_back(c.y - hitboxSize.height * 0.5f);
        }

        std::sort(candidateLeftX.begin(), candidateLeftX.end());
        candidateLeftX.erase(std::unique(candidateLeftX.begin(), candidateLeftX.end()), candidateLeftX.end());
        std::sort(candidateLeftY.begin(), candidateLeftY.end());
        candidateLeftY.erase(std::unique(candidateLeftY.begin(), candidateLeftY.end()), candidateLeftY.end());

        int bestCount = -1;
        bool bestAtMonsterCenter = false;
        float bestDist2 = std::numeric_limits<float>::max();
        Vec2 bestCenter = playerCenter;

        for (float leftX : candidateLeftX)
        {
            for (float leftY : candidateLeftY)
            {
                const Rect candidate(leftX, leftY, hitboxSize.width, hitboxSize.height);

                int count = 0;
                for (const auto& enemy : enemyBoxes)
                {
                    if (candidate.intersectsRect(enemy))
                    {
                        ++count;
                    }
                }

                const Vec2 center(leftX + hitboxSize.width * 0.5f, leftY + hitboxSize.height * 0.5f);
                const float dist2 = center.distanceSquared(playerCenter);

                bool atMonsterCenter = false;
                constexpr float EPS = 0.01f;
                for (const auto& monsterCenter : enemyCenters)
                {
                    if (std::fabs(monsterCenter.x - center.x) <= EPS && std::fabs(monsterCenter.y - center.y) <= EPS)
                    {
                        atMonsterCenter = true;
                        break;
                    }
                }

                // 选择规则：
                // 1) 覆盖数量最多
                // 2) 覆盖数量相同：优先“落在某个怪物位置”（怪物中心点）
                // 3) 仍相同：离玩家更近（避免命中框“跳太远”）
                if (count > bestCount ||
                    (count == bestCount && atMonsterCenter && !bestAtMonsterCenter) ||
                    (count == bestCount && atMonsterCenter == bestAtMonsterCenter && dist2 < bestDist2))
                {
                    bestCount = count;
                    bestAtMonsterCenter = atMonsterCenter;
                    bestDist2 = dist2;
                    bestCenter = center;
                }
            }
        }

        return bestCenter;
    }
}

void WarriorSkillSet::initSkills(PlayerCharacter& player)
{
    // 战士：默认解锁并装备 Fire 技能
    auto skillComp = player.getSkillComponent();
    if (!skillComp)
    {
        return;
    }

    auto existing = std::dynamic_pointer_cast<ActiveSkill>(
        skillComp->findLearnedSkillById(GameConfig::Warrior::FireSkill::FIRE_ID));
    std::shared_ptr<ActiveSkill> fireSkill = existing;
    if (!fireSkill)
    {
        fireSkill = std::make_shared<ActiveSkill>();
        fireSkill->id = GameConfig::Warrior::FireSkill::FIRE_ID;
        fireSkill->name = "火焰";
        fireSkill->description = "释放火焰冲击，在第3帧对覆盖区域内的敌人造成伤害。";
        fireSkill->manaCost = GameConfig::Warrior::FireSkill::FIRE_MP;
        fireSkill->cooldown = GameConfig::Warrior::FireSkill::FIRE_CD;
        fireSkill->currentCooldown = 0.0f;
        skillComp->learnSkill(fireSkill);
    }

    skillComp->equipActiveSkill(fireSkill, GameConfig::Warrior::FireSkill::SKILL_SLOT);

    // 预加载 fire 动作帧（把首次解码/上传的卡顿挪到角色创建阶段）
    const auto& skillDir = player.getSkillSpriteDir();
    if (!skillDir.empty())
    {
        SpriteFrameCacheHelper::getOrCreateSpriteFrame(skillDir + "/fire_1.png");
        SpriteFrameCacheHelper::getOrCreateSpriteFrame(skillDir + "/fire_2.png");
        SpriteFrameCacheHelper::getOrCreateSpriteFrame(skillDir + "/fire_3.png");
    }
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

bool WarriorSkillSet::tryUseSkill(PlayerCharacter& player, size_t slotIndex, const std::function<void()>& onFinished)
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
    if (skill.id != GameConfig::Warrior::FireSkill::FIRE_ID)
    {
        return false;
    }

    const std::string& skillDir = player.getSkillSpriteDir();
    if (skillDir.empty())
    {
        return false;
    }

    std::vector<std::string> castPaths;
    castPaths.reserve(3);
    castPaths.push_back(skillDir + "/fire_1.png");
    castPaths.push_back(skillDir + "/fire_2.png");
    castPaths.push_back(skillDir + "/fire_3.png");

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
            // 第 3 帧开始触发伤害与特效
            const float triggerDelay = GameConfig::Warrior::FireSkill::CAST_ANIM_FRAME_DELAY *
                                       static_cast<float>(GameConfig::Warrior::FireSkill::HIT_TRIGGER_FRAME_INDEX - 1);

            player.scheduleOnce(
                [&player](float)
                {
                    if (player.isDead())
                    {
                        return;
                    }

                    auto combatLayer = player.getParent();
                    const Rect box = player.getBoundingBox();

                    const float hitW = std::max(10.0f, box.size.width * GameConfig::Warrior::FireSkill::HITBOX_WIDTH_MULTIPLIER);
                    const float hitH = std::max(10.0f, box.size.height * GameConfig::Warrior::FireSkill::HITBOX_HEIGHT_MULTIPLIER);
                    const Size hitboxSize(hitW, hitH);

                    // “覆盖敌人最多”的中心点：在 combatLayer 坐标系中计算
                    const Vec2 center = computeBestFireHitboxCenter(player, combatLayer, hitboxSize);

                    const float damage = player.getAttackPower() * GameConfig::Warrior::FireSkill::DAMAGE_SCALE;
                    const bool isCrit = rollCritical(player);

                    auto hitboxNode = player.spawnPlayerAttackHitbox(center,
                                                                     hitboxSize,
                                                                     damage,
                                                                     isCrit,
                                                                     GameConfig::Warrior::FireSkill::HITBOX_LIFE_SECONDS);

                    if (hitboxNode)
                    {
                        ParticleVfxHelper::PlayOptions options;
                        options.zOrder = 2;
                        options.positionType = ParticleSystem::PositionType::GROUPED;
                        options.name = "warrior_fire_vfx";
                        // 特效不要随 hitbox 销毁而提前结束：挂到 combatLayer（世界层）上播放
                        options.useBodyCenter = false;
                        // 特效从命中框底部生成（底边中点）
                        options.position = Vec2(center.x, center.y - hitboxSize.height * 0.5f);
                        ParticleVfxHelper::playOnce(combatLayer, "Particle/par_fire.plist", options);
                    }
                },
                triggerDelay,
                "warrior_fire_hitbox");

            player.playOneShotAnimation(castPaths,
                                        GameConfig::Warrior::FireSkill::CAST_ANIM_FRAME_DELAY,
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
        CCLOG("WarriorSkillSet: fire skill started");
    }
    return ok;
}
