#pragma once

#include "Character/Monster/MonsterBase.h"
#include "Configs/GameConfigs.h"
#include <string>

/**
 * @brief Obscur：普通怪物（近战 + 远程冰）
 *
 * - 近战：播放 Obscur_attack_x，命中判定在第 3 帧开始到第 4 帧结束
 * - 远程：播放 Obscur_useice_x 循环，并在“攻击前”锁定玩家位置，在该位置生成 Obscur_ice_x 与命中判定
 *   - 命中判定：冰动画第 2 帧开始到第 3 帧结束
 */
class ObscurMonster : public MonsterBase
{
public:
    ObscurMonster();
    virtual ~ObscurMonster();

    static ObscurMonster* create(const std::string& spriteFrameName = "Sprites/Enemies/Obscur/Obscur_idle.png");

    // 预热资源（贴图/动画缓存），避免首次生成/首次攻击卡顿
    static void preloadResources();

    virtual bool init(const std::string& spriteFrameName) override;
    virtual void attack() override;

protected:
    virtual int getExpReward(int playerLevel) const override;

private:
    void initAttributes();
    void initStateAnimations();
    void initAnimations();

    void performMeleeAttack();
    void performRemoteAttack();

    // 获取目标“脚下居中”的父节点坐标（用于远程冰锁定落点）
    cocos2d::Vec2 getTargetBottomCenterPosInParentSpace() const;

    cocos2d::Animate* _meleeAttackAnimate = nullptr;
};
