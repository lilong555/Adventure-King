#include "SceneTransitionManager.h"
#include "MusicManager.h"


USING_NS_CC;

void SceneTransitionManager::transitionToScene(
    Scene *currentScene,
    Scene *targetScene,
    const std::string &message,
    float delayBeforeFadeOut,
    float fadeDuration)
{
    // 统一封装：遮罩淡入+文字提示+淡出切场景
    if (!currentScene || !targetScene)
        return;

    // 手动保持 targetScene 不被销毁
    targetScene->retain();

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2,
                       origin.y + visibleSize.height / 2);

    // 停止播放当前音乐
    MusicManager::getInstance()->stopBGM();

    // 1. 黑色遮罩
    auto overlay = LayerColor::create(Color4B::BLACK);
    overlay->setOpacity(0);
    currentScene->addChild(overlay, 10000);

    // 淡入遮罩
    overlay->runAction(FadeTo::create(fadeDuration * 0.5f, 255));

    // 2. 提示文字
    if (!message.empty())
    {
        auto label = Label::createWithTTF(message, GameSceneConfig::Scene::DEFAULT_FONT_PATH, 48);
        label->setPosition(center);
        label->setOpacity(0);
        overlay->addChild(label);

        label->runAction(Sequence::create(
            FadeIn::create(fadeDuration * 0.5f),
            DelayTime::create(delayBeforeFadeOut),
            FadeOut::create(fadeDuration * 0.5f),
            nullptr));
    }

    overlay->runAction(Sequence::create(
        DelayTime::create(fadeDuration * 0.5f + delayBeforeFadeOut),

        // 在完全黑屏时，移除 overlay 并执行 TransitionFade 切换
        CallFunc::create([targetScene, fadeDuration]()
                         {
            auto transition = TransitionFade::create(fadeDuration * 0.5f, targetScene, Color3B::BLACK);
            // 执行场景切换 (replaceScene)
            Director::getInstance()->replaceScene(transition);
            targetScene->release(); }),
        nullptr));
}
void SceneTransitionManager::fadeReplace(Scene* target, float duration, bool stopMusic)
{
    if (!target) return;

    if (stopMusic)
        MusicManager::getInstance()->stopBGM();

    auto transition = TransitionFade::create(duration, target, Color3B::BLACK);
    Director::getInstance()->replaceScene(transition);
}

