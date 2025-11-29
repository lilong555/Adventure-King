
#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}

// 当文件不存在时，打印有用的错误消息而不是段错误。
static void problemLoading(const char* filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}

// 在 "init" 上你需要初始化你的实例
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Scene::init() )
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.

    // add a "close" icon to exit the progress. it's an autorelease object
    auto closeItem = MenuItemImage::create(
                                            "Scene/Menu/CloseNormal.png",
                                           "Scene/Menu/CloseSelected.png",
                                           CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));

    if (closeItem == nullptr ||
        closeItem->getContentSize().width <= 0 ||
        closeItem->getContentSize().height <= 0)
    {
        problemLoading("Scene/Menu/CloseNormal.png and Scene/Menu/CloseSelected.png");
    }
    else
    {
        float x = origin.x + visibleSize.width - closeItem->getContentSize().width/2;
        float y = origin.y + closeItem->getContentSize().height/2;
        closeItem->setPosition(Vec2(x,y));
    }
    auto StartItem = MenuItemImage::create(
        "Scene/Menu/StartItemNormal.png",
        "Scene/Menu/StartItemSelect.png",
        CC_CALLBACK_1(HelloWorld::menuStartCallback, this));
    float StartItem_Y = 0,StartItem_X=0;
    if (StartItem == nullptr ||
        StartItem->getContentSize().width <= 0 ||
        StartItem->getContentSize().height <= 0)
    {
        problemLoading("Scene/Menu/CloseNormal.png and Scene/Menu/CloseSelected.png");
    }
    else
    {
        float x = origin.x + visibleSize.width/2;
        float y = origin.y + visibleSize.height/2 - closeItem->getContentSize().height / 2;
        StartItem_Y = y;
        StartItem_X = x;
        StartItem->setPosition(Vec2(x, y));
    }
    auto SetItem = MenuItemImage::create(
        "Scene/Menu/SetingNormal.png",
        "Scene/Menu/SetingSelect.png",
        CC_CALLBACK_1(HelloWorld::menuSetCallback, this));

    if (SetItem == nullptr ||
        SetItem->getContentSize().width <= 0 ||
        SetItem->getContentSize().height <= 0)
    {
        problemLoading("SetingNormal.png and SetingSelec");
    }
    else
    {
        float x = StartItem_X - SetItem->getContentSize().width/2  - StartItem->getContentSize().width / 2;
        float y = StartItem_Y - SetItem->getContentSize().height/2- StartItem->getContentSize().height / 2;
        SetItem->setPosition(Vec2(x, y));
    }
    auto MapItem = MenuItemImage::create(
        "Scene/Menu/MapNormal.png",
        "Scene/Menu/MapSelect.png",
        CC_CALLBACK_1(HelloWorld::menuMapCallback, this));

    if (MapItem == nullptr ||
        MapItem->getContentSize().width <= 0 ||
        MapItem->getContentSize().height <= 0)
    {
        problemLoading("MapNormal.png");
    }
    else
    {
        float x = StartItem_X ;
        float y = StartItem_Y - MapItem->getContentSize().height / 2 - StartItem->getContentSize().height / 2;
        MapItem->setPosition(Vec2(x, y));
    }
    auto SaveItem = MenuItemImage::create(
        "Scene/Menu/SaveNormal.png",
        "Scene/Menu/SaveSelect.png",
        CC_CALLBACK_1(HelloWorld::menuSaveCallback, this));
    //SaveItem;
    if (SaveItem == nullptr ||
        SaveItem->getContentSize().width <= 0 ||
        SaveItem->getContentSize().height <= 0)
    {
        problemLoading("SaveSelect.png");
    }
    else
    {
        float x = StartItem_X + SaveItem->getContentSize().width / 2 + StartItem->getContentSize().width / 2;
        float y = StartItem_Y - SaveItem->getContentSize().height / 2 - StartItem->getContentSize().height / 2;
        SaveItem->setPosition(Vec2(x, y));
    }
    //  创建菜单，它是一个自动释放对象
    auto menu = Menu::create(closeItem, StartItem,SetItem,SaveItem,MapItem, NULL);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 1);

    /////////////////////////////
    // 3. add your codes below...



    // add "HelloWorld" splash screen"
    auto sprite = Sprite::create("Scene/Menu/menuBacground_1.png");
    if (sprite == nullptr)
    {
        problemLoading("'Scene/Menu/menuBacground_1.png'");
    }
    else
    {
        // position the sprite on the center of the screen
        sprite->setPosition(Vec2(visibleSize.width/2 + origin.x, visibleSize.height/2 + origin.y));

        // add the sprite as a child to this layer
        this->addChild(sprite, 0);
    }

    return true;
}


void HelloWorld::menuCloseCallback(Ref* pSender)
{
   // 关闭cocos2d - x游戏场景并退出应用程序
    Director::getInstance()->end();

    /*若要在不退出应用的情况下返回到原生 iOS 屏幕（如果存在），请不要使用上面给出的 Director::getInstance()->end()，而是触发在 RootViewController.mm 中创建的自定义事件，如下所示*/

    //EventCustom customEndEvent("game_scene_close_event");
    //_eventDispatcher->dispatchEvent(&customEndEvent);


}
void HelloWorld::menuStartCallback(Ref* pSender)
{
   

}
void HelloWorld::menuSaveCallback(Ref* pSender)
{
   

}
void HelloWorld::menuMapCallback(Ref* pSender)
{


}
void HelloWorld::menuSetCallback(Ref* pSender)
{


}
