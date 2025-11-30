#include "HomeScene.h"
#include "HelloWorldScene.h" // 包含主菜单场景，以便返回

USING_NS_CC;

// 静态创建场景方法
Scene* HomeScene::createScene()
{
    return HomeScene::create();
}

// 初始化方法
bool HomeScene::init()
{
    // 1. 调用父类初始化
    if (!Scene::init())
    {
        return false;
    }

    // 获取可见尺寸和中心点
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // ==========================================================
    // 核心内容：显示“游戏进行中”文本
    // ==========================================================

    // 创建一个文本标签
    auto label = Label::createWithTTF("进入冒险王之家...", "fonts/ZCOOLKuaiLe-Regular.ttf", 48);

    if (label)
    {
        // 定位到屏幕中心
        label->setPosition(center);

        // 添加到场景
        this->addChild(label, 1);
    }

    // ==========================================================
    // 添加按钮，不过也建议在游戏中不使用鼠标点击的方法实现功能
    // ==========================================================

    // 创建设置按钮（未完成）
    auto setItem = MenuItemImage::create(
        "CloseNormal.png", 
        "CloseSelected.png",
        CC_CALLBACK_1(HomeScene::menuReturnCallback, this));

    if (setItem)
    {
        // 定位到屏幕左上角
        setItem->setPosition(Vec2(origin.x + setItem->getContentSize().width / 2,
            origin.y + visibleSize.height - setItem->getContentSize().height / 2));

        // 创建菜单容器
        auto menu = Menu::create(setItem, NULL);
        menu->setPosition(Vec2(0, -visibleSize.height / 20));
        this->addChild(menu, 1);
    }


    return true;
}

// 返回主菜单的回调函数
void HomeScene::menuReturnCallback(Ref* pSender)
{
    // 创建主菜单场景
    auto helloWorldScene = HelloWorld::createScene();

    // 使用淡出过渡返回主菜单
    const float TRANSITION_DURATION = 1.0f;
    auto transition = TransitionFade::create(TRANSITION_DURATION, helloWorldScene, Color3B::BLUE);

    Director::getInstance()->replaceScene(transition);
}