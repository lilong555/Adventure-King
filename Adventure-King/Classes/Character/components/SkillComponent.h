#pragma once

#include "Character/Base/CharacterData.h"
#include <memory>
#include <vector>

class CharacterBase;

class SkillComponent
{
public:
    explicit SkillComponent(CharacterBase *owner);

    // 学习技能（解锁）
    void learnSkill(const std::shared_ptr<Skill> &skill);

    // 将技能装备到槽位
    void equipActiveSkill(const std::shared_ptr<ActiveSkill> &skill, size_t slotIndex);
    void equipPassiveSkill(const std::shared_ptr<PassiveSkill> &skill, size_t slotIndex);

    // 使用主动技能（成功返回 true）
    bool useActiveSkill(size_t slotIndex);

    // 每帧更新冷却
    void update(float dt);

    const std::vector<std::shared_ptr<ActiveSkill>> &getActiveSlots() const { return _activeSlots; }
    const std::vector<std::shared_ptr<PassiveSkill>> &getPassiveSlots() const { return _passiveSlots; }

    //================== 存档系统支持 ==================

    // 获取已学习的技能列表
    const std::vector<std::shared_ptr<Skill>> &getLearnedSkills() const { return _learnedSkills; }

    // 根据 ID 查找已学习的技能
    std::shared_ptr<Skill> findLearnedSkillById(int skillId) const;

    // 清空并设置主动技能槽位（用于读档）
    void clearAndSetActiveSlots(const std::vector<std::shared_ptr<ActiveSkill>> &slots);

    // 清空并设置被动技能槽位（用于读档）
    void clearAndSetPassiveSlots(const std::vector<std::shared_ptr<PassiveSkill>> &slots);

    // 清空所有技能与槽位（用于读档前重置状态）
    void resetSkills();

private:
    CharacterBase *_owner = nullptr;

    std::vector<std::shared_ptr<Skill>> _learnedSkills;
    std::vector<std::shared_ptr<ActiveSkill>> _activeSlots;
    std::vector<std::shared_ptr<PassiveSkill>> _passiveSlots;

    void applyPassiveSkill(const std::shared_ptr<PassiveSkill> &skill);
    void removePassiveSkill(const std::shared_ptr<PassiveSkill> &skill);
};
