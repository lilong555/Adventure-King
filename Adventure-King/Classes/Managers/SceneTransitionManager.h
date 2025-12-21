#ifndef __SCENE_TRANSITION_MANAGER_H__
#define __SCENE_TRANSITION_MANAGER_H__

#include "cocos2d.h"
#include "Configs/GameConfigs.h"
#include <functional>
#include <string>

class SceneTransitionManager
{
public:
    /**
     * @brief 通用场景切换函数
     * @param currentScene 当前场景指针（过渡动画在当前场景执行）
     * @param targetScene 要切换到的场景
     * @param message 提示文字，可为空
     * @param delayBeforeFadeOut 文字停留时间
     * @param fadeDuration 淡入/淡出时长
     * @param musicFile 切换到目标场景后播放的背景音乐文件，可为空表示不播放
     * @param musicVolume 音乐音量 (0~1)
     */
    static void transitionToScene(
        cocos2d::Scene* currentScene,
        cocos2d::Scene* targetScene,
        const std::string& message = "",
        float delayBeforeFadeOut = GameConfig::Scene::TRANSITION_MESSAGE_DELAY,
        float fadeDuration = GameConfig::Scene::TRANSITION_FADE_DURATION
    );
    static void fadeReplace(
        cocos2d::Scene* target,
        float duration = 0.5f,
        bool stopMusic = true
    );
};

#endif // __SCENE_TRANSITION_MANAGER_H__
