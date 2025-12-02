#include "SceneTransitionHelper.h"

void SceneTransitionHelper::runEnterTransition(
    Scene* scene,
    Menu* menu,
    const std::string& message,
    float delayBeforeFadeOut,
    float fadeDuration
)
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2);

    // 1. 黑色遮罩
    auto overlay = LayerColor::create(Color4B::BLACK);
    overlay->setOpacity(0); // 先透明
    scene->addChild(overlay, 10000); // 确保最上层

    // 淡入遮罩
    overlay->runAction(FadeTo::create(fadeDuration * 0.5f, 255));

    // 2. 提示文字
    auto label = Label::createWithTTF(message, "fonts/ZCOOLKuaiLe-Regular.ttf", 48);
    if (label)
    {
        label->setPosition(center);
        label->setOpacity(0);
        overlay->addChild(label);

        // 文字淡入 → 停留 → 淡出
        label->runAction(Sequence::create(
            FadeIn::create(fadeDuration * 0.5f),
            DelayTime::create(delayBeforeFadeOut),
            FadeOut::create(fadeDuration * 0.5f),
            nullptr
        ));
    }

    // 3. 遮罩淡出并删除
    overlay->runAction(Sequence::create(
        DelayTime::create(delayBeforeFadeOut + fadeDuration),
        FadeOut::create(fadeDuration * 0.5f),
        RemoveSelf::create(),
        nullptr
    ));

    // 4. 菜单淡入
    if (menu)
    {
        menu->setOpacity(0);
        menu->runAction(Sequence::create(
            DelayTime::create(delayBeforeFadeOut + fadeDuration),
            FadeIn::create(fadeDuration * 0.5f),
            nullptr
        ));
    }
}
