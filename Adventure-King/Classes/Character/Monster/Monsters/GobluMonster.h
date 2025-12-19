#pragma once

#include "Character/Monster/MonsterBase.h"
#include "Configs/GameConfigs.h"
#include <string>

class GobluMonster : public MonsterBase
{
public:
    GobluMonster();
    virtual ~GobluMonster();

    static GobluMonster* create(const std::string& spriteFrameName = "Sprites/Enemies/Goblu/Goblu.png");

    virtual bool init(const std::string& spriteFrameName) override;

    virtual void attack() override;

protected:
    void initAnimations();
    void initStateAnimations();
    void initAttributes();

    cocos2d::Animate* _attackAnimateA = nullptr;
    cocos2d::Animate* _attackAnimateB = nullptr;
    cocos2d::Size _baseFrameSize;
};
