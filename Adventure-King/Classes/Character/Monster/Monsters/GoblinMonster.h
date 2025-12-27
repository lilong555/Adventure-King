#pragma once

#include "Character/Monster/MonsterBase.h"
#include"Configs/GameConfig.h"
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

    // 预加载哥布林相关资源（贴图/动画缓存），用于避免首次生成卡顿
    static void preloadResources();

    // 初始化哥布林资源与数据
    virtual bool init(const std::string& spriteFrameName);

    // 实现普通攻击
    virtual void attack() override;

protected:
    // 经验奖励：按玩家等级缩放
    virtual int getExpReward(int playerLevel) const override;

    // 初始化攻击动画
    void initAnimations();

    cocos2d::Animate *_attackAnimate = nullptr;
    // 初始化状态机动画
    void initStateAnimations();
    // 初始化属性数据
    void initAttributes();
};
