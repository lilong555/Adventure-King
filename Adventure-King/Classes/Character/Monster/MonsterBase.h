#pragma once
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include "Character/components/AttributeComponent.h"
#include "Character/Base/CharacterBase.h"
#include "Configs/GamePhysicsCategory.h"

namespace cocos2d { class DrawNode; }

class MonsterBase : public CharacterBase
{
public:
    /// @brief 构造怪物基类
    MonsterBase();
    /// @brief 析构怪物基类
    virtual ~MonsterBase();

    /// @brief 初始化怪物（加载贴图/组件/物理）
    virtual bool init(const std::string& spriteFrameName);

    /// @brief 每帧更新（含 AI/移动/攻击节流）
    virtual void update(float dt) override;

    // 必须实现的纯虚攻击接口
    /// @brief 普通攻击入口（默认触发技能槽0）
    virtual void attack() override;

    // 受击重写
    /// @brief 受击处理（含硬直/受击动画）
    virtual void takeDamage(const DamageInfo& info) override;

    // 死亡重写
    /// @brief 死亡处理（禁用物理/隐藏血条）
    virtual void die() override;

    /// @brief 设置血条缩放
    void setHpBarScale(float scale);

    /// @brief 设置攻击目标
    void setTarget(cocos2d::Node* target);
    /// @brief 设置出生点/回家点
    void setHome(const cocos2d::Vec2& pos);
    /// @brief 设置仇恨范围
    void setAggroRadius(float r);
    /// @brief 设置牵引范围
    void setLeashRadius(float r);
    /// @brief 启用巡逻并设置边界
    void enablePatrol(const cocos2d::Vec2& left, const cocos2d::Vec2& right);
    /// @brief 当前是否有仇恨目标
    bool hasAggro() const;
    /// @brief 一次性设置索敌/追击/巡逻参数
	void setAIConfig(float AR, float LR, bool PTL);//设置索敌、追击、巡逻

    // 性能：降低 AI/移动/攻击逻辑的更新频率（物理仍由引擎每帧推进）
    /// @brief 设置 AI/移动/攻击 更新间隔
    void setUpdateTickIntervals(float aiIntervalSeconds, float movementIntervalSeconds, float attackIntervalSeconds);
    /// @brief 设置离屏 AI 更新间隔
    void setInactiveAiUpdateInterval(float seconds);
    /// @brief 设置活跃更新距离（水平）
    void setActiveUpdateDistanceX(float distanceX);
protected:

    /// @brief 初始化并缓存基础属性
    void setupCharacterStats(const Attributes& stats);
    /// @brief 刷新缓存属性到成员变量
    void refreshCacheAttributes();

    // 血条
    /// @brief 确保血条节点存在
    void ensureHpBar();
    /// @brief 更新血条绘制
    void updateHpBar();
    // AI 行为
    /// @brief AI 逻辑更新
    virtual void updateAI(float dt);
    /// @brief 移动逻辑更新
    virtual void updateMovement(float dt);
    /// @brief 攻击逻辑更新
    virtual void updateAttack(float dt);

    // ===================================================================
    // 通用工具：坐标/距离/朝向/判定框
    // ===================================================================

    /// @brief 获取节点世界坐标
    cocos2d::Vec2 getWorldPosition(const cocos2d::Node *node) const;
    /// @brief 获取目标在父节点坐标系的位置
    cocos2d::Vec2 getPositionInParentSpace(const cocos2d::Node *node) const;
    /// @brief 计算与目标的水平距离
    float horizontalDistanceTo(const cocos2d::Node *target) const;
    /// @brief 朝向世界坐标 X
    void faceToX(float targetWorldX);

    // 生成一次性的近战判定框（默认伤害通过 PhysicsBody::tag 传递给 onContactBegin）
    /// @brief 生成近战判定框
    void spawnMeleeHitbox(const cocos2d::Vec2 &offsetInParentSpace,
                          const cocos2d::Size &hitboxSize,
                          int damageTag,
                          float lifeSeconds = 0.1f);

    // 工具
    /// @brief 朝向目标节点
    void faceTarget(Node* target);
    /// @brief 计算与目标距离
    float distanceTo(Node* target)const;
    /// @brief 是否在攻击范围内
    bool inAttackRange(Node* target);

    /// @brief 是否处于活跃更新范围内
    bool isWithinActiveUpdateRange() const;

    Node* _target = nullptr;     // 目标（通常是主角）
    Node* _primaryTarget = nullptr; // 主目标引用（用于离屏激活/重新索敌）
    float _attackTimer = 0.0f;   // 攻击间隔计时

    cocos2d::Vec2 _homePos;
    bool _hasHome = false;
    bool _returningHome = false;

    cocos2d::Vec2 _patrolLeft;
    cocos2d::Vec2 _patrolRight;
    int _patrolDir = 1;

    float _attackRange = 50.0f;    // 攻击距离
    float _aggroRadius = 0.0f;     // 仇恨半径
    float _leashRadius = 0.0f;     // 牵引半径（超过就返回出生点）
	float _attackInterval = 1.5f; // 攻击间隔（秒）
	float _moveSpeed = 150.0f;     // 移动速度
    float _baseScaleX = 1.0f;     // 记录基础水平缩放，用于翻转朝向
    bool _patrolEnabled = false;   // 是否允许巡逻
    cocos2d::Vec2 _moveGoalPos;    // 当前移动目标（父节点坐标系）
    bool _hasMoveGoal = false;     // 是否存在移动目标
    bool _isStunned = false;       // 是否硬直中

    // 性能节流参数（默认值适配开发阶段：保证体验的同时降低 CPU）
    float _aiUpdateAccumulator = 0.0f;
    float _moveUpdateAccumulator = 0.0f;
    float _attackUpdateAccumulator = 0.0f;
    float _aiUpdateInterval = 0.1f;
    float _inactiveAiUpdateInterval = 0.3f;
    float _moveUpdateInterval = 0.033f;
    float _attackUpdateInterval = 0.05f;
    float _activeUpdateDistanceX = 0.0f; // 0 表示按屏幕宽度自动计算

    //碰撞盒
    cocos2d::PhysicsBody* _physicsBody = nullptr;

    cocos2d::DrawNode *_hpBar = nullptr;
    float _hpBarScale = 1.0f;

};
