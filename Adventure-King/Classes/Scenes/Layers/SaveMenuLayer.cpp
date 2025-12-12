#include "SaveMenuLayer.h"
#include "Save/SaveManager.h"
#include "Character/Player/PlayerCharacter.h"
#include <ctime>
#include <sstream>
#include <iomanip>

USING_NS_CC;

SaveMenuLayer *SaveMenuLayer::create(Mode mode,
                                      PlayerCharacter *player,
                                      const std::string &sceneName,
                                      const cocos2d::Vec2 &playerPos)
{
    SaveMenuLayer *ret = new (std::nothrow) SaveMenuLayer();
    if (ret && ret->init(mode, player, sceneName, playerPos))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool SaveMenuLayer::init(Mode mode,
                          PlayerCharacter *player,
                          const std::string &sceneName,
                          const cocos2d::Vec2 &playerPos)
{
    if (!Layer::init())
        return false;

    _mode = mode;
    _player = player;
    _sceneName = sceneName;
    _playerPos = playerPos;

    if (!initBackground())
        return false;

    if (!initTitle())
        return false;

    if (!initSlots())
        return false;

    if (!initCloseButton())
        return false;

    layoutUI();

    // 创建并注册触摸事件监听器（防止事件穿透）
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = CC_CALLBACK_2(SaveMenuLayer::onTouchBegan, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    return true;
}

bool SaveMenuLayer::onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *event)
{
    // 吞噬触摸事件，防止穿透到底层
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

    return true;
}

bool SaveMenuLayer::initTitle()
{
    std::string titleText = (_mode == Mode::SAVE) ? "保存游戏" : "加载游戏";

    _titleLabel = Label::createWithTTF(titleText, "fonts/MaShanZheng-Regular.ttf", 55);
    if (!_titleLabel)
    {
        CCLOG("Error: Failed to create title label!");
        return false;
    }

    _titleLabel->setTextColor(Color4B(73, 188, 230, 255));
    _titleLabel->enableOutline(Color4B::BLACK, 1);
    _titleLabel->setPosition(
        _background->getContentSize().width / 2,
        _background->getContentSize().height / 12 * 11);
    _background->addChild(_titleLabel);

    return true;
}

bool SaveMenuLayer::initSlots()
{
    // 获取所有存档槽位信息
    auto saveManager = SaveManager::getInstance();
    auto slotInfos = saveManager->getAllSaveSlotInfos();

    // 创建存档槽位节点
    for (size_t i = 0; i < slotInfos.size(); ++i)
    {
        auto slotNode = createSlotNode(i, slotInfos[i]);
        if (slotNode)
        {
            _background->addChild(slotNode);
            _slotNodes.push_back(slotNode);
        }
    }

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

    closeItem->setPosition(
        _background->getContentSize().width / 2,
        _background->getContentSize().height / 8);

    closeItem->setScale(0.5f);

    return true;
}

void SaveMenuLayer::layoutUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    const float TARGET_WIDTH_RATIO = 0.7f;
    float targetHeight = visibleSize.height * TARGET_WIDTH_RATIO;

    float scaleY = targetHeight / _background->getContentSize().height;
    _background->setScale(scaleY);

    // 将背景居中显示
    _background->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2));

    // 布局存档槽位
    float slotStartY = _background->getContentSize().height / 12 * 9.5f;
    float slotSpacing = _background->getContentSize().height / 12 * 1.5f;

    for (size_t i = 0; i < _slotNodes.size(); ++i)
    {
        _slotNodes[i]->setPosition(
            _background->getContentSize().width / 2,
            slotStartY - i * slotSpacing);
    }
}

cocos2d::Node *SaveMenuLayer::createSlotNode(int slotIndex, const SaveSlotData &slotData)
{
    auto node = Node::create();
    node->setContentSize(Size(_background->getContentSize().width * 0.9f, 80));

    // 背景
    auto bg = LayerColor::create(Color4B(50, 50, 50, 200), node->getContentSize().width, node->getContentSize().height);
    bg->setPosition(Vec2(-node->getContentSize().width / 2, -node->getContentSize().height / 2));
    node->addChild(bg);

    // 槽位标题
    std::string slotTitle = "槽位 " + std::to_string(slotIndex + 1);
    auto titleLabel = Label::createWithTTF(slotTitle, "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24);
    titleLabel->setPosition(Vec2(-node->getContentSize().width / 2 + 80, 0));
    titleLabel->setAnchorPoint(Vec2(0, 0.5f));
    node->addChild(titleLabel);

    // 检查是否有存档
    bool hasSave = (slotData.saveTimestamp > 0);

    if (hasSave)
    {
        // 显示存档信息
        std::string infoText = "等级 " + std::to_string(slotData.playerData.level) +
                               " | " + formatTimestamp(slotData.saveTimestamp);
        auto infoLabel = Label::createWithTTF(infoText, "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 18);
        infoLabel->setPosition(Vec2(-node->getContentSize().width / 2 + 80, -20));
        infoLabel->setAnchorPoint(Vec2(0, 0.5f));
        infoLabel->setTextColor(Color4B(200, 200, 200, 255));
        node->addChild(infoLabel);

        // 删除按钮（仅在有存档时显示）
        auto deleteItem = MenuItemLabel::create(
            Label::createWithTTF("删除", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 20),
            [this, slotIndex](Ref *) {
                onDeleteClicked(slotIndex);
            });
        deleteItem->setPosition(Vec2(node->getContentSize().width / 2 - 80, 0));

        auto deleteMenu = Menu::create(deleteItem, nullptr);
        deleteMenu->setPosition(Vec2::ZERO);
        node->addChild(deleteMenu);
    }
    else
    {
        // 空槽位
        auto emptyLabel = Label::createWithTTF("空槽位", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 18);
        emptyLabel->setPosition(Vec2(-node->getContentSize().width / 2 + 80, -20));
        emptyLabel->setAnchorPoint(Vec2(0, 0.5f));
        emptyLabel->setTextColor(Color4B(150, 150, 150, 255));
        node->addChild(emptyLabel);
    }

    // 主按钮（保存/加载）
    std::string buttonText = (_mode == Mode::SAVE) ? "保存" : "加载";
    auto mainItem = MenuItemLabel::create(
        Label::createWithTTF(buttonText, "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [this, slotIndex](Ref *) {
            onSlotClicked(slotIndex);
        });
    mainItem->setPosition(Vec2(node->getContentSize().width / 2 - 30, 0));

    // 如果是加载模式且槽位为空，禁用按钮
    if (_mode == Mode::LOAD && !hasSave)
    {
        mainItem->setEnabled(false);
        mainItem->setColor(Color3B(100, 100, 100));
    }

    auto mainMenu = Menu::create(mainItem, nullptr);
    mainMenu->setPosition(Vec2::ZERO);
    node->addChild(mainMenu);

    return node;
}

void SaveMenuLayer::onSlotClicked(int slotIndex)
{
    auto saveManager = SaveManager::getInstance();

    if (_mode == Mode::SAVE)
    {
        // 保存模式
        if (saveManager->hasSave(slotIndex))
        {
            // 槽位已有存档，显示确认对话框
            showConfirmDialog("确定要覆盖此存档吗？", [this, saveManager, slotIndex]() {
                if (saveManager->saveGame(slotIndex, _player, _sceneName, _playerPos))
                {
                    CCLOG("SaveMenuLayer - 保存成功到槽位 %d", slotIndex);
                    // 刷新界面
                    this->removeFromParent();
                }
                else
                {
                    CCLOG("SaveMenuLayer - 保存失败");
                }
            });
        }
        else
        {
            // 空槽位，直接保存
            if (saveManager->saveGame(slotIndex, _player, _sceneName, _playerPos))
            {
                CCLOG("SaveMenuLayer - 保存成功到槽位 %d", slotIndex);
                this->removeFromParent();
            }
            else
            {
                CCLOG("SaveMenuLayer - 保存失败");
            }
        }
    }
    else
    {
        // 加载模式
        SaveSlotData loadedData;
        if (saveManager->loadGame(slotIndex, loadedData))
        {
            CCLOG("SaveMenuLayer - 加载成功从槽位 %d", slotIndex);
            if (_loadSuccessCallback)
            {
                _loadSuccessCallback(loadedData);
            }
            this->removeFromParent();
        }
        else
        {
            CCLOG("SaveMenuLayer - 加载失败");
        }
    }
}

void SaveMenuLayer::onDeleteClicked(int slotIndex)
{
    showConfirmDialog("确定要删除此存档吗？", [this, slotIndex]() {
        auto saveManager = SaveManager::getInstance();
        if (saveManager->deleteSave(slotIndex))
        {
            CCLOG("SaveMenuLayer - 删除成功槽位 %d", slotIndex);
            // 刷新界面
            this->removeFromParent();
        }
        else
        {
            CCLOG("SaveMenuLayer - 删除失败");
        }
    });
}

void SaveMenuLayer::showConfirmDialog(const std::string &message, const std::function<void()> &onConfirm)
{
    // 创建半透明背景
    auto dialogBg = LayerColor::create(Color4B(0, 0, 0, 150));
    this->addChild(dialogBg, 100);

    // 创建对话框背景
    auto dialogBox = LayerColor::create(Color4B(40, 40, 40, 255), 400, 200);
    dialogBox->setPosition(Vec2(
        (Director::getInstance()->getVisibleSize().width - 400) / 2,
        (Director::getInstance()->getVisibleSize().height - 200) / 2));
    dialogBg->addChild(dialogBox);

    // 消息文本
    auto messageLabel = Label::createWithTTF(message, "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24);
    messageLabel->setPosition(Vec2(200, 120));
    dialogBox->addChild(messageLabel);

    // 确认按钮
    // 注意：必须先移除 dialogBg，再调用 onConfirm()
    // 因为 onConfirm() 可能会调用 this->removeFromParent() 销毁整个 SaveMenuLayer
    auto confirmItem = MenuItemLabel::create(
        Label::createWithTTF("确认", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [dialogBg, onConfirm](Ref *) {
            dialogBg->removeFromParent();
            onConfirm();
        });
    confirmItem->setPosition(Vec2(120, 50));

    // 取消按钮
    auto cancelItem = MenuItemLabel::create(
        Label::createWithTTF("取消", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [dialogBg](Ref *) {
            dialogBg->removeFromParent();
        });
    cancelItem->setPosition(Vec2(280, 50));

    auto menu = Menu::create(confirmItem, cancelItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    dialogBox->addChild(menu);
}

void SaveMenuLayer::onClose(Ref *)
{
    this->removeFromParent();
}

std::string SaveMenuLayer::formatTimestamp(int64_t timestamp) const
{
    if (timestamp == 0)
        return "";

    time_t time = timestamp / 1000; // 转换为秒
    struct tm *timeinfo = localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(timeinfo, "%Y-%m-%d %H:%M");
    return oss.str();
}
