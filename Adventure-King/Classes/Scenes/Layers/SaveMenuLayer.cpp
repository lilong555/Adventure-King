#include "SaveMenuLayer.h"
USING_NS_CC;

SaveMenuLayer* SaveMenuLayer::create()
{
    SaveMenuLayer* ret = new (std::nothrow) SaveMenuLayer();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool SaveMenuLayer::init()
{
    if (!Layer::init())
        return false;

    if (!initBackground())
        return false;

    if (!initCloseButton())
        return false;

    layoutUI();

    // =========================================================
// 【关键修复】创建并注册触摸事件监听器
// =========================================================
    auto touchListener = EventListenerTouchOneByOne::create();

    // 设置 SwallowTouches 为 true，这是防止事件穿透的关键
    touchListener->setSwallowTouches(true);

    // 绑定触摸回调函数
    touchListener->onTouchBegan = CC_CALLBACK_2(SaveMenuLayer::onTouchBegan, this);

    // 将监听器添加到事件分发器中
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    return true;
}
bool SaveMenuLayer::onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event)
{
	// 无论触摸发生在图层内部还是外部（除了菜单按钮，菜单按钮有自己的事件处理），
	// 都会被这个监听器捕获。
	// 返回 true 表示该事件已被该节点完全处理（“吞噬”），
	// 事件将不会继续传递给 scene graph 中优先级更低的节点（即底层的游戏层）。
	return true;
}
bool SaveMenuLayer::initBackground()
{
    if (!FileUtils::getInstance()->isFileExist("Scene/UI/SaveGround.png"))
    {
        CCLOG("Error: SaveGround.png file not found!");
        return false;
    }

    _background = Sprite::create("Scene/UI/SaveGround.png");
    if (!_background)
    {
        CCLOG("Error: Failed to create SaveGround sprite!");
        return false;
    }
    this->addChild(_background);
	auto labelSave = Label::createWithTTF("存档", "fonts/MaShanZheng-Regular.ttf", 55);
	labelSave->setTextColor(Color4B(73, 188, 230, 255));
    labelSave->enableOutline(Color4B::BLACK, 1);
    labelSave->setPosition(
        _background->getContentSize().width / 2,
        _background->getContentSize().height / 12 * 11
    );
    _background->addChild(labelSave);

    return true;
}

bool SaveMenuLayer::initCloseButton()
{
    auto closeItem = MenuItemImage::create(
        "Scene/UI/CloseSaveMenu.png",
        "Scene/UI/CloseSaveMenuSelected.png",
        CC_CALLBACK_1(SaveMenuLayer::onClose, this));

    if (!closeItem)
    {
        CCLOG("Error: Close button load failed!");
        return false;
    }

    auto menu = Menu::create(closeItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    _background->addChild(menu, 10);

    // 调整按钮位置
    closeItem->setPosition(
        _background->getContentSize().width / 2,
        _background->getContentSize().height / 8
    );

    // 缩放
    float targetScale = 0.5f;
    closeItem->setScale(targetScale);

    return true;
}

void SaveMenuLayer::layoutUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();

    const float TARGET_WIDTH_RATIO = 0.6f;
    float targetHeight = visibleSize.height * TARGET_WIDTH_RATIO;

    float scaleY = targetHeight / _background->getContentSize().height;
    _background->setScale(scaleY);

    // 居中
    _background->setPosition(
        Vec2::ZERO
    );
}

void SaveMenuLayer::onClose(Ref*)
{
    this->removeFromParent();
}
