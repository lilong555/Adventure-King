#pragma once

#include "Character/Player/SkillSets/PlayerSkillSet.h"

struct ActiveSkill;

class KleeSkillSet final : public PlayerSkillSet
{
public:
    void initSkills(PlayerCharacter &player) override;
    bool tryNormalAttack(PlayerCharacter &player, const std::function<void()> &onFinished) override;
    bool tryUseSkill(PlayerCharacter &player, size_t slotIndex, const std::function<void()> &onFinished) override;

private:
    bool tryCastFireball(PlayerCharacter &player,
                         size_t slotIndex,
                         const ActiveSkill &skill,
                         const std::function<void()> &onFinished);
};

