#include "MapScene.h"
#include "Managers/SceneRegistry.h"
#include "Scenes/LoadingScene.h"
#include"Scenes/HelloWorldScene.h"
#include "Managers/MusicManager.h"

USING_NS_CC;

Scene* MapScene::createScene() {
    return MapScene::create();
}

bool MapScene::init() {
    if (!Scene::init()) return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 1. 基础内容容器
    auto container = Node::create();
    this->addChild(container);
    container->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2));

    auto bg = Sprite::create("Scene/Backgrounds/MapBackground.png");
    if (bg) {
        container->addChild(bg, 0);
        float scale = std::min(visibleSize.width / bg->getContentSize().width,
            visibleSize.height / bg->getContentSize().height);
        container->setScale(scale);
    }

    // 2. 地标数据
    std::vector<MapMarkerInfo> infos = {
        {SceneID::LEVEL_ORIGIN_MUSHROOM, "起源之菇", "Scene/UI/mapselectItem_1.png", "Scene/UI/mapselectItem_1_selected.png", Vec2(40, 200), 0.32f},
        {SceneID::LEVEL_MYSTERY_FOREST,  "神秘之森", "Scene/UI/mapselectItem_2.png", "Scene/UI/mapselectItem_2_selected.png", Vec2(140, 70), 0.32f},
        { SceneID::DEBUG,  "调试场景", "Scene/UI/PaintRoom.png", "Scene/UI/PaintRoomSelected.png", Vec2(-100, 0), 0.32f }
    };

    for (const auto& info : infos) {
        _markerMap[info.id] = info;
        auto marker = Sprite::create(info.normalImage);
        marker->setPosition(info.position);
        marker->setScale(info.scale);
        marker->setTag(static_cast<int>(info.id));
        container->addChild(marker, 1);
        _mapMarkers.push_back(marker);

        auto label = Label::createWithTTF(info.name, GameSceneConfig::Scene::DEFAULT_FONT_PATH, 48);
        label->setPosition(Vec2(marker->getContentSize().width / 2, -50));
        marker->addChild(label);
    }

    // 3. UI 层（关闭按钮）
    //auto closeItem = MenuItemImage::create(
    //    "Scene/UI/CloseSaveMenu.png",
    //    "Scene/UI/CloseSaveMenuSelected.png",
    //    CC_CALLBACK_1(MapScene::mapCloseCallback, this));

    //if (closeItem) {
    //    closeItem->setPosition(Vec2(origin.x + visibleSize.width, origin.y + visibleSize.height));
    //    auto menu = Menu::create(closeItem, nullptr);
    //    menu->setPosition(Vec2::ZERO);
    //    this->addChild(menu, 100);
    //}

    // 4. 交互监听
    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseMove = [this](EventMouse* e) {
        updateMarkerTextures(Vec2(e->getCursorX(), e->getCursorY()));
        };
    mouseListener->onMouseDown = [this](EventMouse* e) {
        if (_isTransitioning) return;
        Vec2 screenPos(e->getCursorX(), e->getCursorY());
        for (auto m : _mapMarkers) {
            Vec2 localPos = m->getParent()->convertToNodeSpace(screenPos);
            if (m->getBoundingBox().containsPoint(localPos)) {
                this->onMapMarkerClicked(static_cast<SceneID>(m->getTag()));
                break;
            }
        }
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

    // --- 新增：键盘监听 ---
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE) {
            CCLOG("MapScene: Esc pressed, returning to Main Menu.");
            this->mapCloseCallback(nullptr);
        }
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    return true;
}

void MapScene::updateMarkerTextures(const Vec2& mousePos) {
    if (_isTransitioning) return;
    for (auto marker : _mapMarkers) {
        auto id = static_cast<SceneID>(marker->getTag());
        Vec2 localPos = marker->getParent()->convertToNodeSpace(mousePos);
        bool isHover = marker->getBoundingBox().containsPoint(localPos);

        std::string texturePath = isHover ? _markerMap[id].selectedImage : _markerMap[id].normalImage;
        if (marker->getName() != texturePath) {
            marker->setTexture(texturePath);
            marker->setName(texturePath);
        }
    }
}

void MapScene::enterMap(SceneID id) {
    if (_isTransitioning || id == SceneID::NONE) return;
    _isTransitioning = true;

    // 核心重构：不再判断 readyScenes，始终通过 LoadingScene 进入
    // 这样保证了资源加载逻辑的 100% 统一，减少 Bug 发生率
    auto loadingScene = LoadingScene::createScene(id);

    if (loadingScene) {
        auto transition = TransitionFade::create(GameSceneConfig::Scene::MENU_TRANSITION_DURATION, loadingScene, Color3B::BLACK);
        Director::getInstance()->replaceScene(transition);
    }
    else {
        _isTransitioning = false;
    }
}

void MapScene::onMapMarkerClicked(SceneID id) {
    this->enterMap(id);
}

void MapScene::mapCloseCallback(Ref* pSender) {
    if (_isTransitioning) return;
    _isTransitioning = true;

    CCLOG("MapScene: Transitioning back to HelloWorld");

    // 强制 replace 回主菜单并释放 MapScene 资源防止pop堆积
    auto helloWorld = HelloWorld::createScene();
    if (helloWorld) {
        auto transition = TransitionFade::create(GameSceneConfig::Scene::MENU_TRANSITION_DURATION, helloWorld, Color3B::BLACK);
        Director::getInstance()->replaceScene(transition);
    } else {
        _isTransitioning = false;
    }
    
}
