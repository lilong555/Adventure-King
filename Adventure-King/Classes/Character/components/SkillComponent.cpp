#include "SkillComponent.h"
#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h" // 确保包含属性组件头文件
#include "Configs/GameConfigs.h"

USING_NS_CC;

SkillComponent::SkillComponent()
{
    setName("SkillComponent");
}

SkillComponent::~SkillComponent()
{
}

bool SkillComponent::init()
{
    if (!Component::init())
    {
        return false;
    }
    // 初始化槽位大小，防止越界 (假设默认3个槽位)
    _activeSlots.resize(GameConfig::UI::SKILL_BAR_SLOT_COUNT, nullptr);
    // 被动技能取消槽位限制：用列表表示“已装备的被动技能”
    _passiveSlots.clear();
    return true;
}

void SkillComponent::onAdd()
{
    // 当组件被 addComponent 到 Character 时，自动获取 Owner 并转换类型
    if (getOwner())
    {
        _cachedOwner = dynamic_cast<CharacterBase*>(getOwner());

        // 开启 update 调度 (让 update 函数每帧被调用)
        getOwner()->scheduleUpdate();
    }
}

CharacterBase* SkillComponent::getCharacterOwner() const
{
    return _cachedOwner;
}

void SkillComponent::learnSkill(const std::shared_ptr<Skill>& skill)
{
    if (!skill)
        return;

    // 去重：同 ID 的技能不重复加入
    if (findLearnedSkillById(skill->id))
    {
        return;
    }
    _learnedSkills.push_back(skill);
}

// 修改返回值类型为 bool
bool SkillComponent::equipActiveSkill(const std::shared_ptr<ActiveSkill>& skill, size_t slotIndex)
{
    if (!skill)
        return false;

    if (slotIndex >= _activeSlots.size())
    {
        // 如果想支持动态扩容可以保留，或者直接返回 false
        _activeSlots.resize(slotIndex + 1);
    }
    _activeSlots[slotIndex] = skill;
    return true;
}

// 修改返回值类型为 bool
bool SkillComponent::equipPassiveSkill(const std::shared_ptr<PassiveSkill>& skill, size_t slotIndex)
{
    if (!skill)
        return false;

    if (slotIndex >= _passiveSlots.size())
    {
        _passiveSlots.resize(slotIndex + 1);
    }

    // 如果该槽位原来有技能，先移除原有加成
    if (_passiveSlots[slotIndex])
    {
        removePassiveSkill(_passiveSlots[slotIndex]);
    }

    _passiveSlots[slotIndex] = skill;

    // 应用新技能加成
    applyPassiveSkill(skill);

    return true;
}

bool SkillComponent::isPassiveSkillEquipped(int skillId) const
{
    for (const auto& skill : _passiveSlots)
    {
        if (skill && skill->id == skillId)
        {
            return true;
        }
    }
    return false;
}

bool SkillComponent::equipPassiveSkill(const std::shared_ptr<PassiveSkill>& skill)
{
    if (!skill)
    {
        return false;
    }

    // 已经装备则不重复应用
    if (isPassiveSkillEquipped(skill->id))
    {
        return false;
    }

    _passiveSlots.push_back(skill);
    applyPassiveSkill(skill);
    return true;
}

bool SkillComponent::unequipPassiveSkillById(int skillId)
{
    for (size_t i = 0; i < _passiveSlots.size(); ++i)
    {
        const auto& skill = _passiveSlots[i];
        if (!skill || skill->id != skillId)
        {
            continue;
        }

        removePassiveSkill(skill);
        _passiveSlots.erase(_passiveSlots.begin() + static_cast<std::vector<std::shared_ptr<PassiveSkill>>::difference_type>(i));
        return true;
    }
    return false;
}

bool SkillComponent::unequipActiveSkill(size_t slotIndex)
{
    if (slotIndex >= _activeSlots.size())
    {
        return false;
    }
    _activeSlots[slotIndex] = nullptr;
    return true;
}

bool SkillComponent::unequipPassiveSkill(size_t slotIndex)
{
    if (slotIndex >= _passiveSlots.size())
    {
        return false;
    }

    if (_passiveSlots[slotIndex])
    {
        removePassiveSkill(_passiveSlots[slotIndex]);
    }
    _passiveSlots[slotIndex] = nullptr;
    return true;
}

void SkillComponent::applyPassiveSkill(const std::shared_ptr<PassiveSkill>& skill)
{
    // 使用 _cachedOwner
    if (!_cachedOwner)
        return;

    auto attr = _cachedOwner->getAttributeComponent();
    if (!attr)
        return;

    attr->addPassiveSkillBonus(skill->attributeBonus);
}

void SkillComponent::removePassiveSkill(const std::shared_ptr<PassiveSkill>& skill)
{
    // 使用 _cachedOwner
    if (!_cachedOwner)
        return;

    auto attr = _cachedOwner->getAttributeComponent();
    if (!attr)
        return;

    attr->removePassiveSkillBonus(skill->attributeBonus);
}

bool SkillComponent::useActiveSkill(size_t slotIndex)
{
    if (slotIndex >= _activeSlots.size())
        return false;

    auto skill = _activeSlots[slotIndex];

    // 检查 skill 和 _cachedOwner
    if (!skill || !_cachedOwner)
        return false;

    // 1. 检查冷却
    if (skill->currentCooldown > 0.0f)
    {
        // 可以在这里打印日志 "技能冷却中"
        return false;
    }

    // 2. 检查 MP
    // 假设 CharacterBase 有 getCurrentMP() 和 attribute 相关方法
    float currentMP = _cachedOwner->getCurrentMP();

    // 如果有 AttributeComponent，最好从那里获取，或者 CharacterBase 封装了接口
    // 这里假设 CharacterBase 直接提供了 getCurrentMP
    if (currentMP < skill->manaCost)
    {
        // 可以在这里打印日志 "法力不足"
        return false;
    }

    // 3. 消耗资源
    _cachedOwner->setCurrentMP(currentMP - skill->manaCost);

    // 4. 设置冷却
    skill->currentCooldown = skill->cooldown;

    // 5. 执行技能逻辑
    // 调用 CharacterBase 的回调，通知它播放动画或生成投掷物
    _cachedOwner->onUseActiveSkill(*skill);

    return true;
}

void SkillComponent::update(float dt)
{
    // 遍历所有装备的主动技能，减少冷却时间
    for (auto& skill : _activeSlots)
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
    for (const auto& skill : _learnedSkills)
    {
        if (skill && skill->id == skillId)
        {
            return skill;
        }
    }
    return nullptr;
}

void SkillComponent::clearAndSetActiveSlots(const std::vector<std::shared_ptr<ActiveSkill>>& slots)
{
    _activeSlots.clear();
    _activeSlots = slots; // 拷贝列表
}

void SkillComponent::clearAndSetPassiveSlots(const std::vector<std::shared_ptr<PassiveSkill>>& slots)
{
    // 1. 先移除旧的属性加成
    for (const auto& skill : _passiveSlots)
    {
        if (skill)
        {
            removePassiveSkill(skill);
        }
    }

    // 2. 更新槽位
    _passiveSlots.clear();
    _passiveSlots = slots;

    // 3. 应用新的属性加成
    for (const auto& skill : _passiveSlots)
    {
        if (skill)
        {
            applyPassiveSkill(skill);
        }
    }
}

void SkillComponent::resetSkills()
{
    // 先清空被动技能以移除属性加成
    clearAndSetPassiveSlots({});
    _activeSlots.clear();
    _learnedSkills.clear();
}
