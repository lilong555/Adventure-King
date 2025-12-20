#pragma once

#include "Character/Player/SkillSets/PlayerSkillSet.h"

struct ActiveSkill;

class KleeSkillSet final : public PlayerSkillSet
{
public:
    // 初始化可学习/可用技能
    void initSkills(PlayerCharacter &player) override;
    // 处理普通攻击逻辑
    bool tryNormalAttack(PlayerCharacter &player, const std::function<void()> &onFinished) override;
    // 处理技能释放逻辑
    bool tryUseSkill(PlayerCharacter &player, size_t slotIndex, const std::function<void()> &onFinished) override;

private:
    // 释放火箭技能并绑定完成回调
    bool tryCastFireball(PlayerCharacter &player,
                         size_t slotIndex,
                         const ActiveSkill &skill,
                         const std::function<void()> &onFinished);

    // 释放炸弹技能并绑定完成回调
    bool tryCastBomb(PlayerCharacter &player,
                     size_t slotIndex,
                     const ActiveSkill &skill,
                     const std::function<void()> &onFinished);
};
