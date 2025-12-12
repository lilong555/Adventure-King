#include "SetMenuLayer.h"
#include "Managers/MusicManager.h" // 引入音乐管理器
#include "ui/UISlider.h"  // 引入 Slider
#include "cocos2d.h"

USING_NS_CC;
using namespace ui;

SettingMenuLayer* SettingMenuLayer::create()
{
    SettingMenuLayer* ret = new (std::nothrow) SettingMenuLayer();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool SettingMenuLayer::init()
{
    if (!Layer::init())
        return false;

    if (!initBackground())
        return false;

    if (!initCloseButton())
        return false;

    if (!initMusicToggle())
        return false;

    layoutUI();

    // =========================================================
    // 【关键修复】创建并注册触摸事件监听器
    // =========================================================
    auto touchListener = EventListenerTouchOneByOne::create();

    // 设置 SwallowTouches 为 true，这是防止事件穿透的关键
    touchListener->setSwallowTouches(true);

    // 绑定触摸回调函数
    touchListener->onTouchBegan = CC_CALLBACK_2(SettingMenuLayer::onTouchBegan, this);

    // 将监听器添加到事件分发器中
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    return true;
}
bool SettingMenuLayer::onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event)
{
    // 无论触摸发生在图层内部还是外部（除了菜单按钮，菜单按钮有自己的事件处理），
    // 都会被这个监听器捕获。

    // 返回 true 表示该事件已被该节点完全处理（“吞噬”），
    // 事件将不会继续传递给 scene graph 中优先级更低的节点（即底层的游戏层）。
    return true;
}

bool SettingMenuLayer::initBackground()
{
    if (!FileUtils::getInstance()->isFileExist("Scene/UI/SettingBackground.png"))
    {
        CCLOG("Error: SaveGround.png file not found!");
        return false;
    }

    _background = Sprite::create("Scene/UI/SettingBackground.png");
    if (!_background)
    {
        CCLOG("Error: Failed to create SettingGround sprite!");
        return false;
    }

    this->addChild(_background);
    auto labelSetting = Label::createWithTTF("设置", "fonts/NotoSansSC/NotoSansSC-Bold.ttf", 32);
    labelSetting->setTextColor(Color4B(73, 188, 230, 255));
    labelSetting->enableOutline(Color4B::BLACK, 1);

    labelSetting->setPosition(
        _background->getContentSize().width / 2,
		_background->getContentSize().height / 9 * 8
    );
    _background->addChild(labelSetting);
    return true;
}

bool SettingMenuLayer::initCloseButton()
{
    auto closeItem = MenuItemImage::create(
        "Scene/UI/Button_ApplySetting.png",
        "Scene/UI/Button_ApplySettingSelected.png",
        CC_CALLBACK_1(SettingMenuLayer::onClose, this));

    if (!closeItem)
    {
        CCLOG("Error: Close button load failed!");
        return false;
    }

    // 创建按钮文字（Label）
    auto labelClose = Label::createWithTTF("确定", "fonts/NotoSansSC/NotoSansSC-Medium.ttf", 32);
    labelClose->setTextColor(Color4B(252, 246, 222, 255));
    labelClose->enableOutline(Color4B(88, 54, 12,255), 1);//加描边
    labelClose->setPosition(
        closeItem->getContentSize().width / 2,
        closeItem->getContentSize().height / 2
    );
    closeItem->addChild(labelClose);

    // 创建菜单
    auto menu = Menu::create(closeItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    _background->addChild(menu, 10);

    // 调整按钮位置
    closeItem->setPosition(
        _background->getContentSize().width / 2,
        _background->getContentSize().height / 8
    );

    // 缩放
    closeItem->setScale(1.2f);

    return true;
}

bool SettingMenuLayer::initMusicToggle()
{
    // 初始状态：音乐是否开启
    bool isMusicOn = MusicManager::getInstance()->isEnabled();

    // 创建两个标签
    auto labelOn = Label::createWithTTF("背景音乐：开", "fonts/MaShanZheng-Regular.ttf", 40);
    auto labelOff = Label::createWithTTF("背景音乐：关", "fonts/MaShanZheng-Regular.ttf", 40);
	

    auto itemOn = MenuItemLabel::create(labelOn);
    auto itemOff = MenuItemLabel::create(labelOff);

    // 创建 Toggle 选项
    auto toggleItem = MenuItemToggle::createWithCallback(
        CC_CALLBACK_1(SettingMenuLayer::onMusicToggle, this),
        itemOn,
        itemOff,
        nullptr
    );

    // 设置初始选择
    toggleItem->setSelectedIndex(isMusicOn ? 0 : 1);

    // 创建菜单
    auto menu = Menu::create(toggleItem, nullptr);
    menu->setPosition(
        _background->getContentSize().width / 2,
        _background->getContentSize().height * 0.75f
    );

    _background->addChild(menu);

    _musicToggle = toggleItem;
    return true;
}
// 音乐开关回调
void SettingMenuLayer::onMusicToggle(Ref* sender)
{
    auto toggle = static_cast<MenuItemToggle*>(sender);
    int index = toggle->getSelectedIndex();

    bool turnOn = (index == 0);

    MusicManager::getInstance()->setEnabled(turnOn);

    if (turnOn)
        MusicManager::getInstance()->resumeBGM();
    else
        MusicManager::getInstance()->pauseBGM();

    CCLOG("Music toggled: %s", turnOn ? "ON" : "OFF");
}



void SettingMenuLayer::layoutUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    const float TARGET_WIDTH_RATIO = 0.6f;
    float targetHeight = visibleSize.height * TARGET_WIDTH_RATIO;

    float scaleY = targetHeight / _background->getContentSize().height;
    _background->setScale(scaleY);

    // 将背景居中显示
    _background->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2));
}

void SettingMenuLayer::onClose(Ref*)
{
    this->removeFromParent();
}
