#pragma once

#include "Character/CharacterData.h"
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

private:
    CharacterBase *_owner = nullptr;

    std::vector<std::shared_ptr<Skill>> _learnedSkills;
    std::vector<std::shared_ptr<ActiveSkill>> _activeSlots;
    std::vector<std::shared_ptr<PassiveSkill>> _passiveSlots;

    void applyPassiveSkill(const std::shared_ptr<PassiveSkill> &skill);
    void removePassiveSkill(const std::shared_ptr<PassiveSkill> &skill);
};
