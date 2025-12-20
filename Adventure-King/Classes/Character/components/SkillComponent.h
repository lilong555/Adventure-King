#pragma once

#include "cocos2d.h" // 1. 必须引入 Cocos 头文件
#include "Character/Base/CharacterData.h"
#include <memory>
#include <vector>

class CharacterBase;

// 2. 继承自 cocos2d::Component
class SkillComponent : public cocos2d::Component
{
public:
    // 3. 添加标准创建宏 (解决 create 报错的核心)
    CREATE_FUNC(SkillComponent);

    // 初始化技能组件
    SkillComponent();
    // 释放技能组件
    virtual ~SkillComponent();

    // 4. 覆盖 Component 的生命周期方法
    // 初始化技能槽位
    virtual bool init() override;
    // 每帧更新冷却与被动效果
    virtual void update(float dt) override;
    // 组件挂载时缓存 Owner
    virtual void onAdd() override; // 当组件被添加到节点时调用

    // 学习技能（解锁）
    void learnSkill(const std::shared_ptr<Skill>& skill);

    // 将技能装备到槽位 (建议改为 bool 返回值，方便判断是否装备成功)
    bool equipActiveSkill(const std::shared_ptr<ActiveSkill>& skill, size_t slotIndex);
    bool equipPassiveSkill(const std::shared_ptr<PassiveSkill>& skill, size_t slotIndex);

    // 卸下技能（将对应槽位置空；被动技能会移除属性加成）
    bool unequipActiveSkill(size_t slotIndex);
    bool unequipPassiveSkill(size_t slotIndex);

    // 使用主动技能（成功返回 true）
    bool useActiveSkill(size_t slotIndex);

    // 获取主动技能槽位
    const std::vector<std::shared_ptr<ActiveSkill>>& getActiveSlots() const { return _activeSlots; }
    // 获取被动技能槽位
    const std::vector<std::shared_ptr<PassiveSkill>>& getPassiveSlots() const { return _passiveSlots; }

    //================== 存档系统支持 ==================

    // 获取已学习的技能列表
    const std::vector<std::shared_ptr<Skill>>& getLearnedSkills() const { return _learnedSkills; }

    // 根据 ID 查找已学习的技能
    std::shared_ptr<Skill> findLearnedSkillById(int skillId) const;

    // 清空并设置主动技能槽位（用于读档）
    void clearAndSetActiveSlots(const std::vector<std::shared_ptr<ActiveSkill>>& slots);

    // 清空并设置被动技能槽位（用于读档）
    void clearAndSetPassiveSlots(const std::vector<std::shared_ptr<PassiveSkill>>& slots);

    // 清空所有技能与槽位（用于读档前重置状态）
    void resetSkills();

private:
    // 辅助函数：获取 Owner 并转换为 CharacterBase
    // Component 自带 _owner 指针 (Node*)，我们需要转为 CharacterBase*
    CharacterBase* getCharacterOwner() const;

    // 缓存一个指针，避免每帧 dynamic_cast (在 onAdd 中赋值)
    CharacterBase* _cachedOwner = nullptr;

    std::vector<std::shared_ptr<Skill>> _learnedSkills;
    std::vector<std::shared_ptr<ActiveSkill>> _activeSlots;
    std::vector<std::shared_ptr<PassiveSkill>> _passiveSlots;

    // 应用被动技能效果
    void applyPassiveSkill(const std::shared_ptr<PassiveSkill>& skill);
    // 移除被动技能效果
    void removePassiveSkill(const std::shared_ptr<PassiveSkill>& skill);
};
