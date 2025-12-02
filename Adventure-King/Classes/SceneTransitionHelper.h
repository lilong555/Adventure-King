#pragma once
#pragma once
#include "cocos2d.h"

USING_NS_CC;

class SceneTransitionHelper
{
public:
    // 进入场景时的黑色遮罩过场动画
    static void runEnterTransition(
        Scene *scene,
        Menu *menu,
        const std::string &message,
        float delayBeforeFadeOut = 0.8f,
        float fadeDuration = 0.6f);

    // 离开场景时的黑色遮罩过场动画
    // onComplete: 动画完成后执行的回调
    static void runExitTransition(
        Scene *scene,
        const std::string &message,
        const std::function<void()> &onComplete,
        float delayBeforeFadeOut = 0.5f,
        float fadeDuration = 0.4f);
};
