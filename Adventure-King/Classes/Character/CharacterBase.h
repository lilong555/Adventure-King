#pragma once

#include "cocos2d.h"
#include "Character/CharacterData.h"
#include <memory>

class AttributeComponent;
class StateMachineComponent;
class SkillComponent;
class CharacterBase;

// 伤害信息结构体
struct DamageInfo
{
    float amount;                      // 基础攻击力
    float penetration = 0;             // 护甲穿透（固定值或百分比）
    bool isCritical = false;           // 是否暴击
    float critMultiplier = 1.5f;       // 暴击倍率
    CharacterBase *attacker = nullptr; // 攻击来源（用于反伤或仇恨统计）
};

// 角色基础类
class CharacterBase : public cocos2d::Sprite
{
public:
    virtual ~CharacterBase();

    // 组件 getter
    AttributeComponent *getAttributeComponent() const { return _attributeComponent.get(); }          // 获取属性组件
    StateMachineComponent *getStateMachineComponent() const { return _stateMachineComponent.get(); } // 获取状态机组件
    SkillComponent *getSkillComponent() const { return _skillComponent.get(); }                      // 获取技能组件

    // 核心战斗接口
    virtual void attack() = 0;                       // 普通攻击（子类必须实现）
    virtual void takeDamage(const DamageInfo &info); // 受击
    virtual void die();                              // 死亡

    // HP / MP
    float getCurrentHP() const { return _currentHP; }
    float getCurrentMP() const { return _currentMP; }

    void setCurrentHP(float hp);
    void setCurrentMP(float mp);

    bool isDead() const { return _currentHP <= 0.0f; }

    // 等级/经验
    int getLevel() const { return _level; }
    void setLevel(int level) { _level = level; }

    int getExperience() const { return _experience; }
    void setExperience(int exp) { _experience = exp; }

    // SkillComponent 使用技能时的回调
    virtual void onUseActiveSkill(const ActiveSkill &skill) {}

protected:
    CharacterBase();

    // 子类在 create 中调用，用于初始化贴图和组件
    bool initWithSpriteFrameName(const std::string &spriteFrameName);
    // 使用普通文件路径初始化（用于调试或没有精灵帧缓存时）
    bool initWithFile(const std::string &filename);

    virtual void update(float dt) override;
    // 组件
    std::unique_ptr<AttributeComponent> _attributeComponent;       // 属性组件
    std::unique_ptr<StateMachineComponent> _stateMachineComponent; // 状态机组件
    std::unique_ptr<SkillComponent> _skillComponent;               // 技能组件

    int _level = 1;          // 角色等级
    int _experience = 0;     // 经验值
    float _currentHP = 0.0f; // 当前生命值
    float _currentMP = 0.0f; // 当前能量值
    float _maxHP = 0.0f;     // 最大生命值（用于受击阈值判断）
};
