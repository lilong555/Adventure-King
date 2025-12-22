#pragma once

#include "Character/Monster/MonsterBase.h"

/**
 * @brief 训练木桩（用于 DebugScene 的伤害/技能/特效验证）
 *
 * 设计要点：
 * - 固定站立、不移动、不攻击
 * - 生命值极高（21 亿），避免测试时很快死亡
 * - 依然保留 MONSTER 类别，保证投掷物（炸弹）触碰即可爆炸
 */
class TrainingDummyMonster : public MonsterBase
{
public:
    TrainingDummyMonster() = default;
    virtual ~TrainingDummyMonster() = default;

    static TrainingDummyMonster* create(const std::string& spriteFrameName = "Sprites/Enemies/Goblin/Goblin_idle.png");

    virtual bool init(const std::string& spriteFrameName) override;
    virtual void attack() override;

protected:
    // 禁用 AI/移动/攻击逻辑
    virtual void updateAI(float dt) override;
    virtual void updateMovement(float dt) override;
    virtual void updateAttack(float dt) override;

private:
    static constexpr float DUMMY_MAX_HP = 2100000000.0f; // 21 亿
};

