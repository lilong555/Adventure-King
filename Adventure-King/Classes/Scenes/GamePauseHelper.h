/**
 * @file GamePauseHelper.h
 * @brief 暂停/恢复的共享逻辑（GameScene/DebugScene 共用）
 *
 * 设计目标：
 * - 暂停时冻结物理模拟与动作/定时器，避免角色在坡上滑动、怪物继续攻击等问题。
 * - 不影响 UI 层：只暂停游戏内容层（gameLayer），UI 仍可交互。
 */

#pragma once

#include "cocos2d.h"

namespace GamePauseHelper
{
    /**
     * @brief 暂停/恢复世界：暂停 gameLayer，并停止/恢复 PhysicsWorld 自动 step
     * @param scene      物理场景（必须非空）
     * @param gameLayer  游戏内容层（必须非空，UI 不应挂在此层）
     * @param paused     是否暂停
     * @param cachedAutoStep 暂停前 PhysicsWorld::isAutoStep() 的缓存（用于恢复）
     * @param cachedSpeed    暂停前 PhysicsWorld::getSpeed() 的缓存（用于恢复）
     */
    void setWorldPaused(cocos2d::Scene *scene,
                        cocos2d::Node *gameLayer,
                        bool paused,
                        bool &cachedAutoStep,
                        float &cachedSpeed);
} // namespace GamePauseHelper

