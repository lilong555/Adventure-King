#include "MapScene.h"
USING_NS_CC;

static void problemLoading(const char *filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}
MenuItemImage *MapScene::createMenuItem(
    const char *normal,
    const char *selected,
    const ccMenuCallback &callback)
{
    auto item = MenuItemImage::create(normal, selected, callback);
    return item;
}
bool MapScene::init()
{
    if (!Scene::init())
    {
        return false;
    }
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // ===============================================
    // 内容容器节点，用于统一缩放和定位
    // ===============================================
    // 创建一个父级容器节点
    auto contentContainer = Node::create();
    contentContainer->setTag(TAG_CONTENT_CONTAINER);
    // 将容器节点添加到场景中
    this->addChild(contentContainer, TAG_CONTENT_CONTAINER); // 确保 z-order 高于背景 (背景 z=0)
    // 将容器节点定位
    contentContainer->setPosition(center);
    // ==========================================================
    // 添加背景精灵
    // ==========================================================
    auto sprite = Sprite::create("Scene/Scence/MapBackground.png");
    if (sprite == nullptr)
    {
        problemLoading("'Scene/Scence/MapBackground.png'");
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
    // 用 Sprite
    auto mapselectItem_1 = Sprite::create("Scene/UI/mapselectItem_1.png");
    mapselectItem_1->setPosition(origin.x + visibleSize.height - mapselectItem_1->getContentSize().width / 1.5,
                                 origin.y + visibleSize.height / 1.5);
    mapselectItem_1->setTag(1);
    contentContainer->addChild(mapselectItem_1, 1);

    auto mouseListener = EventListenerMouse::create();

    mouseListener->onMouseMove = [this, mapselectItem_1](EventMouse *event)
    {
        Vec2 mousePos = Vec2(event->getCursorX(), event->getCursorY());
        Vec2 localPos = mapselectItem_1->convertToNodeSpace(mousePos);
        Rect rect = Rect(Vec2::ZERO, mapselectItem_1->getContentSize());

        if (rect.containsPoint(localPos))
        {
            mapselectItem_1->setTexture("Scene/UI/mapselectItem_1_selected.png");
        }
        else
        {
            mapselectItem_1->setTexture("Scene/UI/mapselectItem_1.png");
        }
    };

    mouseListener->onMouseDown = [this, mapselectItem_1](EventMouse *event)
    {
        Vec2 mousePos = Vec2(event->getCursorX(), event->getCursorY());
        Vec2 localPos = mapselectItem_1->convertToNodeSpace(mousePos);
        Rect rect = Rect(Vec2::ZERO, mapselectItem_1->getContentSize());

        if (rect.containsPoint(localPos))
        {
            this->mapSelectCallback(nullptr);
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, mapselectItem_1);
    auto mapselectItem_2 = createMenuItem(
        "Scene/UI/mapselectItem_2.png",
        "Scene/UI/mapselectItem_2_selected.png",
        CC_CALLBACK_1(MapScene::mapSelectCallback, this));
    mapselectItem_2->setPosition(origin.x + visibleSize.height - mapselectItem_2->getContentSize().width / 1.5,
                                 origin.y + visibleSize.height / 1.5);
    auto mapMenu = Menu::create(mapselectItem_2, mapselectItem_2, nullptr);
    mapMenu->setPosition(Vec2::ZERO);
    contentContainer->addChild(mapMenu, 1);
    return true;
}

void MapScene::mapCloseCallback(cocos2d::Ref *pSender)
{
    // 返回上一个场景（场景栈）
    cocos2d::Director::getInstance()->popScene();
}