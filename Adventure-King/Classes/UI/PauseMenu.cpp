/**
 * @file PauseMenu.cpp
 * @brief 暂停菜单组件实现
 */

#include "PauseMenu.h"
#include <vector>

USING_NS_CC;

PauseMenu *PauseMenu::create()
{
    PauseMenu *ret = new (std::nothrow) PauseMenu();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool PauseMenu::init()
{
    if (!Layer::init())
    {
        return false;
    }

    // 创建容器
    _container = Node::create();
    this->addChild(_container);

    createBackground();
    createTitle();
    createMenuButtons();

    // 初始隐藏
    this->setVisible(false);
    _isShowing = false;

    // 阻止触摸事件穿透
    _touchListener = EventListenerTouchOneByOne::create();
    _touchListener->setSwallowTouches(true);
    _touchListener->onTouchBegan = [this](Touch *touch, Event *event) -> bool
    {
        return _isShowing; // 只有显示时才拦截触摸
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_touchListener, this);
    _touchListener->setEnabled(false); // 初始禁用

    return true;
}

void PauseMenu::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 全屏半透明黑色背景
    _background = DrawNode::create();
    _background->drawSolidRect(
        origin,
        Vec2(origin.x + visibleSize.width, origin.y + visibleSize.height),
        Color4F(0.0f, 0.0f, 0.0f, 0.7f));
    _container->addChild(_background, 0);

    // 中央面板
    float panelWidth = 300;
    float panelHeight = 500;
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    auto panel = DrawNode::create();
    panel->drawSolidRect(
        Vec2(center.x - panelWidth / 2, center.y - panelHeight / 2),
        Vec2(center.x + panelWidth / 2, center.y + panelHeight / 2),
        Color4F(0.1f, 0.1f, 0.15f, 0.95f));

    // 面板边框
    panel->drawRect(
        Vec2(center.x - panelWidth / 2, center.y - panelHeight / 2),
        Vec2(center.x + panelWidth / 2, center.y + panelHeight / 2),
        Color4F(0.4f, 0.4f, 0.5f, 1.0f));

    _container->addChild(panel, 1);
}

void PauseMenu::createTitle()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    _titleLabel = Label::createWithTTF("游戏暂停", "fonts/ZCOOLKuaiLe-Regular.ttf", 36);
    _titleLabel->setPosition(Vec2(center.x, center.y + 130));
    _titleLabel->setColor(Color3B(255, 220, 100));
    _titleLabel->enableOutline(Color4B::BLACK, 2);
    _container->addChild(_titleLabel, 2);
}

void PauseMenu::createMenuButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // 创建菜单按钮
    auto resumeBtn = createButton("继续游戏", CC_CALLBACK_1(PauseMenu::onResumeClicked, this));
    auto inventoryBtn = createButton("背包 / 技能", CC_CALLBACK_1(PauseMenu::onInventoryClicked, this));
    auto saveBtn = createButton("保存游戏", CC_CALLBACK_1(PauseMenu::onSaveClicked, this));
    auto loadBtn = createButton("加载游戏", CC_CALLBACK_1(PauseMenu::onLoadClicked, this));
    auto settingsBtn = createButton("设置", CC_CALLBACK_1(PauseMenu::onSettingsClicked, this));
    auto mainMenuBtn = createButton("返回主菜单", CC_CALLBACK_1(PauseMenu::onMainMenuClicked, this));
    auto quitBtn = createButton("退出游戏", CC_CALLBACK_1(PauseMenu::onQuitClicked, this));

    std::vector<MenuItemLabel *> buttons = {
        resumeBtn,
        inventoryBtn,
        saveBtn,
        loadBtn,
        settingsBtn,
        mainMenuBtn,
        quitBtn,
    };

    // 设置按钮位置
    float buttonSpacing = 46.0f;
    float startY = center.y + 110.0f;
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        buttons[i]->setPosition(Vec2(center.x, startY - buttonSpacing * static_cast<float>(i)));
    }

    _menu = Menu::create();
    _menu->setPosition(Vec2::ZERO);
    for (auto btn : buttons)
    {
        _menu->addChild(btn);
    }
    _container->addChild(_menu, 2);
}

MenuItemLabel *PauseMenu::createButton(const std::string &text, const ccMenuCallback &callback)
{
    auto label = Label::createWithTTF(text, "fonts/ZCOOLKuaiLe-Regular.ttf", 24);
    label->setColor(Color3B::WHITE);

    auto button = MenuItemLabel::create(label, callback);

    return button;
}

void PauseMenu::show()
{
    if (_isShowing)
        return;

    _isShowing = true;
    this->setVisible(true);

    // 启用触摸监听器
    if (_touchListener)
    {
        _touchListener->setEnabled(true);
    }

    // 淡入动画
    _container->setOpacity(0);
    _container->runAction(FadeIn::create(0.2f));

    // 面板从上方滑入
    auto visibleSize = Director::getInstance()->getVisibleSize();
    _container->setPositionY(50);
    _container->runAction(EaseBackOut::create(MoveTo::create(0.3f, Vec2::ZERO)));
}

void PauseMenu::hide()
{
    if (!_isShowing)
        return;

    _isShowing = false;

    // 禁用触摸监听器
    if (_touchListener)
    {
        _touchListener->setEnabled(false);
    }

    // 淡出动画
    auto fadeOut = FadeOut::create(0.2f);
    auto callback = CallFunc::create([this]()
                                     { this->setVisible(false); });

    _container->runAction(Sequence::create(fadeOut, callback, nullptr));
}

void PauseMenu::onResumeClicked(Ref *sender)
{
    hide();
    if (_resumeCallback)
    {
        _resumeCallback();
    }
}

void PauseMenu::onSaveClicked(Ref *sender)
{
    if (_saveCallback)
    {
        _saveCallback();
    }
}

void PauseMenu::onLoadClicked(Ref *sender)
{
    if (_loadCallback)
    {
        _loadCallback();
    }
}

void PauseMenu::onInventoryClicked(Ref *sender)
{
    if (_inventoryCallback)
    {
        _inventoryCallback();
    }
}

void PauseMenu::onSettingsClicked(Ref *sender)
{
    if (_settingsCallback)
    {
        _settingsCallback();
    }
}

void PauseMenu::onMainMenuClicked(Ref *sender)
{
    CCLOG("PauseMenu: Main menu button clicked");
    hide();
    if (_mainMenuCallback)
    {
        CCLOG("PauseMenu: Calling main menu callback");
        _mainMenuCallback();
    }
    else
    {
        CCLOG("PauseMenu: Warning - no main menu callback set");
    }
}

void PauseMenu::onQuitClicked(Ref *sender)
{
    if (_quitCallback)
    {
        // 默认退出游戏
        Director::getInstance()->end();
    }
    else
    {
        Director::getInstance()->end();
    }
}

void PauseMenu::updatePosition(const Vec2 &cameraOffset)
{
    // GameUI 已经设置了位置偏移，PauseMenu 作为子节点会自动跟随
    // 不需要额外的位置更新
    // _container 的位置保持在 (0, 0)
}
