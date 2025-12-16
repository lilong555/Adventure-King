/**
 * @file DebugScene.cpp
 * @brief 角色功能调试场景实现
 *
 * 本文件实现了 DebugScene 类的所有功能，可作为正式关卡开发的参考模板。
 *
 * @note 文件结构说明（共12部分）：
 * - 第1部分：场景初始化（init 系列函数）
 * - 第2部分：主循环更新（update 系列函数）
 * - 第3部分：UI按钮回调 - 基础功能
 * - 第4部分：状态效果系统（中毒、亢奋、眩晕）
 * - 第5部分：装备系统（武器切换、属性加成）
 * - 第6部分：被动技能系统（常规被动、条件性被动）
 * - 第7部分：输入处理（键盘、移动、动画）
 * - 第8部分：攻击和战斗系统（伤害计算、武器特效）
 * - 第9部分：物理碰撞系统（落地检测、碰撞响应）
 * - 第10部分：跳跃与技能系统（跳跃、技能释放）
 * - 第11部分：炸弹物理系统（投掷、爆炸）
 * - 第12部分：伤害显示系统（飘字、血条）
 *
 * @author Adventure-King Team
 * @version 1.0
 */

#include "DebugScene.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include "MapScene.h"
#include <algorithm>

USING_NS_CC;
using namespace cocos2d::ui;

//=============================================================================
// 第1部分：场景初始化
//=============================================================================

Scene *DebugScene::createScene()
{
    return DebugScene::create();
}

/**
 * @brief 场景初始化入口
 *
 * 初始化顺序很重要，必须按以下顺序执行：
 * 1. 物理世界初始化
 * 2. 背景和平台（提供物理碰撞）
 * 3. 玩家（需要物理世界和平台）
 * 4. 装备和技能系统（依赖玩家）
 * 5. UI（显示玩家信息）
 * 6. 输入监听器
 *
 * @return 初始化是否成功
 */
bool DebugScene::init()
{
    //-------------------------------------------------------------------------
    // 步骤1：物理引擎初始化
    //-------------------------------------------------------------------------
    if (!Scene::initWithPhysics())
    {
        return false;
    }

    // 配置物理世界参数
    auto physicsWorld = this->getPhysicsWorld();
    physicsWorld->setGravity(Vec2(0, -800.0f)); // 设置重力加速度（向下）

    // 开启物理调试绘制（开发时可视化碰撞体，发布时应关闭）
    physicsWorld->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);

    //-------------------------------------------------------------------------
    // 步骤2：初始化场景元素
    //-------------------------------------------------------------------------
    initBackground();             // 背景和网格线
    initPlatforms();              // 平台和地面（物理刚体）
    initPlayer();                 // 玩家角色（物理刚体）
    initEquipments();             // 装备系统
    initPassiveSkills();          // 被动技能系统
    initTargetDummy();            // 测试用木桩
    initDebugUI();                // UI面板
    initControlButtons();         // 控制按钮
    initPhysicsContactListener(); // 物理碰撞监听

    //-------------------------------------------------------------------------
    // 步骤3：启用输入事件
    //-------------------------------------------------------------------------
    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = CC_CALLBACK_2(DebugScene::onKeyPressed, this);
    keyboardListener->onKeyReleased = CC_CALLBACK_2(DebugScene::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    //-------------------------------------------------------------------------
    // 步骤4：启用帧更新
    //-------------------------------------------------------------------------
    scheduleUpdate();

    CCLOG("DebugScene initialized with Physics Engine");
    return true;
}

/**
 * @brief 初始化背景
 *
 * 创建深灰色背景和网格线，帮助开发时定位元素位置。
 * 网格线间距为50像素。
 */
void DebugScene::initBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 创建深灰色背景层
    auto background = LayerColor::create(Color4B(40, 40, 50, 255));
    this->addChild(background, -1);

    // 添加网格线（开发辅助，可在正式关卡中移除）
    auto drawNode = DrawNode::create();
    Color4F gridColor(0.2f, 0.2f, 0.3f, 1.0f);

    // 绘制垂直网格线
    for (float x = origin.x; x < origin.x + visibleSize.width; x += 50)
    {
        drawNode->drawLine(Vec2(x, origin.y), Vec2(x, origin.y + visibleSize.height), gridColor);
    }
    // 绘制水平网格线
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

/**
 * @brief 初始化平台和地面
 *
 * 创建场景中的静态平台，包括：
 * - 地面平台（横跨整个屏幕底部）
 * - 可选的浮空平台
 *
 * 每个平台都有对应的物理刚体，用于与玩家和炸弹进行碰撞检测。
 *
 * @note 物理材质参数说明：
 * - 密度(density)：影响碰撞时的力传递
 * - 弹性(restitution)：设为0避免弹跳
 * - 摩擦(friction)：影响在表面上的滑动
 */
void DebugScene::initPlatforms()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 创建平台可视化绘制节点
    auto platformDraw = DrawNode::create();
    platformDraw->setTag(200);

    // 定义物理材质：密度=1.0, 弹性=0, 摩擦=0.8
    PhysicsMaterial platformMaterial(1.0f, 0.0f, 0.8f);

    /**
     * @brief 辅助Lambda：创建带物理的平台
     * @param rect 平台矩形区域
     * @param color 可视化颜色
     */
    auto createPlatform = [&](const Rect &rect, const Color4F &color)
    {
        // 绘制可视化矩形
        platformDraw->drawSolidRect(
            Vec2(rect.origin.x, rect.origin.y),
            Vec2(rect.origin.x + rect.size.width, rect.origin.y + rect.size.height),
            color);

        // 创建平台节点
        auto platformNode = Node::create();
        platformNode->setPosition(Vec2(
            rect.origin.x + rect.size.width / 2,
            rect.origin.y + rect.size.height / 2));

        // 创建静态物理刚体
        auto physicsBody = PhysicsBody::createBox(
            Size(rect.size.width, rect.size.height),
            platformMaterial);
        physicsBody->setDynamic(false); // 静态刚体不受力影响

        // 配置碰撞掩码
        physicsBody->setCategoryBitmask(CATEGORY_PLATFORM);
        physicsBody->setCollisionBitmask(CATEGORY_PLAYER | CATEGORY_BOMB);
        physicsBody->setContactTestBitmask(CATEGORY_PLAYER | CATEGORY_BOMB);

        platformNode->addComponent(physicsBody);
        this->addChild(platformNode, 1);

        _platforms.push_back(rect);
    };

    // 创建地面平台（覆盖屏幕底部）
    Rect groundPlatform(origin.x, origin.y + GROUND_Y - 20, visibleSize.width, 20);
    createPlatform(groundPlatform, Color4F(0.4f, 0.3f, 0.2f, 1.0f));

    // 可选：创建浮空平台（取消注释以启用）
    // Rect leftPlatform(origin.x + 50, origin.y + 200, 200, 20);
    // createPlatform(leftPlatform, Color4F(0.5f, 0.4f, 0.3f, 1.0f));

    this->addChild(platformDraw, 1);

    CCLOG("Platforms initialized with physics: %zu platforms", _platforms.size());
}

/**
 * @brief 初始化玩家角色
 *
 * 创建玩家角色实例，配置物理刚体，并设置初始位置。
 *
 * 玩家物理配置说明：
 * - 使用动态刚体（受重力影响）
 * - 碰撞体略小于精灵尺寸（80%宽，95%高）以获得更好的手感
 * - 禁用旋转，避免角色倾倒
 * - 与平台碰撞，检测炸弹接触
 */
void DebugScene::initPlayer()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 计算玩家初始位置（屏幕中央，地面上方）
    Vec2 startPos(origin.x + visibleSize.width / 2, origin.y + GROUND_Y + getContentSize().height / 2);

    // 创建玩家角色（战士职业）
    _player = PlayerCharacter::create(CharacterRole::WARRIOR, "Sprites/Characters/Player/Klee/defalt/spr_klee_run.png");

    if (!_player)
    {
        // 创建失败时显示占位符
        CCLOG("Failed to create player with sprite, creating placeholder");

        auto placeholder = DrawNode::create();
        placeholder->drawSolidRect(Vec2(-25, -40), Vec2(25, 40), Color4F::GREEN);
        placeholder->setPosition(startPos);
        this->addChild(placeholder, 5);

        auto label = Label::createWithTTF("玩家占位符\n(需要精灵帧)", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
        label->setPosition(startPos + Vec2(0, 60));
        this->addChild(label, 6);

        return;
    }

	    // 配置玩家位置和锚点
	    _player->setPosition(startPos);
	    _player->setAnchorPoint(Vec2(0.5f, 0.5f)); // 物理引擎要求锚点在中心
	    _player->setScale(0.5f);

    //-------------------------------------------------------------------------
    // 创建玩家物理刚体
    //-------------------------------------------------------------------------

    // 物理材质：密度=1.0, 弹性=0（不弹跳）, 摩擦=0（水平移动流畅）
    PhysicsMaterial playerMaterial(1.0f, 0.0f, 0.0f);

    // 计算碰撞体尺寸（略小于精灵以获得更好的游戏体验）
    Size playerSize = _player->getContentSize();
    float scale = _player->getScale();
    float boxWidth = playerSize.width * scale * 0.8f;    // 宽度80%
    float boxHeight = playerSize.height * scale * 0.95f; // 高度95%

    auto physicsBody = PhysicsBody::createBox(Size(boxWidth, boxHeight), playerMaterial);
    physicsBody->setDynamic(true);         // 动态刚体，受力影响
    physicsBody->setRotationEnable(false); // 禁止旋转
    physicsBody->setMass(1.0f);            // 质量1kg
    physicsBody->setLinearDamping(0.0f);   // 无线性阻尼

    // 配置碰撞掩码
    physicsBody->setCategoryBitmask(CATEGORY_PLAYER);
    physicsBody->setCollisionBitmask(CATEGORY_PLATFORM);
    physicsBody->setContactTestBitmask(CATEGORY_PLATFORM | CATEGORY_BOMB);

    _player->addComponent(physicsBody);
    this->addChild(_player, 5);

    // 设置死亡时不自动移除（由DebugScene控制重置）
    _player->setAutoRemoveOnDeath(false);

    // 初始化地面状态
    _isGrounded = true;
    _groundContactCount = 1;

    // 初始化玩家技能
    initPlayerSkills();

    CCLOG("Player created with physics at position (%.0f, %.0f)", startPos.x, startPos.y);
}

/**
 * @brief 初始化玩家技能
 *
 * 通过 SkillComponent 创建并装备主动技能。
 * 当前实现了炸弹技能作为示例。
 *
 * 技能系统工作流程：
 * 1. 创建技能数据（ActiveSkill）
 * 2. 学习技能（learnSkill）
 * 3. 装备到槽位（equipActiveSkill）
 * 4. 使用时通过 useActiveSkill() 触发
 */
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

    // 学习并装备技能
    skillComp->learnSkill(bombSkill);
    skillComp->equipActiveSkill(bombSkill, BOMB_SKILL_SLOT);

    CCLOG("Player skills initialized: Bomb skill equipped to slot %zu", BOMB_SKILL_SLOT);
}

/**
 * @brief 初始化测试用木桩
 *
 * 木桩是一个静态的攻击目标，用于测试伤害计算和攻击系统。
 * 包含：
 * - 木桩精灵
 * - 血条显示
 * - HP数值标签
 */
void DebugScene::initTargetDummy()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 创建木桩精灵
    _targetDummy.sprite = Sprite::create("Sprites/Enemies/YuanSheMenJiang/ysmj_stand.png");
    if (_targetDummy.sprite)
    {
        // 放置在场景右侧
        Vec2 dummyPos(origin.x + visibleSize.width * 0.75f, origin.y + GROUND_Y);
        _targetDummy.sprite->setPosition(dummyPos);
        _targetDummy.sprite->setAnchorPoint(Vec2(0.5f, 0)); // 锚点在脚底
        _targetDummy.sprite->setScale(0.5f);
        this->addChild(_targetDummy.sprite, 4);

        // 初始化血量（使用较大的值方便测试）
        _targetDummy.maxHP = 2147483674.0f;
        _targetDummy.currentHP = 2147483674.0f;

        // 创建血条绘制节点
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

/**
 * @brief 初始化调试UI面板
 *
 * 创建以下UI元素：
 * - 左上角：HP/MP进度条
 * - 左侧：角色属性信息面板
 * - 右侧：状态信息面板（状态机状态、状态效果、装备、被动技能、伤害日志）
 *
 * UI层级说明：
 * - z-order 9：进度条背景
 * - z-order 10：进度条填充和标签
 */
void DebugScene::initDebugUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    //=========================================================================
    // 左侧：属性信息面板
    //=========================================================================
    float panelX = origin.x + 20;
    float panelY = origin.y + visibleSize.height - 80;

    _infoLabel = Label::createWithTTF("加载中...", "fonts/ZCOOLKuaiLe-Regular.ttf", 18);
    _infoLabel->setAnchorPoint(Vec2(0, 1));
    _infoLabel->setPosition(Vec2(panelX, panelY));
    _infoLabel->setColor(Color3B::WHITE);
    this->addChild(_infoLabel, 10);

    //=========================================================================
    // 右侧：状态信息面板
    //=========================================================================
    float rightPanelX = origin.x + visibleSize.width - 250;

    // 状态机状态标签
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

    //=========================================================================
    // 左上角：HP/MP进度条
    //=========================================================================
    float barWidth = 200.0f;
    float barHeight = 18.0f;
    float barX = origin.x + 20;
    float barY = origin.y + visibleSize.height - 20;

    // HP标签（显示在进度条右侧）
    _hpLabel = Label::createWithTTF("HP: 0/0", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _hpLabel->setAnchorPoint(Vec2(0, 0.5f));
    _hpLabel->setPosition(Vec2(barX + barWidth + 10, barY - barHeight / 2));
    _hpLabel->setColor(Color3B::WHITE);
    this->addChild(_hpLabel, 10);

    // HP进度条背景（深红色）
    _hpBarBg = DrawNode::create();
    _hpBarBg->drawSolidRect(Vec2(0, 0), Vec2(barWidth, barHeight), Color4F(0.3f, 0.1f, 0.1f, 0.8f));
    _hpBarBg->setPosition(Vec2(barX, barY - barHeight));
    this->addChild(_hpBarBg, 9);

    // HP进度条填充（红色）
    _hpBarFill = DrawNode::create();
    _hpBarFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth, barHeight), Color4F(0.8f, 0.2f, 0.2f, 1.0f));
    _hpBarFill->setPosition(Vec2(barX, barY - barHeight));
    _hpBarFill->setTag(100);
    this->addChild(_hpBarFill, 10);

    // MP标签
    _mpLabel = Label::createWithTTF("MP: 0/0", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _mpLabel->setAnchorPoint(Vec2(0, 0.5f));
    _mpLabel->setPosition(Vec2(barX + barWidth + 10, barY - barHeight - 8 - barHeight / 2));
    _mpLabel->setColor(Color3B::WHITE);
    this->addChild(_mpLabel, 10);

    // MP进度条背景（深蓝色）
    _mpBarBg = DrawNode::create();
    _mpBarBg->drawSolidRect(Vec2(0, 0), Vec2(barWidth, barHeight), Color4F(0.1f, 0.1f, 0.3f, 0.8f));
    _mpBarBg->setPosition(Vec2(barX, barY - barHeight * 2 - 8));
    this->addChild(_mpBarBg, 9);

    // MP进度条填充（蓝色）
    _mpBarFill = DrawNode::create();
    _mpBarFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth, barHeight), Color4F(0.2f, 0.4f, 0.9f, 1.0f));
    _mpBarFill->setPosition(Vec2(barX, barY - barHeight * 2 - 8));
    _mpBarFill->setTag(101);
    this->addChild(_mpBarFill, 10);
}

/**
 * @brief 初始化控制按钮
 *
 * 创建4行功能按钮：
 * - 第1行：基础功能（受击、暴击、治疗、攻击、升级、重置、返回）
 * - 第2行：状态效果（中毒、亢奋、眩晕）
 * - 第3行：装备切换（剑、法杖、匕首、卸下）
 * - 第4行：被动技能（力量+5、防御+3、满血暴击）
 *
 * @note 按钮使用 MenuItemLabel，通过回调函数响应点击事件
 */
void DebugScene::initControlButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    //=========================================================================
    // 按钮配置数据结构
    //=========================================================================
    struct ButtonInfo
    {
        std::string label;                   ///< 按钮文本
        std::function<void(Ref *)> callback; ///< 点击回调
        Color3B color;                       ///< 按钮颜色
    };

    //-------------------------------------------------------------------------
    // 第1行：基础功能按钮
    //-------------------------------------------------------------------------
    std::vector<ButtonInfo> buttons = {
        {"受击 (10伤害)", CC_CALLBACK_1(DebugScene::onTakeDamageClicked, this), Color3B::RED},
        {"暴击 (25伤害)", CC_CALLBACK_1(DebugScene::onTakeCriticalDamageClicked, this), Color3B::ORANGE},
        {"治疗 (+20HP)", CC_CALLBACK_1(DebugScene::onHealClicked, this), Color3B::GREEN},
        {"攻击", CC_CALLBACK_1(DebugScene::onAttackClicked, this), Color3B::YELLOW},
        {"升级", CC_CALLBACK_1(DebugScene::onLevelUpClicked, this), Color3B::MAGENTA},
        {"重置角色", CC_CALLBACK_1(DebugScene::onResetClicked, this), Color3B::GRAY},
        {"返回地图", CC_CALLBACK_1(DebugScene::onBackClicked, this), Color3B::WHITE},
    };

    //-------------------------------------------------------------------------
    // 第2行：状态效果按钮
    //-------------------------------------------------------------------------
    std::vector<ButtonInfo> statusButtons = {
        {"中毒 (5秒)", CC_CALLBACK_1(DebugScene::onPoisonClicked, this), Color3B(148, 0, 211)},
        {"亢奋 (8秒)", CC_CALLBACK_1(DebugScene::onExcitedClicked, this), Color3B(255, 165, 0)},
        {"眩晕 (3秒)", CC_CALLBACK_1(DebugScene::onStunnedClicked, this), Color3B(70, 130, 180)},
    };

    //-------------------------------------------------------------------------
    // 第3行：装备按钮
    //-------------------------------------------------------------------------
    std::vector<ButtonInfo> equipButtons = {
        {"装备剑", CC_CALLBACK_1(DebugScene::onEquipSwordClicked, this), Color3B(192, 192, 192)},
        {"装备法杖", CC_CALLBACK_1(DebugScene::onEquipStaffClicked, this), Color3B(138, 43, 226)},
        {"装备匕首", CC_CALLBACK_1(DebugScene::onEquipDaggerClicked, this), Color3B(50, 205, 50)},
        {"卸下武器", CC_CALLBACK_1(DebugScene::onUnequipWeaponClicked, this), Color3B(128, 128, 128)},
    };

    //-------------------------------------------------------------------------
    // 第4行：被动技能按钮
    //-------------------------------------------------------------------------
    std::vector<ButtonInfo> passiveButtons = {
        {"力量+5", CC_CALLBACK_1(DebugScene::onLearnPassive1Clicked, this), Color3B(255, 100, 100)},
        {"防御+3", CC_CALLBACK_1(DebugScene::onLearnPassive2Clicked, this), Color3B(100, 100, 255)},
        {"满血暴击", CC_CALLBACK_1(DebugScene::onLearnPassive3Clicked, this), Color3B(255, 215, 0)},
    };

    //=========================================================================
    // 布局参数
    //=========================================================================
    float buttonWidth = 150.0f;
    float buttonHeight = 40.0f;
    float spacing = 10.0f;
    float startY = origin.y + 40;
    float centerX = origin.x + visibleSize.width / 2;

    //=========================================================================
    // 辅助Lambda：创建一行按钮
    //=========================================================================
    auto createButtonRow = [&](const std::vector<ButtonInfo> &buttonInfos, float rowY)
    {
        float totalWidth = buttonInfos.size() * buttonWidth + (buttonInfos.size() - 1) * spacing;
        float startX = centerX - totalWidth / 2 + buttonWidth / 2;

        for (size_t i = 0; i < buttonInfos.size(); ++i)
        {
            const auto &info = buttonInfos[i];

            auto button = MenuItemLabel::create(
                Label::createWithTTF(info.label, "fonts/ZCOOLKuaiLe-Regular.ttf", 16),
                info.callback);

            if (button)
            {
                button->setColor(info.color);
                button->setPosition(Vec2(startX + i * (buttonWidth + spacing), rowY));
            }

            auto menu = Menu::create(button, nullptr);
            menu->setPosition(Vec2::ZERO);
            this->addChild(menu, 20);
        }
    };

    // 创建各行按钮
    createButtonRow(buttons, startY);
    createButtonRow(statusButtons, startY + 30);
    createButtonRow(equipButtons, startY + 60);
    createButtonRow(passiveButtons, startY + 90);

    //=========================================================================
    // 快捷键提示
    //=========================================================================
    auto hintLabel = Label::createWithTTF(
        "[AD] 移动  [W/Space] 跳跃  [E] 丢炸弹  [4] 攻击  [R] 重置  [ESC] 返回",
        "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    hintLabel->setPosition(Vec2(centerX, origin.y + 160));
    hintLabel->setColor(Color3B(150, 150, 150));
    this->addChild(hintLabel, 10);
}

//=============================================================================
// 第2部分：主循环更新
//=============================================================================

/**
 * @brief 每帧更新函数
 *
 * 按顺序处理：
 * 1. 玩家移动（物理驱动）
 * 2. 更新UI显示
 * 3. 技能冷却
 * 4. 条件性被动技能检查
 * 5. 中毒持续伤害
 *
 * @param dt 距离上一帧的时间间隔（秒）
 */
void DebugScene::update(float dt)
{
    Scene::update(dt);

    //-------------------------------------------------------------------------
    // 死亡重置检测
    //-------------------------------------------------------------------------
    if (_player && _player->isDead())
    {
        if (!_isDeathResetPending)
        {
            // 首次检测到死亡，启动重置倒计时
            _isDeathResetPending = true;
            _deathResetTimer = DEATH_RESET_DELAY;
            addDamageLog(StringUtils::format("角色死亡! %.0f秒后自动重置...", DEATH_RESET_DELAY));
            CCLOG("Player died, will reset in %.1f seconds", DEATH_RESET_DELAY);
        }
        else
        {
            // 更新重置倒计时
            _deathResetTimer -= dt;
            if (_deathResetTimer <= 0.0f)
            {
                // 执行重置
                _isDeathResetPending = false;
                _deathResetTimer = 0.0f;
                onResetClicked(nullptr);
                CCLOG("Player auto-reset after death");
                return; // 重置后跳过本帧其他更新
            }
        }

        // 死亡状态下只更新UI，不处理其他逻辑
        updateDebugInfo();
        return;
    }

    // 如果之前在等待重置但玩家复活了（例如通过治疗），取消重置
    if (_isDeathResetPending && _player && !_player->isDead())
    {
        _isDeathResetPending = false;
        _deathResetTimer = 0.0f;
        addDamageLog("重置取消（角色复活）");
    }

    // 更新玩家移动（基于输入和物理）
    updatePlayerMovement(dt);

    // 更新调试UI显示
    updateDebugInfo();

    // 更新技能冷却时间
    if (_player)
    {
        auto skillComp = _player->getSkillComponent();
        if (skillComp)
        {
            skillComp->update(dt);
        }
    }

    // 更新条件性被动技能（如满血暴击）
    updateConditionalPassives();

    //-------------------------------------------------------------------------
    // 中毒持续伤害处理
    //-------------------------------------------------------------------------
    static float poisonTimer = 0.0f;
    if (_player && !_player->isDead())
    {
        auto attr = _player->getAttributeComponent();
        if (attr)
        {
            // 检查中毒状态
            _isPoisoned = attr->hasStatusEffect(StatusEffectType::POISONED);

            if (_isPoisoned)
            {
                poisonTimer += dt;
                if (poisonTimer >= 1.0f) // 每秒触发一次伤害
                {
                    applyPoisonDamage(dt);
                    poisonTimer = 0.0f;
                }
            }
            else
            {
                poisonTimer = 0.0f;
            }
        }
    }
}
/**
 * @brief 更新调试信息显示
 *
 * 刷新所有UI面板的显示内容：
 * - 角色属性面板
 * - 状态机状态
 * - 状态效果列表
 * - HP/MP进度条
 */
void DebugScene::updateDebugInfo()
{
    if (!_player)
        return;

    auto attr = _player->getAttributeComponent();
    auto sm = _player->getStateMachineComponent();

    //=========================================================================
    // 更新角色属性面板
    //=========================================================================
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

    //=========================================================================
    // 更新状态机状态显示
    //=========================================================================
    if (sm)
    {
        // 状态枚举转字符串
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

        // 根据状态设置标签颜色
        if (sm->getCurrentState() == CharacterState::DEAD)
            _stateLabel->setColor(Color3B::RED);
        else if (sm->getCurrentState() == CharacterState::HURT)
            _stateLabel->setColor(Color3B::ORANGE);
        else if (sm->getCurrentState() == CharacterState::ATTACKING)
            _stateLabel->setColor(Color3B::YELLOW);
        else
            _stateLabel->setColor(Color3B::GREEN);
    }

    //=========================================================================
    // 更新状态效果显示
    //=========================================================================
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

                // 状态类型转名称
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
                case StatusEffectType::FULL_HP_CRIT:
                    effectName = "满血暴击";
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

    //=========================================================================
    // 更新HP/MP进度条
    //=========================================================================
    if (attr)
    {
        float maxHP = attr->getAttributeValue(AttributeType::MAX_HP);
        float maxMP = attr->getAttributeValue(AttributeType::MAX_MP);
        float currentHP = std::max(0.0f, std::min(_player->getCurrentHP(), maxHP));
        float currentMP = std::max(0.0f, std::min(_player->getCurrentMP(), maxMP));

        float hpPercent = maxHP > 0 ? currentHP / maxHP : 0;
        float mpPercent = maxMP > 0 ? currentMP / maxMP : 0;

        // 更新标签文本
        if (_hpLabel)
            _hpLabel->setString(StringUtils::format("HP: %.0f/%.0f", currentHP, maxHP));
        if (_mpLabel)
            _mpLabel->setString(StringUtils::format("MP: %.0f/%.0f", currentMP, maxMP));

        // 更新进度条填充宽度
        float barWidth = 200.0f;
        float barHeight = 18.0f;

        if (_hpBarFill)
        {
            _hpBarFill->clear();
            _hpBarFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth * hpPercent, barHeight),
                                      Color4F(0.8f, 0.2f, 0.2f, 1.0f));
        }

        if (_mpBarFill)
        {
            _mpBarFill->clear();
            _mpBarFill->drawSolidRect(Vec2(0, 0), Vec2(barWidth * mpPercent, barHeight),
                                      Color4F(0.2f, 0.4f, 0.9f, 1.0f));
        }
    }
}

//=============================================================================
// 第3部分：UI按钮回调 - 基础功能
//=============================================================================

/**
 * @brief 受击按钮回调 - 模拟受到10点普通伤害
 * @param sender 按钮引用（未使用）
 */
void DebugScene::onTakeDamageClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    // 构造伤害信息
    DamageInfo info;
    info.amount = 10.0f;     // 基础伤害
    info.penetration = 0.0f; // 无护甲穿透
    info.isCritical = false; // 非暴击
    info.attacker = nullptr;

    float hpBefore = _player->getCurrentHP();
    _player->takeDamage(info);
    float hpAfter = _player->getCurrentHP();

    addDamageLog(StringUtils::format("普通伤害: %.0f -> %.0f (-%0.f)",
                                     hpBefore, hpAfter, hpBefore - hpAfter));

    CCLOG("Player took 10 damage, HP: %.0f", _player->getCurrentHP());
}

/**
 * @brief 暴击按钮回调 - 模拟受到25点暴击伤害
 * @param sender 按钮引用（未使用）
 */
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

/**
 * @brief 治疗按钮回调 - 恢复20点HP
 * @param sender 按钮引用（未使用）
 */
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

/**
 * @brief 攻击按钮回调 - 触发攻击动画和伤害判定
 * @param sender 按钮引用（未使用）
 */
void DebugScene::onAttackClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    // 动画互斥检查
    if (_isAttacking || _isCastingSkill)
        return;

    _isAttacking = true;
    _player->attackAnimated([this]()
                            { this->onAttackAnimationFinished(); });

    // 显示攻击信息
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

/**
 * @brief 升级按钮回调 - 增加经验值触发升级
 * @param sender 按钮引用（未使用）
 */
void DebugScene::onLevelUpClicked(Ref *sender)
{
    if (!_player)
        return;

    _player->addExperience(100 * _player->getLevel());
    addDamageLog(StringUtils::format("升级! 当前等级: %d", _player->getLevel()));

    CCLOG("Player leveled up to %d", _player->getLevel());
}

/**
 * @brief 重置按钮回调 - 销毁并重新创建玩家
 * @param sender 按钮引用（未使用）
 */
void DebugScene::onResetClicked(Ref *sender)
{
    if (_player)
    {
        // 停止所有动作（包括死亡闪烁动画）
        _player->stopAllActions();
        // 恢复颜色和透明度
        _player->setColor(Color3B::WHITE);
        _player->setOpacity(255);
        // 移除玩家
        _player->removeFromParent();
        _player = nullptr;
    }

    // 重置死亡相关状态
    _isDeathResetPending = false;
    _deathResetTimer = 0.0f;

    // 重置移动和战斗状态
    _isMovingLeft = false;
    _isMovingRight = false;
    _isAttacking = false;
    _isCastingSkill = false;

    initPlayer();
    _damageLog.clear();
    addDamageLog("角色已重置");

    CCLOG("Player reset");
}

/**
 * @brief 返回按钮回调 - 使用场景入栈方式返回地图
 * @param sender 按钮引用（未使用）
 *
 * @note 使用 pushScene 而非 replaceScene，以便支持场景返回
 */
void DebugScene::onBackClicked(Ref *sender)
{
    auto mapScene = MapScene::createScene();
    Director::getInstance()->pushScene(
        TransitionFade::create(0.5f, mapScene, Color3B::BLACK));
}

//=============================================================================
// 第4部分：状态效果系统
//=============================================================================

/**
 * @brief 中毒按钮回调 - 添加5秒中毒效果
 *
 * 中毒效果特点：
 * - 持续5秒
 * - 每秒造成5点真实伤害（无视防御）
 * - 显示绿色伤害飘字
 *
 * @param sender 按钮引用（未使用）
 */
void DebugScene::onPoisonClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto attr = _player->getAttributeComponent();
    if (!attr)
        return;

    // 创建中毒状态效果实例
    StatusEffectInstance poison;
    poison.type = StatusEffectType::POISONED;
    poison.duration = 5.0f;
    poison.elapsed = 0.0f;
    // 中毒不提供属性加成，持续伤害在 update() 中处理

    attr->addStatusEffect(poison);
    _isPoisoned = true;

    addDamageLog("中毒! 持续5秒");
    CCLOG("Poison effect applied for 5 seconds");
}

/**
 * @brief 亢奋按钮回调 - 添加8秒亢奋效果
 *
 * 亢奋效果特点：
 * - 持续8秒
 * - 移速+50
 * - 暴击率+10%
 * - 力量+10
 *
 * @param sender 按钮引用（未使用）
 */
void DebugScene::onExcitedClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto attr = _player->getAttributeComponent();
    if (!attr)
        return;

    StatusEffectInstance excited;
    excited.type = StatusEffectType::EXCITED;
    excited.duration = 8.0f;
    excited.elapsed = 0.0f;
    // 设置属性加成
    excited.attributeBonus.set(AttributeType::MOVE_SPEED, 50.0f);
    excited.attributeBonus.set(AttributeType::CRITICAL_RATE, 0.10f);
    excited.attributeBonus.set(AttributeType::STRENGTH, 10.0f);

    attr->addStatusEffect(excited);

    addDamageLog("亢奋! 移速+50, 暴击+10% (8秒)");
    CCLOG("Excited effect applied: +50 move speed, +10%% crit for 8 seconds");
}

/**
 * @brief 眩晕按钮回调 - 添加3秒眩晕效果
 *
 * 眩晕效果特点：
 * - 持续3秒
 * - 移速大幅降低（-200）
 *
 * @param sender 按钮引用（未使用）
 */
void DebugScene::onStunnedClicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto attr = _player->getAttributeComponent();
    if (!attr)
        return;

    StatusEffectInstance stunned;
    stunned.type = StatusEffectType::STUNNED;
    stunned.duration = 3.0f;
    stunned.elapsed = 0.0f;
    stunned.attributeBonus.set(AttributeType::MOVE_SPEED, -200.0f);

    attr->addStatusEffect(stunned);

    addDamageLog("眩晕! 无法移动 (3秒)");
    CCLOG("Stunned effect applied for 3 seconds");
}

/**
 * @brief 应用中毒持续伤害
 *
 * 由 update() 每秒调用一次，造成5点真实伤害并显示飘字。
 *
 * @param dt 时间间隔（未使用）
 */
void DebugScene::applyPoisonDamage(float dt)
{
    if (!_player || _player->isDead() || !_isPoisoned)
        return;

    // 中毒伤害：5点/秒，穿透所有防御
    DamageInfo poisonDamage;
    poisonDamage.amount = 5.0f;
    poisonDamage.penetration = 1000.0f; // 高穿透 = 真实伤害
    poisonDamage.isCritical = false;
    poisonDamage.attacker = nullptr;

    _player->takeDamage(poisonDamage);

    // 显示绿色中毒伤害飘字
    auto damageLabel = Label::createWithTTF(
        StringUtils::format("-%.0f", poisonDamage.amount),
        "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
    damageLabel->setColor(Color3B(0, 200, 0));
    damageLabel->setPosition(_player->getPosition() + Vec2(rand() % 30 - 15, 50));
    this->addChild(damageLabel, 100);

    // 飘字动画：上移 + 淡出
    auto moveUp = MoveBy::create(0.8f, Vec2(0, 30));
    auto fadeOut = FadeOut::create(0.8f);
    auto spawn = Spawn::create(moveUp, fadeOut, nullptr);
    auto remove = RemoveSelf::create();
    damageLabel->runAction(Sequence::create(spawn, remove, nullptr));
}

//=============================================================================
// 第7部分：输入处理
//=============================================================================

/**
 * @brief 键盘按下事件处理
 *
 * 按键映射：
 * - A/左箭头：向左移动
 * - D/右箭头：向右移动
 * - W/上箭头/空格：跳跃
 * - E：释放炸弹技能
 * - 1-5：功能测试快捷键
 * - R：重置角色
 * - ESC：返回地图
 *
 * @param keyCode 按键代码
 * @param event 事件对象（未使用）
 */
void DebugScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event *event)
{
    switch (keyCode)
    {
    //-------------------------------------------------------------------------
    // 移动按键
    //-------------------------------------------------------------------------
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _isMovingLeft = true;
        if (_player)
            _player->setFlippedX(true); // 朝左
        if (!_isAttacking && !_isCastingSkill && _player)
            _player->setMoving(true, _isRunPressed);
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = true;
        if (_player)
            _player->setFlippedX(false); // 朝右
        if (!_isAttacking && !_isCastingSkill && _player)
            _player->setMoving(true, _isRunPressed);
        break;

    case EventKeyboard::KeyCode::KEY_SHIFT:
    case EventKeyboard::KeyCode::KEY_RIGHT_SHIFT:
        _isRunPressed = true;
        if ((_isMovingLeft || _isMovingRight) && !_isAttacking && !_isCastingSkill && _player)
        {
            _player->setMoving(true, true);
        }
        break;

    //-------------------------------------------------------------------------
    // 跳跃按键
    //-------------------------------------------------------------------------
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
    case EventKeyboard::KeyCode::KEY_SPACE:
        jump();
        break;

    //-------------------------------------------------------------------------
    // 技能按键
    //-------------------------------------------------------------------------
    case EventKeyboard::KeyCode::KEY_E:
        throwBomb();
        break;

    //-------------------------------------------------------------------------
    // 功能快捷键
    //-------------------------------------------------------------------------
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

/**
 * @brief 键盘释放事件处理
 *
 * 主要处理移动按键的释放，停止对应方向的移动。
 * 当所有移动按键都释放时，停止行走动画。
 *
 * @param keyCode 按键代码
 * @param event 事件对象（未使用）
 */
void DebugScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_SHIFT:
    case EventKeyboard::KeyCode::KEY_RIGHT_SHIFT:
        _isRunPressed = false;
        if ((_isMovingLeft || _isMovingRight) && !_isAttacking && !_isCastingSkill && _player)
        {
            _player->setMoving(true, false);
        }
        break;

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

    // 所有方向键释放时停止动画（避免打断攻击/技能）
    if (!_isMovingLeft && !_isMovingRight && !_isAttacking && !_isCastingSkill && _player)
    {
        _player->setMoving(false);
    }
}

/**
 * @brief 更新玩家移动
 *
 * 通过修改物理刚体的水平速度来实现移动。
 * 移动速度受角色属性（MOVE_SPEED）影响，可被状态效果修改。
 *
 * @param dt 时间间隔（未使用）
 */
void DebugScene::updatePlayerMovement(float dt)
{
    if (!_player || _player->isDead())
        return;

    auto physicsBody = _player->getPhysicsBody();
    if (!physicsBody)
        return;

    // 从属性组件获取当前移动速度
    float currentMoveSpeed = _moveSpeed;
    auto attr = _player->getAttributeComponent();
    if (attr)
    {
        currentMoveSpeed = attr->getAttributeValue(AttributeType::MOVE_SPEED);
    }
    if (_isRunPressed)
    {
        currentMoveSpeed *= 1.6f;
    }

    // 保持垂直速度（重力效果）
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

    // 设置速度（保持垂直分量）
    physicsBody->setVelocity(Vec2(velocityX, currentVelocity.y));

    //-------------------------------------------------------------------------
    // 屏幕边界限制
    //-------------------------------------------------------------------------
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

//=============================================================================
// 第5部分：装备系统
//=============================================================================

/**
 * @brief 初始化测试用装备
 *
 * 创建三种不同类型的武器用于测试：
 * - 铁剑(SWORD)：高伤害，普通攻速
 * - 魔法杖(STAFF)：中伤害，远距离，MP加成
 * - 锋利匕首(DAGGER)：低伤害，快攻速，高暴击
 */
void DebugScene::initEquipments()
{
    //-------------------------------------------------------------------------
    // 创建剑武器
    //-------------------------------------------------------------------------
    _swordWeapon = std::make_shared<Weapon>();
    _swordWeapon->id = 1;
    _swordWeapon->name = "铁剑";
    _swordWeapon->description = "一把普通的铁剑";
    _swordWeapon->type = WeaponType::SWORD;
    _swordWeapon->attackDamage = 15.0f;
    _swordWeapon->attackRange = 50.0f;
    _swordWeapon->attackSpeed = 1.0f;
    _swordWeapon->attackAnimationPrefix = "spr_klee_attack";
    _swordWeapon->attackFrameCount = 3;
    _swordWeapon->attributeBonus.set(AttributeType::STRENGTH, 5.0f);
    _swordWeapon->spritePath = "default";

    //-------------------------------------------------------------------------
    // 创建法杖武器
    //-------------------------------------------------------------------------
    _staffWeapon = std::make_shared<Weapon>();
    _staffWeapon->id = 2;
    _staffWeapon->name = "魔法杖";
    _staffWeapon->description = "蕴含魔力的法杖";
    _staffWeapon->type = WeaponType::STAFF;
    _staffWeapon->attackDamage = 8.0f;
    _staffWeapon->attackRange = 100.0f;
    _staffWeapon->attackSpeed = 0.8f;
    _staffWeapon->attackAnimationPrefix = "spr_klee_attack";
    _staffWeapon->attackFrameCount = 3;
    _staffWeapon->attributeBonus.set(AttributeType::MAX_MP, 20.0f);
    _staffWeapon->attributeBonus.set(AttributeType::CRITICAL_RATE, 0.05f);
    _staffWeapon->spritePath = "default";

    //-------------------------------------------------------------------------
    // 创建匕首武器
    //-------------------------------------------------------------------------
    _daggerWeapon = std::make_shared<Weapon>();
    _daggerWeapon->id = 3;
    _daggerWeapon->name = "锋利匕首";
    _daggerWeapon->description = "快速但短距离的匕首";
    _daggerWeapon->type = WeaponType::DAGGER;
    _daggerWeapon->attackDamage = 10.0f;
    _daggerWeapon->attackRange = 30.0f;
    _daggerWeapon->attackSpeed = 1.5f;
    _daggerWeapon->attackAnimationPrefix = "spr_klee_attack";
    _daggerWeapon->attackFrameCount = 3;
    _daggerWeapon->attributeBonus.set(AttributeType::CRITICAL_RATE, 0.15f);
    _daggerWeapon->attributeBonus.set(AttributeType::MOVE_SPEED, 20.0f);
    _daggerWeapon->spritePath = "default";

    //-------------------------------------------------------------------------
    // 设置装备变更回调
    //-------------------------------------------------------------------------
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

/**
 * @brief 装备变更回调处理
 *
 * 当玩家装备/卸下武器时被调用，更新UI显示。
 *
 * @param slot 装备槽位
 * @param equipment 新装备（nullptr表示卸下）
 */
void DebugScene::onEquipmentChanged(EquipmentSlot slot, const std::shared_ptr<Equipment> &equipment)
{
    if (slot != EquipmentSlot::WEAPON)
        return;

    if (equipment)
    {
        auto weapon = std::dynamic_pointer_cast<Weapon>(equipment);
        if (weapon)
        {
            // 武器类型转换为显示文本
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
                _equipmentLabel->setString(StringUtils::format(
                    "武器: %s (%s)\n攻击力+%.0f 范围%.0f",
                    weapon->name.c_str(), weaponTypeStr.c_str(),
                    weapon->attackDamage, weapon->attackRange));
            }

            addDamageLog(StringUtils::format("装备: %s", weapon->name.c_str()));
        }
    }
    else
    {
        if (_equipmentLabel)
            _equipmentLabel->setString("武器: 无");
        addDamageLog("卸下武器");
    }
}

// 装备按钮回调
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

//=============================================================================
// 第6部分：被动技能系统
//=============================================================================

/**
 * @brief 初始化被动技能
 *
 * 创建三种被动技能用于测试：
 * - 力量精通：永久+5力量
 * - 铁壁：永久+3防御
 * - 满血暴击：条件性被动，血量满时暴击率+25%
 */
void DebugScene::initPassiveSkills()
{
    // 被动技能1：力量提升（常规被动）
    _passiveSkill1 = std::make_shared<PassiveSkill>();
    _passiveSkill1->id = 2001;
    _passiveSkill1->name = "力量精通";
    _passiveSkill1->description = "永久增加5点力量";
    _passiveSkill1->attributeBonus.set(AttributeType::STRENGTH, 5.0f);

    // 被动技能2：防御提升（常规被动）
    _passiveSkill2 = std::make_shared<PassiveSkill>();
    _passiveSkill2->id = 2002;
    _passiveSkill2->name = "铁壁";
    _passiveSkill2->description = "永久增加3点防御";
    _passiveSkill2->attributeBonus.set(AttributeType::DEFENSE, 3.0f);

    // 被动技能3：满血暴击（条件性被动）
    _passiveSkill3 = std::make_shared<PassiveSkill>();
    _passiveSkill3->id = 2003;
    _passiveSkill3->name = "满血暴击";
    _passiveSkill3->description = "血量满时暴击率+25%";
    // 注意：条件性被动不设置固定属性加成，由 updateConditionalPassives() 动态处理

    CCLOG("Passive skills initialized: 3 skills created");
}

// 被动技能按钮回调
void DebugScene::onLearnPassive1Clicked(Ref *sender)
{
    if (!_player || _player->isDead())
        return;

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
        return;

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

    // 条件性被动只学习，不通过 equipPassiveSkill 添加固定属性
    skillComp->learnSkill(_passiveSkill3);
    _hasFullHpCritPassive = true;

    addDamageLog(StringUtils::format("学习被动: %s (满血时暴击+25%%)", _passiveSkill3->name.c_str()));
    updatePassiveSkillLabel();
    CCLOG("Learned passive skill: %s", _passiveSkill3->name.c_str());
}

/**
 * @brief 更新条件性被动技能效果
 *
 * 检查条件性被动的触发条件，动态添加/移除效果。
 * 当前实现：满血暴击 - 血量满时通过状态效果添加25%暴击率。
 */
void DebugScene::updateConditionalPassives()
{
    if (!_player || _player->isDead())
        return;

    auto attr = _player->getAttributeComponent();
    if (!attr)
        return;

    // 满血暴击被动检查
    if (_hasFullHpCritPassive)
    {
        float currentHP = _player->getCurrentHP();
        float maxHP = attr->getAttributeValue(AttributeType::MAX_HP);
        bool isFullHP = (currentHP >= maxHP - 0.01f); // 允许微小误差

        if (isFullHP && !_isFullHpCritActive)
        {
            // 激活效果：通过状态效果系统添加暴击加成
            StatusEffectInstance fullHpCrit;
            fullHpCrit.type = StatusEffectType::FULL_HP_CRIT;
            fullHpCrit.duration = 999999.0f; // 超长持续时间（由代码控制）
            fullHpCrit.elapsed = 0.0f;
            fullHpCrit.attributeBonus.set(AttributeType::CRITICAL_RATE, 0.25f);

            attr->addStatusEffect(fullHpCrit);
            _isFullHpCritActive = true;

            addDamageLog("满血暴击激活! 暴击+25%");
            CCLOG("Full HP Crit passive activated!");
        }
        else if (!isFullHP && _isFullHpCritActive)
        {
            // 失效：标记状态，等待状态效果系统自然更新
            _isFullHpCritActive = false;
            addDamageLog("满血暴击失效!");
            CCLOG("Full HP Crit passive deactivated!");
        }
    }
}

/**
 * @brief 更新被动技能UI显示
 */
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

    std::string passiveText = "被动技能:\n";
    bool hasPassive = false;

    // 显示已装备的常规被动
    for (size_t i = 0; i < passiveSlots.size(); ++i)
    {
        if (passiveSlots[i])
        {
            passiveText += "  - " + passiveSlots[i]->name + "\n";
            hasPassive = true;
        }
    }

    // 显示条件性被动
    if (_hasFullHpCritPassive)
    {
        std::string status = _isFullHpCritActive ? "(激活中)" : "(未激活)";
        passiveText += "  - 满血暴击 " + status + "\n";
        hasPassive = true;
    }

    _passiveSkillLabel->setString(hasPassive ? passiveText : "被动技能: 无");
}

/**
 * @brief 添加伤害日志
 *
 * 将日志添加到列表并更新UI显示。
 * 日志数量超过 MAX_LOG_LINES 时自动移除最旧的条目。
 *
 * @param log 日志内容
 */
void DebugScene::addDamageLog(const std::string &log)
{
    _damageLog.push_back(log);

    // 限制日志行数
    while (_damageLog.size() > MAX_LOG_LINES)
    {
        _damageLog.erase(_damageLog.begin());
    }

    // 更新显示
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

//=============================================================================
// 第9部分：攻击与技能系统
//=============================================================================

/**
 * @brief 攻击动画结束回调
 *
 * 执行伤害判定逻辑，包括：
 * - 距离检测（武器攻击范围 + 目标碰撞半径）
 * - 伤害计算（武器伤害 + 力量加成）
 * - 暴击判定
 * - 武器特殊效果（法杖消耗MP、匕首背刺）
 */
void DebugScene::onAttackAnimationFinished()
{
    _isAttacking = false;

    //-------------------------------------------------------------------------
    // 伤害判定
    //-------------------------------------------------------------------------
    if (_player && _targetDummy.sprite && _targetDummy.currentHP > 0)
    {
        Vec2 playerPos = _player->getPosition();
        Vec2 dummyPos = _targetDummy.sprite->getPosition();
        float distance = playerPos.distance(dummyPos);

        // 获取武器属性
        auto weapon = _player->getEquippedWeapon();
        float attackRange = weapon ? weapon->attackRange : 60.0f;
        float weaponDamage = weapon ? weapon->attackDamage : 5.0f;

        // 获取角色属性
        auto attr = _player->getAttributeComponent();
        float strength = attr ? attr->getAttributeValue(AttributeType::STRENGTH) : 10.0f;
        float critRate = attr ? attr->getAttributeValue(AttributeType::CRITICAL_RATE) : 0.1f;

        // 计算木桩碰撞半径
        Size dummySize = _targetDummy.sprite->getContentSize();
        float dummyScale = _targetDummy.sprite->getScale();
        float dummyHitRadius = dummySize.width * dummyScale;

        // 判定攻击范围
        float totalHitRange = attackRange + dummyHitRadius;
        if (distance <= totalHitRange)
        {
            // 计算伤害：武器伤害 + 力量 × 1.5
            float baseDamage = weaponDamage + strength * 1.5f;

            // 暴击判定
            bool isCrit = (static_cast<float>(rand()) / RAND_MAX) < critRate;
            float finalDamage = isCrit ? baseDamage * 1.5f : baseDamage;

            // 武器特殊效果
            WeaponType weaponType = _player->getCurrentWeaponType();
            switch (weaponType)
            {
            case WeaponType::SWORD:
                // 剑：无特殊效果
                break;

            case WeaponType::STAFF:
                // 法杖：消耗5MP增加50%伤害
                if (_player->getCurrentMP() >= 5.0f)
                {
                    _player->setCurrentMP(_player->getCurrentMP() - 5.0f);
                    finalDamage *= 1.5f;
                    addDamageLog("法杖魔力攻击! (消耗5MP)");
                }
                break;

            case WeaponType::DAGGER:
                // 匕首：20%额外暴击几率（背刺）
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

    //-------------------------------------------------------------------------
    // 恢复角色状态
    //-------------------------------------------------------------------------
    if (_player)
    {
        _player->setMoving(_isMovingLeft || _isMovingRight, _isRunPressed);
    }

    CCLOG("Attack animation finished");
}

//=============================================================================
// 第9部分：物理碰撞系统 (Physics Collision System)
//=============================================================================
// 本部分处理 cocos2d-x 物理引擎的碰撞检测逻辑，主要包括：
// 1. 玩家与平台的碰撞 - 用于判定角色是否在地面上
// 2. 炸弹与平台的碰撞 - 触发炸弹爆炸
// 3. 碰撞分离检测 - 判断角色离开平台进入空中
//
// 碰撞类别(Category)使用位掩码区分：
// - CATEGORY_PLAYER (0x01)   : 玩家
// - CATEGORY_PLATFORM (0x02) : 平台/地面
// - CATEGORY_BOMB (0x04)     : 炸弹
// - CATEGORY_ENEMY (0x08)    : 敌人
//=============================================================================

/**
 * @brief 初始化物理碰撞监听器
 *
 * 创建物理接触事件监听器，设置三个关键回调：
 * - onContactBegin: 碰撞开始时调用
 * - onContactPreSolve: 碰撞处理前调用，可修改碰撞参数
 * - onContactSeparate: 碰撞分离时调用
 *
 * @note 该函数在场景初始化时调用，必须在物理世界创建后执行
 */
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

/**
 * @brief 碰撞开始回调函数
 *
 * 当两个物理刚体开始接触时调用。主要处理：
 * 1. 玩家落地检测 - 通过碰撞法向量判断是否从上方落下
 * 2. 炸弹碰撞平台 - 触发炸弹爆炸效果
 *
 * @param contact 物理接触对象，包含碰撞双方的信息
 * @return true 允许碰撞响应，false 忽略此次碰撞
 *
 * @note 使用 _groundContactCount 引用计数处理多平台接触
 */
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
            _jumpCount = 0; // 落地后重置二段跳计数
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

/**
 * @brief 碰撞分离回调函数
 *
 * 当两个物理刚体分离时调用。主要用于：
 * - 检测玩家离开平台，进入空中状态
 * - 使用引用计数确保只有完全离开所有平台才设为空中状态
 *
 * @param contact 物理接触对象
 */
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

//=============================================================================
// 第10部分：跳跃与技能系统 (Jump & Skill System)
//=============================================================================
// 本部分包含角色的跳跃机制和技能释放逻辑：
// 1. 跳跃 - 基于物理引擎的冲量跳跃
// 2. 技能释放 - 通过技能组件管理 MP 消耗和冷却
// 3. 技能动画 - 播放施法动画后执行技能效果
//
// 技能系统常量：
// - BOMB_SKILL_SLOT (0)    : 炸弹技能的槽位索引
// - BOMB_SKILL_ID (1001)   : 炸弹技能的唯一标识
// - BOMB_SKILL_MP_COST (10): 释放技能的 MP 消耗
// - BOMB_SKILL_COOLDOWN (1): 技能冷却时间（秒）
// - JUMP_IMPULSE (350)     : 跳跃冲量值
//=============================================================================

/**
 * @brief 执行跳跃动作
 *
 * 使用物理引擎的冲量(Impulse)实现跳跃，只有在地面上时才能跳跃。
 * 跳跃冲量会考虑角色质量，确保跳跃高度一致。
 *
 * @note 跳跃后立即设置 _isGrounded = false，防止连跳
 */
void DebugScene::jump()
{
    if (!_player || _player->isDead())
        return;

    if (_isGrounded)
    {
        _jumpCount = 0;
    }

    if (_jumpCount >= 2)
        return;

    // 主动跳跃时清空地面接触计数，避免空中误判为落地
    _groundContactCount = 0;

    auto physicsBody = _player->getPhysicsBody();
    if (physicsBody)
    {
        Vec2 velocity = physicsBody->getVelocity();
        velocity.y = 0.0f;
        physicsBody->setVelocity(velocity);

        // 使用物理引擎的冲量实现跳跃
        physicsBody->applyImpulse(Vec2(0, JUMP_IMPULSE * physicsBody->getMass()));
    }

    _isGrounded = false;
    _jumpCount++;
    addDamageLog(_jumpCount == 1 ? "跳跃!" : "二段跳!");
    CCLOG(_jumpCount == 1 ? "Player jumped with impulse" : "Player double jumped with impulse");
}

/**
 * @brief 释放炸弹技能
 *
 * 通过技能组件检查并释放炸弹技能，流程如下：
 * 1. 检查角色状态（死亡、正在攻击、正在施法则不可释放）
 * 2. 调用 SkillComponent::useActiveSkill() 检查 MP 和冷却
 * 3. 如果条件满足，播放技能动画
 * 4. 动画结束后实际创建炸弹
 *
 * @note 使用 _isCastingSkill 标记防止技能打断
 */
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
    _isCastingSkill = true;
    _player->castSkillAnimated([this]()
                               { this->onSkillAnimationFinished(); });
    addDamageLog(StringUtils::format("施放技能: 丢炸弹! (消耗 %.0f MP)", BOMB_SKILL_MP_COST));
    CCLOG("Skill started: Throw Bomb");
}

/**
 * @brief 技能动画播放完成回调
 *
 * 当技能施法动画播放完毕后调用，执行以下操作：
 * 1. 重置 _isCastingSkill 标记
 * 2. 调用 doThrowBomb() 实际创建并投掷炸弹
 * 3. 根据移动状态恢复行走动画或静止贴图
 *
 * @note 保持角色朝向不变
 */
void DebugScene::onSkillAnimationFinished()
{
    _isCastingSkill = false;

    // 动画结束后实际丢出炸弹
    doThrowBomb();

    if (_player)
    {
        _player->setMoving(_isMovingLeft || _isMovingRight, _isRunPressed);
    }

    CCLOG("Skill animation finished");
}

//=============================================================================
// 第11部分：炸弹物理系统 (Bomb Physics System)
//=============================================================================
// 本部分处理炸弹的创建、物理模拟和爆炸效果：
// 1. 炸弹创建 - 带物理刚体的抛物线投掷
// 2. 爆炸效果 - 视觉特效和范围伤害计算
//
// 炸弹物理参数：
// - BOMB_THROW_SPEED_X (300) : 水平抛出速度
// - BOMB_THROW_SPEED_Y (350) : 垂直抛出速度
// - BOMB_DAMAGE (150)        : 基础爆炸伤害
// - BOMB_EXPLOSION_RADIUS (80): 爆炸范围半径
//=============================================================================

/**
 * @brief 实际创建并投掷炸弹
 *
 * 创建带有物理刚体的炸弹精灵，并施加初始冲量实现抛物线轨迹。
 *
 * 物理设置：
 * - 使用圆形碰撞体(半径15)
 * - 碰撞类别: CATEGORY_BOMB
 * - 只与 CATEGORY_PLATFORM 产生碰撞
 * - 允许旋转，模拟真实物理效果
 *
 * @note 炸弹方向由角色当前朝向决定
 */
void DebugScene::doThrowBomb()
{
    if (!_player || _player->isDead())
        return;

    // 创建炸弹精灵
    auto bombSprite = Sprite::create("Sprites/Characters/Player/Klee/defalt/TNT.png");
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

/**
 * @brief 处理炸弹爆炸
 *
 * 当炸弹碰撞平台时触发爆炸，执行以下操作：
 * 1. 移除炸弹精灵
 * 2. 在爆炸位置创建爆炸视觉特效（放大+淡出）
 * 3. 检测爆炸范围内的目标并计算伤害
 * 4. 伤害公式: (BOMB_DAMAGE + 力量*5) × 暴击倍率
 *
 * @param bomb 要爆炸的炸弹对象引用
 *
 * @note 暴击率从角色属性组件读取
 */
void DebugScene::explodeBomb(Bomb &bomb)
{
    if (!bomb.sprite)
        return;

    Vec2 explodePos = bomb.sprite->getPosition();

    // 移除炸弹精灵
    bomb.sprite->removeFromParent();

    // 创建爆炸效果
    auto boomSprite = Sprite::create("Sprites/Characters/Player/Klee/defalt/BOOM_1.png");
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
        Size dummySize = _targetDummy.sprite->getContentSize();       // 获取木桩大小
        float distance = explodePos.distance(dummyPos);               // 计算与木桩的距离
        float dummyScale = _targetDummy.sprite->getScale();           // 获取木桩缩放比例
        float dummyHitRadius = (dummySize.width * dummyScale) * 0.5f; // 木桩的碰撞半径
        if (distance - dummyHitRadius <= BOMB_EXPLOSION_RADIUS)
        {
            // 从角色属性组件获取暴击率和攻击力
            float critRate = 0.0f;          // 默认暴击率 0%
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

//=============================================================================
// 第12部分：伤害显示系统 (Damage Display System)
//=============================================================================
// 本部分负责目标受击时的视觉反馈，包括：
// 1. 伤害数字飘字 - 在目标位置显示伤害值
// 2. 血条更新 - 实时更新目标血量显示
// 3. 受击特效 - 目标精灵闪红效果
//=============================================================================

/**
 * @brief 对目标造成伤害
 *
 * 处理伤害结算的核心函数，执行以下操作：
 * 1. 扣除目标 HP（不会低于0）
 * 2. 在目标位置显示伤害飘字
 * 3. 更新血条显示
 * 4. 播放受击闪红特效
 * 5. 添加伤害日志
 *
 * @param damage 造成的伤害值
 * @param isCrit 是否暴击（影响显示样式）
 */
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

/**
 * @brief 显示伤害飘字
 *
 * 在指定位置创建伤害数字标签，带有飘字动画效果。
 *
 * 样式设置：
 * - 暴击: 红色(255,50,50)，字号28，显示"暴击 xxx!"
 * - 普通: 黄色(255,200,50)，字号22
 * - 黑色描边增强可读性
 *
 * 动画效果：
 * - 向上飘动 60 像素
 * - 持续 0.8 秒
 * - 淡出后自动移除
 *
 * @param pos 显示位置（通常为目标头顶）
 * @param damage 伤害数值
 * @param isCrit 是否暴击
 */
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

/**
 * @brief 更新目标血条显示
 *
 * 使用 DrawNode 重新绘制目标的血条 UI。
 *
 * 血条配置：
 * - 宽度: 60 像素
 * - 高度: 8 像素
 * - 位置: 目标头顶上方 75 像素
 *
 * 血条颜色随血量变化：
 * - HP > 50%: 绿色
 * - HP > 25%: 黄色
 * - HP <= 25%: 红色（危险状态）
 */
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
