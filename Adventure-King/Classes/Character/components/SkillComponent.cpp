#include "Character/components/SkillComponent.h"
#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"

SkillComponent::SkillComponent(CharacterBase *owner)
    : _owner(owner)
{
}

void SkillComponent::learnSkill(const std::shared_ptr<Skill> &skill)
{
    if (!skill)
        return;
    _learnedSkills.push_back(skill);
}

void SkillComponent::equipActiveSkill(const std::shared_ptr<ActiveSkill> &skill, size_t slotIndex)
{
    if (!skill)
        return;

    if (slotIndex >= _activeSlots.size())
    {
        _activeSlots.resize(slotIndex + 1);
    }
    _activeSlots[slotIndex] = skill;
}

void SkillComponent::equipPassiveSkill(const std::shared_ptr<PassiveSkill> &skill, size_t slotIndex)
{
    if (!skill)
        return;

    if (slotIndex >= _passiveSlots.size())
    {
        _passiveSlots.resize(slotIndex + 1);
    }

    // 如果原来有技能，先移除原有加成
    if (_passiveSlots[slotIndex])
    {
        removePassiveSkill(_passiveSlots[slotIndex]);
    }

    _passiveSlots[slotIndex] = skill;
    applyPassiveSkill(skill);
}

void SkillComponent::applyPassiveSkill(const std::shared_ptr<PassiveSkill> &skill)
{
    if (!_owner)
        return;
    auto attr = _owner->getAttributeComponent();
    if (!attr)
        return;

    attr->addPassiveSkillBonus(skill->attributeBonus);
}

void SkillComponent::removePassiveSkill(const std::shared_ptr<PassiveSkill> &skill)
{
    if (!_owner)
        return;
    auto attr = _owner->getAttributeComponent();
    if (!attr)
        return;

    attr->removePassiveSkillBonus(skill->attributeBonus);
}

bool SkillComponent::useActiveSkill(size_t slotIndex)
{
    if (slotIndex >= _activeSlots.size())
        return false;
    auto skill = _activeSlots[slotIndex];
    if (!skill || !_owner)
        return false;

    // 冷却中
    if (skill->currentCooldown > 0.0f)
    {
        return false;
    }

    // MP 不足
    if (_owner->getCurrentMP() < skill->manaCost)
    {
        return false;
    }

    // 扣 MP、设置冷却
    _owner->setCurrentMP(_owner->getCurrentMP() - skill->manaCost);
    skill->currentCooldown = skill->cooldown;

    // 交给角色执行具体技能效果（生成子弹/范围伤害等）
    _owner->onUseActiveSkill(*skill);

    return true;
}

void SkillComponent::update(float dt)
{
    for (auto &skill : _activeSlots)
    {
        if (skill && skill->currentCooldown > 0.0f)
        {
            skill->currentCooldown -= dt;
            if (skill->currentCooldown < 0.0f)
            {
                skill->currentCooldown = 0.0f;
            }
        }
    }
}

//================== 存档系统支持 ==================

std::shared_ptr<Skill> SkillComponent::findLearnedSkillById(int skillId) const
{
    for (const auto &skill : _learnedSkills)
    {
        if (skill && skill->id == skillId)
        {
            return skill;
        }
    }
    return nullptr;
}

void SkillComponent::clearAndSetActiveSlots(const std::vector<std::shared_ptr<ActiveSkill>> &slots)
{
    _activeSlots.clear();
    _activeSlots = slots;
}

void SkillComponent::clearAndSetPassiveSlots(const std::vector<std::shared_ptr<PassiveSkill>> &slots)
{
    // 先移除所有旧的被动技能加成
    for (const auto &skill : _passiveSlots)
    {
        if (skill)
        {
            removePassiveSkill(skill);
        }
    }

    _passiveSlots.clear();
    _passiveSlots = slots;

    // 应用所有新的被动技能加成
    for (const auto &skill : _passiveSlots)
    {
        if (skill)
        {
            applyPassiveSkill(skill);
        }
    }
}
