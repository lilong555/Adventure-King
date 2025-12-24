#pragma once

#include "Character/Monster/MonsterBase.h"
#include "Configs/GameConfig.h"
#include <string>
#include <vector>

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

    /// @brief 预热动画缓存（AnimationCache），保证近战/远程动作可播放，并降低首次攻击卡顿
    /// @note 贴图预加载只解决“首刷卡顿”，但不创建动画缓存会导致动作缺失或首次播放抖动
    static void preloadResources();

    static std::vector<std::string> getPreloadResourcePaths();

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
