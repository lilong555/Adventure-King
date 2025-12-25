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
	            // 兜底：没有 owner 时无法做坐标换算，这里按原样返回（调用方需确保坐标系一致）
	            return localRect;
	        }

	        // 注意：这里返回的是“转换后四个角点”的 AABB。
	        // 如果 owner 有旋转，AABB 会比原矩形更大；当前项目的战斗层/角色节点通常不旋转，可按 AABB 使用。
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
	    // 算法说明（按当前玩法做了简化与提速）：
	    // - 敌人通常可视为处在同一高度（主要差异在 X 轴），因此这里将命中框的 Y 固定在玩家中心，只在 X 轴上做最优选点
	    // - 先用 PhysicsWorld::queryRect 在玩家附近做一次空间查询，只拿到“附近怪物”的包围盒（避免遍历整张地图所有节点）
	    // - 生成一组候选 leftX（命中框左边界），逐个统计覆盖数量（仅做一维枚举，避免二维枚举 O(n^3)）
	    // - 覆盖数量相同时：优先“中心 X 落在某个怪物中心”，仍相同则选离玩家更近的点（防止命中框跳太远）
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

	        constexpr size_t ENEMY_QUERY_RESERVE = 32;

	        std::vector<Rect> enemyBoxes;
	        enemyBoxes.reserve(ENEMY_QUERY_RESERVE);
	        std::vector<Vec2> enemyCenters;
	        enemyCenters.reserve(ENEMY_QUERY_RESERVE);
	        std::unordered_set<Node*> visited;
	        visited.reserve(ENEMY_QUERY_RESERVE * 2);

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
	        candidateLeftX.reserve(enemyBoxes.size() * 5 + 1);

	        // 注意：当前算法仅在 X 轴上做一维搜索，Y 固定在玩家中心。
	        // 设计假设是“敌人基本与玩家处于同一高度”，适用于多数近战地面怪物场景。
	        // 如果后续关卡或怪物设计引入明显的高度差（高台怪物 / 飞行怪物等），
	        // 需要重新审视这里的逻辑（例如增加 Y 方向的候选或更智能的兜底策略），
	        // 否则命中框可能无法覆盖到垂直方向上相距较远但水平方向在范围内的敌人。
	        const float fixedCenterY = playerCenter.y;
	        const float fixedLeftY = fixedCenterY - hitboxSize.height * 0.5f;

        // 兜底候选：以玩家为中心
        candidateLeftX.push_back(playerCenter.x - hitboxSize.width * 0.5f);

        // 一维候选生成：
        // 对某个怪物的 X 区间 [minX, maxX]，命中框 [leftX, leftX+w] 与其相交的充分必要条件：
        // leftX <= maxX 且 leftX+w >= minX  =>  leftX ∈ [minX - w, maxX]
        // 覆盖数量随 leftX 变化只会在这些边界附近发生改变，因此把边界点加入候选集合即可
        for (const auto& box : enemyBoxes)
        {
            candidateLeftX.push_back(box.getMinX() - hitboxSize.width);
            candidateLeftX.push_back(box.getMaxX());

            // 保留“边缘对齐”的直觉候选，便于调参时行为更可预期
            candidateLeftX.push_back(box.getMinX());
            candidateLeftX.push_back(box.getMaxX() - hitboxSize.width);
        }

        // 同覆盖数量时，优先让中心点落在“某个怪物的位置”（怪物包围盒中心 X）
        // 将怪物中心 X 作为候选中心：leftX = centerX - hitboxW/2，这样 hitboxCenterX 会精确对齐怪物
        for (const auto& c : enemyCenters)
        {
            candidateLeftX.push_back(c.x - hitboxSize.width * 0.5f);
        }

        std::sort(candidateLeftX.begin(), candidateLeftX.end());
        candidateLeftX.erase(std::unique(candidateLeftX.begin(), candidateLeftX.end()), candidateLeftX.end());

	        int bestCount = -1;
	        bool bestAtMonsterCenter = false;
	        float bestDist2 = std::numeric_limits<float>::max();
	        Vec2 bestCenter = playerCenter;

        for (float leftX : candidateLeftX)
        {
            const Rect candidate(leftX, fixedLeftY, hitboxSize.width, hitboxSize.height);

            int count = 0;
            for (const auto& enemy : enemyBoxes)
            {
                if (candidate.intersectsRect(enemy))
                {
                    ++count;
                }
            }

            const float centerX = leftX + hitboxSize.width * 0.5f;
            const Vec2 center(centerX, fixedCenterY);
            const float dx = centerX - playerCenter.x;
            const float dist2 = dx * dx;

	            bool atMonsterCenter = false;
	            // 像素级容差：中心点对齐属于“体验优化”类的 tie-break，不需要过于苛刻
	            constexpr float EPS = 1.0f;
	            for (const auto& monsterCenter : enemyCenters)
	            {
	                // 只比较 X：同覆盖数量时优先“在某个怪物位置生成”（按当前玩法理解为怪物所在 X）
	                if (std::fabs(monsterCenter.x - centerX) <= EPS)
                {
                    atMonsterCenter = true;
                    break;
                }
            }

            // 选择规则：
            // 1) 覆盖数量最多
            // 2) 覆盖数量相同：优先“落在某个怪物位置”（怪物中心 X）
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

	        // 兜底：如果固定 Y 导致一个都覆盖不到，返回玩家中心，避免命中框“跳到怪物 X 但仍然打不到”
	        if (bestCount <= 0)
	        {
	            return playerCenter;
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

                    player.spawnPlayerAttackHitbox(Vec2(cx, cy),
                                                   Size(w, h),
                                                   damage,
                                                   isCrit,
                                                   HITBOX_LIFE_SECONDS,
                                                   GameConfig::Combat::BREAK_DAMAGE_NORMAL);
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

	                    auto combatLayer = player.getCombatLayer();
	                    if (!combatLayer)
	                    {
	                        return;
	                    }
	                    const Rect box = player.getBoundingBox();

                    const float hitW = std::max(10.0f, box.size.width * GameConfig::Warrior::FireSkill::HITBOX_WIDTH_MULTIPLIER);
                    const float hitH = std::max(10.0f, box.size.height * GameConfig::Warrior::FireSkill::HITBOX_HEIGHT_MULTIPLIER);
                    const Size hitboxSize(hitW, hitH);

	                    // 火焰技能为全向 AoE，不限制朝向，按覆盖敌人最多的方式选点（并在同覆盖时优先贴近怪物位置）
	                    // “覆盖敌人最多”的中心点：在 combatLayer 坐标系中计算
	                    const Vec2 center = computeBestFireHitboxCenter(player, combatLayer, hitboxSize);

	                    // 特效即使在极端情况下 hitbox 生成失败，也优先保证可见反馈（避免“扣蓝/进 CD 但没有任何效果”的体验）
	                    ParticleVfxHelper::PlayOptions options;
	                    options.zOrder = 2;
	                    options.positionType = ParticleSystem::PositionType::GROUPED;
	                    options.name = "warrior_fire_vfx";
	                    // 特效不要随 hitbox 销毁而提前结束：挂到 combatLayer（世界层）上播放
	                    options.useBodyCenter = false;
	                    // 特效从命中框底部生成（底边中点）
	                    options.position = Vec2(center.x, center.y - hitboxSize.height * 0.5f);
	                    ParticleVfxHelper::playOnce(combatLayer, "Particle/par_fire.plist", options);

	                    const float damage = player.getAttackPower() * GameConfig::Warrior::FireSkill::DAMAGE_SCALE;
	                    const bool isCrit = rollCritical(player);
	                    player.spawnPlayerAttackHitbox(center,
		                                                  hitboxSize,
		                                                  damage,
		                                                  isCrit,
		                                                  GameConfig::Warrior::FireSkill::HITBOX_LIFE_SECONDS,
		                                                  GameConfig::Combat::BREAK_DAMAGE_SKILL);
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
