/**
 * @file GameUI.cpp
 * @brief 游戏内 UI 层实现
 */

#include "GameUI.h"

USING_NS_CC;

GameUI *GameUI::create()
{
    GameUI *ret = new (std::nothrow) GameUI();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GameUI::init()
{
    if (!Node::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 计算 UI 元素相对于屏幕的位置
    float padding = 20.0f;

    // 地图按钮位置：左上角
    _mapButtonPos = Vec2(origin.x + padding + 40, origin.y + visibleSize.height - padding - 40);

    // 交互提示位置：屏幕底部中央
    _interactionHintPos = Vec2(origin.x + visibleSize.width / 2, origin.y + 80);

    // 关卡名称位置：右上角
    _levelNamePos = Vec2(origin.x + visibleSize.width - 100, origin.y + visibleSize.height - 60);

    // 创建 UI 元素
    createMapButton();
    createInteractionHint();
    createLevelNameLabel();

    CCLOG("GameUI initialized");
    return true;
}

void GameUI::createMapButton()
{
    // 创建地图按钮
    _mapButton = MenuItemImage::create(
        "Scene/UI/MapInGame.png",
        "Scene/UI/MapInGameSelected.png",
        CC_CALLBACK_1(GameUI::onMapButtonClicked, this));

    if (_mapButton)
    {
        _mapButton->setScale(0.5f);
        _mapButton->setPosition(_mapButtonPos);

        _mapMenu = Menu::create(_mapButton, nullptr);
        _mapMenu->setPosition(Vec2::ZERO);
        this->addChild(_mapMenu, 10);
    }
}

void GameUI::createInteractionHint()
{
    _interactionHint = Label::createWithTTF(
        "",
        "fonts/ZCOOLKuaiLe-Regular.ttf",
        28);

    if (_interactionHint)
    {
        _interactionHint->setPosition(_interactionHintPos);
        _interactionHint->setColor(Color3B::WHITE);
        _interactionHint->enableOutline(Color4B::BLACK, 2);
        _interactionHint->setVisible(false);
        this->addChild(_interactionHint, 10);
    }
}

void GameUI::createLevelNameLabel()
{
    _levelNameLabel = Label::createWithTTF(
        "",
        "fonts/ZCOOLKuaiLe-Regular.ttf",
        36);

    if (_levelNameLabel)
    {
        _levelNameLabel->setPosition(_levelNamePos);
        _levelNameLabel->setColor(Color3B::WHITE);
        _levelNameLabel->setOpacity(180);
        this->addChild(_levelNameLabel, 10);
    }
}

void GameUI::setMapButtonCallback(const std::function<void()> &callback)
{
    _mapButtonCallback = callback;
}

void GameUI::onMapButtonClicked(Ref *sender)
{
    CCLOG("GameUI: Map button clicked");
    if (_mapButtonCallback)
    {
        _mapButtonCallback();
    }
}

void GameUI::showInteractionHint(const std::string &message)
{
    if (_interactionHint)
    {
        _interactionHint->setString(message);
        _interactionHint->setVisible(true);

        // 添加淡入效果
        _interactionHint->setOpacity(0);
        _interactionHint->runAction(FadeIn::create(0.3f));
    }
}

void GameUI::hideInteractionHint()
{
    if (_interactionHint && _interactionHint->isVisible())
    {
        // 添加淡出效果
        _interactionHint->runAction(Sequence::create(
            FadeOut::create(0.2f),
            CallFunc::create([this]()
                             { _interactionHint->setVisible(false); }),
            nullptr));
    }
}

void GameUI::setLevelName(const std::string &name)
{
    if (_levelNameLabel)
    {
        _levelNameLabel->setString(name);
    }
}

void GameUI::updatePosition(const Vec2 &cameraOffset)
{
    // 更新所有 UI 元素的位置，使其相对于相机保持固定
    // cameraOffset 是场景位置的负值（即相机的偏移量）

    if (_mapMenu)
    {
        _mapButton->setPosition(_mapButtonPos + cameraOffset);
    }

    if (_interactionHint)
    {
        _interactionHint->setPosition(_interactionHintPos + cameraOffset);
    }

    if (_levelNameLabel)
    {
        _levelNameLabel->setPosition(_levelNamePos + cameraOffset);
    }
}
