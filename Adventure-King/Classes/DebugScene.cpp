/**
 * @file DebugScene.cpp
 * @brief 角色功能调试场景实现
 */

#include "DebugScene.h"
#include "Character/PlayerCharacter.h"
#include "Character/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "HomeScene.h"

USING_NS_CC;
using namespace cocos2d::ui;

Scene *DebugScene::createScene()
{
    return DebugScene::create();
}

bool DebugScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    initBackground();
    initPlayer();
    initDebugUI();
    initControlButtons();

    // 启用键盘事件
    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = CC_CALLBACK_2(DebugScene::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    // 启用 update
    scheduleUpdate();

    CCLOG("DebugScene initialized");
    return true;
}

void DebugScene::initBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 创建深灰色背景
    auto background = LayerColor::create(Color4B(40, 40, 50, 255));
    this->addChild(background, -1);

    // 添加网格线帮助定位
    auto drawNode = DrawNode::create();
    Color4F gridColor(0.2f, 0.2f, 0.3f, 1.0f);

    // 垂直线
    for (float x = origin.x; x < origin.x + visibleSize.width; x += 50)
    {
        drawNode->drawLine(Vec2(x, origin.y), Vec2(x, origin.y + visibleSize.height), gridColor);
    }
    // 水平线
    for (float y = origin.y; y < origin.y + visibleSize.height; y += 50)
    {
        drawNode->drawLine(Vec2(origin.x, y), Vec2(origin.x + visibleSize.width, y), gridColor);
    }
    this->addChild(drawNode, 0);

    // 场景标题
    auto titleLabel = Label::createWithTTF("角色调试场景", "fonts/ZCOOLKuaiLe-Regular.ttf", 36);
    titleLabel->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height - 30));
    titleLabel->setColor(Color3B::WHITE);
    this->addChild(titleLabel, 10);
}

void DebugScene::initPlayer()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // 加载精灵帧缓存（如果需要的话）
    auto cache = SpriteFrameCache::getInstance();

    // 尝试创建玩家角色
    // 先尝试用精灵帧，如果失败则用普通精灵创建
    _player = PlayerCharacter::create(CharacterRole::WARRIOR, "Sprites/Characters/Player/Klee/spr_klee_run.png");

    if (!_player)
    {
        // 如果无法加载图片，创建一个简单的占位精灵
        CCLOG("Failed to create player with sprite, creating placeholder");

        // 创建一个简单的占位符
        auto placeholder = DrawNode::create();
        placeholder->drawSolidRect(Vec2(-25, -40), Vec2(25, 40), Color4F::GREEN);
        placeholder->setPosition(center);
        this->addChild(placeholder, 5);

        // 添加提示标签
        auto label = Label::createWithTTF("玩家占位符\n(需要精灵帧)", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
        label->setPosition(center + Vec2(0, 60));
        this->addChild(label, 6);

        return;
    }

    _player->setPosition(center);
    _player->setScale(2.0f); // 放大以便观察
    this->addChild(_player, 5);

    CCLOG("Player created at position (%.0f, %.0f)", center.x, center.y);
}

void DebugScene::initDebugUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // ========== 左侧：属性信息面板 ==========
    float panelX = origin.x + 20;
    float panelY = origin.y + visibleSize.height - 80;

    // 属性信息标签
    _infoLabel = Label::createWithTTF("加载中...", "fonts/ZCOOLKuaiLe-Regular.ttf", 18);
    _infoLabel->setAnchorPoint(Vec2(0, 1));
    _infoLabel->setPosition(Vec2(panelX, panelY));
    _infoLabel->setColor(Color3B::WHITE);
    this->addChild(_infoLabel, 10);

    // ========== 右侧：状态信息面板 ==========
    float rightPanelX = origin.x + visibleSize.width - 250;

    // 状态标签
    _stateLabel = Label::createWithTTF("状态: 未知", "fonts/ZCOOLKuaiLe-Regular.ttf", 20);
    _stateLabel->setAnchorPoint(Vec2(0, 1));
    _stateLabel->setPosition(Vec2(rightPanelX, panelY));
    _stateLabel->setColor(Color3B::YELLOW);
    this->addChild(_stateLabel, 10);

    // 伤害日志标签
    _damageLogLabel = Label::createWithTTF("--- 伤害日志 ---", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _damageLogLabel->setAnchorPoint(Vec2(0, 1));
    _damageLogLabel->setPosition(Vec2(rightPanelX, panelY - 80));
    _damageLogLabel->setColor(Color3B(200, 200, 200));
    this->addChild(_damageLogLabel, 10);

    // ========== HP/MP 进度条 ==========
    float barWidth = 200.0f;
    float barY = origin.y + 100;

    // HP 标签
    auto hpLabel = Label::createWithTTF("HP:", "fonts/ZCOOLKuaiLe-Regular.ttf", 18);
    hpLabel->setAnchorPoint(Vec2(1, 0.5f));
    hpLabel->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2 - 10, barY + 25));
    hpLabel->setColor(Color3B::RED);
    this->addChild(hpLabel, 10);

    // HP 进度条背景
    auto hpBg = DrawNode::create();
    hpBg->drawSolidRect(Vec2(0, 0), Vec2(barWidth, 20), Color4F(0.3f, 0.1f, 0.1f, 1.0f));
    hpBg->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2, barY + 15));
    this->addChild(hpBg, 9);

    // HP 进度条（使用 DrawNode 模拟）
    _hpBar = LoadingBar::create("Scene/UI/hp_bar.png");
    if (!_hpBar)
    {
        // 如果没有资源，创建一个简单的进度条
        auto hpFill = DrawNode::create();
        hpFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth, 20), Color4F::RED);
        hpFill->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2, barY + 15));
        hpFill->setTag(100); // 用于后续更新
        this->addChild(hpFill, 10);
    }

    // MP 标签
    auto mpLabel = Label::createWithTTF("MP:", "fonts/ZCOOLKuaiLe-Regular.ttf", 18);
    mpLabel->setAnchorPoint(Vec2(1, 0.5f));
    mpLabel->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2 - 10, barY - 5));
    mpLabel->setColor(Color3B::BLUE);
    this->addChild(mpLabel, 10);

    // MP 进度条背景
    auto mpBg = DrawNode::create();
    mpBg->drawSolidRect(Vec2(0, 0), Vec2(barWidth, 20), Color4F(0.1f, 0.1f, 0.3f, 1.0f));
    mpBg->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2, barY - 15));
    this->addChild(mpBg, 9);

    // MP 进度条
    auto mpFill = DrawNode::create();
    mpFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth, 20), Color4F::BLUE);
    mpFill->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2, barY - 15));
    mpFill->setTag(101); // 用于后续更新
    this->addChild(mpFill, 10);
}

void DebugScene::initControlButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 按钮配置
    struct ButtonInfo
    {
        std::string label;
        std::function<void(Ref *)> callback;
        Color3B color;
    };

    std::vector<ButtonInfo> buttons = {
        {"受击 (10伤害)", CC_CALLBACK_1(DebugScene::onTakeDamageClicked, this), Color3B::RED},
        {"暴击 (25伤害)", CC_CALLBACK_1(DebugScene::onTakeCriticalDamageClicked, this), Color3B::ORANGE},
        {"治疗 (+20HP)", CC_CALLBACK_1(DebugScene::onHealClicked, this), Color3B::GREEN},
        {"攻击", CC_CALLBACK_1(DebugScene::onAttackClicked, this), Color3B::YELLOW},
        {"升级", CC_CALLBACK_1(DebugScene::onLevelUpClicked, this), Color3B::MAGENTA},
        {"重置角色", CC_CALLBACK_1(DebugScene::onResetClicked, this), Color3B::GRAY},
        {"返回主菜单", CC_CALLBACK_1(DebugScene::onBackClicked, this), Color3B::WHITE},
    };

    float buttonWidth = 150.0f;
    float buttonHeight = 40.0f;
    float spacing = 10.0f;
    float startY = origin.y + 40;
    float centerX = origin.x + visibleSize.width / 2;

    // 计算按钮起始X位置（居中排列）
    float totalWidth = buttons.size() * buttonWidth + (buttons.size() - 1) * spacing;
    float startX = centerX - totalWidth / 2 + buttonWidth / 2;

    for (size_t i = 0; i < buttons.size(); ++i)
    {
        const auto &info = buttons[i];

        // 创建按钮背景
        auto button = MenuItemLabel::create(
            Label::createWithTTF(info.label, "fonts/ZCOOLKuaiLe-Regular.ttf", 16),
            info.callback);

        if (button)
        {
            button->setColor(info.color);
            button->setPosition(Vec2(startX + i * (buttonWidth + spacing), startY));
        }

        auto menu = Menu::create(button, nullptr);
        menu->setPosition(Vec2::ZERO);
        this->addChild(menu, 20);
    }

    // 添加快捷键提示
    auto hintLabel = Label::createWithTTF(
        "[1] 受击  [2] 暴击  [3] 治疗  [4] 攻击  [5] 升级  [R] 重置  [ESC] 返回",
        "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    hintLabel->setPosition(Vec2(centerX, origin.y + 70));
    hintLabel->setColor(Color3B(150, 150, 150));
    this->addChild(hintLabel, 10);
}

void DebugScene::update(float dt)
{
    Scene::update(dt);
    updateDebugInfo();
}

void DebugScene::updateDebugInfo()
{
    if (!_player)
        return;

    auto attr = _player->getAttributeComponent();
    auto sm = _player->getStateMachineComponent();

    // 更新属性信息
    std::string info;
    info += "=== 角色属性 ===\n";
    info += StringUtils::format("等级: %d\n", _player->getLevel());
    info += StringUtils::format("经验: %d\n", _player->getExperience());
    info += StringUtils::format("HP: %.0f / %.0f\n", _player->getCurrentHP(),
                                attr ? attr->getAttributeValue(AttributeType::MAX_HP) : 0.0f);
    info += StringUtils::format("MP: %.0f / %.0f\n", _player->getCurrentMP(),
                                attr ? attr->getAttributeValue(AttributeType::MAX_MP) : 0.0f);

    if (attr)
    {
        info += "\n=== 战斗属性 ===\n";
        info += StringUtils::format("力量: %.0f\n", attr->getAttributeValue(AttributeType::STRENGTH));
        info += StringUtils::format("防御: %.0f\n", attr->getAttributeValue(AttributeType::DEFENSE));
        info += StringUtils::format("暴击率: %.0f%%\n", attr->getAttributeValue(AttributeType::CRITICAL_RATE) * 100);
        info += StringUtils::format("移速: %.0f\n", attr->getAttributeValue(AttributeType::MOVE_SPEED));
    }

    _infoLabel->setString(info);

    // 更新状态信息
    if (sm)
    {
        std::string stateStr;
        switch (sm->getCurrentState())
        {
        case CharacterState::IDLE:
            stateStr = "待机 (IDLE)";
            break;
        case CharacterState::WALKING:
            stateStr = "行走 (WALKING)";
            break;
        case CharacterState::RUNNING:
            stateStr = "跑步 (RUNNING)";
            break;
        case CharacterState::JUMPING:
            stateStr = "跳跃 (JUMPING)";
            break;
        case CharacterState::DOUBLE_JUMPING:
            stateStr = "二段跳 (DOUBLE_JUMPING)";
            break;
        case CharacterState::ATTACKING:
            stateStr = "攻击 (ATTACKING)";
            break;
        case CharacterState::CLIMBING:
            stateStr = "攀爬 (CLIMBING)";
            break;
        case CharacterState::HURT:
            stateStr = "受伤 (HURT)";
            break;
        case CharacterState::DEAD:
            stateStr = "死亡 (DEAD)";
            break;
        default:
            stateStr = "未知";
            break;
        }
        _stateLabel->setString("状态: " + stateStr);

        // 根据状态改变颜色
        if (sm->getCurrentState() == CharacterState::DEAD)
            _stateLabel->setColor(Color3B::RED);
        else if (sm->getCurrentState() == CharacterState::HURT)
            _stateLabel->setColor(Color3B::ORANGE);
        else if (sm->getCurrentState() == CharacterState::ATTACKING)
            _stateLabel->setColor(Color3B::YELLOW);
        else
            _stateLabel->setColor(Color3B::GREEN);
    }

    // 更新 HP/MP 进度条
    if (attr)
    {
        float maxHP = attr->getAttributeValue(AttributeType::MAX_HP);
        float maxMP = attr->getAttributeValue(AttributeType::MAX_MP);
        float hpPercent = maxHP > 0 ? _player->getCurrentHP() / maxHP : 0;
        float mpPercent = maxMP > 0 ? _player->getCurrentMP() / maxMP : 0;

        // 更新 HP 条宽度
        auto hpFill = dynamic_cast<DrawNode *>(this->getChildByTag(100));
        if (hpFill)
        {
            hpFill->clear();
            hpFill->drawSolidRect(Vec2(0, 0), Vec2(200.0f * hpPercent, 20), Color4F::RED);
        }

        // 更新 MP 条宽度
        auto mpFill = dynamic_cast<DrawNode *>(this->getChildByTag(101));
        if (mpFill)
        {
            mpFill->clear();
            mpFill->drawSolidRect(Vec2(0, 0), Vec2(200.0f * mpPercent, 20), Color4F::BLUE);
        }
    }
}

void DebugScene::onTakeDamageClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    DamageInfo info;
    info.amount = 10.0f;
    info.penetration = 0.0f;
    info.isCritical = false;
    info.attacker = nullptr;

    float hpBefore = _player->getCurrentHP();
    _player->takeDamage(info);
    float hpAfter = _player->getCurrentHP();

    addDamageLog(StringUtils::format("普通伤害: %.0f -> %.0f (-%0.f)",
                                     hpBefore, hpAfter, hpBefore - hpAfter));

    CCLOG("Player took 10 damage, HP: %.0f", _player->getCurrentHP());
}

void DebugScene::onTakeCriticalDamageClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    DamageInfo info;
    info.amount = 25.0f;
    info.penetration = 2.0f; // 穿透2点护甲
    info.isCritical = true;
    info.critMultiplier = 1.5f;
    info.attacker = nullptr;

    float hpBefore = _player->getCurrentHP();
    _player->takeDamage(info);
    float hpAfter = _player->getCurrentHP();

    addDamageLog(StringUtils::format("暴击伤害: %.0f -> %.0f (-%.0f) [穿透:2]",
                                     hpBefore, hpAfter, hpBefore - hpAfter));

    CCLOG("Player took critical damage, HP: %.0f", _player->getCurrentHP());
}

void DebugScene::onHealClicked(Ref *sender)
{
    if (!_player)
        return;

    float hpBefore = _player->getCurrentHP();
    _player->setCurrentHP(hpBefore + 20.0f);
    float hpAfter = _player->getCurrentHP();

    addDamageLog(StringUtils::format("治疗: %.0f -> %.0f (+%.0f)",
                                     hpBefore, hpAfter, hpAfter - hpBefore));

    CCLOG("Player healed, HP: %.0f", _player->getCurrentHP());
}

void DebugScene::onAttackClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    _player->attack();
    addDamageLog("执行攻击动作");

    CCLOG("Player attacked");
}

void DebugScene::onLevelUpClicked(Ref *sender)
{
    if (!_player)
        return;

    _player->addExperience(100 * _player->getLevel());
    addDamageLog(StringUtils::format("升级! 当前等级: %d", _player->getLevel()));

    CCLOG("Player leveled up to %d", _player->getLevel());
}

void DebugScene::onResetClicked(Ref *sender)
{
    // 移除旧玩家并创建新玩家
    if (_player)
    {
        _player->removeFromParent();
        _player = nullptr;
    }

    initPlayer();
    _damageLog.clear();
    addDamageLog("角色已重置");

    CCLOG("Player reset");
}

void DebugScene::onBackClicked(Ref *sender)
{
    auto homeScene = HomeScene::createScene();
    Director::getInstance()->replaceScene(
        TransitionFade::create(0.5f, homeScene, Color3B::BLACK));
}

void DebugScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event *event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_1:
        onTakeDamageClicked(nullptr);
        break;
    case EventKeyboard::KeyCode::KEY_2:
        onTakeCriticalDamageClicked(nullptr);
        break;
    case EventKeyboard::KeyCode::KEY_3:
        onHealClicked(nullptr);
        break;
    case EventKeyboard::KeyCode::KEY_4:
        onAttackClicked(nullptr);
        break;
    case EventKeyboard::KeyCode::KEY_5:
        onLevelUpClicked(nullptr);
        break;
    case EventKeyboard::KeyCode::KEY_R:
        onResetClicked(nullptr);
        break;
    case EventKeyboard::KeyCode::KEY_ESCAPE:
        onBackClicked(nullptr);
        break;
    default:
        break;
    }
}

void DebugScene::addDamageLog(const std::string &log)
{
    _damageLog.push_back(log);

    // 保持日志在最大行数内
    while (_damageLog.size() > MAX_LOG_LINES)
    {
        _damageLog.erase(_damageLog.begin());
    }

    // 更新日志显示
    std::string logText = "--- 伤害日志 ---\n";
    for (const auto &line : _damageLog)
    {
        logText += line + "\n";
    }

    if (_damageLogLabel)
    {
        _damageLogLabel->setString(logText);
    }
}
