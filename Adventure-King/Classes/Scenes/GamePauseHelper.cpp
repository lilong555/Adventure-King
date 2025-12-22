/**
 * @file GamePauseHelper.cpp
 * @brief 暂停/恢复共享实现
 */

#include "Scenes/GamePauseHelper.h"

USING_NS_CC;

namespace GamePauseHelper
{
    void setWorldPaused(Scene *scene,
                        Node *gameLayer,
                        bool paused,
                        bool &cachedAutoStep,
                        float &cachedSpeed)
    {
        if (!scene || !gameLayer)
        {
            CCASSERT(false, "GamePauseHelper::setWorldPaused 需要有效的 scene 与 gameLayer");
            return;
        }

        // 只冻结游戏内容层，保证 UI（暂停菜单/背包等）仍然可交互。
        if (paused)
        {
            gameLayer->pause();
        }
        else
        {
            gameLayer->resume();
        }

        // 物理世界默认由 Scene 自动 step；仅靠 update() 早退无法阻止物理模拟。
        // 暂停时关闭 autoStep（并把 speed 置 0），确保角色/怪物/投掷物完全静止。
        if (auto world = scene->getPhysicsWorld())
        {
            if (paused)
            {
                cachedAutoStep = world->isAutoStep();
                cachedSpeed = world->getSpeed();
                world->setAutoStep(false);
                world->setSpeed(0.0f);
            }
            else
            {
                world->setAutoStep(cachedAutoStep);
                world->setSpeed(cachedSpeed);
            }
        }
    }
} // namespace GamePauseHelper

