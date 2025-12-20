#pragma once

#include <cstddef>
#include <functional>

class PlayerCharacter;

class PlayerSkillSet
{
public:
    // 虚析构，保证派生类正确释放
    virtual ~PlayerSkillSet() = default;

    // 初始化角色可用技能与槽位
    virtual void initSkills(PlayerCharacter &player) = 0;
    // 尝试触发普通攻击
    virtual bool tryNormalAttack(PlayerCharacter &player, const std::function<void()> &onFinished) = 0;
    // 尝试释放指定槽位的技能
    virtual bool tryUseSkill(PlayerCharacter &player, size_t slotIndex, const std::function<void()> &onFinished) = 0;
};
