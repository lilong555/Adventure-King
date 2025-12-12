#pragma once

#include "Character/Monster/MonsterBase.h"
#include <string>

class GoblinMonster : public MonsterBase
{
public:
    GoblinMonster();
    virtual ~GoblinMonster();

    static GoblinMonster* create(const std::string& spriteFrameName = "Sprites/Enemies/Goblin/Goblin_idle.png");

    virtual bool init(const std::string& spriteFrameName);

    // 实现普通攻击
    virtual void attack() override;

    // 根据自身特色扩展 AI
    virtual void updateAI(float dt) override;

protected:
    void initAnimations();            // 初始化攻击动画

    cocos2d::Animate *_attackAnimate = nullptr;
    void initStateAnimations();       // 初始化状态机动画
    void initAttributes();            // 初始化属性
};
