#include "HelloWorldScene.h"
#include "HomeScene.h"
#include "MapScene.h"
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

    // if (closeItem)
    //{
    //     // 定位到右下角
    //     float x = origin.x + visibleSize.width - closeItem->getContentSize().width / 2;
    //     float y = origin.y + closeItem->getContentSize().height / 2;
    //     closeItem->setPosition(Vec2(x, y));
    // }

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
void HelloWorld::menuStartCallback(Ref *pSender)
{
    auto newScene = HomeScene::createScene();

    if (newScene)
    {
        // 4. 使用 Director 切换场景

        // 场景过渡：使用 Director::replaceScene 替换当前场景
        // 建议使用过渡效果，让画面切换更平滑。例如：FadeTransition（淡入淡出）
        const float TRANSITION_DURATION = 1.0f; // 过渡时长1.0秒

        // 创建一个过渡场景
        auto transition = cocos2d::TransitionFade::create(TRANSITION_DURATION, newScene, cocos2d::Color3B::BLACK);

        // 运行场景过渡
        cocos2d::Director::getInstance()->replaceScene(transition);

        CCLOG("--- Game Started: Transitioning to GameScene ---");
    }
    else
    {
        CCLOG("Error: Failed to create the new GameScene!");
    }
}

void HelloWorld::menuSaveCallback(Ref *pSender)
{
    // 检查资源文件是否存在
    if (!FileUtils::getInstance()->isFileExist("Scene/UI/SaveGround.png"))
    {
        CCLOG("Error: SaveGround.png file not found!");
        return;
    }

    // 创建精灵并检查是否成功
    auto SaveMenu = Sprite::create("Scene/UI/SaveGround.png");
    if (!SaveMenu)
    {
        CCLOG("Error: Failed to create SaveGround sprite!");
        return;
    }
    SaveMenu->setTag(TAG_SAVE_MENU);
    // 1. 创建关闭按钮
    auto CloseItem = createMenuItem(
        "Scene/UI/CloseSaveMenu.png",
        "Scene/UI/CloseSaveMenuSelected.png",
        CC_CALLBACK_1(HelloWorld::menuSaveCloseCallback, this));

    // 2. 创建菜单
    auto closeMenu = Menu::create(CloseItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO); // 让菜单坐标相对 SaveMenu
    SaveMenu->addChild(closeMenu, 10);

    // 设置关闭按钮的位置
    CloseItem->setPosition(
        SaveMenu->getContentSize().width / 2,
        SaveMenu->getContentSize().height / 8);

    const float TARGET_WIDTH_RATIO = 0.6f;

    // 计算 X 轴缩放因子
    auto visibleSize = Director::getInstance()->getVisibleSize();
    float targetHeight = visibleSize.height * TARGET_WIDTH_RATIO;
    float scaleY = targetHeight / SaveMenu->getContentSize().height;

    // 2. 将等比例缩放因子应用到 SaveMenu
    // 为了保持图片不失真，我们应用同一个比例到 X 和 Y
    SaveMenu->setScale(scaleY);
    float buttonTargetScale = 0.3f; // 你想要的按钮缩放(最终视觉效果)
    CloseItem->setScale(buttonTargetScale / scaleY);
    // 设置合理的位置
    SaveMenu->setPosition(Vec2::ZERO);
    // 获取 contentContainer 并检查
    auto contentContainer = this->getChildByTag(TAG_CONTENT_CONTAINER);
    if (!contentContainer)
    {
        CCLOG("Error: contentContainer with tag 5 not found!");
        SaveMenu->release(); // 清理资源
        return;
    }
    // 5. 安全地添加到容器
    contentContainer->addChild(SaveMenu, 1);

    CCLOG("Save menu created successfully");
}

void HelloWorld::menuSaveCloseCallback(Ref *pSender)
{
    // 找到 contentContainer
    auto contentContainer = this->getChildByTag(TAG_CONTENT_CONTAINER);
    if (!contentContainer)
        return;

    // ⭐ 直接移除 SaveMenu
    auto saveMenu = contentContainer->getChildByTag(TAG_SAVE_MENU);
    if (saveMenu)
    {
        contentContainer->removeChild(saveMenu, true);
        CCLOG("Save menu removed.");
    }
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
}
