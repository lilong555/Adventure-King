#include "HelloWorldScene.h" // 包含主菜单场景，以便返回
#include "HomeScene.h"
#include "GameScene.h"
#include "SceneTransitionManager.h"
#include "MusicManager.h"

USING_NS_CC;

// 静态创建场景方法
Scene *HomeScene::createScene()
{
    return HomeScene::create();
}
// 当文件不存在时，打印有用的错误消息而不是错误。
static void problemLoading(const char *filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}
// 初始化方法
bool HomeScene::init()
{
    if (!Scene::init())
        return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2,
                       origin.y + visibleSize.height / 2);

    // 1. 背景
    auto bg = Sprite::create("Scene/Backgrounds/HomeBackground_1.jpg");
    if (bg)
    {
        Size textureSize = bg->getContentSize();
        float scaleX = visibleSize.width / textureSize.width;
        float scaleY = visibleSize.height / textureSize.height;
        float scaleFactor = std::max(scaleX, scaleY); // 覆盖屏幕
        bg->setScale(scaleFactor);
        bg->setPosition(center);
        this->addChild(bg, 0);
    }
    else
    {
        problemLoading("'Scene/Backgrounds/HomeBackground_1.jpg'");
    }

    // 2. 菜单按钮
    auto setItem = MenuItemImage::create(
        "CloseNormal.png",
        "CloseSelected.png",
        CC_CALLBACK_1(HomeScene::menuReturnCallback, this));

    setItem->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
    setItem->setPosition(Vec2(origin.x, origin.y + visibleSize.height));

    auto menu = Menu::create(setItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 5);

    // 播放背景音乐
    std::string musicFile = "Scene/MusicOfScene/Music_HomeScene.mp3";
    float musicVolume = 0.5f;
    this->scheduleOnce(
        [musicFile, musicVolume](float dt)
        {
            MusicManager::getInstance()->playBGM(musicFile, true, musicVolume);
        },
        1.2f,
        "PlayMusicAfterSceneChange");
    return true;
}

// 返回主菜单的回调函数
void HomeScene::menuReturnCallback(Ref *pSender)
{
    MusicManager::getInstance()->stopBGM();

    auto helloWorldScene = HelloWorld::createScene();

    SceneTransitionManager::transitionToScene(
        this,
        helloWorldScene,
        "返回主菜单...",
        1.0f);
}
