#pragma once

#include "Character/Player/SkillSets/PlayerSkillSet.h"

// 刺客技能集：
// - 普通攻击：短距离快速近战判定框（PLAYER_ATTACK -> MONSTER）
// - 主动技能：当前留空（后续可扩展为位移/隐身等机制）
class AssassinSkillSet final : public PlayerSkillSet
{
public:
    void initSkills(PlayerCharacter& player) override;
    bool tryNormalAttack(PlayerCharacter& player, const std::function<void()>& onFinished) override;
    bool tryUseSkill(PlayerCharacter& player, size_t slotIndex, const std::function<void()>& onFinished) override;
};

