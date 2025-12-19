#pragma once

#include "Character/Monster/MonsterBase.h"
#include "Configs/GameConfigs.h"
#include <string>
#include <vector>

class GobluMonster : public MonsterBase
{
public:
    GobluMonster();
    virtual ~GobluMonster();

    static GobluMonster* create(const std::string& spriteFrameName = "Sprites/Enemies/Goblu/Goblu.png");

    virtual bool init(const std::string& spriteFrameName) override;
    virtual void update(float dt) override;

    virtual void attack() override;

protected:
    void initAnimations();
    void initStateAnimations();
    void initAttributes();
    float getNodeHalfWidth(cocos2d::Node* node);
    float getGapXToTarget(cocos2d::Node* target);
    float getAttackReachX(bool useNear);

    cocos2d::Animate* _attackAnimateNear = nullptr;
    cocos2d::Animate* _attackAnimateFar = nullptr;
    float _baseAttackRange = 0.0f;
};
