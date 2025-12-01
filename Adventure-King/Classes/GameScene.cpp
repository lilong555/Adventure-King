/**
 * @file GameScene.cpp
 * @brief 游戏关卡场景实现
 */

#include "GameScene.h"
#include "MapScene.h"

USING_NS_CC;

// ============================================================
// GameScene 基类实现
// ============================================================

bool GameScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    // 创建地图按钮（左上角）
    createMapButton();

    return true;
}

void GameScene::createMapButton()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 创建地图按钮
    auto mapButton = MenuItemImage::create(
        "Scene/UI/MapInGame.png",
        "Scene/UI/MapInGameSelected.png",
        CC_CALLBACK_1(GameScene::onMapButtonClicked, this));

    if (mapButton)
    {
        // 设置按钮缩放（根据需要调整）
        mapButton->setScale(0.5f);

        // 定位到左上角（留出边距）
        float padding = 20.0f;
        float buttonX = origin.x + mapButton->getContentSize().width * mapButton->getScale() / 2 + padding;
        float buttonY = origin.y + visibleSize.height - mapButton->getContentSize().height * mapButton->getScale() / 2 - padding;
        mapButton->setPosition(Vec2(buttonX, buttonY));

        // 创建菜单并添加到场景
        auto menu = Menu::create(mapButton, nullptr);
        menu->setPosition(Vec2::ZERO);
        menu->setTag(TAG_MAP_BUTTON);
        this->addChild(menu, 100); // 高 z-order 确保显示在最上层
    }
}

void GameScene::onMapButtonClicked(cocos2d::Ref *pSender)
{
    CCLOG("Map button clicked, returning to map scene");

    // 创建地图场景并切换
    auto mapScene = MapScene::createScene();
    if (mapScene)
    {
        const float TRANSITION_DURATION = 0.5f;
        auto transition = TransitionFade::create(TRANSITION_DURATION, mapScene, Color3B::BLACK);
        Director::getInstance()->replaceScene(transition);
    }
}

void GameScene::setupBackground(const std::string &backgroundPath)
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    auto background = Sprite::create(backgroundPath);
    if (background)
    {
        background->setPosition(center);

        // 缩放背景以填满屏幕
        Size textureSize = background->getContentSize();
        float scaleX = visibleSize.width / textureSize.width;
        float scaleY = visibleSize.height / textureSize.height;
        float scaleFactor = std::max(scaleX, scaleY);
        background->setScale(scaleFactor);

        background->setTag(TAG_BACKGROUND);
        this->addChild(background, 0);
    }
}

// ============================================================
// OriginMushroomScene 实现（起源之菇）
// ============================================================

Scene *OriginMushroomScene::createScene()
{
    return OriginMushroomScene::create();
}

bool OriginMushroomScene::init()
{
    if (!GameScene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // TODO: 设置起源之菇场景的背景
    // setupBackground("Scene/Backgrounds/OriginMushroom.png");

    // 临时：添加场景标题标签
    auto titleLabel = Label::createWithTTF(
        "起源之菇",
        "fonts/ZCOOLKuaiLe-Regular.ttf",
        72);

    if (titleLabel)
    {
        titleLabel->setPosition(center);
        titleLabel->setColor(Color3B::WHITE);
        this->addChild(titleLabel, 1);
    }

    // 添加提示文字
    auto hintLabel = Label::createWithTTF(
        "点击左上角地图进入选择界面",
        "fonts/ZCOOLKuaiLe-Regular.ttf",
        32);

    if (hintLabel)
    {
        hintLabel->setPosition(Vec2(center.x, center.y - 80));
        hintLabel->setColor(Color3B(200, 200, 200));
        this->addChild(hintLabel, 1);
    }

    CCLOG("OriginMushroomScene initialized");
    return true;
}

// ============================================================
// MysteryForestScene 实现（神秘之森）
// ============================================================

Scene *MysteryForestScene::createScene()
{
    return MysteryForestScene::create();
}

bool MysteryForestScene::init()
{
    if (!GameScene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // TODO: 设置神秘之森场景的背景
    // setupBackground("Scene/Backgrounds/MysteryForest.png");

    // 临时：添加场景标题标签
    auto titleLabel = Label::createWithTTF(
        "神秘之森",
        "fonts/ZCOOLKuaiLe-Regular.ttf",
        72);

    if (titleLabel)
    {
        titleLabel->setPosition(center);
        titleLabel->setColor(Color3B::WHITE);
        this->addChild(titleLabel, 1);
    }

    // 添加提示文字
    auto hintLabel = Label::createWithTTF(
        "点击左上角地图进入选择界面",
        "fonts/ZCOOLKuaiLe-Regular.ttf",
        32);

    if (hintLabel)
    {
        hintLabel->setPosition(Vec2(center.x, center.y - 80));
        hintLabel->setColor(Color3B(200, 200, 200));
        this->addChild(hintLabel, 1);
    }

    CCLOG("MysteryForestScene initialized");
    return true;
}
