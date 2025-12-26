/**
 * @file GameUI.cpp
 * @brief 游戏内 UI 层实现
 */

#include "GameUI.h"
#include "UI/PlayerStatusBar.h"
#include "UI/SkillBar.h"
#include "UI/BossHealthBar.h"
#include "UI/PauseMenu.h"
#include "UI/PlayerDeathMenu.h"
#include "UI/InventoryLayer.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/Base/CharacterBase.h"
#include "Configs/GameSceneConfig.h"

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
    float padding = GameSceneConfig::UI::PADDING;

    // 玩家状态栏位置：左上角
    _statusBarPos = Vec2(origin.x + padding + GameSceneConfig::UI::STATUS_BAR_OFFSET_X,
                         origin.y + visibleSize.height - padding);

    // 技能栏位置：屏幕底部中央偏右
    _skillBarPos = Vec2(origin.x + visibleSize.width - GameSceneConfig::UI::SKILL_BAR_OFFSET_X,
                        origin.y + GameSceneConfig::UI::SKILL_BAR_OFFSET_Y);

    // Boss血条位置：屏幕顶部中央
    _bossHealthBarPos = Vec2(origin.x + visibleSize.width / 2,
                             origin.y + visibleSize.height - GameSceneConfig::UI::BOSS_BAR_OFFSET_Y);

    // 地图按钮位置：右上角
    /*_mapButtonPos = Vec2(origin.x + visibleSize.width  - GameSceneConfig::UI::MAP_BUTTON_OFFSET,
                         origin.y + visibleSize.height  - GameSceneConfig::UI::MAP_BUTTON_OFFSET);*/

    // 交互提示位置：屏幕底部中央
    _interactionHintPos = Vec2(origin.x + visibleSize.width / 2,
                               origin.y + GameSceneConfig::UI::INTERACTION_HINT_OFFSET_Y);

    // 关卡名称位置：右上角（地图按钮下方）
    _levelNamePos = Vec2((origin.x + visibleSize.width)*0.5,
        origin.y + visibleSize.height - GameSceneConfig::UI::LEVEL_NAME_OFFSET_Y);

    // 创建 UI 元素
    createPlayerStatusBar();
    createSkillBar();
    createBossHealthBar();
    createMapButton();
    createInteractionHint();
    createLevelNameLabel();
    createPauseMenu();
    createDeathMenu();
    createInventoryLayer();

    CCLOG("GameUI initialized with all components");
    return true;
}

void GameUI::createPlayerStatusBar()
{
    _playerStatusBar = PlayerStatusBar::create();
    if (_playerStatusBar)
    {
        _playerStatusBar->setPosition(_statusBarPos);
        this->addChild(_playerStatusBar, 10);
    }
}

void GameUI::createSkillBar()
{
    _skillBar = SkillBar::create(GameSceneConfig::UI::SKILL_BAR_SLOT_COUNT);
    if (_skillBar)
    {
        _skillBar->setPosition(_skillBarPos);

        // 设置默认快捷键提示
        _skillBar->setSlotHotkey(0, "E");
        _skillBar->setSlotHotkey(1, "Q");
        _skillBar->setSlotHotkey(2, "R");
        _skillBar->setSlotHotkey(3, "F");

        this->addChild(_skillBar, 10);
    }
}

void GameUI::createBossHealthBar()
{
    _bossHealthBar = BossHealthBar::create();
    if (_bossHealthBar)
    {
        _bossHealthBar->setPosition(_bossHealthBarPos);
        this->addChild(_bossHealthBar, 10);
    }
}

void GameUI::createPauseMenu()
{
    _pauseMenu = PauseMenu::create();
    if (_pauseMenu)
    {
        this->addChild(_pauseMenu, 100); // 最高层级
    }
}

void GameUI::createDeathMenu()
{
    _deathMenu = PlayerDeathMenu::create();
    if (_deathMenu)
    {
        // 高于背包/暂停菜单，避免死亡时仍能操作其它 UI
        this->addChild(_deathMenu, 200);
    }
}

void GameUI::createInventoryLayer()
{
    _inventoryLayer = InventoryLayer::create();
    if (_inventoryLayer)
    {
        this->addChild(_inventoryLayer, 101); // 高于 PauseMenu
    }
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
        _mapButton->setScale(0.3f);
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

void GameUI::bindPlayer(PlayerCharacter *player)
{
    _player = player;

    if (_playerStatusBar)
    {
        _playerStatusBar->bindPlayer(player);
    }

    if (_skillBar)
    {
        _skillBar->bindPlayer(player);

        // 设置技能1图标（火球）
        _skillBar->setSlotIcon(0, "Sprites/Characters/Player/Klee/rocket/spr_vfx_rocket_trail_long_1.png");
    }

    if (_inventoryLayer)
    {
        _inventoryLayer->bindPlayer(player);
    }
}

void GameUI::bindBoss(CharacterBase *boss, const std::string &bossName, int phaseCount)
{
    if (_bossHealthBar)
    {
        _bossHealthBar->bindBoss(boss, bossName, phaseCount);
        _bossHealthBar->show();
    }
}

void GameUI::unbindBoss()
{
    if (_bossHealthBar)
    {
        _bossHealthBar->unbindBoss();
    }
}

void GameUI::showPauseMenu()
{
    if (_pauseMenu)
    {
        _pauseMenu->show();
    }
}

void GameUI::hidePauseMenu()
{
    if (_pauseMenu)
    {
        _pauseMenu->hide();
    }
}

bool GameUI::isPauseMenuShowing() const
{
    return _pauseMenu && _pauseMenu->isShowing();
}

void GameUI::showDeathMenu()
{
    if (_deathMenu)
    {
        _deathMenu->show();
    }
}

void GameUI::hideDeathMenu()
{
    if (_deathMenu)
    {
        _deathMenu->hide();
    }
}

bool GameUI::isDeathMenuShowing() const
{
    return _deathMenu && _deathMenu->isShowing();
}

void GameUI::showInventory()
{
    if (_inventoryLayer)
    {
        _inventoryLayer->show();
    }
}

void GameUI::hideInventory()
{
    if (_inventoryLayer)
    {
        _inventoryLayer->hide();
    }
}

bool GameUI::isInventoryShowing() const
{
    return _inventoryLayer && _inventoryLayer->isShowing();
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

void GameUI::updateDisplay()
{
    // 更新玩家状态栏
    if (_playerStatusBar)
    {
        _playerStatusBar->updateDisplay();
    }

    // 更新技能栏
    if (_skillBar)
    {
        _skillBar->updateDisplay();
    }

    // 更新Boss血条
    if (_bossHealthBar)
    {
        _bossHealthBar->updateDisplay();
    }
}

void GameUI::updatePosition(const Vec2 &cameraOffset)
{
    // 现在 GameUI 直接添加到场景中，而场景不再移动
    // Follow 动作只作用于 _gameLayer，所以 UI 位置不需要更新
    // 保留此方法以保持接口兼容性
}
