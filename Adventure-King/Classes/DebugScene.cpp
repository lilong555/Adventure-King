/**
 * @file DebugScene.cpp
 * @brief 角色功能调试场景实现
 */

#include "DebugScene.h"
#include "Character/PlayerCharacter.h"
#include "Character/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
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
    // 使用物理引擎初始化场景
    if (!Scene::initWithPhysics())
    {
        return false;
    }

    // 设置物理世界属性
    auto physicsWorld = this->getPhysicsWorld();
    physicsWorld->setGravity(Vec2(0, -800.0f));                  // 设置重力
    physicsWorld->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL); // 开启调试绘制（可选）

    initBackground();
    initPlatforms();
    initPlayer();
    initEquipments();    // 初始化装备系统
    initPassiveSkills(); // 初始化被动技能
    initTargetDummy();
    initDebugUI();
    initControlButtons();
    initPhysicsContactListener(); // 初始化物理碰撞监听

    // 启用键盘事件
    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = CC_CALLBACK_2(DebugScene::onKeyPressed, this);
    keyboardListener->onKeyReleased = CC_CALLBACK_2(DebugScene::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    // 启用 update
    scheduleUpdate();

    CCLOG("DebugScene initialized with Physics Engine");
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

    // 物理材质：密度、弹性、摩擦力
    PhysicsMaterial platformMaterial(1.0f, 0.0f, 0.8f);

    // 辅助函数：创建平台节点并添加物理刚体
    auto createPlatform = [&](const Rect &rect, const Color4F &color)
    {
        // 绘制可视化
        platformDraw->drawSolidRect(
            Vec2(rect.origin.x, rect.origin.y),
            Vec2(rect.origin.x + rect.size.width, rect.origin.y + rect.size.height),
            color);

        // 创建一个节点作为平台
        auto platformNode = Node::create();
        platformNode->setPosition(Vec2(rect.origin.x + rect.size.width / 2,
                                       rect.origin.y + rect.size.height / 2));

        // 创建静态物理刚体（EdgeBox 用于边缘检测更稳定）
        auto physicsBody = PhysicsBody::createBox(
            Size(rect.size.width, rect.size.height),
            platformMaterial);
        physicsBody->setDynamic(false); // 静态刚体

        // 设置碰撞掩码
        physicsBody->setCategoryBitmask(CATEGORY_PLATFORM);
        physicsBody->setCollisionBitmask(CATEGORY_PLAYER | CATEGORY_BOMB);
        physicsBody->setContactTestBitmask(CATEGORY_PLAYER | CATEGORY_BOMB);

        platformNode->addComponent(physicsBody);
        this->addChild(platformNode, 1);

        _platforms.push_back(rect);
    };

    // 地面平台 (底部)
    Rect groundPlatform(origin.x, origin.y + GROUND_Y - 20, visibleSize.width, 20);
    createPlatform(groundPlatform, Color4F(0.4f, 0.3f, 0.2f, 1.0f));

    // 左侧平台
    Rect leftPlatform(origin.x + 50, origin.y + 200, 200, 20);
    // createPlatform(leftPlatform, Color4F(0.5f, 0.4f, 0.3f, 1.0f));

    this->addChild(platformDraw, 1);

    CCLOG("Platforms initialized with physics: %zu platforms", _platforms.size());
}

void DebugScene::initPlayer()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 加载精灵帧缓存（如果需要的话）
    auto cache = SpriteFrameCache::getInstance();

    // 角色初始位置在地面上
    Vec2 startPos(origin.x + visibleSize.width / 2, origin.y + GROUND_Y + getContentSize().height / 2);

    // 尝试创建玩家角色
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
    _player->setAnchorPoint(Vec2(0.5f, 0.5f)); // 物理引擎要求锚点在中心
    _player->setScale(1.0f);

    // 创建玩家物理刚体
    // 物理材质：密度、弹性、摩擦力
    // 弹性设为0避免弹跳，摩擦力设为0避免与平台边缘卡住
    PhysicsMaterial playerMaterial(1.0f, 0.0f, 0.0f);

    // 使用简单的矩形碰撞体
    Size playerSize = _player->getContentSize();
    float scale = _player->getScale();
    float boxWidth = playerSize.width * scale * 0.8f;    // 宽度缩小到80%更贴合角色
    float boxHeight = playerSize.height * scale * 0.95f; // 高度缩小到95%

    auto physicsBody = PhysicsBody::createBox(Size(boxWidth, boxHeight), playerMaterial);
    physicsBody->setDynamic(true);         // 动态刚体
    physicsBody->setRotationEnable(false); // 禁止旋转
    physicsBody->setMass(1.0f);            // 设置质量
    physicsBody->setLinearDamping(0.0f);   // 无线性阻尼，确保移动流畅

    // 设置碰撞掩码
    physicsBody->setCategoryBitmask(CATEGORY_PLAYER);
    physicsBody->setCollisionBitmask(CATEGORY_PLATFORM);                   // 只与平台碰撞
    physicsBody->setContactTestBitmask(CATEGORY_PLATFORM | CATEGORY_BOMB); // 检测与平台和炸弹的接触

    _player->addComponent(physicsBody);
    this->addChild(_player, 5);

    _isGrounded = true; // 初始在地面上
    _groundContactCount = 1;

    // 初始化玩家技能
    initPlayerSkills();

    CCLOG("Player created with physics at position (%.0f, %.0f)", startPos.x, startPos.y);
}

void DebugScene::initPlayerSkills()
{
    if (!_player)
        return;

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        CCLOG("Failed to get skill component");
        return;
    }

    // 创建炸弹技能
    auto bombSkill = std::make_shared<ActiveSkill>();
    bombSkill->id = BOMB_SKILL_ID;
    bombSkill->name = "炸弹";
    bombSkill->description = "丢出一个炸弹，造成范围伤害";
    bombSkill->manaCost = BOMB_SKILL_MP_COST;
    bombSkill->cooldown = BOMB_SKILL_COOLDOWN;
    bombSkill->currentCooldown = 0.0f;

    // 学习技能并装备到槽位
    skillComp->learnSkill(bombSkill);
    skillComp->equipActiveSkill(bombSkill, BOMB_SKILL_SLOT);

    CCLOG("Player skills initialized: Bomb skill equipped to slot %zu", BOMB_SKILL_SLOT);
}

void DebugScene::initTargetDummy()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 创建木桩精灵
    _targetDummy.sprite = Sprite::create("Sprites/Enemies/YuanSheMenJiang/ysmj_stand.png");
    if (_targetDummy.sprite)
    {
        // 放置在场景右侧的地面上
        Vec2 dummyPos(origin.x + visibleSize.width * 0.75f, origin.y + GROUND_Y);
        _targetDummy.sprite->setPosition(dummyPos);
        _targetDummy.sprite->setAnchorPoint(Vec2(0.5f, 0)); // 锚点在脚底
        _targetDummy.sprite->setScale(0.5f);
        this->addChild(_targetDummy.sprite, 4);

        // 初始化属性
        _targetDummy.maxHP = 2147483674.0f;
        _targetDummy.currentHP = 2147483674.0f;

        // 创建血条背景
        _targetDummy.hpBar = DrawNode::create();
        this->addChild(_targetDummy.hpBar, 6);

        // 创建HP数值标签
        _targetDummy.hpLabel = Label::createWithTTF("1000/1000", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
        _targetDummy.hpLabel->setPosition(dummyPos + Vec2(0, 90));
        _targetDummy.hpLabel->setColor(Color3B::WHITE);
        this->addChild(_targetDummy.hpLabel, 7);

        updateTargetHPBar();

        CCLOG("Target dummy created at position (%.0f, %.0f)", dummyPos.x, dummyPos.y);
    }
    else
    {
        CCLOG("Failed to create target dummy sprite");
    }
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

    // 状态效果标签
    _statusEffectLabel = Label::createWithTTF("状态效果: 无", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
    _statusEffectLabel->setAnchorPoint(Vec2(0, 1));
    _statusEffectLabel->setPosition(Vec2(rightPanelX, panelY - 30));
    _statusEffectLabel->setColor(Color3B(200, 150, 255));
    this->addChild(_statusEffectLabel, 10);

    // 装备信息标签
    _equipmentLabel = Label::createWithTTF("武器: 无", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
    _equipmentLabel->setAnchorPoint(Vec2(0, 1));
    _equipmentLabel->setPosition(Vec2(rightPanelX, panelY - 55));
    _equipmentLabel->setColor(Color3B(192, 192, 192));
    this->addChild(_equipmentLabel, 10);

    // 被动技能标签
    _passiveSkillLabel = Label::createWithTTF("被动技能: 无", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
    _passiveSkillLabel->setAnchorPoint(Vec2(0, 1));
    _passiveSkillLabel->setPosition(Vec2(rightPanelX, panelY - 80));
    _passiveSkillLabel->setColor(Color3B(100, 200, 100));
    this->addChild(_passiveSkillLabel, 10);

    // 伤害日志标签
    _damageLogLabel = Label::createWithTTF("--- 伤害日志 ---", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _damageLogLabel->setAnchorPoint(Vec2(0, 1));
    _damageLogLabel->setPosition(Vec2(rightPanelX, panelY - 155));
    _damageLogLabel->setColor(Color3B(200, 200, 200));
    this->addChild(_damageLogLabel, 10);

    // ========== 左上角：HP/MP 进度条 ==========
    float barWidth = 200.0f;
    float barHeight = 18.0f;
    float barX = origin.x + 20;
    float barY = origin.y + visibleSize.height - 20;

    // HP 标签
    _hpLabel = Label::createWithTTF("HP: 0/0", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _hpLabel->setAnchorPoint(Vec2(0, 0.5f));
    _hpLabel->setPosition(Vec2(barX + barWidth + 10, barY - barHeight / 2));
    _hpLabel->setColor(Color3B::WHITE);
    this->addChild(_hpLabel, 10);

    // HP 进度条背景
    _hpBarBg = DrawNode::create();
    _hpBarBg->drawSolidRect(Vec2(0, 0), Vec2(barWidth, barHeight), Color4F(0.3f, 0.1f, 0.1f, 0.8f));
    _hpBarBg->setPosition(Vec2(barX, barY - barHeight));
    this->addChild(_hpBarBg, 9);

    // HP 进度条填充
    _hpBarFill = DrawNode::create();
    _hpBarFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth, barHeight), Color4F(0.8f, 0.2f, 0.2f, 1.0f));
    _hpBarFill->setPosition(Vec2(barX, barY - barHeight));
    _hpBarFill->setTag(100);
    this->addChild(_hpBarFill, 10);

    // MP 标签
    _mpLabel = Label::createWithTTF("MP: 0/0", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _mpLabel->setAnchorPoint(Vec2(0, 0.5f));
    _mpLabel->setPosition(Vec2(barX + barWidth + 10, barY - barHeight - 8 - barHeight / 2));
    _mpLabel->setColor(Color3B::WHITE);
    this->addChild(_mpLabel, 10);

    // MP 进度条背景
    _mpBarBg = DrawNode::create();
    _mpBarBg->drawSolidRect(Vec2(0, 0), Vec2(barWidth, barHeight), Color4F(0.1f, 0.1f, 0.3f, 0.8f));
    _mpBarBg->setPosition(Vec2(barX, barY - barHeight * 2 - 8));
    this->addChild(_mpBarBg, 9);

    // MP 进度条填充
    _mpBarFill = DrawNode::create();
    _mpBarFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth, barHeight), Color4F(0.2f, 0.4f, 0.9f, 1.0f));
    _mpBarFill->setPosition(Vec2(barX, barY - barHeight * 2 - 8));
    _mpBarFill->setTag(101);
    this->addChild(_mpBarFill, 10);
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

    // 第一行：基础功能按钮
    std::vector<ButtonInfo> buttons = {
        {"受击 (10伤害)", CC_CALLBACK_1(DebugScene::onTakeDamageClicked, this), Color3B::RED},
        {"暴击 (25伤害)", CC_CALLBACK_1(DebugScene::onTakeCriticalDamageClicked, this), Color3B::ORANGE},
        {"治疗 (+20HP)", CC_CALLBACK_1(DebugScene::onHealClicked, this), Color3B::GREEN},
        {"攻击", CC_CALLBACK_1(DebugScene::onAttackClicked, this), Color3B::YELLOW},
        {"升级", CC_CALLBACK_1(DebugScene::onLevelUpClicked, this), Color3B::MAGENTA},
        {"重置角色", CC_CALLBACK_1(DebugScene::onResetClicked, this), Color3B::GRAY},
        {"返回主菜单", CC_CALLBACK_1(DebugScene::onBackClicked, this), Color3B::WHITE},
    };

    // 第二行：状态效果按钮
    std::vector<ButtonInfo> statusButtons = {
        {"中毒 (5秒)", CC_CALLBACK_1(DebugScene::onPoisonClicked, this), Color3B(148, 0, 211)},   // 紫色
        {"亢奋 (8秒)", CC_CALLBACK_1(DebugScene::onExcitedClicked, this), Color3B(255, 165, 0)},  // 橙色
        {"眩晕 (3秒)", CC_CALLBACK_1(DebugScene::onStunnedClicked, this), Color3B(70, 130, 180)}, // 钢蓝色
    };

    // 第三行：装备按钮
    std::vector<ButtonInfo> equipButtons = {
        {"装备剑", CC_CALLBACK_1(DebugScene::onEquipSwordClicked, this), Color3B(192, 192, 192)},      // 银色
        {"装备法杖", CC_CALLBACK_1(DebugScene::onEquipStaffClicked, this), Color3B(138, 43, 226)},     // 紫罗兰
        {"装备匕首", CC_CALLBACK_1(DebugScene::onEquipDaggerClicked, this), Color3B(50, 205, 50)},     // 酸橙绿
        {"卸下武器", CC_CALLBACK_1(DebugScene::onUnequipWeaponClicked, this), Color3B(128, 128, 128)}, // 灰色
    };

    // 第四行：被动技能按钮
    std::vector<ButtonInfo> passiveButtons = {
        {"力量+5", CC_CALLBACK_1(DebugScene::onLearnPassive1Clicked, this), Color3B(255, 100, 100)}, // 红色
        {"防御+3", CC_CALLBACK_1(DebugScene::onLearnPassive2Clicked, this), Color3B(100, 100, 255)}, // 蓝色
        {"暴击+10%", CC_CALLBACK_1(DebugScene::onLearnPassive3Clicked, this), Color3B(255, 215, 0)}, // 金色
    };

    float buttonWidth = 150.0f;
    float buttonHeight = 40.0f;
    float spacing = 10.0f;
    float startY = origin.y + 40;
    float centerX = origin.x + visibleSize.width / 2;

    // 计算按钮起始X位置（居中排列）- 第一行
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

    // 第二行：状态效果按钮
    float statusStartY = startY + 30;
    float statusTotalWidth = statusButtons.size() * buttonWidth + (statusButtons.size() - 1) * spacing;
    float statusStartX = centerX - statusTotalWidth / 2 + buttonWidth / 2;

    for (size_t i = 0; i < statusButtons.size(); ++i)
    {
        const auto &info = statusButtons[i];

        auto button = MenuItemLabel::create(
            Label::createWithTTF(info.label, "fonts/ZCOOLKuaiLe-Regular.ttf", 16),
            info.callback);

        if (button)
        {
            button->setColor(info.color);
            button->setPosition(Vec2(statusStartX + i * (buttonWidth + spacing), statusStartY));
        }

        auto menu = Menu::create(button, nullptr);
        menu->setPosition(Vec2::ZERO);
        this->addChild(menu, 20);
    }

    // 第三行：装备按钮
    float equipStartY = statusStartY + 30;
    float equipTotalWidth = equipButtons.size() * buttonWidth + (equipButtons.size() - 1) * spacing;
    float equipStartX = centerX - equipTotalWidth / 2 + buttonWidth / 2;

    for (size_t i = 0; i < equipButtons.size(); ++i)
    {
        const auto &info = equipButtons[i];

        auto button = MenuItemLabel::create(
            Label::createWithTTF(info.label, "fonts/ZCOOLKuaiLe-Regular.ttf", 16),
            info.callback);

        if (button)
        {
            button->setColor(info.color);
            button->setPosition(Vec2(equipStartX + i * (buttonWidth + spacing), equipStartY));
        }

        auto menu = Menu::create(button, nullptr);
        menu->setPosition(Vec2::ZERO);
        this->addChild(menu, 20);
    }

    // 第四行：被动技能按钮
    float passiveStartY = equipStartY + 30;
    float passiveTotalWidth = passiveButtons.size() * buttonWidth + (passiveButtons.size() - 1) * spacing;
    float passiveStartX = centerX - passiveTotalWidth / 2 + buttonWidth / 2;

    for (size_t i = 0; i < passiveButtons.size(); ++i)
    {
        const auto &info = passiveButtons[i];

        auto button = MenuItemLabel::create(
            Label::createWithTTF(info.label, "fonts/ZCOOLKuaiLe-Regular.ttf", 16),
            info.callback);

        if (button)
        {
            button->setColor(info.color);
            button->setPosition(Vec2(passiveStartX + i * (buttonWidth + spacing), passiveStartY));
        }

        auto menu = Menu::create(button, nullptr);
        menu->setPosition(Vec2::ZERO);
        this->addChild(menu, 20);
    }

    // 添加快捷键提示
    auto hintLabel = Label::createWithTTF(
        "[AD] 移动  [W/Space] 跳跃  [E] 丢炸弹  [4] 攻击  [R] 重置  [ESC] 返回",
        "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    hintLabel->setPosition(Vec2(centerX, origin.y + 160));
    hintLabel->setColor(Color3B(150, 150, 150));
    this->addChild(hintLabel, 10);
}

void DebugScene::update(float dt)
{
    Scene::update(dt);
    // 物理引擎自动处理重力和碰撞，不需要手写更新
    updatePlayerMovement(dt);
    updateDebugInfo();

    // 更新技能冷却
    if (_player)
    {
        auto skillComp = _player->getSkillComponent();
        if (skillComp)
        {
            skillComp->update(dt);
        }
    }

    // 处理中毒持续伤害
    static float poisonTimer = 0.0f;
    if (_player && !_player->isDead())
    {
        auto attr = _player->getAttributeComponent();
        if (attr)
        {
            // 检查是否仍有中毒效果
            _isPoisoned = attr->hasStatusEffect(StatusEffectType::POISONED);

            if (_isPoisoned)
            {
                poisonTimer += dt;
                if (poisonTimer >= 1.0f) // 每秒造成伤害
                {
                    applyPoisonDamage(dt);
                    poisonTimer = 0.0f;
                }
            }
            else
            {
                poisonTimer = 0.0f; // 重置计时器
            }
        }
    }
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

    // 更新状态效果显示
    if (attr && _statusEffectLabel)
    {
        const auto &effects = attr->getStatusEffects();
        if (effects.empty())
        {
            _statusEffectLabel->setString("状态效果: 无");
            _statusEffectLabel->setColor(Color3B(150, 150, 150));
        }
        else
        {
            std::string effectStr = "状态效果: ";
            for (size_t i = 0; i < effects.size(); ++i)
            {
                const auto &eff = effects[i];
                float remaining = eff.duration - eff.elapsed;
                std::string effectName;
                switch (eff.type)
                {
                case StatusEffectType::POISONED:
                    effectName = "中毒";
                    break;
                case StatusEffectType::EXCITED:
                    effectName = "亢奋";
                    break;
                case StatusEffectType::STUNNED:
                    effectName = "眩晕";
                    break;
                }
                effectStr += StringUtils::format("%s(%.1fs)", effectName.c_str(), remaining);
                if (i < effects.size() - 1)
                    effectStr += ", ";
            }
            _statusEffectLabel->setString(effectStr);
            _statusEffectLabel->setColor(Color3B(255, 200, 100));
        }
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

        // 更新 HP 条宽度（使用成员变量）
        if (_hpBarFill)
        {
            _hpBarFill->clear();
            float hpBarWidth = 200.0f * hpPercent;
            float barHeight = 18.0f;
            _hpBarFill->drawSolidRect(Vec2(0, 0), Vec2(hpBarWidth, barHeight), Color4F(0.8f, 0.2f, 0.2f, 1.0f));
        }

        // 更新 MP 条宽度（使用成员变量）
        if (_mpBarFill)
        {
            _mpBarFill->clear();
            float mpBarWidth = 200.0f * mpPercent;
            float barHeight = 18.0f;
            _mpBarFill->drawSolidRect(Vec2(0, 0), Vec2(mpBarWidth, barHeight), Color4F(0.2f, 0.4f, 0.9f, 1.0f));
        }
    }
}

void DebugScene::onTakeDamageClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    // 模拟敌人攻击：使用固定基础伤害
    DamageInfo info;
    info.amount = 10.0f; // 敌人基础攻击力
    info.penetration = 0.0f;
    info.isCritical = false;
    info.attacker = nullptr;
    // 注意：实际伤害会被 takeDamage 中的防御计算减少

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

    // 如果正在攻击或施放技能中，禁用攻击
    if (_isAttacking || _isCastingSkill)
    {
        return;
    }

    _player->attack();
    playAttackAnimation();

    // 显示角色的实际攻击力信息
    float strength = 0.0f;
    float critRate = 0.0f;
    auto attr = _player->getAttributeComponent();
    if (attr)
    {
        strength = attr->getAttributeValue(AttributeType::STRENGTH);
        critRate = attr->getAttributeValue(AttributeType::CRITICAL_RATE);
    }
    addDamageLog(StringUtils::format("攻击! 力量:%.0f 暴击率:%.0f%%", strength, critRate * 100));

    CCLOG("Player attack started with STR: %.0f, Crit: %.0f%%", strength, critRate * 100);
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

// ==================== 状态效果回调 ====================

void DebugScene::onPoisonClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto attr = _player->getAttributeComponent();
    if (!attr)
        return;

    // 创建中毒状态效果
    StatusEffectInstance poison;
    poison.type = StatusEffectType::POISONED;
    poison.duration = 5.0f; // 持续5秒
    poison.elapsed = 0.0f;
    // 中毒不提供属性加成，但会造成持续伤害（在 update 中处理）

    attr->addStatusEffect(poison);
    _isPoisoned = true;

    addDamageLog("中毒! 持续5秒");
    CCLOG("Poison effect applied for 5 seconds");
}

void DebugScene::onExcitedClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto attr = _player->getAttributeComponent();
    if (!attr)
        return;

    // 创建亢奋状态效果
    StatusEffectInstance excited;
    excited.type = StatusEffectType::EXCITED;
    excited.duration = 8.0f; // 持续8秒
    excited.elapsed = 0.0f;
    // 亢奋效果：移速+50，力量+10, 暴击+10%
    excited.attributeBonus.set(AttributeType::MOVE_SPEED, 50.0f);
    excited.attributeBonus.set(AttributeType::CRITICAL_RATE, 0.10f);
    excited.attributeBonus.set(AttributeType::STRENGTH, 10.0f);

    attr->addStatusEffect(excited);

    addDamageLog("亢奋! 移速+50, 暴击+10% (8秒)");
    CCLOG("Excited effect applied: +50 move speed, +10%% crit for 8 seconds");
}

void DebugScene::onStunnedClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto attr = _player->getAttributeComponent();
    if (!attr)
        return;

    // 创建眩晕状态效果
    StatusEffectInstance stunned;
    stunned.type = StatusEffectType::STUNNED;
    stunned.duration = 3.0f; // 持续3秒
    stunned.elapsed = 0.0f;
    // 眩晕效果：移速降为0
    stunned.attributeBonus.set(AttributeType::MOVE_SPEED, -200.0f); // 移速大幅降低

    attr->addStatusEffect(stunned);

    addDamageLog("眩晕! 无法移动 (3秒)");
    CCLOG("Stunned effect applied for 3 seconds");
}

void DebugScene::applyPoisonDamage(float dt)
{
    if (!_player || _player->isDead() || !_isPoisoned)
        return;

    // 中毒伤害：每秒造成 5 点伤害
    DamageInfo poisonDamage;
    poisonDamage.amount = 5.0f;
    poisonDamage.penetration = 1000.0f; // 中毒伤害穿透防御
    poisonDamage.isCritical = false;
    poisonDamage.attacker = nullptr;

    _player->takeDamage(poisonDamage);

    // 在玩家位置显示中毒伤害数字（绿色）
    auto damageLabel = Label::createWithTTF(StringUtils::format("-%.0f", poisonDamage.amount),
                                            "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
    damageLabel->setColor(Color3B(0, 200, 0)); // 绿色表示中毒伤害
    damageLabel->setPosition(_player->getPosition() + Vec2(rand() % 30 - 15, 50));
    this->addChild(damageLabel, 100);

    // 飘字动画
    auto moveUp = MoveBy::create(0.8f, Vec2(0, 30));
    auto fadeOut = FadeOut::create(0.8f);
    auto spawn = Spawn::create(moveUp, fadeOut, nullptr);
    auto remove = RemoveSelf::create();
    damageLabel->runAction(Sequence::create(spawn, remove, nullptr));
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
    // 技能按键 - 丢炸弹
    case EventKeyboard::KeyCode::KEY_E:
        throwBomb();
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

    auto physicsBody = _player->getPhysicsBody();
    if (!physicsBody)
        return;

    // 从角色属性组件获取移动速度
    float currentMoveSpeed = _moveSpeed; // 默认值
    auto attr = _player->getAttributeComponent();
    if (attr)
    {
        currentMoveSpeed = attr->getAttributeValue(AttributeType::MOVE_SPEED);
    }

    // 获取当前垂直速度（保持重力效果）
    Vec2 currentVelocity = physicsBody->getVelocity();

    // 计算水平速度
    float velocityX = 0.0f;

    if (_isMovingLeft && !_isMovingRight)
    {
        velocityX = -currentMoveSpeed;
    }
    else if (_isMovingRight && !_isMovingLeft)
    {
        velocityX = currentMoveSpeed;
    }

    // 设置物理刚体的速度（保持垂直速度不变）
    physicsBody->setVelocity(Vec2(velocityX, currentVelocity.y));

    // 限制在屏幕范围内
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    float margin = 30.0f;
    Vec2 pos = _player->getPosition();

    if (pos.x < origin.x + margin)
    {
        _player->setPositionX(origin.x + margin);
        physicsBody->setVelocity(Vec2(0, currentVelocity.y));
    }
    else if (pos.x > origin.x + visibleSize.width - margin)
    {
        _player->setPositionX(origin.x + visibleSize.width - margin);
        physicsBody->setVelocity(Vec2(0, currentVelocity.y));
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

    // 保存当前翻转状态
    bool wasFlippedX = _player->isFlippedX();

    // 停止行走动画
    _player->stopActionByTag(999);

    // 恢复到默认静止图片
    auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_run.png");
    if (defaultTexture)
    {
        _player->setTexture(defaultTexture);
        // 设置正确的纹理矩形
        _player->setTextureRect(Rect(0, 0, defaultTexture->getContentSize().width,
                                     defaultTexture->getContentSize().height));
        // 恢复翻转状态
        _player->setFlippedX(wasFlippedX);
    }

    CCLOG("Walk animation stopped");
}

// ==================== 装备系统 ====================

void DebugScene::initEquipments()
{
    // 创建剑武器
    _swordWeapon = std::make_shared<Weapon>();
    _swordWeapon->id = 1;
    _swordWeapon->name = "铁剑";
    _swordWeapon->description = "一把普通的铁剑";
    _swordWeapon->type = WeaponType::SWORD;
    _swordWeapon->attackDamage = 15.0f;
    _swordWeapon->attackRange = 50.0f;
    _swordWeapon->attackSpeed = 1.0f;
    _swordWeapon->attackAnimationPrefix = "spr_klee_attack"; // 暂用默认动画
    _swordWeapon->attackFrameCount = 3;
    _swordWeapon->attributeBonus.set(AttributeType::STRENGTH, 5.0f);
    _swordWeapon->spritePath = "default"; // 后期替换为实际路径

    // 创建法杖武器
    _staffWeapon = std::make_shared<Weapon>();
    _staffWeapon->id = 2;
    _staffWeapon->name = "魔法杖";
    _staffWeapon->description = "蕴含魔力的法杖";
    _staffWeapon->type = WeaponType::STAFF;
    _staffWeapon->attackDamage = 8.0f;
    _staffWeapon->attackRange = 100.0f;
    _staffWeapon->attackSpeed = 0.8f;
    _staffWeapon->attackAnimationPrefix = "spr_klee_attack"; // 暂用默认动画
    _staffWeapon->attackFrameCount = 3;
    _staffWeapon->attributeBonus.set(AttributeType::MAX_MP, 20.0f);
    _staffWeapon->attributeBonus.set(AttributeType::CRITICAL_RATE, 0.05f);
    _staffWeapon->spritePath = "default"; // 后期替换为实际路径

    // 创建匕首武器
    _daggerWeapon = std::make_shared<Weapon>();
    _daggerWeapon->id = 3;
    _daggerWeapon->name = "锋利匕首";
    _daggerWeapon->description = "快速但短距离的匕首";
    _daggerWeapon->type = WeaponType::DAGGER;
    _daggerWeapon->attackDamage = 10.0f;
    _daggerWeapon->attackRange = 30.0f;
    _daggerWeapon->attackSpeed = 1.5f;
    _daggerWeapon->attackAnimationPrefix = "spr_klee_attack"; // 暂用默认动画
    _daggerWeapon->attackFrameCount = 3;
    _daggerWeapon->attributeBonus.set(AttributeType::CRITICAL_RATE, 0.15f);
    _daggerWeapon->attributeBonus.set(AttributeType::MOVE_SPEED, 20.0f);
    _daggerWeapon->spritePath = "default"; // 后期替换为实际路径

    // 设置装备变更回调
    if (_player)
    {
        _player->setEquipmentChangeCallback(
            [this](EquipmentSlot slot, const std::shared_ptr<Equipment> &equipment)
            {
                this->onEquipmentChanged(slot, equipment);
            });
    }

    CCLOG("Equipment system initialized: 3 weapons created");
}

void DebugScene::onEquipmentChanged(EquipmentSlot slot, const std::shared_ptr<Equipment> &equipment)
{
    if (slot == EquipmentSlot::WEAPON)
    {
        if (equipment)
        {
            auto weapon = std::dynamic_pointer_cast<Weapon>(equipment);
            if (weapon)
            {
                // 更新装备标签
                std::string weaponTypeStr;
                switch (weapon->type)
                {
                case WeaponType::SWORD:
                    weaponTypeStr = "剑";
                    break;
                case WeaponType::STAFF:
                    weaponTypeStr = "法杖";
                    break;
                case WeaponType::DAGGER:
                    weaponTypeStr = "匕首";
                    break;
                }

                if (_equipmentLabel)
                {
                    _equipmentLabel->setString(StringUtils::format("武器: %s (%s)\n攻击力+%.0f 范围%.0f",
                                                                   weapon->name.c_str(),
                                                                   weaponTypeStr.c_str(),
                                                                   weapon->attackDamage,
                                                                   weapon->attackRange));
                }

                addDamageLog(StringUtils::format("装备: %s", weapon->name.c_str()));

                // TODO: 后期根据 weapon->spritePath 更换角色贴图
                // 目前使用默认贴图
            }
        }
        else
        {
            if (_equipmentLabel)
            {
                _equipmentLabel->setString("武器: 无");
            }
            addDamageLog("卸下武器");
        }
    }
}

void DebugScene::onEquipSwordClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    _player->equip(_swordWeapon);
    CCLOG("Equipped sword: %s", _swordWeapon->name.c_str());
}

void DebugScene::onEquipStaffClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    _player->equip(_staffWeapon);
    CCLOG("Equipped staff: %s", _staffWeapon->name.c_str());
}

void DebugScene::onEquipDaggerClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    _player->equip(_daggerWeapon);
    CCLOG("Equipped dagger: %s", _daggerWeapon->name.c_str());
}

void DebugScene::onUnequipWeaponClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    _player->unequip(EquipmentSlot::WEAPON);
    CCLOG("Weapon unequipped");
}

// ==================== 被动技能系统 ====================

void DebugScene::initPassiveSkills()
{
    // 创建被动技能1：力量提升
    _passiveSkill1 = std::make_shared<PassiveSkill>();
    _passiveSkill1->id = 2001;
    _passiveSkill1->name = "力量精通";
    _passiveSkill1->description = "永久增加5点力量";
    _passiveSkill1->attributeBonus.set(AttributeType::STRENGTH, 5.0f);

    // 创建被动技能2：防御提升
    _passiveSkill2 = std::make_shared<PassiveSkill>();
    _passiveSkill2->id = 2002;
    _passiveSkill2->name = "铁壁";
    _passiveSkill2->description = "永久增加3点防御";
    _passiveSkill2->attributeBonus.set(AttributeType::DEFENSE, 3.0f);

    // 创建被动技能3：暴击提升
    _passiveSkill3 = std::make_shared<PassiveSkill>();
    _passiveSkill3->id = 2003;
    _passiveSkill3->name = "致命一击";
    _passiveSkill3->description = "永久增加10%暴击率";
    _passiveSkill3->attributeBonus.set(AttributeType::CRITICAL_RATE, 0.10f);

    CCLOG("Passive skills initialized: 3 skills created");
}

void DebugScene::onLearnPassive1Clicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
        return;

    // 学习并装备到槽位0
    skillComp->learnSkill(_passiveSkill1);
    skillComp->equipPassiveSkill(_passiveSkill1, 0);

    addDamageLog(StringUtils::format("学习被动: %s (力量+5)", _passiveSkill1->name.c_str()));
    updatePassiveSkillLabel();
    CCLOG("Learned passive skill: %s", _passiveSkill1->name.c_str());
}

void DebugScene::onLearnPassive2Clicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
        return;

    // 学习并装备到槽位1
    skillComp->learnSkill(_passiveSkill2);
    skillComp->equipPassiveSkill(_passiveSkill2, 1);

    addDamageLog(StringUtils::format("学习被动: %s (防御+3)", _passiveSkill2->name.c_str()));
    updatePassiveSkillLabel();
    CCLOG("Learned passive skill: %s", _passiveSkill2->name.c_str());
}

void DebugScene::onLearnPassive3Clicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
        return;

    // 学习并装备到槽位2
    skillComp->learnSkill(_passiveSkill3);
    skillComp->equipPassiveSkill(_passiveSkill3, 2);

    addDamageLog(StringUtils::format("学习被动: %s (暴击+10%%)", _passiveSkill3->name.c_str()));
    updatePassiveSkillLabel();
    CCLOG("Learned passive skill: %s", _passiveSkill3->name.c_str());
}

void DebugScene::updatePassiveSkillLabel()
{
    if (!_passiveSkillLabel || !_player)
        return;

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        _passiveSkillLabel->setString("被动技能: 无");
        return;
    }

    const auto &passiveSlots = skillComp->getPassiveSlots();
    if (passiveSlots.empty())
    {
        _passiveSkillLabel->setString("被动技能: 无");
        return;
    }

    std::string passiveText = "被动技能:\n";
    for (size_t i = 0; i < passiveSlots.size(); ++i)
    {
        if (passiveSlots[i])
        {
            passiveText += "  - " + passiveSlots[i]->name + "\n";
        }
    }

    _passiveSkillLabel->setString(passiveText);
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

    // 获取当前装备的武器类型
    WeaponType currentWeaponType = _player->getCurrentWeaponType();
    auto equippedWeapon = _player->getEquippedWeapon();

    // 根据武器类型设置不同的攻击动画速度
    float animSpeed = 0.15f; // 默认动画速度
    if (equippedWeapon)
    {
        animSpeed = 0.15f / equippedWeapon->attackSpeed; // 攻击速度越高，动画越快
    }

    // 加载3张攻击图片 (目前使用默认动画，后期根据武器类型替换)
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

        auto animation = Animation::createWithSpriteFrames(frames, animSpeed);
        auto animate = Animate::create(animation);

        // 停止之前的攻击动画
        _player->stopActionByTag(1000);

        // 创建动画序列：播放动画 -> 回调结束
        auto callbackAction = CallFunc::create([this]()
                                               { this->onAttackAnimationFinished(); });
        auto sequence = Sequence::create(animate, callbackAction, nullptr);
        sequence->setTag(1000);

        _player->runAction(sequence);

        // 根据武器类型输出不同的攻击提示
        std::string attackTypeStr;
        switch (currentWeaponType)
        {
        case WeaponType::SWORD:
            attackTypeStr = "挥剑攻击";
            break;
        case WeaponType::STAFF:
            attackTypeStr = "法杖施法";
            break;
        case WeaponType::DAGGER:
            attackTypeStr = "匕首突刺";
            break;
        }

        CCLOG("Attack animation started: %s (speed: %.2f)", attackTypeStr.c_str(), animSpeed);
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

    // 保存当前翻转状态
    bool wasFlippedX = _player ? _player->isFlippedX() : false;

    // 执行攻击伤害判定
    if (_player && _targetDummy.sprite && _targetDummy.currentHP > 0)
    {
        Vec2 playerPos = _player->getPosition();
        Vec2 dummyPos = _targetDummy.sprite->getPosition();
        float distance = playerPos.distance(dummyPos);

        // 获取当前武器和攻击范围
        auto weapon = _player->getEquippedWeapon();
        float attackRange = weapon ? weapon->attackRange : 60.0f;  // 默认徒手攻击范围
        float weaponDamage = weapon ? weapon->attackDamage : 5.0f; // 默认徒手伤害

        // 获取角色属性
        auto attr = _player->getAttributeComponent();
        float strength = attr ? attr->getAttributeValue(AttributeType::STRENGTH) : 10.0f;
        float critRate = attr ? attr->getAttributeValue(AttributeType::CRITICAL_RATE) : 0.1f;

        // 检查是否在攻击范围内
        if (distance <= attackRange + 50.0f) // 加一些容差
        {
            // 计算基础伤害 = 武器伤害 + 力量 * 系数
            float baseDamage = weaponDamage + strength * 1.5f;

            // 判断暴击
            bool isCrit = (static_cast<float>(rand()) / RAND_MAX) < critRate;
            float finalDamage = isCrit ? baseDamage * 1.5f : baseDamage;

            // 根据武器类型添加特殊效果
            WeaponType weaponType = _player->getCurrentWeaponType();
            switch (weaponType)
            {
            case WeaponType::SWORD:
                // 剑：普通伤害
                break;
            case WeaponType::STAFF:
                // 法杖：消耗 MP 增加 50% 伤害
                if (_player->getCurrentMP() >= 5.0f)
                {
                    _player->setCurrentMP(_player->getCurrentMP() - 5.0f);
                    finalDamage *= 1.5f;
                    addDamageLog("法杖魔力攻击! (消耗5MP)");
                }
                break;
            case WeaponType::DAGGER:
                // 匕首：背刺判定（简单模拟：额外暴击几率）
                if (!isCrit && (static_cast<float>(rand()) / RAND_MAX) < 0.2f)
                {
                    isCrit = true;
                    finalDamage = baseDamage * 1.5f;
                    addDamageLog("背刺暴击!");
                }
                break;
            }

            dealDamageToTarget(finalDamage, isCrit);
        }
    }

    // 如果玩家仍在移动，恢复行走动画
    if (_isMovingLeft || _isMovingRight)
    {
        startWalkAnimation();
    }
    else
    {
        // 恢复到默认静止图片
        auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
            "Sprites/Characters/Player/Klee/spr_klee_run.png");
        if (defaultTexture && _player)
        {
            _player->setTexture(defaultTexture);
            // 设置正确的纹理矩形
            _player->setTextureRect(Rect(0, 0, defaultTexture->getContentSize().width,
                                         defaultTexture->getContentSize().height));
            // 恢复翻转状态
            _player->setFlippedX(wasFlippedX);
        }
    }

    CCLOG("Attack animation finished");
}

// 初始化物理碰撞监听器
void DebugScene::initPhysicsContactListener()
{
    auto contactListener = EventListenerPhysicsContact::create();

    // 碰撞开始回调
    contactListener->onContactBegin = CC_CALLBACK_1(DebugScene::onContactBegin, this);

    // 碰撞预处理回调 - 用于控制碰撞响应
    contactListener->onContactPreSolve = [this](PhysicsContact &contact, PhysicsContactPreSolve &solve) -> bool
    {
        auto nodeA = contact.getShapeA()->getBody()->getNode();
        auto nodeB = contact.getShapeB()->getBody()->getNode();

        if (!nodeA || !nodeB)
            return true;

        int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
        int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

        // 玩家与平台碰撞时，保持玩家的水平速度
        if ((categoryA == CATEGORY_PLAYER && categoryB == CATEGORY_PLATFORM) ||
            (categoryA == CATEGORY_PLATFORM && categoryB == CATEGORY_PLAYER))
        {
            // 设置碰撞的弹性为0，避免弹跳
            solve.setRestitution(0.0f);
            // 设置摩擦力为0，避免水平方向受阻
            solve.setFriction(0.0f);
        }

        return true;
    };

    // 碰撞分离回调
    contactListener->onContactSeparate = CC_CALLBACK_1(DebugScene::onContactSeparate, this);

    _eventDispatcher->addEventListenerWithSceneGraphPriority(contactListener, this);

    CCLOG("Physics contact listener initialized");
}

bool DebugScene::onContactBegin(PhysicsContact &contact)
{
    auto nodeA = contact.getShapeA()->getBody()->getNode();
    auto nodeB = contact.getShapeB()->getBody()->getNode();

    if (!nodeA || !nodeB)
        return true;

    // 获取碰撞双方的类别
    int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
    int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

    // 玩家与平台碰撞 - 检测是否落地
    if ((categoryA == CATEGORY_PLAYER && categoryB == CATEGORY_PLATFORM) ||
        (categoryA == CATEGORY_PLATFORM && categoryB == CATEGORY_PLAYER))
    {
        // 获取碰撞法向量，判断是从上方落下还是侧面碰撞
        auto contactData = contact.getContactData();
        Vec2 normal = contactData->normal;

        // 只有当碰撞法向量主要朝上时（从上方落下），才认为是落地
        // normal.y > 0.7 表示法向量主要朝上
        if (std::abs(normal.y) > 0.5f)
        {
            _groundContactCount++;
            _isGrounded = true;
            CCLOG("Player landed on platform, contact count: %d, normal: (%.2f, %.2f)",
                  _groundContactCount, normal.x, normal.y);
        }
        else
        {
            // 侧面碰撞，不处理落地逻辑
            CCLOG("Side collision detected, normal: (%.2f, %.2f)", normal.x, normal.y);
        }
    }

    // 炸弹与平台碰撞 - 触发爆炸
    if ((categoryA == CATEGORY_BOMB && categoryB == CATEGORY_PLATFORM) ||
        (categoryA == CATEGORY_PLATFORM && categoryB == CATEGORY_BOMB))
    {
        Node *bombNode = (categoryA == CATEGORY_BOMB) ? nodeA : nodeB;

        // 查找对应的炸弹并爆炸
        for (auto &bomb : _bombs)
        {
            if (bomb.sprite == bombNode && !bomb.isExploded)
            {
                explodeBomb(bomb);
                break;
            }
        }
    }

    return true;
}

void DebugScene::onContactSeparate(PhysicsContact &contact)
{
    auto nodeA = contact.getShapeA()->getBody()->getNode();
    auto nodeB = contact.getShapeB()->getBody()->getNode();

    if (!nodeA || !nodeB)
        return;

    int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
    int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

    // 玩家离开平台
    if ((categoryA == CATEGORY_PLAYER && categoryB == CATEGORY_PLATFORM) ||
        (categoryA == CATEGORY_PLATFORM && categoryB == CATEGORY_PLAYER))
    {
        // 只有当接触计数大于0时才减少
        if (_groundContactCount > 0)
        {
            _groundContactCount--;
        }
        if (_groundContactCount <= 0)
        {
            _groundContactCount = 0;
            _isGrounded = false;
            CCLOG("Player left platform, now airborne");
        }
    }
}

void DebugScene::jump()
{
    if (!_player || _player->isDead())
        return;

    // 只有在地面上才能跳跃
    if (_isGrounded)
    {
        auto physicsBody = _player->getPhysicsBody();
        if (physicsBody)
        {
            // 使用物理引擎的冲量实现跳跃
            physicsBody->applyImpulse(Vec2(0, JUMP_IMPULSE * physicsBody->getMass()));
        }
        _isGrounded = false;
        addDamageLog("跳跃!");
        CCLOG("Player jumped with impulse");
    }
}

void DebugScene::throwBomb()
{
    if (!_player || _player->isDead())
        return;

    // 如果正在施放技能或攻击中，禁用技能
    if (_isCastingSkill || _isAttacking)
    {
        return;
    }

    // 通过技能组件释放技能（会自动检查 MP、冷却，并扣除 MP）
    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        CCLOG("Skill component not found");
        return;
    }

    // 尝试使用槽位 0 的技能（炸弹技能）
    if (!skillComp->useActiveSkill(BOMB_SKILL_SLOT))
    {
        // 技能释放失败（可能是 MP 不足或冷却中）
        float currentMP = _player->getCurrentMP();
        auto activeSlots = skillComp->getActiveSlots();
        if (BOMB_SKILL_SLOT < activeSlots.size() && activeSlots[BOMB_SKILL_SLOT])
        {
            auto skill = activeSlots[BOMB_SKILL_SLOT];
            if (skill->currentCooldown > 0)
            {
                addDamageLog(StringUtils::format("技能冷却中: %.1f秒", skill->currentCooldown));
            }
            else if (currentMP < skill->manaCost)
            {
                addDamageLog(StringUtils::format("MP不足! 需要: %.0f, 当前: %.0f", skill->manaCost, currentMP));
            }
        }
        CCLOG("Skill cast failed - MP insufficient or on cooldown");
        return;
    }

    // 技能释放成功，播放技能动画
    playSkillAnimation();
    addDamageLog(StringUtils::format("施放技能: 丢炸弹! (消耗 %.0f MP)", BOMB_SKILL_MP_COST));
    CCLOG("Skill started: Throw Bomb");
}

void DebugScene::playSkillAnimation()
{
    if (!_player)
        return;

    // 停止行走动画（如果在播放）
    if (_isWalkAnimationPlaying)
    {
        stopWalkAnimation();
    }

    _isCastingSkill = true;

    // 加载3张攻击图片（技能动画暂时使用攻击动画，后期可改）
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
        auto animation = Animation::createWithSpriteFrames(frames, 0.13f);
        auto animate = Animate::create(animation);

        // 停止之前的技能动画
        _player->stopActionByTag(1001);

        // 创建动画序列：播放动画 -> 回调结束
        auto callbackAction = CallFunc::create([this]()
                                               { this->onSkillAnimationFinished(); });
        auto sequence = Sequence::create(animate, callbackAction, nullptr);
        sequence->setTag(1001);

        _player->runAction(sequence);

        CCLOG("Skill animation started (3-hit combo)");
    }
    else
    {
        CCLOG("Failed to load skill sprites");
        _isCastingSkill = false;
    }
}

void DebugScene::onSkillAnimationFinished()
{
    _isCastingSkill = false;

    // 动画结束后实际丢出炸弹
    doThrowBomb();

    // 保存当前翻转状态
    bool wasFlippedX = _player ? _player->isFlippedX() : false;

    // 如果玩家仍在移动，恢复行走动画
    if (_isMovingLeft || _isMovingRight)
    {
        startWalkAnimation();
    }
    else
    {
        // 恢复到默认静止图片
        auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
            "Sprites/Characters/Player/Klee/spr_klee_run.png");
        if (defaultTexture && _player)
        {
            _player->setTexture(defaultTexture);
            // 设置正确的纹理矩形
            _player->setTextureRect(Rect(0, 0, defaultTexture->getContentSize().width,
                                         defaultTexture->getContentSize().height));
            // 恢复翻转状态
            _player->setFlippedX(wasFlippedX);
        }
    }

    CCLOG("Skill animation finished");
}

void DebugScene::doThrowBomb()
{
    if (!_player || _player->isDead())
        return;

    // 创建炸弹精灵
    auto bombSprite = Sprite::create("Sprites/Characters/Player/Klee/TNT.png");
    if (!bombSprite)
    {
        CCLOG("Failed to create bomb sprite");
        return;
    }

    // 创建炸弹对象
    Bomb bomb;
    bomb.isExploded = false;
    bomb.sprite = bombSprite;

    // 根据角色朝向决定炸弹方向
    bool facingLeft = _player->isFlippedX();
    float throwDirX = facingLeft ? -1.0f : 1.0f;

    // 设置炸弹初始位置（角色上方）
    Vec2 playerPos = _player->getPosition();
    float offsetX = throwDirX * bomb.sprite->getContentSize().width;
    float offsetY = bomb.sprite->getContentSize().height;
    bombSprite->setPosition(playerPos + Vec2(offsetX, offsetY));
    bombSprite->setScale(0.5f);

    // 创建炸弹物理刚体
    PhysicsMaterial bombMaterial(0.5f, 0.3f, 0.2f);                    // 密度、弹性、摩擦
    auto physicsBody = PhysicsBody::createCircle(15.0f, bombMaterial); // 圆形碰撞体
    physicsBody->setDynamic(true);
    physicsBody->setMass(0.5f);
    physicsBody->setRotationEnable(true); // 允许旋转

    // 设置碰撞掩码
    physicsBody->setCategoryBitmask(CATEGORY_BOMB);
    physicsBody->setCollisionBitmask(CATEGORY_PLATFORM);   // 只与平台碰撞
    physicsBody->setContactTestBitmask(CATEGORY_PLATFORM); // 检测与平台的接触

    bombSprite->addComponent(physicsBody);
    this->addChild(bombSprite, 4);

    // 施加初始速度（冲量）
    Vec2 impulse(throwDirX * BOMB_THROW_SPEED_X * physicsBody->getMass(),
                 BOMB_THROW_SPEED_Y * physicsBody->getMass());
    physicsBody->applyImpulse(impulse);

    _bombs.push_back(bomb);

    addDamageLog("丢出炸弹!");
    CCLOG("Bomb thrown with physics!");
}

void DebugScene::explodeBomb(Bomb &bomb)
{
    if (!bomb.sprite)
        return;

    Vec2 explodePos = bomb.sprite->getPosition();

    // 移除炸弹精灵
    bomb.sprite->removeFromParent();

    // 创建爆炸效果
    auto boomSprite = Sprite::create("Sprites/Characters/Player/Klee/BOOM_1.png");
    if (boomSprite)
    {
        boomSprite->setPosition(explodePos);
        boomSprite->setScale(0.8f);
        this->addChild(boomSprite, 6);

        // 爆炸动画：放大 + 淡出
        auto scaleUp = ScaleTo::create(0.2f, 1.2f);
        auto fadeOut = FadeOut::create(0.3f);
        auto spawn = Spawn::create(scaleUp, fadeOut, nullptr);
        auto remove = RemoveSelf::create();
        auto sequence = Sequence::create(spawn, remove, nullptr);
        boomSprite->runAction(sequence);
    }

    // 检测是否命中木桩
    if (_targetDummy.sprite && _targetDummy.currentHP > 0)
    {
        Vec2 dummyPos = _targetDummy.sprite->getPosition();
        float distance = explodePos.distance(dummyPos);

        if (distance <= BOMB_EXPLOSION_RADIUS)
        {
            // 从角色属性组件获取暴击率和攻击力
            float critRate = 0.3f;          // 默认暴击率 30%
            float critMultiplier = 1.5f;    // 默认暴击倍率 1.5
            float baseDamage = BOMB_DAMAGE; // 基础伤害

            if (_player)
            {
                auto attr = _player->getAttributeComponent();
                if (attr)
                {
                    // 从属性组件读取暴击率
                    critRate = attr->getAttributeValue(AttributeType::CRITICAL_RATE);
                    // 伤害 = 炸弹基础伤害 + 角色力量值 * 倍率
                    float strength = attr->getAttributeValue(AttributeType::STRENGTH);
                    baseDamage = BOMB_DAMAGE + strength * 5.0f; // 每点力量增加5点伤害
                }
            }

            // 根据暴击率计算是否暴击
            bool isCrit = (rand() % 100) < static_cast<int>(critRate * 100);
            float damage = baseDamage;
            if (isCrit)
            {
                damage *= critMultiplier;
            }
            dealDamageToTarget(damage, isCrit);
        }
    }

    bomb.sprite = nullptr;

    addDamageLog("炸弹爆炸!");
    CCLOG("Bomb exploded at (%.0f, %.0f)", explodePos.x, explodePos.y);
}

void DebugScene::dealDamageToTarget(float damage, bool isCrit)
{
    if (!_targetDummy.sprite || _targetDummy.currentHP <= 0)
        return;

    // 扣血
    _targetDummy.currentHP -= damage;
    if (_targetDummy.currentHP < 0)
    {
        _targetDummy.currentHP = 0;
    }

    // 显示伤害数字
    Vec2 dummyPos = _targetDummy.sprite->getPosition();
    // 随机偏移让伤害数字不重叠
    float offsetX = (rand() % 40) - 20;
    float offsetY = 50 + (rand() % 30);
    showDamageNumber(dummyPos + Vec2(offsetX, offsetY), damage, isCrit);

    // 更新血条
    updateTargetHPBar();

    // 受击闪烁效果
    auto tintRed = TintTo::create(0.1f, 255, 100, 100);
    auto tintBack = TintTo::create(0.1f, 255, 255, 255);
    _targetDummy.sprite->runAction(Sequence::create(tintRed, tintBack, nullptr));

    addDamageLog(StringUtils::format("%s %.0f 伤害!", isCrit ? "暴击!" : "造成", damage));
    CCLOG("Dealt %.0f damage to target (crit: %d), HP: %.0f/%.0f",
          damage, isCrit, _targetDummy.currentHP, _targetDummy.maxHP);
}

void DebugScene::showDamageNumber(const Vec2 &pos, float damage, bool isCrit)
{
    // 创建伤害数字标签
    std::string damageText = StringUtils::format("%.0f", damage);
    if (isCrit)
    {
        damageText = "暴击 " + damageText + "!";
    }

    auto damageLabel = Label::createWithTTF(damageText, "fonts/ZCOOLKuaiLe-Regular.ttf", isCrit ? 28 : 22);
    damageLabel->setPosition(pos);
    damageLabel->setColor(isCrit ? Color3B(255, 50, 50) : Color3B(255, 200, 50)); // 暴击红色，普通黄色
    damageLabel->enableOutline(Color4B::BLACK, 2);
    this->addChild(damageLabel, 100);

    // 伤害数字动画：向上飘 + 淡出
    auto moveUp = MoveBy::create(0.8f, Vec2(0, 60));
    auto fadeOut = FadeOut::create(0.5f);
    auto spawn = Spawn::create(moveUp, fadeOut, nullptr);
    auto remove = RemoveSelf::create();
    damageLabel->runAction(Sequence::create(spawn, remove, nullptr));
}

void DebugScene::updateTargetHPBar()
{
    if (!_targetDummy.sprite || !_targetDummy.hpBar)
        return;

    _targetDummy.hpBar->clear();

    Vec2 dummyPos = _targetDummy.sprite->getPosition();
    float barWidth = 60.0f;
    float barHeight = 8.0f;
    float barY = 75.0f; // 血条在头顶上方

    Vec2 barPos(dummyPos.x - barWidth / 2, dummyPos.y + barY);

    // 绘制血条背景（黑色）
    _targetDummy.hpBar->drawSolidRect(
        barPos,
        barPos + Vec2(barWidth, barHeight),
        Color4F(0.2f, 0.2f, 0.2f, 1.0f));

    // 绘制当前血量（红色渐变）
    float hpRatio = _targetDummy.currentHP / _targetDummy.maxHP;
    float currentWidth = barWidth * hpRatio;

    Color4F hpColor;
    if (hpRatio > 0.5f)
    {
        hpColor = Color4F(0.2f, 0.8f, 0.2f, 1.0f); // 绿色
    }
    else if (hpRatio > 0.25f)
    {
        hpColor = Color4F(1.0f, 0.8f, 0.0f, 1.0f); // 黄色
    }
    else
    {
        hpColor = Color4F(1.0f, 0.2f, 0.2f, 1.0f); // 红色
    }

    _targetDummy.hpBar->drawSolidRect(
        barPos,
        barPos + Vec2(currentWidth, barHeight),
        hpColor);

    // 绘制边框
    _targetDummy.hpBar->drawRect(
        barPos,
        barPos + Vec2(barWidth, barHeight),
        Color4F::WHITE);

    // 更新HP文字
    if (_targetDummy.hpLabel)
    {
        _targetDummy.hpLabel->setString(StringUtils::format("%.0f/%.0f",
                                                            _targetDummy.currentHP, _targetDummy.maxHP));
        _targetDummy.hpLabel->setPosition(dummyPos + Vec2(0, barY + 15));
    }
}
