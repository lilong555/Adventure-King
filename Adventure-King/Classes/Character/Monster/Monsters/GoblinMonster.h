#pragma once

#include "Character/Monster/MonsterBase.h"
#include"Configs/GameConfigs.h"
#include <string>

class GoblinMonster : public MonsterBase
{
public:
    // 初始化哥布林实例
    GoblinMonster();
    // 释放哥布林资源
    virtual ~GoblinMonster();

    // 创建哥布林角色
    static GoblinMonster* create(const std::string& spriteFrameName = "Sprites/Enemies/Goblin/Goblin_idle.png");

    // 初始化哥布林资源与数据
    virtual bool init(const std::string& spriteFrameName);

    // 根据玩家等级缩放哥布林血量：200 + L*100 + floor(L/10)*1000
    void applyHpScalingForPlayerLevel(int playerLevel);

    // 实现普通攻击
    virtual void attack() override;

protected:
    // 初始化攻击动画
    void initAnimations();

    cocos2d::Animate *_attackAnimate = nullptr;
    // 初始化状态机动画
    void initStateAnimations();
    // 初始化属性数据
    void initAttributes();
};
