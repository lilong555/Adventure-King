/**
 * @file GameScene.cpp
 * @brief 游戏关卡场景实现
 *
 * GameScene 只负责流程编排：
 * - 初始化物理/关卡地图/玩家
 * - 绑定输入与 UI 控制器
 * - 每帧编排（输入 -> UI -> 刷怪 -> 投掷物清理）
 */

#include "GameScene.h"
#include "MapScene.h"
#include "Scenes/GameInputController.h"
#include "Scenes/GameUIController.h"
#include "Scenes/LevelMap.h"
#include "Character/Base/CharacterBase.h"
#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/Monster/Monsters/GobluMonster.h"
#include "Character/Player/PlayerCharacter.h"
#include "GameUI.h"
#include "Configs/GameConfigs.h"
#include "Save/SaveData.h"
#include "Save/SaveManager.h"
#include <algorithm>
#include <cctype>
#include <memory>

USING_NS_CC;

namespace
{
    const char *const DEFAULT_FONT_PATH = GameConfig::Scene::DEFAULT_FONT_PATH;
    const char *const DEFAULT_PLAYER_SPRITE = GameConfig::Scene::DEFAULT_PLAYER_SPRITE;
    const char *const MAP_LOAD_FAILED_TEXT = GameConfig::Scene::MAP_LOAD_FAILED_TEXT;

    const PhysicsMaterial PLAYER_PHYSICS_MATERIAL = GameConfig::Material::PLAYER;
}

cocos2d::Scene *GameScene::createScene()
{
    CCLOG("GameScene::createScene called on abstract base scene");
    return nullptr;
}

GameScene::GameScene() = default;

GameScene::~GameScene() = default;

bool GameScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    return true;
}

bool GameScene::initWithPhysicsConfig(const LevelConfig &config)
{
    //-------------------------------------------------------------------------
    // 步骤1：物理引擎初始化
    //-------------------------------------------------------------------------
    if (!Scene::initWithPhysics())
    {
        CCLOG("Error: Failed to initialize physics scene");
        return false;
    }

    auto physicsWorld = getPhysicsWorld();
    physicsWorld->setGravity(Vec2(0, config.gravity));

    if (config.enablePhysicsDebug)
    {
        physicsWorld->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
    }

    //-------------------------------------------------------------------------
    // 步骤1.5：创建游戏内容层（用于相机跟随）
    //-------------------------------------------------------------------------
    _gameLayer = Node::create();
    addChild(_gameLayer, 0);

    //-------------------------------------------------------------------------
    // 步骤2：加载关卡地图（TMX/碰撞/门区/敌人点）
    //-------------------------------------------------------------------------
    if (!initLevelMap(config))
    {
        CCLOG("Warning: Failed to load TMX map: %s", config.tmxMapPath.c_str());
        showMapLoadFailedUI();
        return true;
    }

    //-------------------------------------------------------------------------
    // 步骤3：初始化玩家角色（从 born 图层获取出生点）
    //-------------------------------------------------------------------------
    Vec2 playerStartPos = _levelMap ? _levelMap->getPlayerSpawnPoint(config.bornLayerName) : Vec2::ZERO;
    initPlayer(playerStartPos);

    //-------------------------------------------------------------------------
    // 步骤4：初始化物理碰撞监听和输入
    //-------------------------------------------------------------------------
    initPhysicsContactListener();
    initInputController();

    //-------------------------------------------------------------------------
    // 步骤5：设置相机跟随
    //-------------------------------------------------------------------------
    initCameraFollow();

    //-------------------------------------------------------------------------
    // 步骤6：启用帧更新和初始化 UI
    //-------------------------------------------------------------------------
    scheduleUpdate();
    initUIController();

    // 处理“出生点一开始就在视野内”的情况
    if (_levelMap && _player)
    {
        _levelMap->updateEnemySpawns(
            _player,
            _gameLayer,
            [this](const std::string &type)
            { return this->createMonsterByType(type); },
            getEnemySpawnViewDistance(),
            0.0f);
    }

    CCLOG("Scene initialized with physics config: %s", getLevelName().c_str());
    return true;
}

bool GameScene::initLevelMap(const LevelConfig &config)
{
    if (!_gameLayer)
    {
        CCLOG("Error: Cannot init level map - gameLayer not created");
        return false;
    }

    _levelMap = std::make_unique<LevelMap>();
    if (!_levelMap->load(_gameLayer, config.tmxMapPath))
    {
        _levelMap.reset();
        return false;
    }

    if (!config.backgroundSeriesPaths.empty())
    {
        _levelMap->setupBackgroundSeries(_gameLayer, config.backgroundSeriesPaths);
    }
    else if (!config.backgroundPath.empty())
    {
        _levelMap->setupRepeatingBackground(_gameLayer, config.backgroundPath, _levelMap->getMapSizeInPixels().width);
    }

    _levelMap->createCollisionBodiesFromTMX(_gameLayer, config.collisionLayerName);
    _levelMap->loadGateAreas(config.gateLayerName);
    _levelMap->loadEnemySpawnPoints("enemy_g");
    return true;
}

void GameScene::initPlayer(const Vec2 &startPos)
{
    // 创建玩家角色（战士职业）
    auto playerSprite = PlayerCharacter::create(CharacterRole::WARRIOR, DEFAULT_PLAYER_SPRITE);
    if (!playerSprite)
    {
        CCLOG("Error: Failed to create player character");
        return;
    }

    Size originalSize = playerSprite->getContentSize();
    float scale = GameConfig::Player::SCALE;
    playerSprite->setScale(scale);

    float scaledHeight = originalSize.height * scale;
    playerSprite->setAnchorPoint(Vec2(0.5f, 0.5f));

    Vec2 playerPos = startPos + Vec2(0, scaledHeight / 2);
    playerSprite->setPosition(playerPos);

    float boxWidth = originalSize.width * GameConfig::Player::COLLISION_BOX_RATIO_W;
    float boxHeight = originalSize.height * GameConfig::Player::COLLISION_BOX_RATIO_H;

    auto physicsBody = PhysicsBody::createBox(Size(boxWidth, boxHeight), PLAYER_PHYSICS_MATERIAL);
    physicsBody->setDynamic(true);
    physicsBody->setRotationEnable(false);
    physicsBody->setMass(1.0f);
    physicsBody->setLinearDamping(0.0f);

    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLAYER));
    physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLATFORM |
                                            GamePhysicsCategory::COLLISION |
                                            GamePhysicsCategory::MONSTER_ATTACK));
    physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLATFORM |
                                              GamePhysicsCategory::COLLISION |
                                              GamePhysicsCategory::MONSTER_ATTACK));

    playerSprite->setPhysicsBody(physicsBody);

    _player = playerSprite;
    _player->setTag(TAG_PLAYER);
    _player->setAutoRemoveOnDeath(false);

    if (_gameLayer)
    {
        _gameLayer->addChild(_player, PLAYER_Z_ORDER);
    }
    else
    {
        addChild(_player, PLAYER_Z_ORDER);
    }

    CCLOG("Player created: pos=(%.0f, %.0f), boxSize=(%.0f, %.0f)",
          playerPos.x, playerPos.y, boxWidth, boxHeight);
}

void GameScene::initPhysicsContactListener()
{
    auto contactListener = EventListenerPhysicsContact::create();
    contactListener->onContactBegin = CC_CALLBACK_1(GameScene::onContactBegin, this);

    contactListener->onContactPreSolve = [](PhysicsContact &contact, PhysicsContactPreSolve &solve) -> bool
    {
        auto nodeA = contact.getShapeA()->getBody()->getNode();
        auto nodeB = contact.getShapeB()->getBody()->getNode();
        if (!nodeA || !nodeB)
            return true;

        int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
        int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

        bool playerInvolved = (categoryA & ToMask(GamePhysicsCategory::PLAYER)) || (categoryB & ToMask(GamePhysicsCategory::PLAYER));
        bool platformInvolved =
            (categoryA & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) ||
            (categoryB & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION));

        if (playerInvolved && platformInvolved)
        {
            solve.setRestitution(0.0f);
            solve.setFriction(0.0f);
        }

        return true;
    };

    contactListener->onContactSeparate = CC_CALLBACK_1(GameScene::onContactSeparate, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(contactListener, this);

    CCLOG("Physics contact listener initialized");
}

void GameScene::initInputController()
{
    _inputController = std::make_unique<GameInputController>();
    _inputController->bindPlayer(_player);
    _inputController->setPauseToggle([this]()
                                     { togglePauseMenu(); });
    _inputController->setIsPausedGetter([this]()
                                        { return _isPaused; });
    _inputController->setGateQuery([this]()
                                   {
                                       return _levelMap && _player && _levelMap->isPointAtGate(_player->getPosition());
                                   });
    _inputController->setGateEnter([this]()
                                   { returnToMapScene(); });

    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event *event)
    {
        if (_inputController)
        {
            _inputController->onKeyPressed(keyCode);
        }
    };
    keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event)
    {
        if (_inputController)
        {
            _inputController->onKeyReleased(keyCode);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    CCLOG("Input controller initialized");
}

void GameScene::initUIController()
{
    _uiController = std::make_unique<GameUIController>();

    bool ok = _uiController->init(
        this,
        _player,
        getLevelName(),
        [this]()
        { returnToMapScene(); },
        [this](bool paused)
        { _isPaused = paused; },
        [this]()
        {
            return _levelMap && _player && _levelMap->isPointAtGate(_player->getPosition());
        },
        [](const SaveSlotData &saveData)
        {
            CCLOG("GameScene - 加载存档成功，场景: %s", saveData.progressData.currentSceneName.c_str());

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
                CCLOG("GameScene - 未知的场景名称: %s", sceneName.c_str());
                return;
            }

            if (!targetScene)
            {
                return;
            }

            auto gameScene = dynamic_cast<GameScene *>(targetScene);
            if (gameScene)
            {
                auto saveManager = SaveManager::getInstance();
                auto playerData = saveData.playerData;
                auto playerPos = Vec2(saveData.progressData.playerPosX, saveData.progressData.playerPosY);

                gameScene->scheduleOnce([saveManager, playerData, playerPos](float dt)
                                        {
                                            auto currentScene = Director::getInstance()->getRunningScene();
                                            auto currentGameScene = dynamic_cast<GameScene *>(currentScene);
                                            if (!currentGameScene)
                                                return;

                                            auto player = currentGameScene->getPlayer();
                                            if (!player)
                                                return;

                                            saveManager->applyPlayerData(player, playerData);
                                            player->setPosition(playerPos);
                                            CCLOG("GameScene - 玩家数据已恢复，位置: (%.1f, %.1f)", playerPos.x, playerPos.y);
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

void GameScene::initCameraFollow()
{
    if (!_player || !_gameLayer)
    {
        CCLOG("Warning: Cannot init camera follow - player or gameLayer not created");
        return;
    }

    Size mapSize = _levelMap ? _levelMap->getMapSizeInPixels() : Director::getInstance()->getVisibleSize();
    Rect worldBound(0, 0, mapSize.width, mapSize.height);

    auto followAction = Follow::create(_player, worldBound);
    _gameLayer->runAction(followAction);

    CCLOG("Camera follow enabled on gameLayer, world bound: (%.0f, %.0f, %.0f, %.0f)",
          worldBound.origin.x, worldBound.origin.y,
          worldBound.size.width, worldBound.size.height);
}

void GameScene::returnToMapScene()
{
    CCLOG("Returning to map scene from: %s", getLevelName().c_str());

    auto mapScene = MapScene::createScene();
    if (!mapScene)
    {
        CCLOG("Error: Failed to create map scene");
        return;
    }

    auto transition = TransitionFade::create(SCENE_TRANSITION_DURATION, mapScene, Color3B::BLACK);
    Director::getInstance()->pushScene(transition);
}

void GameScene::togglePauseMenu()
{
    if (_uiController)
    {
        _uiController->togglePauseMenu();
    }
}

float GameScene::getEnemySpawnViewDistance() const
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    return visibleSize.width * 0.5f;
}

MonsterBase *GameScene::createMonsterByType(const std::string &monsterType)
{
    std::string key = monsterType;
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });

    if (key == "goblin" || key == "goblinmonster")
    {
        auto goblin = GoblinMonster::create();
        if (goblin && _player)
        {
            goblin->applyHpScalingForPlayerLevel(_player->getLevel());
        }
        return goblin;
    }

    if (key == "goblu" || key == "gobluboss")
    {
        auto goblu = GobluMonster::create();
        if (goblu)
        {
            goblu->setAutoRemoveOnDeath(false);
        }

        if (goblu && _uiController)
        {
            if (auto ui = _uiController->getGameUI())
            {
                ui->bindBoss(goblu, "Goblu", 1);
            }
            _boss = goblu;
        }
        return goblu;
    }

    CCLOG("Warning: Unknown monster type '%s'", monsterType.c_str());
    return nullptr;
}

bool GameScene::onContactBegin(PhysicsContact &contact)
{
    auto nodeA = contact.getShapeA()->getBody()->getNode();
    auto nodeB = contact.getShapeB()->getBody()->getNode();
    auto bodyA = contact.getShapeA()->getBody();
    auto bodyB = contact.getShapeB()->getBody();

    if (!nodeA || !nodeB)
        return true;

    auto getWorldPos = [](Node* node) -> Vec2 {
        if (!node)
        {
            return Vec2::ZERO;
        }
        auto parent = node->getParent();
        return parent ? parent->convertToWorldSpace(node->getPosition()) : node->getPosition();
    };

    int categoryA = bodyA->getCategoryBitmask();
    int categoryB = bodyB->getCategoryBitmask();

    // 玩家与平台/碰撞体的接触（用于跳跃/落地判定）
    bool playerIsA = (categoryA & ToMask(GamePhysicsCategory::PLAYER)) != 0;
    bool playerIsB = (categoryB & ToMask(GamePhysicsCategory::PLAYER)) != 0;
    bool platformContact =
        (playerIsA && ((categoryB & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) != 0)) ||
        (playerIsB && ((categoryA & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) != 0));

    if (platformContact && _inputController)
    {
        if (auto contactData = contact.getContactData())
        {
            Vec2 normal = contactData->normal;
            if (playerIsB)
            {
                normal = -normal;
            }
            _inputController->onGroundContactBegin(normal.y);
        }
    }

    // ============================================================
    // 战斗：怪物攻击 -> 玩家
    // ============================================================
    bool monsterAttackVsPlayer =
        ((categoryA & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) != 0 && (categoryB & ToMask(GamePhysicsCategory::PLAYER)) != 0) ||
        ((categoryB & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) != 0 && (categoryA & ToMask(GamePhysicsCategory::PLAYER)) != 0);

    if (monsterAttackVsPlayer)
    {
        auto attackBody = ((categoryA & ToMask(GamePhysicsCategory::MONSTER_ATTACK)) != 0) ? bodyA : bodyB;
        auto playerNode = ((categoryA & ToMask(GamePhysicsCategory::PLAYER)) != 0) ? nodeA : nodeB;
        auto player = dynamic_cast<CharacterBase *>(playerNode);

        if (player && !player->isDead())
        {
            float rawDamage = static_cast<float>(attackBody->getTag());
            if (rawDamage <= 0.0f)
            {
                rawDamage = 1.0f;
            }

            DamageInfo dmg{};
            dmg.amount = rawDamage;
            if (auto attackNode = attackBody->getNode())
            {
                dmg.attacker = dynamic_cast<CharacterBase *>(attackNode->getUserObject());
                // 记录命中位置（世界坐标），用于受击方向判断
                dmg.hitWorldPos = getWorldPos(attackNode);
                dmg.hasHitWorldPos = true;
            }

            // 避免在物理回调中直接修改角色/物理状态（可能导致物理引擎内部状态被破坏）。
            // 延迟到下一帧执行伤害结算。
            std::string key = StringUtils::format("defer_monster_dmg_%p_%p",
                                                  static_cast<void*>(attackBody),
                                                  static_cast<void*>(player));
            player->scheduleOnce(
                [player, dmg](float)
                {
                    if (!player || player->isDead())
                    {
                        return;
                    }
                    player->takeDamage(dmg);
                },
                0.0f,
                key);
        }
    }

    // ============================================================
    // 战斗：玩家攻击 -> 怪物（近战/判定框）
    // 注：投掷物/爆炸由投掷物逻辑处理
    // ============================================================
    bool playerAttackVsMonster =
        ((categoryA & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) != 0 && (categoryB & ToMask(GamePhysicsCategory::MONSTER)) != 0) ||
        ((categoryB & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) != 0 && (categoryA & ToMask(GamePhysicsCategory::MONSTER)) != 0);

    if (playerAttackVsMonster)
    {
        auto attackBody = ((categoryA & ToMask(GamePhysicsCategory::PLAYER_ATTACK)) != 0) ? bodyA : bodyB;
        auto monsterNode = ((categoryA & ToMask(GamePhysicsCategory::MONSTER)) != 0) ? nodeA : nodeB;
        auto monster = dynamic_cast<CharacterBase *>(monsterNode);

        if (monster && !monster->isDead())
        {
            float rawDamage = static_cast<float>(attackBody->getTag());
            if (rawDamage > 0.0f)
            {
                DamageInfo dmg{};
                dmg.amount = rawDamage;
                dmg.attacker = _player;
                if (auto attackNode = attackBody->getNode())
                {
                    // 记录命中位置（世界坐标），用于受击方向判断
                    dmg.hitWorldPos = getWorldPos(attackNode);
                    dmg.hasHitWorldPos = true;
                }

                // 避免在物理回调中直接修改角色/物理状态（可能导致物理引擎内部状态被破坏）。
                // 延迟到下一帧执行伤害结算。
                std::string key = StringUtils::format("defer_player_dmg_%p_%p",
                                                      static_cast<void*>(attackBody),
                                                      static_cast<void*>(monster));
                monster->scheduleOnce(
                    [monster, dmg](float)
                    {
                        if (!monster || monster->isDead())
                        {
                            return;
                        }
                        monster->takeDamage(dmg);
                    },
                    0.0f,
                    key);
            }
        }
    }

    return true;
}

void GameScene::onContactSeparate(PhysicsContact &contact)
{
    if (!_inputController)
        return;

    auto nodeA = contact.getShapeA()->getBody()->getNode();
    auto nodeB = contact.getShapeB()->getBody()->getNode();
    if (!nodeA || !nodeB)
        return;

    int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
    int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

    bool playerIsA = (categoryA & ToMask(GamePhysicsCategory::PLAYER)) != 0;
    bool playerIsB = (categoryB & ToMask(GamePhysicsCategory::PLAYER)) != 0;
    bool platformContact =
        (playerIsA && ((categoryB & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) != 0)) ||
        (playerIsB && ((categoryA & ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION)) != 0));

    if (!platformContact)
        return;

    if (auto contactData = contact.getContactData())
    {
        Vec2 normal = contactData->normal;
        if (playerIsB)
        {
            normal = -normal;
        }
        _inputController->onGroundContactEnd(normal.y);
    }
}

void GameScene::update(float dt)
{
    if (_isPaused)
    {
        if (_uiController)
        {
            _uiController->update(dt);
        }
        return;
    }

    if (_inputController)
    {
        _inputController->update(dt);
    }

    if (_uiController)
    {
        _uiController->update(dt);
    }

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

    if (_levelMap && _player)
    {
        _levelMap->updateEnemySpawns(
            _player,
            _gameLayer,
            [this](const std::string &type)
            { return this->createMonsterByType(type); },
            getEnemySpawnViewDistance(),
            dt);
    }

}

void GameScene::showMapLoadFailedUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    std::string errorText = getLevelName() + MAP_LOAD_FAILED_TEXT;
    auto titleLabel = Label::createWithTTF(errorText, DEFAULT_FONT_PATH, 48);
    if (titleLabel)
    {
        titleLabel->setPosition(center);
        titleLabel->setColor(Color3B::RED);
        addChild(titleLabel, 1);
    }
}

// ============================================================
// OriginMushroomScene 实现（起源之菇）
// ============================================================

Scene *OriginMushroomScene::createScene()
{
    return OriginMushroomScene::create();
}

LevelConfig OriginMushroomScene::getLevelConfig() const
{
    LevelConfig config;
    config.tmxMapPath = "Map/Origin_Mushroom/Origin_Mushroom.tmx";
    config.backgroundPath = "";
    config.backgroundSeriesPaths.reserve(GameConfig::Map::OriginMushroom::BACKGROUND_COUNT);
    for (int i = 0; i < GameConfig::Map::OriginMushroom::BACKGROUND_COUNT; ++i)
    {
        config.backgroundSeriesPaths.emplace_back(
            StringUtils::format("%s%02d.png",
                                GameConfig::Map::OriginMushroom::BACKGROUND_PREFIX,
                                i));
    }
    config.playerSpritePath = DEFAULT_PLAYER_SPRITE;
    config.collisionLayerName = "collisions";
    config.bornLayerName = "born";
    config.gateLayerName = "gate";
    config.gravity = -1000.0f;
    config.enablePhysicsDebug = true;
    return config;
}

bool OriginMushroomScene::init()
{
    LevelConfig config = getLevelConfig();
    if (!initWithPhysicsConfig(config))
    {
        return false;
    }
    CCLOG("OriginMushroomScene initialized");
    return true;
}

// ============================================================
// MysteryForestScene 实现（神秘之森）
// ============================================================

Scene *MysteryForestScene::createScene()
{
    return MysteryForestScene::create();
}

LevelConfig MysteryForestScene::getLevelConfig() const
{
    LevelConfig config;
    config.tmxMapPath = "";
    config.backgroundPath = "";
    config.gravity = -800.0f;
    config.enablePhysicsDebug = false;
    return config;
}

bool MysteryForestScene::init()
{
    if (!GameScene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    auto titleLabel = Label::createWithTTF(getLevelName(), DEFAULT_FONT_PATH, 72);
    if (titleLabel)
    {
        titleLabel->setPosition(center);
        titleLabel->setColor(Color3B::WHITE);
        addChild(titleLabel, 1);
    }

    auto hintLabel = Label::createWithTTF("Click map button to return", DEFAULT_FONT_PATH, 32);
    if (hintLabel)
    {
        hintLabel->setPosition(Vec2(center.x, center.y - 80));
        hintLabel->setColor(Color3B(200, 200, 200));
        addChild(hintLabel, 1);
    }

    initUIController();

    CCLOG("MysteryForestScene initialized (placeholder)");
    return true;
}
