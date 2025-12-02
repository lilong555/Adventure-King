#pragma once
#pragma once
#include "cocos2d.h"

USING_NS_CC;

class SceneTransitionHelper
{
public:
    // 黑色遮罩过场动画
    static void runEnterTransition(
        Scene* scene,
        Menu* menu,
        const std::string& message,
        float delayBeforeFadeOut = 0.8f,
        float fadeDuration = 0.6f
    );
};

