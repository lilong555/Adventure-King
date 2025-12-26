/**
 * @file PlayerDeathMenu.cpp
 * @brief 角色死亡菜单（强制暂停）实现
 */

#include "UI/PlayerDeathMenu.h"

USING_NS_CC;

PlayerDeathMenu *PlayerDeathMenu::create()
{
    PlayerDeathMenu *ret = new (std::nothrow) PlayerDeathMenu();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool PlayerDeathMenu::init()
{
    if (!Layer::init())
    {
        return false;
    }

    _container = Node::create();
    this->addChild(_container);

    createBackground();
    createTitle();
    createMenuButtons();

    this->setVisible(false);
    _isShowing = false;

    // 阻止触摸事件穿透
    _touchListener = EventListenerTouchOneByOne::create();
    _touchListener->setSwallowTouches(true);
    _touchListener->onTouchBegan = [this](Touch *, Event *) -> bool
    {
        return _isShowing;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_touchListener, this);
    _touchListener->setEnabled(false);

    return true;
}

void PlayerDeathMenu::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 全屏半透明背景
    _background = DrawNode::create();
    _background->drawSolidRect(
        origin,
        Vec2(origin.x + visibleSize.width, origin.y + visibleSize.height),
        Color4F(0.0f, 0.0f, 0.0f, 0.75f));
    _container->addChild(_background, 0);

    // 中央面板
    const float panelWidth = 380.0f;
    const float panelHeight = 240.0f;
    const Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    auto panel = DrawNode::create();
    panel->drawSolidRect(
        Vec2(center.x - panelWidth / 2, center.y - panelHeight / 2),
        Vec2(center.x + panelWidth / 2, center.y + panelHeight / 2),
        Color4F(0.1f, 0.1f, 0.15f, 0.95f));
    panel->drawRect(
        Vec2(center.x - panelWidth / 2, center.y - panelHeight / 2),
        Vec2(center.x + panelWidth / 2, center.y + panelHeight / 2),
        Color4F(0.4f, 0.4f, 0.5f, 1.0f));

    _container->addChild(panel, 1);
}

void PlayerDeathMenu::createTitle()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    _titleLabel = Label::createWithTTF("角色死亡", "fonts/ZCOOLKuaiLe-Regular.ttf", 40);
    _titleLabel->setPosition(Vec2(center.x, center.y + 70.0f));
    _titleLabel->setColor(Color3B(255, 200, 120));
    _titleLabel->enableOutline(Color4B::BLACK, 2);
    _container->addChild(_titleLabel, 2);
}

void PlayerDeathMenu::createMenuButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    auto restartBtn = createButton("重新挑战", CC_CALLBACK_1(PlayerDeathMenu::onRestartClicked, this));
    auto returnToMapBtn = createButton("返回地图", CC_CALLBACK_1(PlayerDeathMenu::onReturnToMapClicked, this));

    const float buttonSpacing = 60.0f;
    restartBtn->setPosition(Vec2(center.x, center.y + 0.0f));
    returnToMapBtn->setPosition(Vec2(center.x, center.y - buttonSpacing));

    _menu = Menu::create();
    _menu->setPosition(Vec2::ZERO);
    _menu->addChild(restartBtn);
    _menu->addChild(returnToMapBtn);
    _container->addChild(_menu, 2);
}

MenuItemLabel *PlayerDeathMenu::createButton(const std::string &text, const ccMenuCallback &callback)
{
    auto label = Label::createWithTTF(text, "fonts/ZCOOLKuaiLe-Regular.ttf", 28);
    label->setColor(Color3B::WHITE);
    label->enableOutline(Color4B::BLACK, 2);
    return MenuItemLabel::create(label, callback);
}

void PlayerDeathMenu::show()
{
    if (_isShowing)
    {
        return;
    }

    _isShowing = true;
    this->setVisible(true);

    if (_touchListener)
    {
        _touchListener->setEnabled(true);
    }

    _container->setOpacity(0);
    _container->setPosition(Vec2::ZERO);
    _container->runAction(FadeIn::create(0.2f));
}

void PlayerDeathMenu::hide()
{
    if (!_isShowing)
    {
        return;
    }

    _isShowing = false;

    if (_touchListener)
    {
        _touchListener->setEnabled(false);
    }

    auto fadeOut = FadeOut::create(0.2f);
    auto callback = CallFunc::create([this]()
                                     { this->setVisible(false); });
    _container->runAction(Sequence::create(fadeOut, callback, nullptr));
}

void PlayerDeathMenu::onRestartClicked(Ref *)
{
    if (_restartCallback)
    {
        _restartCallback();
    }
}

void PlayerDeathMenu::onReturnToMapClicked(Ref *)
{
    if (_returnToMapCallback)
    {
        _returnToMapCallback();
    }
}

