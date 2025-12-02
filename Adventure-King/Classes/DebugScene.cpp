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
#include <algorithm>

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
    initPlatforms();
    initPlayer();
    initDebugUI();
    initControlButtons();

    // 启用键盘事件
    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = CC_CALLBACK_2(DebugScene::onKeyPressed, this);
    keyboardListener->onKeyReleased = CC_CALLBACK_2(DebugScene::onKeyReleased, this);
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

void DebugScene::initPlatforms()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto platformDraw = DrawNode::create();
    platformDraw->setTag(200); // 平台绘制节点标签

    // 地面平台 (底部)
    Rect groundPlatform(origin.x, origin.y + GROUND_Y - 20, visibleSize.width, 20);
    _platforms.push_back(groundPlatform);
    platformDraw->drawSolidRect(
        Vec2(groundPlatform.origin.x, groundPlatform.origin.y),
        Vec2(groundPlatform.origin.x + groundPlatform.size.width,
             groundPlatform.origin.y + groundPlatform.size.height),
        Color4F(0.4f, 0.3f, 0.2f, 1.0f));

    // 左侧平台
    Rect leftPlatform(origin.x + 50, origin.y + 200, 200, 20);
    _platforms.push_back(leftPlatform);
    platformDraw->drawSolidRect(
        Vec2(leftPlatform.origin.x, leftPlatform.origin.y),
        Vec2(leftPlatform.origin.x + leftPlatform.size.width,
             leftPlatform.origin.y + leftPlatform.size.height),
        Color4F(0.5f, 0.4f, 0.3f, 1.0f));

    // 中间平台
    Rect middlePlatform(origin.x + visibleSize.width / 2 - 100, origin.y + 300, 200, 20);
    _platforms.push_back(middlePlatform);
    platformDraw->drawSolidRect(
        Vec2(middlePlatform.origin.x, middlePlatform.origin.y),
        Vec2(middlePlatform.origin.x + middlePlatform.size.width,
             middlePlatform.origin.y + middlePlatform.size.height),
        Color4F(0.5f, 0.4f, 0.3f, 1.0f));

    // 右侧平台
    Rect rightPlatform(origin.x + visibleSize.width - 250, origin.y + 200, 200, 20);
    _platforms.push_back(rightPlatform);
    platformDraw->drawSolidRect(
        Vec2(rightPlatform.origin.x, rightPlatform.origin.y),
        Vec2(rightPlatform.origin.x + rightPlatform.size.width,
             rightPlatform.origin.y + rightPlatform.size.height),
        Color4F(0.5f, 0.4f, 0.3f, 1.0f));

    // 高层平台
    Rect topPlatform(origin.x + visibleSize.width / 2 - 75, origin.y + 420, 150, 20);
    _platforms.push_back(topPlatform);
    platformDraw->drawSolidRect(
        Vec2(topPlatform.origin.x, topPlatform.origin.y),
        Vec2(topPlatform.origin.x + topPlatform.size.width,
             topPlatform.origin.y + topPlatform.size.height),
        Color4F(0.6f, 0.5f, 0.4f, 1.0f));

    this->addChild(platformDraw, 1);

    CCLOG("Platforms initialized: %zu platforms", _platforms.size());
}

void DebugScene::initPlayer()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 加载精灵帧缓存（如果需要的话）
    auto cache = SpriteFrameCache::getInstance(); // 角色初始位置在地面上
    Vec2 startPos(origin.x + visibleSize.width / 2 + getContentSize().width / 2, origin.y + GROUND_Y + getContentSize().height / 2);
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
        placeholder->setPosition(startPos);
        this->addChild(placeholder, 5);

        // 添加提示标签
        auto label = Label::createWithTTF("玩家占位符\n(需要精灵帧)", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
        label->setPosition(startPos + Vec2(0, 60));
        this->addChild(label, 6);

        return;
    }

    _player->setPosition(startPos);
    _player->setAnchorPoint(Vec2(0.5f, 0)); // 锚点设置在脚底中心
    _player->setScale(1.0f);
    _isGrounded = true; // 初始在地面上
    this->addChild(_player, 5);

    // 初始化碰撞箱 (相对于角色位置，锚点在脚底)
    // 碰撞箱：宽30，高60，从脚底向上
    _collisionBox = Rect(-15, 0, 30, 60);

    // 创建碰撞箱可视化（调试用）
    _collisionBoxDebug = DrawNode::create();
    _collisionBoxDebug->setTag(300);
    this->addChild(_collisionBoxDebug, 10);

    CCLOG("Player created at position (%.0f, %.0f)", startPos.x, startPos.y);
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
    _hpLabel = Label::createWithTTF("HP: 0/0", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
    _hpLabel->setAnchorPoint(Vec2(1, 0.5f));
    _hpLabel->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2 - 10, barY + 15));
    _hpLabel->setColor(Color3B::RED);
    this->addChild(_hpLabel, 10);

    // HP 进度条背景
    auto hpBg = DrawNode::create();
    hpBg->drawSolidRect(Vec2(0, 0), Vec2(barWidth, 20), Color4F(0.3f, 0.1f, 0.1f, 1.0f));
    hpBg->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2, barY + 15));
    this->addChild(hpBg, 9);

    // HP 进度条（使用 DrawNode 模拟）
    auto hpFill = DrawNode::create();
    hpFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth, 20), Color4F::RED);
    hpFill->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2, barY + 15));
    hpFill->setTag(100); // 用于后续更新
    this->addChild(hpFill, 10);

    // MP 标签
    _mpLabel = Label::createWithTTF("MP: 0/0", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
    _mpLabel->setAnchorPoint(Vec2(1, 0.5f));
    _mpLabel->setPosition(Vec2(origin.x + visibleSize.width / 2 - barWidth / 2 - 10, barY - 15));
    _mpLabel->setColor(Color3B::BLUE);
    this->addChild(_mpLabel, 10);

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
        "[AD] 移动  [W/Space] 跳跃  [1] 受击  [2] 暴击  [3] 治疗  [4] 攻击  [5] 升级  [R] 重置  [ESC] 返回",
        "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    hintLabel->setPosition(Vec2(centerX, origin.y + 70));
    hintLabel->setColor(Color3B(150, 150, 150));
    this->addChild(hintLabel, 10);
}

void DebugScene::update(float dt)
{
    Scene::update(dt);
    updateGravity(dt);
    updatePlayerMovement(dt);
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
        float currentHP = _player->getCurrentHP();
        float currentMP = _player->getCurrentMP();

        // 确保HP和MP不超过最大值，且不低于0
        currentHP = std::max(0.0f, std::min(currentHP, maxHP));
        currentMP = std::max(0.0f, std::min(currentMP, maxMP));

        float hpPercent = maxHP > 0 ? currentHP / maxHP : 0;
        float mpPercent = maxMP > 0 ? currentMP / maxMP : 0;

        // 更新 HP 标签
        if (_hpLabel)
        {
            _hpLabel->setString(StringUtils::format("HP: %.0f/%.0f",
                                                    currentHP, maxHP));
        }

        // 更新 MP 标签
        if (_mpLabel)
        {
            _mpLabel->setString(StringUtils::format("MP: %.0f/%.0f",
                                                    currentMP, maxMP));
        }

        // 更新 HP 条宽度
        auto hpFill = dynamic_cast<DrawNode *>(this->getChildByTag(100));
        if (hpFill)
        {
            hpFill->clear();
            float hpBarWidth = 200.0f * hpPercent;
            hpFill->drawSolidRect(Vec2(0, 0), Vec2(hpBarWidth, 20), Color4F::RED);
        }

        // 更新 MP 条宽度
        auto mpFill = dynamic_cast<DrawNode *>(this->getChildByTag(101));
        if (mpFill)
        {
            mpFill->clear();
            float mpBarWidth = 200.0f * mpPercent;
            mpFill->drawSolidRect(Vec2(0, 0), Vec2(mpBarWidth, 20), Color4F::BLUE);
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

    // 如果正在攻击中，禁用攻击
    if (_isAttacking)
    {
        return;
    }

    _player->attack();
    playAttackAnimation();
    addDamageLog("执行攻击动作 (3连击)");

    CCLOG("Player attack started");
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
    // 移动按键
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _isMovingLeft = true;
        if (_player)
            _player->setFlippedX(true); // 向左翻转
        startWalkAnimation();
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = true;
        if (_player)
            _player->setFlippedX(false); // 向右不翻转
        startWalkAnimation();
        break;
    // 跳跃按键
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
    case EventKeyboard::KeyCode::KEY_SPACE:
        jump();
        break;
    // 功能按键
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

void DebugScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _isMovingLeft = false;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = false;
        break;
    default:
        break;
    }

    // 如果没有任何方向键按下，停止动画
    if (!_isMovingLeft && !_isMovingRight)
    {
        stopWalkAnimation();
    }
}

void DebugScene::updatePlayerMovement(float dt)
{
    if (!_player || _player->isDead())
        return;

    // 只处理水平移动，垂直由重力系统处理
    float velocityX = 0;

    if (_isMovingLeft)
        velocityX -= 1;
    if (_isMovingRight)
        velocityX += 1;

    if (velocityX != 0)
    {
        float moveX = velocityX * _moveSpeed * dt;
        Vec2 newPos = _player->getPosition() + Vec2(moveX, 0);

        // 限制在屏幕范围内
        auto visibleSize = Director::getInstance()->getVisibleSize();
        auto origin = Director::getInstance()->getVisibleOrigin();
        float margin = 30.0f;

        newPos.x = std::max(origin.x + margin, std::min(newPos.x, origin.x + visibleSize.width - margin));

        _player->setPosition(newPos);
    }
}

void DebugScene::startWalkAnimation()
{
    if (!_player || _isWalkAnimationPlaying)
        return;

    _isWalkAnimationPlaying = true;

    // 创建行走动画帧 - 使用Texture自动获取尺寸
    Vector<SpriteFrame *> frames;
    auto textureCache = Director::getInstance()->getTextureCache();

    auto texture1 = textureCache->addImage("Sprites/Characters/Player/Klee/spr_klee_run_1.png");
    auto texture2 = textureCache->addImage("Sprites/Characters/Player/Klee/spr_klee_run_2.png");
    auto texture3 = textureCache->addImage("Sprites/Characters/Player/Klee/spr_klee_run.png");

    if (texture1 && texture2 && texture3)
    {
        Size size1 = texture1->getContentSize();
        Size size2 = texture2->getContentSize();
        Size size3 = texture3->getContentSize();

        auto frame1 = SpriteFrame::createWithTexture(texture1, Rect(0, 0, size1.width, size1.height));
        auto frame2 = SpriteFrame::createWithTexture(texture2, Rect(0, 0, size2.width, size2.height));
        auto frame3 = SpriteFrame::createWithTexture(texture3, Rect(0, 0, size3.width, size3.height));

        if (frame1 && frame2 && frame3)
        {
            frames.pushBack(frame1);
            frames.pushBack(frame2);
            frames.pushBack(frame3);

            // 创建动画，每帧0.15秒
            auto animation = Animation::createWithSpriteFrames(frames, 0.15f);
            auto animate = Animate::create(animation);
            auto repeatAnimate = RepeatForever::create(animate);
            repeatAnimate->setTag(999); // 设置标签以便后续停止

            _player->runAction(repeatAnimate);
            CCLOG("Walk animation started (frame size: %.0fx%.0f)", size1.width, size1.height);
        }
        else
        {
            CCLOG("Failed to create sprite frames");
            _isWalkAnimationPlaying = false;
        }
    }
    else
    {
        CCLOG("Failed to load walk animation textures");
        _isWalkAnimationPlaying = false;
    }
}

void DebugScene::stopWalkAnimation()
{
    if (!_player || !_isWalkAnimationPlaying)
        return;

    _isWalkAnimationPlaying = false;

    // 停止行走动画
    _player->stopActionByTag(999);

    // 恢复到默认静止图片
    auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_run.png");
    if (defaultTexture)
    {
        _player->setTexture(defaultTexture);
    }

    CCLOG("Walk animation stopped");
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

void DebugScene::playAttackAnimation()
{
    if (!_player)
        return;

    // 停止行走动画（如果在播放）
    if (_isWalkAnimationPlaying)
    {
        stopWalkAnimation();
    }

    _isAttacking = true;

    // 加载3张攻击图片
    auto texture1 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_1.png");
    auto texture2 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_2.png");
    auto texture3 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_3.png");

    if (texture1 && texture2 && texture3)
    {
        // 创建精灵帧
        auto frame1 = SpriteFrame::createWithTexture(texture1,
                                                     Rect(0, 0, texture1->getContentSize().width, texture1->getContentSize().height));
        auto frame2 = SpriteFrame::createWithTexture(texture2,
                                                     Rect(0, 0, texture2->getContentSize().width, texture2->getContentSize().height));
        auto frame3 = SpriteFrame::createWithTexture(texture3,
                                                     Rect(0, 0, texture3->getContentSize().width, texture3->getContentSize().height));

        // 创建动画帧序列
        Vector<SpriteFrame *> frames;
        frames.pushBack(frame1);
        frames.pushBack(frame2);
        frames.pushBack(frame3);

        // 每帧0.15秒，共0.45秒
        auto animation = Animation::createWithSpriteFrames(frames, 0.15f);
        auto animate = Animate::create(animation);

        // 停止之前的攻击动画
        _player->stopActionByTag(1000);

        // 创建动画序列：播放动画 -> 回调结束
        auto callbackAction = CallFunc::create([this]()
                                               { this->onAttackAnimationFinished(); });
        auto sequence = Sequence::create(animate, callbackAction, nullptr);
        sequence->setTag(1000);

        _player->runAction(sequence);

        CCLOG("Attack animation started (3-hit combo)");
    }
    else
    {
        CCLOG("Failed to load attack sprites");
        _isAttacking = false;
    }
}

void DebugScene::onAttackAnimationFinished()
{
    _isAttacking = false;

    // 恢复到默认静止图片
    auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_run.png");
    if (defaultTexture && _player)
    {
        _player->setTexture(defaultTexture);
    }

    CCLOG("Attack animation finished");
}

void DebugScene::updateGravity(float dt)
{
    if (!_player || _player->isDead())
        return;

    // 应用重力
    _velocityY += GRAVITY * dt;

    // 更新位置
    Vec2 pos = _player->getPosition();
    float previousY = pos.y;
    pos.y += _velocityY * dt;

    // 获取世界坐标下的碰撞箱
    // 角色锚点在脚底中心 (0.5, 0)，所以碰撞箱直接加上角色位置
    Rect worldCollisionBox(
        pos.x + _collisionBox.origin.x,
        pos.y + _collisionBox.origin.y,
        _collisionBox.size.width,
        _collisionBox.size.height);

    _isGrounded = false;

    for (const auto &platform : _platforms)
    {
        // 只有向下移动时才检测地面碰撞
        if (_velocityY <= 0)
        {
            // 使用碰撞箱检测水平范围是否重叠
            float playerLeft = worldCollisionBox.origin.x;
            float playerRight = worldCollisionBox.origin.x + worldCollisionBox.size.width;
            float platformLeft = platform.origin.x;
            float platformRight = platform.origin.x + platform.size.width;

            bool horizontalOverlap = playerRight > platformLeft && playerLeft < platformRight;

            if (horizontalOverlap)
            {
                float platformTop = platform.origin.y + platform.size.height;
                float playerBottom = worldCollisionBox.origin.y; // 碰撞箱底部（脚底）

                // 如果之前脚底在平台上方，现在穿过了平台顶部
                if (previousY >= platformTop && playerBottom < platformTop)
                {
                    pos.y = platformTop; // 角色脚底对齐平台顶部
                    _velocityY = 0;
                    _isGrounded = true;
                    break;
                }
                // 或者脚底正好在平台顶部附近
                else if (playerBottom >= platformTop - 5 && playerBottom <= platformTop + 5 && _velocityY <= 0)
                {
                    pos.y = platformTop;
                    _velocityY = 0;
                    _isGrounded = true;
                    break;
                }
            }
        }
    }

    // 限制最低位置（防止掉出屏幕）
    auto origin = Director::getInstance()->getVisibleOrigin();
    if (pos.y < origin.y + 50)
    {
        pos.y = origin.y + 50;
        _velocityY = 0;
        _isGrounded = true;
    }

    _player->setPosition(pos);

    // 更新碰撞箱可视化（调试用）
    if (_collisionBoxDebug)
    {
        _collisionBoxDebug->clear();

        // 绘制碰撞箱边框
        Rect debugBox(
            pos.x + _collisionBox.origin.x,
            pos.y + _collisionBox.origin.y,
            _collisionBox.size.width,
            _collisionBox.size.height);

        Color4F boxColor = _isGrounded ? Color4F(0, 1, 0, 0.5f) : Color4F(1, 0, 0, 0.5f);
        _collisionBoxDebug->drawRect(
            Vec2(debugBox.origin.x, debugBox.origin.y),
            Vec2(debugBox.origin.x + debugBox.size.width, debugBox.origin.y + debugBox.size.height),
            boxColor);

        // 绘制脚底点
        _collisionBoxDebug->drawDot(pos, 3, Color4F::YELLOW);
    }
}

bool DebugScene::checkGrounded()
{
    return _isGrounded;
}

void DebugScene::jump()
{
    if (!_player || _player->isDead())
        return;

    // 只有在地面上才能跳跃
    if (_isGrounded)
    {
        _velocityY = JUMP_FORCE;
        _isGrounded = false;
        addDamageLog("跳跃!");
        CCLOG("Player jumped");
    }
}
