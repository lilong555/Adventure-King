#pragma once

#include "Character/Monster/MonsterBase.h"
#include "Configs/GameConfig.h"
#include <string>
#include <vector>

class GobluMonster : public MonsterBase
{
public:
    GobluMonster();
    virtual ~GobluMonster();

    static GobluMonster* create(const std::string& spriteFrameName = "Sprites/Enemies/Goblu/Goblu.png");

    // 预加载 Goblu（Boss）相关资源（贴图/动画缓存），用于避免首次生成卡顿
    /*static void preloadResources();*/

    virtual bool init(const std::string& spriteFrameName) override;
    virtual void update(float dt) override;

    virtual void attack() override;
    virtual void takeDamage(const DamageInfo& info) override;
    virtual void die() override;

protected:
    // 经验奖励：按玩家等级缩放
    virtual int getExpReward(int playerLevel) const override;

    void initAnimations();
    void initStateAnimations();
    void initAttributes();
    cocos2d::Size getBodyWorldSize() const;
    cocos2d::Size getNodeBodyWorldSize(const cocos2d::Node* node) const;
    bool canHitTarget(bool useNear) const;
    float getNodeHalfWidth(cocos2d::Node* node);
    float getGapXToTarget(cocos2d::Node* target);
    float getAttackReachX(bool useNear);

    cocos2d::Animate* _attackAnimateNear = nullptr;
    cocos2d::Animate* _attackAnimateFar = nullptr;
    float _baseAttackRange = 0.0f;
    bool _deathSequenceStarted = false;

    // ==========================================================
    // Goblu Boss：击破机制（替代传统受击硬直）
    // ==========================================================
    enum class BreakState
    {
        NONE = 0,
        FALLING,
        DOWN,
        RISING,
    };

    BreakState _breakState = BreakState::NONE;
    int _breakMeter = 0;

    void addBreakDamage(const DamageInfo& info);
    void startBreakSequence();
};
