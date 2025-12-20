#include "MapScene.h"
#include "GameScene.h"
#include "DebugScene.h"
#include "Configs/GameConfigs.h"
USING_NS_CC;

// 统一的资源缺失提示
static void problemLoading(const char *filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}

Scene *MapScene::createScene()
{
    return MapScene::create();
}

MapScene::~MapScene()
{
    for (auto marker : _mapMarkers)
    {
        auto selectedImage = static_cast<std::string *>(marker->getUserData());
        delete selectedImage;
        marker->setUserData(nullptr);
    }
    _mapMarkers.clear();
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
    // 1. 初始化容器节点
    // ===============================================
    auto contentContainer = Node::create();
    contentContainer->setTag(TAG_CONTENT_CONTAINER);
    this->addChild(contentContainer, TAG_CONTENT_CONTAINER);
    contentContainer->setPosition(center);

    // ==========================================================
    // 2. 添加背景精灵
    // ==========================================================
    auto sprite = Sprite::create("Scene/Backgrounds/MapBackground.png");
    if (sprite == nullptr)
    {
        problemLoading("'Scene/Backgrounds/MapBackground.png'");
    }
    else
    {
        sprite->setPosition(Vec2::ZERO);
        contentContainer->addChild(sprite, 0);

        Size textureSize = sprite->getContentSize();
        float scaleX = visibleSize.width / textureSize.width;
        float scaleY = visibleSize.height / textureSize.height;
        float scaleFactor = std::min(scaleX, scaleY);
        contentContainer->setScale(scaleFactor);
    }

    // ==========================================================
    // 3. 初始化地图标记数据
    // ==========================================================
    Size backgroundSize = sprite->getContentSize();
    _markerInfos = {
        {"Scene/UI/mapselectItem_1.png", "Scene/UI/mapselectItem_1_selected.png", Vec2(backgroundSize.width / 20, backgroundSize.height / 2.7), 1, 0.32f, "起源之菇"},
        {"Scene/UI/mapselectItem_2.png", "Scene/UI/mapselectItem_2_selected.png", Vec2(backgroundSize.width / 8.46, backgroundSize.height / 8), 2, 0.32f, "神秘之森"},
        {"Scene/UI/PaintRoom.png", "Scene/UI/PaintRoomSelected.png", Vec2(-backgroundSize.width * 0.2f, backgroundSize.height * 0.2f), 99, 0.35f, "画室"},
    };

    // ==========================================================
    // 4. 创建并添加地标精灵
    // ==========================================================
    for (const auto &info : _markerInfos)
    {
        auto marker = Sprite::create(info.normalImage);
        if (!marker)
        {
            problemLoading(info.normalImage.c_str());
            continue;
        }

        marker->setPosition(info.position);
        marker->setTag(info.mapId);
        marker->setName(info.normalImage);
        marker->setUserData(new std::string(info.selectedImage));
        marker->setScale(info.scale);

        auto nameLabel = Label::createWithTTF(info.name, "fonts/ZCOOLKuaiLe-Regular.ttf", 48);
        if (nameLabel)
        {
            Size markerSize = marker->getContentSize();
            nameLabel->setPosition(Vec2(markerSize.width / 2, -nameLabel->getContentSize().height / 2 - 10));
            nameLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
            marker->addChild(nameLabel, 1);
        }

        contentContainer->addChild(marker, 1);
        _mapMarkers.push_back(marker);
    }

    // ==========================================================
    // 5. 添加鼠标事件监听器
    // ==========================================================
    auto mouseListener = EventListenerMouse::create();

    mouseListener->onMouseMove = [this](EventMouse *event)
    {
        Vec2 mousePos = Vec2(event->getCursorX(), event->getCursorY());

        for (auto marker : _mapMarkers)
        {
            Vec2 localPos = marker->convertToNodeSpace(mousePos);
            Rect rect = Rect(Vec2::ZERO, marker->getContentSize());

            std::string *selectedImage = static_cast<std::string *>(marker->getUserData());

            if (rect.containsPoint(localPos))
            {
                marker->setTexture(*selectedImage);
            }
            else
            {
                marker->setTexture(marker->getName());
            }
        }
    };

    mouseListener->onMouseDown = [this](EventMouse *event)
    {
        Vec2 mousePos = Vec2(event->getCursorX(), event->getCursorY());

        for (auto marker : _mapMarkers)
        {
            Vec2 localPos = marker->convertToNodeSpace(mousePos);
            Rect rect = Rect(Vec2::ZERO, marker->getContentSize());

            if (rect.containsPoint(localPos))
            {
                int mapId = marker->getTag();
                this->onMapMarkerClicked(mapId);
                break;
            }
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

    auto closeItem = MenuItemImage::create(
        "CloseNormal.png",
        "CloseSelected.png",
        CC_CALLBACK_1(MapScene::mapCloseCallback, this));

    if (closeItem)
    {
        float closeX = origin.x + visibleSize.width - closeItem->getContentSize().width / 2;
        float closeY = origin.y + visibleSize.height - closeItem->getContentSize().height / 2;
        closeItem->setPosition(Vec2(closeX, closeY));

        auto closeMenu = Menu::create(closeItem, nullptr);
        closeMenu->setPosition(Vec2::ZERO);
        closeMenu->setTag(TAG_MAP_MENU);
        this->addChild(closeMenu, TAG_MAP_MENU);
    }

    return true;
}

void MapScene::onMapMarkerClicked(int mapId)
{
    CCLOG("Clicked map: %d", mapId);

    auto destinationScene = createDestinationScene(mapId);
    if (!destinationScene)
    {
        CCLOG("Failed to create destination scene for map: %d", mapId);
        return;
    }

    auto director = Director::getInstance();
    // 清空场景栈，回到根场景
    director->popToRootScene();
    // 用 replaceScene 替换当前场景（这样返回时才能回到 MapScene）
    auto transition = TransitionFade::create(GameConfig::Scene::MENU_TRANSITION_DURATION, destinationScene, Color3B::BLACK);
    director->replaceScene(transition);
}

void MapScene::mapCloseCallback(cocos2d::Ref *pSender)
{
    cocos2d::Director::getInstance()->popScene();
}

cocos2d::Scene *MapScene::createDestinationScene(int mapId)
{
    Scene *scene = nullptr;
    switch (mapId)
    {
    case 1:
        scene = OriginMushroomScene::createScene();
        break;
    case 2:
        scene = MysteryForestScene::createScene();
        break;
    case 99: // 测试场景（画室）
        scene = DebugScene::createScene();
        break;
    default:
        CCLOG("Unknown mapId: %d", mapId);
        break;
    }

    if (!scene)
    {
        CCLOG("Failed to create scene for mapId: %d", mapId);
    }

    return scene;
}
