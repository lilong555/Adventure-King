#pragma once

#include "Character/Monster/MonsterBase.h"
#include"Configs/GameConfigs.h"
#include <string>

class GoblinMonster : public MonsterBase
{
public:
    GoblinMonster();
    virtual ~GoblinMonster();

    static GoblinMonster* create(const std::string& spriteFrameName = "Sprites/Enemies/Goblin/Goblin_idle.png");

    virtual bool init(const std::string& spriteFrameName);

    // 根据玩家等级缩放哥布林血量：200 + L*100 + floor(L/10)*1000
    void applyHpScalingForPlayerLevel(int playerLevel);

    // 实现普通攻击
    virtual void attack() override;

protected:
    void initAnimations();            // 初始化攻击动画

    cocos2d::Animate *_attackAnimate = nullptr;
    void initStateAnimations();       // 初始化状态机动画
    void initAttributes();            // 初始化属性
};
