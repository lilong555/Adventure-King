#pragma once

#include <cstddef>
#include <functional>

class PlayerCharacter;

class PlayerSkillSet
{
public:
    virtual ~PlayerSkillSet() = default;

    virtual void initSkills(PlayerCharacter &player) = 0;
    virtual bool tryNormalAttack(PlayerCharacter &player, const std::function<void()> &onFinished) = 0;
    virtual bool tryUseSkill(PlayerCharacter &player, size_t slotIndex, const std::function<void()> &onFinished) = 0;
};

