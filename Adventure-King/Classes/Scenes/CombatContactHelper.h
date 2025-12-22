/**
 * @file CombatContactHelper.h
 * @brief 战斗/落地碰撞结算的共享逻辑（GameScene/DebugScene 共用）
 *
 * 设计目标：
 * - 避免 GameScene 与 DebugScene 各自维护一套 onContactBegin/onContactSeparate，导致行为不一致。
 * - 把“碰撞 -> 伤害/落地判定”的结算方式统一在一处，后续只改一个地方。
 *
 * 注意：
 * - 伤害结算会延迟到下一帧（scheduleOnce 0s），避免在物理回调中直接修改角色/物理状态导致崩溃。
 */

#pragma once

#include "cocos2d.h"

class PlayerCharacter;
class GameInputController;

namespace CombatContactHelper
{
    /// @brief 处理碰撞开始：落地判定、怪物攻击命中、玩家近战命中
    bool handleContactBegin(cocos2d::PhysicsContact& contact,
                            PlayerCharacter* player,
                            GameInputController* inputController);

    /// @brief 处理碰撞分离：落地离开判定
    void handleContactSeparate(cocos2d::PhysicsContact& contact,
                               GameInputController* inputController);

    /// @brief 处理碰撞预解算：统一设置玩家与地形碰撞的摩擦/弹性（保持横向手感）
    bool handleContactPreSolve(cocos2d::PhysicsContact& contact,
                               cocos2d::PhysicsContactPreSolve& solve);
} // namespace CombatContactHelper

