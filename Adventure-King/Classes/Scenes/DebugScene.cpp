/**
 * @file DebugScene.cpp
 * @brief 角色功能调试场景实现
 *
 * 本文件实现了 DebugScene 类的所有功能，可作为正式关卡开发的参考模板。
 *
 * @note 文件结构说明（主要模块）：
 * - 场景初始化（init 系列函数）
 * - 主循环更新（update 系列函数）
 * - UI按钮回调（基础/状态/装备/被动）
 * - 输入处理：委托 GameInputController（与 GameScene 同链路）
 * - 碰撞/命中结算：委托 CombatContactHelper（与 GameScene 同链路）
 *
 * @author Adventure-King Team
 * @version 1.0
 */

#include "DebugScene.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/Base/CharacterBase.h"
#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/Monster/Monsters/GobluMonster.h"
#include "Character/Monster/Monsters/TrainingDummyMonster.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include "Configs/GamePhysicsCategory.h"
#include "Managers/SceneRegistry.h"
#include "MapScene.h"
#include "Save/SaveData.h"
#include "Save/SaveManager.h"
#include "Scenes/GameScene.h"
#include "Scenes/GameInputController.h"
#include "Scenes/GameUIController.h"
#include "Scenes/CombatContactHelper.h"
#include "GameUI.h"
#include "Scenes/LevelScenes/MysteryForestScene.h"
#include "Scenes/LevelScenes/OriginMushroomScene.h"
#include "2d/CCTransition.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;
using namespace cocos2d::ui;

//=============================================================================
// 第1部分：场景初始化
//=============================================================================

Scene *DebugScene::createScene()
{
    return DebugScene::create();
}

DebugScene::DebugScene() = default;

DebugScene::~DebugScene() = default;

void DebugScene::setupRegistry()
{
    SceneInfo info;
    info.creator = []()
    { return DebugScene::createScene(); };

    // DebugScene 主要用于功能验证，资源依赖较分散；这里保持最小注册即可。
    // 若后续需要进一步降低首次进入卡顿，可逐步补齐 imagePaths。
    info.imagePaths = {};

    SceneRegistry::getInstance()->registerScene(99, info);
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
    const float gravityY = LevelConfig{}.gravity; // 关卡默认重力（见 Configs/GameSceneConfig.h）
    physicsWorld->setGravity(Vec2(0, gravityY));

    // 开启物理调试绘制（开发时可视化碰撞体，发布时应关闭）
    physicsWorld->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);

    //-------------------------------------------------------------------------
    // 步骤1.5：创建游戏内容层（用于暂停时冻结世界，避免影响 UI）
    //-------------------------------------------------------------------------
    _gameLayer = Node::create();
    addChild(_gameLayer, 0);

    //-------------------------------------------------------------------------
    // 步骤2：初始化场景元素
    //-------------------------------------------------------------------------
    initBackground();             // 背景和网格线
    initPlatforms();              // 平台和地面（物理刚体）
    initPlayer();                 // 玩家角色（物理刚体）
    initGameUIController();       // 与 GameScene 同款 UI（暂停/背包/技能栏等）
    initInputController();        // 与 GameScene 同款输入（移动/跳跃/攻击/技能）
    initEquipments();             // 装备系统
    initPassiveSkills();          // 被动技能系统
    initTestMonsters();           // 测试用怪物（验证命中/受击/状态效果）
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
    if (_gameLayer)
    {
        _gameLayer->addChild(background, -1);
    }
    else
    {
        this->addChild(background, -1);
    }

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
    if (_gameLayer)
    {
        _gameLayer->addChild(drawNode, 0);
    }
    else
    {
        this->addChild(drawNode, 0);
    }

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
        physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLATFORM));
        physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLAYER |
                                               GamePhysicsCategory::BOMB |
                                               GamePhysicsCategory::MONSTER));
        physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER |
                                                 GamePhysicsCategory::BOMB |
                                                 GamePhysicsCategory::MONSTER));

        platformNode->addComponent(physicsBody);
        if (_gameLayer)
        {
            _gameLayer->addChild(platformNode, 1);
        }
        else
        {
            this->addChild(platformNode, 1);
        }

        _platforms.push_back(rect);
    };

    // 创建地面平台（覆盖屏幕底部）
    Rect groundPlatform(origin.x, origin.y + GROUND_Y - 20, visibleSize.width, 20);
    createPlatform(groundPlatform, Color4F(0.4f, 0.3f, 0.2f, 1.0f));

    // 可选：创建浮空平台（取消注释以启用）
    // Rect leftPlatform(origin.x + 50, origin.y + 200, 200, 20);
    // createPlatform(leftPlatform, Color4F(0.5f, 0.4f, 0.3f, 1.0f));

    if (_gameLayer)
    {
        _gameLayer->addChild(platformDraw, 1);
    }
    else
    {
        this->addChild(platformDraw, 1);
    }

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

    // 玩家初始位置：屏幕中央、地面上方（用于创建失败占位符；创建成功后会按角色高度修正 Y）
    Vec2 startPos(origin.x + visibleSize.width * 0.5f, origin.y + GROUND_Y + 40.0f);

    // 创建玩家角色（战士职业）
    _player = PlayerCharacter::create(CharacterRole::WARRIOR, "Sprites/Characters/Player/Klee/default/spr_klee_run.png");

    if (!_player)
    {
        // 创建失败时显示占位符
        CCLOG("Failed to create player with sprite, creating placeholder");

        auto placeholder = DrawNode::create();
        placeholder->drawSolidRect(Vec2(-25, -40), Vec2(25, 40), Color4F::GREEN);
        placeholder->setPosition(startPos);
        if (_gameLayer)
        {
            _gameLayer->addChild(placeholder, 4);
        }
        else
        {
            this->addChild(placeholder, 4);
        }

        auto label = Label::createWithTTF("玩家占位符\n(需要精灵帧)", "fonts/ZCOOLKuaiLe-Regular.ttf", 16);
        label->setPosition(startPos + Vec2(0, 60));
        if (_gameLayer)
        {
            _gameLayer->addChild(label, 6);
        }
        else
        {
            this->addChild(label, 6);
        }

        return;
    }

    // 配置玩家锚点与缩放（与 GameScene 保持一致）
    _player->setAnchorPoint(Vec2(0.5f, 0.5f)); // 物理引擎要求锚点在中心
    _player->setScale(GameConfig::Player::SCALE);

    // 计算玩家初始位置（屏幕中央，地面上方）
    const float scaledHalfHeight = (_player->getContentSize().height * std::fabs(_player->getScaleY())) * 0.5f;
    startPos = Vec2(origin.x + visibleSize.width * 0.5f, origin.y + GROUND_Y + scaledHalfHeight);
    _player->setPosition(startPos);

    //-------------------------------------------------------------------------
    // 创建玩家物理刚体
    //-------------------------------------------------------------------------

    // 计算碰撞体尺寸（略小于精灵以获得更好的游戏体验）
    Size playerSize = _player->getContentSize();
    float boxWidth = playerSize.width * GameConfig::Player::COLLISION_BOX_RATIO_W;
    float boxHeight = playerSize.height * GameConfig::Player::COLLISION_BOX_RATIO_H;

    auto physicsBody = PhysicsBody::createBox(Size(boxWidth, boxHeight), GameConfig::Material::PLAYER);
    physicsBody->setDynamic(true);         // 动态刚体，受力影响
    physicsBody->setRotationEnable(false); // 禁止旋转
    physicsBody->setMass(1.0f);            // 质量1kg
    physicsBody->setLinearDamping(0.0f);   // 无线性阻尼

    // 配置碰撞掩码
    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLAYER));
    physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLATFORM |
                                            GamePhysicsCategory::COLLISION |
                                            GamePhysicsCategory::MONSTER_ATTACK));
    physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLATFORM |
                                              GamePhysicsCategory::COLLISION |
                                              GamePhysicsCategory::MONSTER_ATTACK));

    // 说明：
    // - PlayerCharacter 初始化时会挂载一套「通用 / 默认」PhysicsBody，供普通关卡直接使用。
    // - DebugScene 作为功能调试场景，需要自定义碰撞盒尺寸与更精细的碰撞 / 接触掩码配置，
    //   因此在这里显式创建并设置一套 PhysicsBody 配置，用于覆盖默认配置。
    // - 使用 setPhysicsBody 替换已有 PhysicsBody 是 cocos2d-x 推荐的做法，可避免重复添加导致断言。
    _player->setPhysicsBody(physicsBody);
    if (_gameLayer)
    {
        _gameLayer->addChild(_player, 5);
    }
    else
    {
        this->addChild(_player, 5);
    }

    // 设置死亡时不自动移除（由DebugScene控制重置）
    _player->setAutoRemoveOnDeath(false);

    CCLOG("Player created with physics at position (%.0f, %.0f)", startPos.x, startPos.y);
}

void DebugScene::initTestMonsters()
{
    _testMonsters.clear();
    _boss = nullptr;

    if (!_player)
    {
        return;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 训练木桩：21 亿血，固定站立，用于测试伤害/技能/状态/粒子
    if (auto dummy = TrainingDummyMonster::create())
    {
        Vec2 pos(origin.x + visibleSize.width * 0.75f, origin.y + GROUND_Y);
        dummy->setPosition(pos);
        dummy->setAutoRemoveOnDeath(false);
        if (_gameLayer)
        {
            _gameLayer->addChild(dummy, 5);
        }
        else
        {
            addChild(dummy, 5);
        }
        _testMonsters.push_back(dummy);
        CCLOG("DebugScene - 已生成训练木桩，HP=%.0f", dummy->getCurrentHP());
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
        "[AD] 移动  [W/Space] 跳跃  [4/J] 普攻  [E/K/Q/R/F] 技能  [ESC] 暂停",
        "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    hintLabel->setPosition(Vec2(centerX, origin.y + 160));
    hintLabel->setColor(Color3B(150, 150, 150));
    this->addChild(hintLabel, 10);
}

void DebugScene::initGameUIController()
{
    if (!_player)
    {
        CCLOG("DebugScene - initGameUIController 失败：玩家未创建");
        return;
    }

    _uiController = std::make_unique<GameUIController>();

    const std::string levelName = "画室";
    bool ok = _uiController->init(
        this,
        _player,
        levelName,
        [this]()
        { returnToMapScene(); },
        [this](bool paused)
        { setGamePaused(paused); },
        []()
        { return false; },
        [](const SaveSlotData &saveData)
        {
            CCLOG("DebugScene - 加载存档成功，场景: %s", saveData.progressData.currentSceneName.c_str());

            Scene *targetScene = nullptr;
            const std::string &sceneName = saveData.progressData.currentSceneName;

            if (sceneName == "起源之菇")
            {
                targetScene = OriginMushroomScene::createScene();
            }
            else if (sceneName == "神秘之森")
            {
                targetScene = MysteryForestScene::createScene();
            }
            else
            {
                CCLOG("DebugScene - 未知的场景名称: %s", sceneName.c_str());
                return;
            }

            if (!targetScene)
            {
                return;
            }

            auto gameScene = dynamic_cast<GameScene *>(targetScene);
            if (gameScene)
            {
                auto playerData = saveData.playerData;
                auto playerPos = Vec2(saveData.progressData.playerPosX, saveData.progressData.playerPosY);

                // 同步运行时数据：保证新场景创建玩家时即可拿到正确的等级/经验等（避免先用旧数据刷怪/显示）
                if (auto saveManager = SaveManager::getInstance())
                {
                    saveManager->setRuntimePlayerData(playerData);
                }

                // 延迟一小段时间，等待目标场景创建玩家；同时确保只作用于本次切换到的目标场景
                gameScene->scheduleOnce([gameScene, playerData, playerPos](float)
                                        {
                                            auto director = Director::getInstance();
                                            auto currentScene = director ? director->getRunningScene() : nullptr;
                                            if (currentScene != gameScene)
                                            {
                                                return;
                                            }

                                            auto player = gameScene->getPlayer();
                                            if (!player)
                                            {
                                                return;
                                            }

                                            if (auto saveManager = SaveManager::getInstance())
                                            {
                                                saveManager->applyPlayerData(player, playerData);
                                            }
                                            player->setPosition(playerPos);
                                            CCLOG("DebugScene - 玩家数据已恢复，位置: (%.1f, %.1f)", playerPos.x, playerPos.y);
                                        },
                                        0.1f,
                                        "apply_save_data");
            }

            auto transition = TransitionFade::create(0.5f, targetScene, Color3B::BLACK);
            Director::getInstance()->replaceScene(transition);
        });

    if (!ok)
    {
        _uiController.reset();
    }
}

void DebugScene::initInputController()
{
    _inputController = std::make_unique<GameInputController>();
    _inputController->bindPlayer(_player);
    _inputController->setPauseToggle([this]()
                                     { togglePauseMenu(); });
    _inputController->setIsPausedGetter([this]()
                                        { return _isPaused; });
    // DebugScene 无门区交互：保持接口一致即可
    _inputController->setGateQuery([]()
                                   { return false; });
    _inputController->setGateEnter([]() {});
}

void DebugScene::returnToMapScene()
{
    auto mapScene = MapScene::createScene();
    if (!mapScene)
    {
        CCLOG("DebugScene - 返回地图失败：无法创建 MapScene");
        return;
    }

    auto director = Director::getInstance();
    director->popToRootScene();
    auto transition = TransitionFade::create(GameConfig::Scene::MENU_TRANSITION_DURATION, mapScene, Color3B::BLACK);
    director->replaceScene(transition);
}

void DebugScene::togglePauseMenu()
{
    if (_uiController)
    {
        _uiController->togglePauseMenu();
    }
}

void DebugScene::setGamePaused(bool paused)
{
    if (_isPaused == paused)
    {
        return;
    }

    _isPaused = paused;

    // 只冻结游戏内容层，保证 UI（暂停菜单/背包等）仍然可交互。
    if (_gameLayer)
    {
        if (paused)
        {
            _gameLayer->pause();
        }
        else
        {
            _gameLayer->resume();
        }
    }

    // DebugScene 同样是物理场景：需要同时停止物理自动 step，否则平台/投掷物仍会运动。
    if (auto world = getPhysicsWorld())
    {
        if (paused)
        {
            _cachedPhysicsAutoStep = world->isAutoStep();
            _cachedPhysicsSpeed = world->getSpeed();
            world->setAutoStep(false);
            world->setSpeed(0.0f);
        }
        else
        {
            world->setAutoStep(_cachedPhysicsAutoStep);
            world->setSpeed(_cachedPhysicsSpeed);
        }
    }
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

    // 与 GameScene 一致：暂停时只刷新 UI，不推进战斗/移动/计时等逻辑
    if (_uiController)
    {
        _uiController->update(dt);
    }
    if (_isPaused)
    {
        updateDebugInfo();
        return;
    }

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

    // 与 GameScene 对齐：输入控制器负责移动/跳跃/攻击/技能触发
    if (_inputController)
    {
        _inputController->update(dt);
    }

    // 与 GameScene 对齐：Boss 死亡后解绑 Boss 血条（避免 UI 残留）
    if (_boss && _boss->isDead())
    {
        if (_uiController)
        {
            if (auto ui = _uiController->getGameUI())
            {
                ui->unbindBoss();
            }
        }
        _boss = nullptr;
    }

    // 更新调试UI显示
    updateDebugInfo();
    // DebugScene 这里只做场景/调试 UI 刷新；技能/状态等逻辑由角色及其组件在各自流程中处理
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
                case StatusEffectType::BURNING:
                    effectName = "燃烧";
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
                default:
                    effectName = "未知";
                    break;
                }

                if (eff.stackable && eff.stacks > 1)
                {
                    effectName += StringUtils::format("x%d", eff.stacks);
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

    // 与 GameScene 对齐：普攻走 PlayerCharacter::tryNormalAttack（由 SkillSet 负责投掷物/判定）
    bool ok = _player->tryNormalAttack([this]()
                                       {
                                           if (_inputController)
                                           {
                                               _inputController->resyncMoveAnimation();
                                           }
                                       });

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

    if (!ok)
    {
        addDamageLog("普攻失败（可能处于动作锁/冷却中）");
    }
    CCLOG("Player tryNormalAttack: %s, STR: %.0f, Crit: %.0f%%", ok ? "OK" : "FAILED", strength, critRate * 100);
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
 * @brief 重置按钮回调 - 重置角色状态并回到出生点
 * @param sender 按钮引用（未使用）
 */
void DebugScene::onResetClicked(Ref *sender)
{
    if (!_player)
    {
        return;
    }

    // 停止动作并恢复可视状态
    _player->stopAllActions();
    _player->setColor(Color3B::WHITE);
    _player->setOpacity(255);

    // 重置死亡相关状态
    _isDeathResetPending = false;
    _deathResetTimer = 0.0f;

    // 清理所有状态效果（避免持续伤害/减速等残留）
    if (auto attr = _player->getAttributeComponent())
    {
        auto effects = attr->getStatusEffects();
        for (const auto &eff : effects)
        {
            attr->removeStatusEffect(eff.type);
        }

        _player->setCurrentHP(attr->getAttributeValue(AttributeType::MAX_HP));
        _player->setCurrentMP(attr->getAttributeValue(AttributeType::MAX_MP));
    }

    if (auto sm = _player->getStateMachineComponent())
    {
        sm->changeState(CharacterState::IDLE);
    }

    if (auto body = _player->getPhysicsBody())
    {
        body->setVelocity(Vec2::ZERO);
    }

    // 回到出生点（与 initPlayer 逻辑保持一致：站在地面上方）
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    const float scaledHalfHeight = (_player->getContentSize().height * std::fabs(_player->getScaleY())) * 0.5f;
    _player->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
                              origin.y + GROUND_Y + scaledHalfHeight));

    // 重置输入绑定（避免指针在重置流程中失效）
    if (_inputController)
    {
        _inputController->bindPlayer(_player);
    }
    _damageLog.clear();
    addDamageLog("角色已重置");

    CCLOG("Player reset");
}

void DebugScene::onBackClicked(Ref *sender)
{
    // 与 GameScene/MapScene 一致：回到地图选择界面
    returnToMapScene();
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

    // 与正式战斗逻辑一致：DOT 参数必须设置 tickInterval/sourceAttackPower，交给 AttributeComponent 自动结算
    auto attr = _player->getAttributeComponent();
    if (!attr)
    {
        return;
    }

    StatusEffectInstance inst;
    inst.type = StatusEffectType::POISONED;
    inst.duration = std::max(0.0f, GameConfig::StatusEffect::Poisoned::DURATION_SECONDS);
    inst.elapsed = 0.0f;
    inst.attributeBonus.clear();

    inst.stacks = 1;
    inst.maxStacks = 0;
    inst.stackable = true;
    inst.refreshOnAdd = true;

    inst.tickInterval = std::max(0.0f, GameConfig::StatusEffect::Poisoned::TICK_INTERVAL_SECONDS);
    inst.tickAccumulator = 0.0f;
    inst.sourceAttackPower = _player->getAttackPower();
    inst.baseDamageScale = std::max(0.0f, GameConfig::StatusEffect::Poisoned::BASE_DAMAGE_SCALE);
    inst.perStackDamageScale = std::max(0.0f, GameConfig::StatusEffect::Poisoned::PER_STACK_DAMAGE_SCALE);

    attr->addStatusEffect(inst);

    addDamageLog(StringUtils::format("中毒! 持续%.1f秒", GameConfig::StatusEffect::Poisoned::DURATION_SECONDS));
    CCLOG("Poison effect applied");
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
    {
        return;
    }

    StatusEffectInstance excited;
    excited.type = StatusEffectType::EXCITED;
    excited.duration = std::max(0.0f, GameConfig::StatusEffect::Excited::DURATION_SECONDS);
    excited.elapsed = 0.0f;
    excited.attributeBonus.set(AttributeType::MOVE_SPEED, GameConfig::StatusEffect::Excited::MOVE_SPEED_BONUS);

    attr->addStatusEffect(excited);

    addDamageLog(StringUtils::format("亢奋! 移速+%.0f (%.1f秒)",
                                     GameConfig::StatusEffect::Excited::MOVE_SPEED_BONUS,
                                     GameConfig::StatusEffect::Excited::DURATION_SECONDS));
    CCLOG("Excited effect applied");
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

//=============================================================================
// 第7部分：输入处理
//=============================================================================

/**
 * @brief 键盘按下事件处理
 *
 * 按键映射：
 * - A/左箭头：向左移动
 * - D/右箭头：向右移动
 * - Shift：跑步
 * - W/空格：跳跃（W 在关卡里也用于门区交互）
 * - 4/J：普攻
 * - E/K/Q/R/F：技能槽 0/1/2/3
 * - 1/2/3/5：调试快捷键（受击/暴击/治疗/升级）
 * - ESC：暂停菜单
 *
 * @param keyCode 按键代码
 * @param event 事件对象（未使用）
 */
void DebugScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event *event)
{
    // 与 GameScene 对齐：输入交给 GameInputController 统一处理（含暂停、移动、跳跃、攻击、技能）
    if (_inputController)
    {
        _inputController->onKeyPressed(keyCode);
    }

    // 暂停时不响应调试快捷键（避免误操作）
    if (_isPaused)
    {
        return;
    }

    // DebugScene 额外调试快捷键（不与 GameInputController 冲突）
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
    case EventKeyboard::KeyCode::KEY_5:
        onLevelUpClicked(nullptr);
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
    if (_inputController)
    {
        _inputController->onKeyReleased(keyCode);
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
    // 被动技能1：体魄强化（提升最大生命值）
    _passiveSkill1 = std::make_shared<PassiveSkill>();
    _passiveSkill1->id = GameConfig::Skill::Passive::TOUGHNESS;
    _passiveSkill1->name = "体魄强化";
    _passiveSkill1->description = "提升最大生命值";
    _passiveSkill1->attributeBonus.add(AttributeType::MAX_HP, 30.0f);

    // 被动技能2：迅捷步伐（提升移动速度）
    _passiveSkill2 = std::make_shared<PassiveSkill>();
    _passiveSkill2->id = GameConfig::Skill::Passive::SWIFTNESS;
    _passiveSkill2->name = "迅捷步伐";
    _passiveSkill2->description = "提升移动速度";
    _passiveSkill2->attributeBonus.add(AttributeType::MOVE_SPEED, 30.0f);

    // 被动技能3：满血暴击（条件触发：由 PlayerCharacter 统一管理）
    _passiveSkill3 = std::make_shared<PassiveSkill>();
    _passiveSkill3->id = GameConfig::Skill::Passive::FULL_HP_CRIT;
    _passiveSkill3->name = "满血暴击";
    _passiveSkill3->description = "生命值满时，暴击率提升";

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

    addDamageLog(StringUtils::format("学习被动: %s", _passiveSkill1->name.c_str()));
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

    addDamageLog(StringUtils::format("学习被动: %s", _passiveSkill2->name.c_str()));
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

    skillComp->learnSkill(_passiveSkill3);
    skillComp->equipPassiveSkill(_passiveSkill3, 2);

    addDamageLog(StringUtils::format("学习被动: %s", _passiveSkill3->name.c_str()));
    updatePassiveSkillLabel();
    CCLOG("Learned passive skill: %s", _passiveSkill3->name.c_str());
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
    auto attr = _player->getAttributeComponent();
    const bool fullHpCritActive = attr && attr->hasStatusEffect(StatusEffectType::FULL_HP_CRIT);

    std::string passiveText = "被动技能:\n";
    bool hasPassive = false;

    // 显示已装备的被动（无槽位限制）
    for (size_t i = 0; i < passiveSlots.size(); ++i)
    {
        if (passiveSlots[i])
        {
            std::string suffix;
            if (passiveSlots[i]->id == GameConfig::Skill::Passive::FULL_HP_CRIT)
            {
                suffix = fullHpCritActive ? "（激活中）" : "（未激活）";
            }
            passiveText += "  - " + passiveSlots[i]->name + suffix + "\n";
            hasPassive = true;
        }
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
// 第9部分：物理碰撞系统 (Physics Collision System)
//=============================================================================
// 本部分处理 cocos2d-x 物理引擎的碰撞检测逻辑，主要包括：
// 1. 玩家与平台的碰撞 - 用于判定角色是否在地面上
// 2. 炸弹与平台的碰撞 - 触发炸弹爆炸
// 3. 碰撞分离检测 - 判断角色离开平台进入空中
//
// 碰撞类别(Category)使用位掩码区分
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

    // 碰撞预处理回调 - 与 GameScene 统一：玩家与地形碰撞时摩擦/弹性设为0，保证横向手感
    contactListener->onContactPreSolve = CombatContactHelper::handleContactPreSolve;

    // 碰撞分离回调
    contactListener->onContactSeparate = CC_CALLBACK_1(DebugScene::onContactSeparate, this);

    _eventDispatcher->addEventListenerWithSceneGraphPriority(contactListener, this);

    CCLOG("Physics contact listener initialized");
}

/**
 * @brief 碰撞开始回调函数
 *
 * 当两个物理刚体开始接触时调用。主要处理：
 * 1. 玩家落地检测（用于跳跃判定）
 * 2. 怪物攻击命中玩家（MONSTER_ATTACK -> PLAYER）
 * 3. 玩家近战命中怪物（PLAYER_ATTACK -> MONSTER）
 *
 * @param contact 物理接触对象，包含碰撞双方的信息
 * @return true 允许碰撞响应，false 忽略此次碰撞
 */
bool DebugScene::onContactBegin(PhysicsContact &contact)
{
    return CombatContactHelper::handleContactBegin(contact, _player, _inputController.get());
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
    CombatContactHelper::handleContactSeparate(contact, _inputController.get());
}
