#include "SaveMenuLayer.h"
#include "Configs/GameConfig.h"
#include "Configs/PlayerRoleConfig.h"
#include "Save/SaveManager.h"
#include "Save/Cloud/CloudSyncService.h"
#include "Character/Player/PlayerCharacter.h"
#include "Scenes/GameScene.h"
#include <cmath>
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

    if (!initCloudControls())
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

void SaveMenuLayer::onExit()
{
    // 说明：关闭存档菜单后，可能需要恢复“暂停菜单显示”或保持暂停状态。
    // 由外部（GameUIController）决定具体行为，避免此处与场景逻辑耦合。
    if (_closeCallback)
    {
        _closeCallback();
    }

    Layer::onExit();
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

bool SaveMenuLayer::initCloudControls()
{
    _cloudStatusLabel = Label::createWithTTF("", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 20);
    if (!_cloudStatusLabel)
    {
        CCLOG("Error: Failed to create cloud status label!");
        return false;
    }
    _cloudStatusLabel->setAnchorPoint(Vec2(1.0f, 0.5f));
    const float y = _titleLabel ? _titleLabel->getPositionY() : (_background->getContentSize().height / 12 * 11);
    _cloudStatusLabel->setPosition(Vec2(_background->getContentSize().width - 40, y));
    _background->addChild(_cloudStatusLabel);
    refreshCloudStatusLabel();

    return true;
}

void SaveMenuLayer::refreshCloudStatusLabel()
{
    if (!_cloudStatusLabel)
    {
        return;
    }

    auto cloud = CloudSyncService::getInstance();
    const bool guest = cloud->isGuestMode();
    const bool configured = cloud->isConfigured();

    std::string statusText;
    Color4B statusColor;
    if (guest)
    {
        statusText = "云端：游客模式";
        statusColor = Color4B(210, 200, 120, 255);
    }
    else if (configured)
    {
        const std::string user = cloud->getActiveUsername();
        statusText = user.empty() ? "云端：已登录" : ("云端：已登录(" + user + ")");
        statusColor = Color4B(120, 220, 120, 255);
    }
    else
    {
        statusText = "云端：未配置";
        statusColor = Color4B(220, 120, 120, 255);
    }

    _cloudStatusLabel->setString(statusText);
    _cloudStatusLabel->setTextColor(statusColor);
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

    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    _background->addChild(menu, 10);

    const float baseX = _background->getContentSize().width / 2;
    const float baseY = _background->getContentSize().height / 8;

    closeItem->setPosition(Vec2(baseX, baseY));
    closeItem->setScale(0.5f);
    menu->addChild(closeItem);

    // 云同步按钮：两种模式都可用
    auto cloudSyncItem = MenuItemLabel::create(
        Label::createWithTTF("云同步", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [this](Ref *) {
            onCloudSyncClicked();
        });
    cloudSyncItem->setPosition(Vec2(baseX + 220.0f, baseY));
    menu->addChild(cloudSyncItem);

    // 云存按钮：仅保存模式显示（上传本地全部存档包）
    if (_mode == Mode::SAVE)
    {
        auto cloudSaveAllItem = MenuItemLabel::create(
            Label::createWithTTF("云存(全量)", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
            [this](Ref *) {
                auto cloud = CloudSyncService::getInstance();
                std::string hint;
                if (!cloud->isConfigured(&hint))
                {
                    showConfirmDialog(hint, []() {});
                    return;
                }

                showConfirmDialog("将上传本地【全部存档+设置】到云端账号（覆盖云端）。\n建议先保存一次当前进度到某个槽位。", [this, cloud]() {
                    this->retain();
                    cloud->uploadAllSaves([this](bool ok, const std::string &msg) {
                        if (!this->getParent())
                        {
                            CCLOG("SaveMenuLayer - 云存回调到达，但菜单已关闭：%s", msg.c_str());
                            this->release();
                            return;
                        }

                        showConfirmDialog(msg, []() {});
                        if (ok)
                        {
                            reloadSlots();
                        }
                        this->release();
                    });
                });
            });
        cloudSaveAllItem->setPosition(Vec2(baseX - 220.0f, baseY));
        menu->addChild(cloudSaveAllItem);
    }

    return true;
}

void SaveMenuLayer::layoutUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    
    float targetHeight = visibleSize.height * GameSceneConfig::UI::SaveMenu::TARGET_HEIGHT_RATIO;

    float scaleY = targetHeight / _background->getContentSize().height;
    _background->setScale(scaleY);

    // 将背景居中显示
    _background->setPosition(Vec2(
        origin.x + visibleSize.width / 2,
        origin.y + visibleSize.height / 2));

    // 布局存档槽位
    float slotStartY = _background->getContentSize().height*0.72f;
    float slotSpacing = _background->getContentSize().height *0.213f;

    for (size_t i = 0; i < _slotNodes.size(); ++i)
    {
        _slotNodes[i]->setPosition(
            _background->getContentSize().width / 2,
            slotStartY - i * slotSpacing);
    }
}

cocos2d::Node* SaveMenuLayer::createSlotNode(int slotIndex, const SaveSlotData& slotData)
{
    auto node = Node::create();
    node->setContentSize(Size(_background->getContentSize().width * 0.6f, 120));

    // 槽位标题
    std::string slotTitle = "槽位 " + std::to_string(slotIndex + 1);
    auto titleLabel = Label::createWithTTF(
        slotTitle,
        "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
        30
    );
    titleLabel->setAnchorPoint(Vec2(0, 0.5f));
    titleLabel->setPosition(Vec2(-node->getContentSize().width / 2 + 40, 20));
    node->addChild(titleLabel);

    // 是否有存档
    bool hasSave = (slotData.saveTimestamp > 0);

    if (hasSave)
    {
        // 存档信息
        const CharacterRole role =
            static_cast<CharacterRole>(slotData.playerData.role);

        const std::string sceneName =
            slotData.progressData.currentSceneName.empty()
            ? "未知地图"
            : slotData.progressData.currentSceneName;

        int posX = static_cast<int>(std::lround(slotData.progressData.playerPosX));
        int posY = static_cast<int>(std::lround(slotData.progressData.playerPosY));

        std::string infoText = StringUtils::format(
            "%s | %s | Lv.%d | (%d,%d) | %s",
            PlayerRoleConfig::getDisplayName(role),
            sceneName.c_str(),
            slotData.playerData.level,
            posX,
            posY,
            formatTimestamp(slotData.saveTimestamp).c_str()
        );

        auto infoLabel = Label::createWithTTF(
            infoText,
            "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
            18
        );
        infoLabel->setAnchorPoint(Vec2(0, 0.5f));
        infoLabel->setPosition(Vec2(-node->getContentSize().width / 2 + 40, -20));
        infoLabel->setTextColor(Color4B(200, 200, 200, 255));
        node->addChild(infoLabel);
    }
    else
    {
        // 空槽位提示
        auto emptyLabel = Label::createWithTTF(
            "空槽位",
            "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
            20
        );
        emptyLabel->setAnchorPoint(Vec2(0, 0.5f));
        emptyLabel->setPosition(Vec2(-node->getContentSize().width / 2 + 40, -30));
        emptyLabel->setTextColor(Color4B(150, 150, 150, 255));
        node->addChild(emptyLabel);
    }

    // ================= 右侧按钮组 =================
    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    node->addChild(menu);

    float x = node->getContentSize().width / 2 - 30.0f;

    // 云存 / 云读
    std::string cloudText = (_mode == Mode::SAVE) ? "云存" : "云读";
    auto cloudItem = MenuItemLabel::create(
        Label::createWithTTF(
            cloudText,
            "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
            20
        ),
        [this, slotIndex](Ref*)
        {
            onCloudClicked(slotIndex);
        }
    );
    cloudItem->setPosition(Vec2(x, 0));
    menu->addChild(cloudItem);
    x -= 70.0f;

    // 保存 / 加载
    std::string buttonText = (_mode == Mode::SAVE) ? "保存" : "加载";
    auto mainItem = MenuItemLabel::create(
        Label::createWithTTF(
            buttonText,
            "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
            24
        ),
        [this, slotIndex](Ref*)
        {
            onSlotClicked(slotIndex);
        }
    );
    mainItem->setPosition(Vec2(x, 0));
    menu->addChild(mainItem);
    x -= 90.0f;

    // 删除（仅有存档时）
    if (hasSave)
    {
        auto deleteItem = MenuItemLabel::create(
            Label::createWithTTF(
                "删除",
                "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
                20
            ),
            [this, slotIndex](Ref*)
            {
                onDeleteClicked(slotIndex);
            }
        );
        deleteItem->setPosition(Vec2(x, 0));
        deleteItem->setColor(Color3B::RED);
        menu->addChild(deleteItem);
    }

    // 加载模式 + 空槽位 → 禁用“加载”
    if (_mode == Mode::LOAD && !hasSave)
    {
        mainItem->setEnabled(false);
        mainItem->setColor(Color3B(100, 100, 100));
    }

    return node;
}


void SaveMenuLayer::onSlotClicked(int slotIndex)
{
    auto saveManager = SaveManager::getInstance();

    if (_mode == Mode::SAVE)
    {
        // 构建完整进度数据：包含刷怪点/竞技场/怪物快照（用于读档恢复）
        GameProgressSaveData progressData;
        if (auto gameScene = dynamic_cast<GameScene *>(this->getScene()))
        {
            gameScene->fillProgressDataForSave(progressData);
        }
        else
        {
            // 兜底：至少写入场景名与位置
            progressData.currentSceneName = _sceneName;
            progressData.playerPosX = _playerPos.x;
            progressData.playerPosY = _playerPos.y;
        }

        // 保存模式
        if (saveManager->hasSave(slotIndex))
        {
            // 槽位已有存档，显示确认对话框
            // 注意：存档菜单打开期间世界应处于暂停状态，此处直接使用点击时采集的快照即可，
            // 避免重复采集逻辑导致维护困难。
            const GameProgressSaveData snapshot = progressData;
            showConfirmDialog("确定要覆盖此存档吗？", [this, saveManager, slotIndex, snapshot]() {
                if (saveManager->saveGame(slotIndex, _player, snapshot))
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
            if (saveManager->saveGame(slotIndex, _player, progressData))
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

void SaveMenuLayer::onCloudClicked(int slotIndex)
{
    auto cloud = CloudSyncService::getInstance();
    std::string hint;
    if (!cloud->isConfigured(&hint))
    {
        showConfirmDialog(hint, []() {});
        return;
    }

    if (_mode == Mode::SAVE)
    {
        // 云存：先保存该槽位，再上传【全部本地存档包】
        showConfirmDialog("将保存该槽位并上传【全部本地存档+设置】到云端账号（覆盖云端）？", [this, slotIndex, cloud]() {
            // 构建完整进度数据：包含刷怪点/竞技场/怪物快照（用于读档恢复）
            GameProgressSaveData progressData;
            if (auto gameScene = dynamic_cast<GameScene *>(this->getScene()))
            {
                gameScene->fillProgressDataForSave(progressData);
            }
            else
            {
                progressData.currentSceneName = _sceneName;
                progressData.playerPosX = _playerPos.x;
                progressData.playerPosY = _playerPos.y;
            }

            auto saveManager = SaveManager::getInstance();
        if (!saveManager->saveGame(slotIndex, _player, progressData))
        {
            showConfirmDialog("云存失败：本地保存失败", []() {});
            return;
        }

            this->retain();
            cloud->uploadAllSaves([this](bool ok, const std::string &msg) {
                if (!this->getParent())
                {
                    CCLOG("SaveMenuLayer - 云存回调到达，但菜单已关闭：%s", msg.c_str());
                    this->release();
                    return;
                }

                showConfirmDialog(msg, []() {});
                if (ok)
                {
                    reloadSlots();
                }
                this->release();
            });
        });
        return;
    }

    // 云读：云同步 -> 加载该槽位
    showConfirmDialog("将先进行云同步（按时间戳取最新），然后加载该槽位？", [this, slotIndex, cloud]() {
        this->retain();
        cloud->syncAll([this, slotIndex](bool ok, const std::string &msg) {
            if (!this->getParent())
            {
                CCLOG("SaveMenuLayer - 云同步回调到达，但菜单已关闭：%s", msg.c_str());
                this->release();
                return;
            }

            if (!ok)
            {
                showConfirmDialog(msg, []() {});
                this->release();
                return;
            }

            auto saveManager = SaveManager::getInstance();
            SaveSlotData loadedData;
            if (saveManager->loadGame(slotIndex, loadedData))
            {
                CCLOG("SaveMenuLayer - 云同步并加载成功: slot=%d", slotIndex);
                if (_loadSuccessCallback)
                {
                    _loadSuccessCallback(loadedData);
                }
                this->removeFromParent();
                this->release();
                return;
            }

            showConfirmDialog("云同步成功，但加载该槽位失败", []() {});
            this->release();
        });
    });
}

void SaveMenuLayer::onCloudSyncClicked()
{
    auto cloud = CloudSyncService::getInstance();
    std::string hint;
    if (!cloud->isConfigured(&hint))
    {
        showConfirmDialog(hint, []() {});
        return;
    }

    showConfirmDialog("将与云端进行双向同步（按时间戳取最新）？", [this, cloud]() {
        this->retain();
        cloud->syncAll([this](bool ok, const std::string &msg) {
            if (!this->getParent())
            {
                CCLOG("SaveMenuLayer - 云同步回调到达，但菜单已关闭：%s", msg.c_str());
                this->release();
                return;
            }

            showConfirmDialog(msg, []() {});
            if (ok)
            {
                reloadSlots();
            }
            this->release();
        });
    });
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

void SaveMenuLayer::reloadSlots()
{
    for (auto *node : _slotNodes)
    {
        if (node)
        {
            node->removeFromParent();
        }
    }
    _slotNodes.clear();

    initSlots();
    layoutUI();
    refreshCloudStatusLabel();
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
