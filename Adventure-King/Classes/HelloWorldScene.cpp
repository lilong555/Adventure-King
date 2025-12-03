#include "HelloWorldScene.h"
#include "HomeScene.h"
#include "MapScene.h"
#include "SaveMenuLayer.h"
#include "SetMenuLayer.h"
#include "SceneTransitionManager.h"
#include "MusicManager.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;

Scene *HelloWorld::createScene()
{
    return HelloWorld::create();
}

// 当文件不存在时，打印有用的错误消息而不是段错误。
static void problemLoading(const char *filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}
MenuItemImage *HelloWorld::createMenuItem(
    const char *normal,
    const char *selected,
    const ccMenuCallback &callback)
{
    auto item = MenuItemImage::create(normal, selected, callback);
    if (!item || item->getContentSize().width <= 0 || item->getContentSize().height <= 0)
    {
        problemLoading(normal);
    }
    return item;
}

// 初始化实例
bool HelloWorld::init()
{
    // 1. super init first
    if (!Scene::init())
    {
        return false;
    }
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // ==========================================================
    // 布局常量和参考点定义
    // ==========================================================
    // 屏幕中心点
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // 按钮之间的水平和垂直间距
    const float BUTTON_HORIZONTAL_SPACING = 180.0f; // 存档-地图-设置 之间的间距
    const float BUTTON_GROUP_OFFSET_Y = 100.0f;     // Start按钮和下方按钮组的垂直偏移量

    // ===============================================
    // 内容容器节点，用于统一缩放和定位
    // ===============================================
    // 创建一个父级容器节点
    auto contentContainer = Node::create();
    contentContainer->setTag(TAG_CONTENT_CONTAINER);
    // 将容器节点添加到场景中
    this->addChild(contentContainer, 5); // 确保 z-order 高于背景 (背景 z=0)
    // 将容器节点定位
    contentContainer->setPosition(center);

    // ==========================================================
    // 2. 菜单项创建和错误检查 (使用 Lambda 简化代码)
    // ==========================================================

    //// 退出按钮 (右下角)
    // auto closeItem = createMenuItem(
    //     "Scene/UI/CloseNormal.png",
    //     "Scene/UI/CloseSelected.png",
    //     CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));

    // 开始按钮 (主菜单中心)
    auto StartItem = createMenuItem(
        "Scene/UI/StartItemNormal.png",
        "Scene/UI/StartItemSelect.png",
        CC_CALLBACK_1(HelloWorld::menuStartCallback, this));

    // 设置按钮 (左侧)
    auto SetItem = createMenuItem(
        "Scene/UI/SetingNormal.png",
        "Scene/UI/SetingSelect.png",
        CC_CALLBACK_1(HelloWorld::menuSetCallback, this));

    // 地图按钮 (中央下方)
    auto MapItem = createMenuItem(
        "Scene/UI/MapNormal.png",
        "Scene/UI/MapSelect.png",
        CC_CALLBACK_1(HelloWorld::menuMapCallback, this));

    // 存档按钮 (右侧)
    auto SaveItem = createMenuItem(
        "Scene/UI/SaveNormal.png",
        "Scene/UI/SaveSelect.png",
        CC_CALLBACK_1(HelloWorld::menuSaveCallback, this));

    // ==========================================================
    // 3. 统一设置按钮位置
    // ==========================================================
    
    // StartItem 定位到屏幕中心
    if (StartItem)
    {
        StartItem->setPosition(Vec2::ZERO);
    }

    // 下方按钮组的 Y 坐标
    float sub_menu_y = -(1.2) * StartItem->getContentSize().height;

    if (MapItem)
    {
        // MapItem 在 StartItem 下方居中
        MapItem->setPosition(Vec2(0, sub_menu_y));
    }

    if (SetItem)
    {
        // SetItem 在 MapItem 左侧
        SetItem->setPosition(Vec2(-BUTTON_HORIZONTAL_SPACING, sub_menu_y));
    }

    if (SaveItem)
    {
        // SaveItem 在 MapItem 右侧
        SaveItem->setPosition(Vec2(BUTTON_HORIZONTAL_SPACING, sub_menu_y));
    }

    // ==========================================================
    // 4. 创建菜单并添加
    // ==========================================================

    auto menu = Menu::create(StartItem, SetItem, SaveItem, MapItem, NULL);
    menu->setPosition(Vec2(0, -visibleSize.height / 20));
    contentContainer->addChild(menu, 1);

    // ==========================================================
    // 5. 添加背景精灵
    // ==========================================================

    auto sprite = Sprite::create("Scene/UI/menuBacground_1.png");
    if (sprite == nullptr)
    {
        problemLoading("'Scene/UI/menuBacground_1.png'");
    }
    else
    {
        sprite->setPosition(Vec2::ZERO);
        contentContainer->addChild(sprite, 0);

        // 缩放整个内容容器以适应屏幕
        Size textureSize = sprite->getContentSize();
        float scaleX = visibleSize.width / textureSize.width;
        float scaleY = visibleSize.height / textureSize.height;
        float scaleFactor = std::min(scaleX, scaleY);

        // 将相同的缩放比例应用到 X 和 Y 轴
        contentContainer->setScale(scaleFactor);
    }
    std::string musicFile = "Scene/MusicOfScene/Music_HelloWorldScene.mp3";
	float musicVolume = 0.5f;
    this->scheduleOnce(
        [musicFile, musicVolume](float dt) {
            MusicManager::getInstance()->playBGM(musicFile, true, musicVolume);
        },
        0.1f, //必须加一定延迟否则会被场景切换截断
        "PlayMusicAfterSceneChange"
    );
    return true;
}

void HelloWorld::menuCloseCallback(Ref *pSender)
{
    // 关闭cocos2d - x游戏场景并退出应用程序
    Director::getInstance()->end();

    /*若要在不退出应用的情况下返回到原生 iOS 屏幕（如果存在），请不要使用上面给出的 Director::getInstance()->end()，而是触发在 RootViewController.mm 中创建的自定义事件，如下所示*/

    // EventCustom customEndEvent("game_scene_close_event");
    //_eventDispatcher->dispatchEvent(&customEndEvent);
}

void HelloWorld::menuStartCallback(Ref* pSender)
{
    auto newScene = HomeScene::createScene();
    SceneTransitionManager::transitionToScene(
        this,                       // 当前场景
        newScene,
        "进入冒险王之家..."         // 文字
    );
}


void HelloWorld::menuSaveCallback(Ref* pSender)
{
    auto saveMenu = SaveMenuLayer::create();

    auto contentContainer = this->getChildByTag(TAG_CONTENT_CONTAINER);
    if (!contentContainer)
    {
        CCLOG("Error: contentContainer with tag 5 not found!");
        return;
    }

    contentContainer->addChild(saveMenu, 1);
}


void HelloWorld::menuMapCallback(Ref *pSender)
{
    auto mapScene = MapScene::createScene();
    if (!mapScene)
    {
        CCLOG("Error: Failed to create MapScene.");
        return;
    }

    const float TRANSITION_DURATION = 0.6f;
    auto transition = TransitionFade::create(TRANSITION_DURATION, mapScene, Color3B::BLACK);
    Director::getInstance()->pushScene(transition);
}
void HelloWorld::menuSetCallback(Ref *pSender)
{
    auto setMenu = SettingMenuLayer::create();

    auto contentContainer = this->getChildByTag(TAG_CONTENT_CONTAINER);
    if (!contentContainer)
    {
        CCLOG("Error: contentContainer with tag 5 not found!");
        return;
    }

    contentContainer->addChild(setMenu, 1);
}
