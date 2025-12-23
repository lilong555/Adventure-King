#pragma once

#include "cocos2d.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include "Character/Base/CharacterData.h"

// 移除 <memory>，因为不再使用 unique_ptr

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
    CharacterBase* attacker = nullptr; // 攻击来源（用于反伤或仇恨统计）
    bool hasHitWorldPos = false;       // 是否提供命中点（世界坐标）
    cocos2d::Vec2 hitWorldPos = cocos2d::Vec2::ZERO; // 命中点（世界坐标）
    bool causesHitStun = true;         // 是否触发受击硬直/打断（DOT 等持续伤害应为 false）
};

// 角色基础类
class CharacterBase : public cocos2d::Sprite
{
public:
    virtual ~CharacterBase();

    // ------------------------------------------------------------
    // ✅ 组件 Getter (核心修改)
    // ------------------------------------------------------------
    // 直接使用 Cocos2d-x 的泛型方法获取组件
    // 这些方法会遍历 Node 的组件列表，不需要我们自己维护指针变量

    /// @brief 获取属性组件
    AttributeComponent* getAttributeComponent();
    /// @brief 获取状态机组件
    StateMachineComponent* getStateMachineComponent();
    /// @brief 获取技能组件
    SkillComponent* getSkillComponent();

    /// @brief 获取用于播放动画的精灵（默认是自身）
    cocos2d::Sprite* getVisualSprite() const;

    // 核心战斗接口
    /// @brief 普通攻击（子类必须实现）
    virtual void attack() = 0;                           // 普通攻击（子类必须实现）
    /// @brief 受击处理
    virtual void takeDamage(const DamageInfo& info);     // 受击
    virtual void heal(float amount);                     // 治疗
    /// @brief 死亡处理
    virtual void die();                                  // 死亡
     
    // 伤害事件回调：用于装备/被动等“触发型机制”（默认空实现）
    /// @brief 自己造成伤害后回调（在目标 HP 已扣除、死亡判定完成后触发）
    virtual void onDealDamage(CharacterBase* target, float finalDamage, const DamageInfo& info, bool targetDied) {}
    /// @brief 自己受到伤害后回调（在 HP 扣除后、死亡判定前触发，可用于濒死救援等机制）
    /// @param wouldDieBeforeCallback 表示“扣血后，回调触发前”是否会死亡；回调中可能修改 HP 导致最终结果不同
    virtual void onReceiveDamage(CharacterBase* attacker, float finalDamage, const DamageInfo& info, bool wouldDieBeforeCallback) {}

    // HP / MP
    /// @brief 获取当前 HP
    float getCurrentHP() const { return _currentHP; }
    /// @brief 获取当前 MP
    float getCurrentMP() const { return _currentMP; }

    /// @brief 设置当前 HP
    void setCurrentHP(float hp);
    /// @brief 设置当前 MP
    void setCurrentMP(float mp);

    /// @brief 是否已死亡
    bool isDead() const { return _currentHP <= 0.0f; }

    /**
     * @brief 设置死亡后是否自动从场景移除
     * @param autoRemove true=自动移除(默认), false=保留在场景中
     */
    void setAutoRemoveOnDeath(bool autoRemove) { _autoRemoveOnDeath = autoRemove; }
    bool getAutoRemoveOnDeath() const { return _autoRemoveOnDeath; }

    // 受击飘字
    /// @brief 启用/关闭受击飘字
    void setDamageNumbersEnabled(bool enabled) { _damageNumbersEnabled = enabled; }
    /// @brief 是否启用受击飘字
    bool getDamageNumbersEnabled() const { return _damageNumbersEnabled; }

    // 等级/经验
    /// @brief 获取角色等级
    int getLevel() const { return _level; }
    /// @brief 设置角色等级
    void setLevel(int level) { _level = level; }

    /// @brief 获取经验值
    int getExperience() const { return _experience; }
    /// @brief 设置经验值
    void setExperience(int exp) { _experience = exp; }

    // SkillComponent 使用技能时的回调
    virtual void onUseActiveSkill(const ActiveSkill& skill) {}

    // 角色攻击力（用于 DOT 等需要“来源攻击力”计算的场景）
    // 默认实现：返回 STRENGTH；玩家角色可覆写为“武器伤害 + 力量加成”等更贴近手感的计算
    /// @brief 获取攻击力（用于 DOT 计算）
    virtual float getAttackPower();

protected:
    CharacterBase();

    // 受击特效调参（在头文件修改即可生效）
    struct HurtVfxParams
    {
        static constexpr float BURST_DURATION_SECONDS = 0.2f;
        static constexpr int TOTAL_PARTICLES = 15;
        static constexpr float LIFE_SECONDS = 0.3f;
        static constexpr float LIFE_VAR_SECONDS = 0.1f;
        // 粒子尺寸：原值偏大，缩小为原来的 1/4
        static constexpr float START_SIZE = 5.0f;
        static constexpr float START_SIZE_VAR = 2.0f;
    };

    // 子类在 create 中调用，用于初始化贴图和组件
    /// @brief 使用精灵帧名初始化角色
    bool initWithSpriteFrameName(const std::string& spriteFrameName);
    // 使用普通文件路径初始化（用于调试或没有精灵帧缓存时）
    /// @brief 使用文件路径初始化角色
    bool initWithFile(const std::string& filename);

    /// @brief 每帧更新
    virtual void update(float dt) override;

    // 显示受击飘字（添加到角色父节点上，避免跟随角色移动）
    /// @brief 显示受击飘字
    void showDamageNumber(float damage, bool isCritical);
    void showHealNumber(float Amount);
    /// @brief 设置用于播放动画的精灵（默认是自身）
    void setVisualSprite(cocos2d::Sprite* sprite);
    /// @brief 停止视觉层动画（用于打断攻击/技能）
    void stopVisualActions();
    /// @brief 生成受击粒子特效（按攻击方向选择 L/R）
    void spawnHurtVfx(const DamageInfo& info);

    // ------------------------------------------------------------
    // 成员变量
    // ------------------------------------------------------------
    // 注意：不再需要 _attributeComponent 等 unique_ptr 变量

    int _level = 1;                 // 角色等级
    int _experience = 0;            // 经验值
    float _currentHP = 0.0f;        // 当前生命值
    float _currentMP = 0.0f;        // 当前能量值
    float _maxHP = 0.0f;            // 最大生命值（用于受击阈值判断，通常应该从属性组件读）
    bool _autoRemoveOnDeath = true; ///< 死亡后是否自动移除
    long long _lastRestoreHealthVfxMs = 0; ///< 回血特效节流：0.5s 内多次回血只播一次

    bool _damageNumbersEnabled = true; ///< 是否启用受击飘字
    cocos2d::Sprite* _visualSprite = nullptr; ///< 实际播放动画的精灵
    float _visualBaseScaleX = 1.0f;
    float _visualBaseScaleY = 1.0f;
};
