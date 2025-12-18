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
    MonsterBase();
    virtual ~MonsterBase();

    virtual bool init(const std::string& spriteFrameName);

    virtual void update(float dt) override;

    // 必须实现的纯虚攻击接口
    virtual void attack() override;

    // 受击重写
    virtual void takeDamage(const DamageInfo& info) override;

    // 死亡重写
    virtual void die() override;

    void setHpBarScale(float scale);

    void setTarget(cocos2d::Node* target);
    void setHome(const cocos2d::Vec2& pos);
    void setAggroRadius(float r);
    void setLeashRadius(float r);
    void enablePatrol(const cocos2d::Vec2& left, const cocos2d::Vec2& right);
    bool hasAggro() const;
	void setAIConfig(float AR, float LR, bool PTL);//设置索敌、追击、巡逻

    // 性能：降低 AI/移动/攻击逻辑的更新频率（物理仍由引擎每帧推进）
    void setUpdateTickIntervals(float aiIntervalSeconds, float movementIntervalSeconds, float attackIntervalSeconds);
    void setInactiveAiUpdateInterval(float seconds);
    void setActiveUpdateDistanceX(float distanceX);
protected:

    void setupCharacterStats(const Attributes& stats);
    void refreshCacheAttributes();

    // 血条
    void ensureHpBar();
    void updateHpBar();
    // AI 行为
    virtual void updateAI(float dt);
    virtual void updateMovement(float dt);
    virtual void updateAttack(float dt);

    // ===================================================================
    // 通用工具：坐标/距离/朝向/判定框
    // ===================================================================

    cocos2d::Vec2 getWorldPosition(const cocos2d::Node *node) const;
    cocos2d::Vec2 getPositionInParentSpace(const cocos2d::Node *node) const;
    float horizontalDistanceTo(const cocos2d::Node *target) const;
    void faceToX(float targetWorldX);

    // 生成一次性的近战判定框（默认伤害通过 PhysicsBody::tag 传递给 onContactBegin）
    void spawnMeleeHitbox(const cocos2d::Vec2 &offsetInParentSpace,
                          const cocos2d::Size &hitboxSize,
                          int damageTag,
                          float lifeSeconds = 0.1f);

    // 工具
    void faceTarget(Node* target);
    float distanceTo(Node* target)const;
    bool inAttackRange(Node* target);

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
