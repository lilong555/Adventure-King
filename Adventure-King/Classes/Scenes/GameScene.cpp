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
#include "LoadingScene.h"
#include "Managers/SceneRegistry.h"
#include "Scenes/GameInputController.h"
#include "Scenes/GameUIController.h"
#include "Scenes/CombatContactHelper.h"
#include "Scenes/GamePauseHelper.h"
#include "Scenes/LevelMap.h"
#include "Character/Base/CharacterBase.h"
#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/Monster/Monsters/GobluMonster.h"
#include "Character/Monster/Monsters/ObscurMonster.h"
#include "Character/Monster/Monsters/TrainingDummyMonster.h"
#include "Character/Player/PlayerCharacter.h"
#include "Configs/PlayerRoleConfig.h"
#include "GameUI.h"
#include "Save/SaveData.h"
#include "Save/SaveManager.h"
#include "Utils/ImeHelper.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>

USING_NS_CC;

namespace
{
    const char *const DEFAULT_FONT_PATH = GameSceneConfig::Scene::DEFAULT_FONT_PATH;
    const char *const DEFAULT_PLAYER_SPRITE = GameSceneConfig::Scene::DEFAULT_PLAYER_SPRITE;
    const char *const MAP_LOAD_FAILED_TEXT = GameSceneConfig::Scene::MAP_LOAD_FAILED_TEXT;

    const PhysicsMaterial PLAYER_PHYSICS_MATERIAL = GameConfig::Material::PLAYER;

    std::string getMonsterTypeForSave(const MonsterBase *monster)
    {
        if (!monster)
        {
            return "";
        }

        if (dynamic_cast<const GoblinMonster *>(monster))
        {
            return "goblin";
        }
        if (dynamic_cast<const GobluMonster *>(monster))
        {
            return "goblu";
        }
        if (dynamic_cast<const ObscurMonster *>(monster))
        {
            return "obscur";
        }
        if (dynamic_cast<const TrainingDummyMonster *>(monster))
        {
            return "trainingdummy";
        }

        return "";
    }
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

void GameScene::onEnter()
{
    Scene::onEnter();
    ImeHelper::pushDisableIme();

    // 读档进入关卡：恢复玩家位置
    // 说明：延后一帧执行，确保派生场景（如 HomeScene）对 _gameLayer 的缩放等初始化已完成
    if (_player)
    {
        if (auto saveManager = SaveManager::getInstance())
        {
            if (saveManager->hasRuntimePlayerPosition())
            {
                const Vec2 savedPos = saveManager->getRuntimePlayerPosition();
                saveManager->clearRuntimePlayerPosition();

                scheduleOnce([this, savedPos](float)
                             {
                                 if (this->_player)
                                 {
                                     this->_player->setPosition(savedPos);
                                 }
                             },
                             0.0f,
                             "ApplyRuntimePlayerPosition");
            }
        }
    }
}

void GameScene::onExit()
{
    ImeHelper::popDisableIme();
    Scene::onExit();
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
    // 步骤3：初始化玩家角色（从特定图层获取出生点）
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

    //-------------------------------------------------------------------------
    // 读档：恢复世界状态（刷怪点/竞技场/怪物快照）
    // 说明：必须在“出生点一开始就在视野内”的刷怪检测前应用，否则会产生重复刷怪。
    //-------------------------------------------------------------------------
    if (_levelMap && _player)
    {
        if (auto saveManager = SaveManager::getInstance())
        {
            if (saveManager->hasRuntimeProgressData())
            {
                GameProgressSaveData progress = saveManager->getRuntimeProgressData();
                saveManager->clearRuntimeProgressData();

                _levelMap->applyEnemySpawnPointStates(progress.enemySpawnPoints);
                _levelMap->applyArenaStates(
                    progress.arenas,
                    _player,
                    _gameLayer,
                    [this](const std::string &type)
                    { return this->createMonsterByType(type); });

                // 恢复场上存活怪物（包含竞技场怪物：若标记了 arenaID，则登记死亡回调避免重刷整波）
                for (const auto &m : progress.aliveMonsters)
                {
                    if (m.monsterType.empty())
                    {
                        continue;
                    }

                    // 注意：必须通过 createMonsterByType 创建怪物，保证与正常刷怪一致的初始化逻辑：
                    // - HP 随玩家等级缩放（如 Goblin）
                    // - Boss 绑定 UI（如 Goblu）
                    auto monster = createMonsterByType(m.monsterType);
                    if (!monster)
                    {
                        continue;
                    }

                    const Vec2 pos(m.posX, m.posY);
                    monster->setPosition(pos);
                    monster->setTarget(_player);
                    monster->setHome(pos);
                    _gameLayer->addChild(monster, GameConfig::LevelMap::DEFAULT_CHARACTER_Z_ORDER);

                    if (m.currentHP > 0.0f)
                    {
                        monster->setCurrentHP(m.currentHP);
                    }
                    if (m.currentMP >= 0.0f)
                    {
                        monster->setCurrentMP(m.currentMP);
                    }

                    // Boss 击破条：仅恢复数值（不恢复倒地/起身序列状态）
                    if (m.breakMeter > 0)
                    {
                        if (auto goblu = dynamic_cast<GobluMonster *>(monster))
                        {
                            goblu->setBreakMeterForSave(m.breakMeter);
                        }
                    }

                    // 竞技场怪物：登记回调与计数，避免读档后重刷整波导致进度倒退
                    if (_levelMap && !m.arenaID.empty())
                    {
                        _levelMap->registerRestoredArenaMonster(
                            m.arenaID,
                            monster,
                            _player,
                            _gameLayer,
                            [this](const std::string &type)
                            { return this->createMonsterByType(type); });
                    }
                }

                // 若竞技场已触发但当前没有存活怪，则补刷当前波次（例如刚清完一波、或老存档未记录竞技场怪物）
                _levelMap->resumeActiveArenasIfNeeded(
                    _player,
                    _gameLayer,
                    [this](const std::string &type)
                    { return this->createMonsterByType(type); });
            }
        }
    }

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
    if (_levelMap) {
        _levelMap->onArenaCameraRequest = [this](bool lock, cocos2d::Vec2 pos) {
            this->handleArenaCamera(lock, pos);
            };
    }
    CCLOG("Scene initialized with physics config: %s", getLevelName().c_str());
    return true;
}

void GameScene::fillProgressDataForSave(GameProgressSaveData &outProgress) const
{
    outProgress.currentSceneName = getLevelName();
    if (_player)
    {
        outProgress.playerPosX = _player->getPositionX();
        outProgress.playerPosY = _player->getPositionY();
    }

    outProgress.enemySpawnPoints.clear();
    outProgress.arenas.clear();
    outProgress.aliveMonsters.clear();

    if (_levelMap)
    {
        outProgress.enemySpawnPoints = _levelMap->exportEnemySpawnPointStates();
        outProgress.arenas = _levelMap->exportArenaStates();
    }

    // 采集场上存活怪物（包含竞技场怪物：通过 arenaID 标记归属）
    if (_gameLayer)
    {
        const auto &children = _gameLayer->getChildren();
        outProgress.aliveMonsters.reserve(children.size());
        for (auto *child : children)
        {
            auto *monster = dynamic_cast<MonsterBase *>(child);
            if (!monster)
            {
                continue;
            }

            if (monster->isDead())
            {
                continue;
            }

            const std::string type = getMonsterTypeForSave(monster);
            if (type.empty())
            {
                continue;
            }

            GameProgressSaveData::MonsterState s;
            s.monsterType = type;
            // 竞技场怪物：名称以 "arena:<arenaID>" 标记，存档时记录归属，读档后可避免重刷整波
            const std::string name = monster->getName();
            if (!name.empty() && name.rfind("arena:", 0) == 0 && name.size() > 6)
            {
                s.arenaID = name.substr(6);
            }
            s.posX = monster->getPositionX();
            s.posY = monster->getPositionY();
            s.currentHP = monster->getCurrentHP();
            s.currentMP = monster->getCurrentMP();
            s.breakMeter = monster->getBreakMeter();
            outProgress.aliveMonsters.push_back(std::move(s));
        }
    }
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
    // --- 新增：加载连战竞技场数据 ---
    _levelMap->loadArenas("ArenaLayer", _gameLayer);
    //最终状态检查
    _levelMap->finalizeInitialState();
    return true;
}

void GameScene::initPlayer(const Vec2 &startPos)
{
    // 创建玩家角色：默认战士（WARRIOR）
    // - 若存在运行时存档（关卡切换/读档），以存档职业为准
    // - 否则若存在会话职业选择（主菜单新开局），以会话选择为准
    CharacterRole role = CharacterRole::WARRIOR;
    bool hasRuntimeData = false;
    bool hasSessionRole = false;
    if (auto saveManager = SaveManager::getInstance())
    {
        hasRuntimeData = saveManager->hasRuntimePlayerData();
        if (hasRuntimeData)
        {
            role = static_cast<CharacterRole>(saveManager->getRuntimePlayerData().role);
        }
        else if (saveManager->hasSessionSelectedRole())
        {
            role = saveManager->getSessionSelectedRole();
            hasSessionRole = true;
        }
    }

    // 若存在运行时存档：优先按职业选择默认贴图，避免“职业已切换但贴图仍是默认 WARRIOR”
    std::string spritePath = PlayerRoleConfig::getDefaultSpritePath(role);

    auto playerSprite = PlayerCharacter::create(role, spritePath);
    if (!playerSprite)
    {
        CCLOG("Error: Failed to create player character");
        return;
    }

    // 注意：玩家的缩放（可视体/物理体/攻击判定）统一由 PlayerCharacter 内部管理：
    // - 先使用 GameConfig::Player::SCALE 作为基准
    // - 再按职业对“素材原始 PNG 尺寸差异”做补偿倍率
    // 因此这里不要再 setScale，否则会覆盖职业补偿导致体型不生效。
    const Size originalSize = playerSprite->getContentSize();
    const float scale = std::fabs(playerSprite->getScaleY());
    const float scaledHeight = originalSize.height * scale;


    Vec2 playerPos = startPos;//+ Vec2(0, scaledHeight / 2);
    playerSprite->setPosition(playerPos);

    // 玩家碰撞盒尺寸：
    // - 默认：按贴图尺寸比例生成
    // - 刺客：素材横向留白很大，使用固定碰撞盒尺寸（再叠加 SCALE 与职业倍率）避免碰撞过宽
    float boxWidth = originalSize.width * GameConfig::Player::COLLISION_BOX_RATIO_W;
    float boxHeight = originalSize.height * GameConfig::Player::COLLISION_BOX_RATIO_H;
    if (role == CharacterRole::ASSASSIN)
    {
        boxWidth = GameConfig::Player::ASSASSIN_COLLISION_BOX_WIDTH;
        boxHeight = GameConfig::Player::ASSASSIN_COLLISION_BOX_HEIGHT;
    }

    auto physicsBody = PhysicsBody::createBox(Size(boxWidth, boxHeight), PLAYER_PHYSICS_MATERIAL);
    physicsBody->setDynamic(true);
    physicsBody->setRotationEnable(false);
    physicsBody->setMass(1.0f);
    physicsBody->setLinearDamping(0.0f);

    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLAYER));
    physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLATFORM |
                                            GamePhysicsCategory::COLLISION |
                                            GamePhysicsCategory::MONSTER_ATTACK |
                                            GamePhysicsCategory::ITEM));
    physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLATFORM |
                                              GamePhysicsCategory::COLLISION |
                                              GamePhysicsCategory::MONSTER_ATTACK |
                                              GamePhysicsCategory::ITEM));

    playerSprite->setPhysicsBody(physicsBody);

    _player = playerSprite;
    _player->setTag(TAG_PLAYER);
    _player->setAutoRemoveOnDeath(false);

    // 如果存在运行时玩家数据（从上一张地图离开时缓存），则在进图时自动恢复
    if (auto saveManager = SaveManager::getInstance())
    {
        if (saveManager->hasRuntimePlayerData())
        {
            saveManager->applyPlayerData(_player, saveManager->getRuntimePlayerData());
        }
    }

    if (_gameLayer)
    {
        _gameLayer->addChild(_player, PLAYER_Z_ORDER);
    }
    else
    {
        addChild(_player, PLAYER_Z_ORDER);
    }
    CCLOG("Tiled Spawn Point: (%f, %f)", startPos.x, startPos.y);
    CCLOG("Player created: pos=(%.0f, %.0f), boxSize=(%.0f, %.0f)",
          playerPos.x, playerPos.y, boxWidth, boxHeight);
}

void GameScene::initPhysicsContactListener()
{
    auto contactListener = EventListenerPhysicsContact::create();
    contactListener->onContactBegin = CC_CALLBACK_1(GameScene::onContactBegin, this);

    contactListener->onContactPreSolve = CombatContactHelper::handleContactPreSolve;

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
        [this]() { returnToMapScene(); },
        [this](bool paused) { setGamePaused(paused); },
        [this]() {
            return _levelMap && _player && _levelMap->isPointAtGate(_player->getPosition());
        },
        [](const SaveSlotData& saveData) // 读档回调
        {
            const std::string& sceneName = saveData.progressData.currentSceneName;

            // 1. 利用重构后的注册表：通过名称获取强类型 SceneID
            auto registry = SceneRegistry::getInstance();
            SceneID targetID = registry ? registry->getSceneIDByName(sceneName) : SceneID::NONE;

            if (targetID == SceneID::NONE)
            {
                CCLOG("GameScene - 读档失败：注册表中不存在场景 [%s]", sceneName.c_str());
                return;
            }

            // 2. 运行时数据同步
            // 目标关卡在创建玩家时会优先检查 SaveManager 里的 runtime 数据
            auto saveManager = SaveManager::getInstance();
            if (saveManager)
            {
                saveManager->setRuntimePlayerData(saveData.playerData);
                saveManager->setRuntimePlayerPosition(Vec2(saveData.progressData.playerPosX, saveData.progressData.playerPosY));
                saveManager->setRuntimeProgressData(saveData.progressData);
            }

            // 3. 统一进入 LoadingScene
            // 现在的 LoadingScene 会根据 targetID 自动去 SceneRegistry 读资源列表并预热
            auto loadingScene = LoadingScene::createScene(targetID);
            if (loadingScene)
            {
                // 使用 GameSceneConfig 中统一定义的转场时间
                float duration = GameSceneConfig::Scene::TRANSITION_DURATION;
                auto transition = TransitionFade::create(duration, loadingScene, Color3B::BLACK);
                Director::getInstance()->replaceScene(transition);

                CCLOG("GameScene - 读档成功，开始通过 LoadingScene 切换至 ID: %d", static_cast<int>(targetID));
            }
        });

    if (!ok)
    {
        _uiController.reset();
    }
}

void GameScene::initCameraFollow()
{
    if (!_player || !_gameLayer)return;
    _gameLayer->stopActionByTag(837);
    Size mapSize = _levelMap ? _levelMap->getMapSizeInPixels() : Director::getInstance()->getVisibleSize();
    Rect worldBound(0, 0, mapSize.width, mapSize.height);

    auto followAction = Follow::create(_player, worldBound);

    CCLOG("Camera follow enabled on gameLayer, world bound: (%.0f, %.0f, %.0f, %.0f)",
          worldBound.origin.x, worldBound.origin.y,
          worldBound.size.width, worldBound.size.height);

    followAction->setTag(837); // 为跟随动作设置一个固定 Tag，方便寻找
    _gameLayer->runAction(followAction);
}

void GameScene::handleArenaCamera(bool lock, cocos2d::Vec2 targetPos) {
    // 停止当前正在执行的所有相机平移动作，防止动作叠加冲突
    _gameLayer->stopActionByTag(1001);
        if (lock) {
            _gameLayer->stopActionByTag(837); // 停止跟随

            auto visibleSize = Director::getInstance()->getVisibleSize();
            float targetScale = 0.85f; // 你的目标缩放值

            // --- 修正后的坐标计算 ---
            // 必须将目标坐标乘以缩放系数，才能抵消缩放带来的视觉偏移
            cocos2d::Vec2 layerPos(
                (visibleSize.width / 2) - (targetPos.x * targetScale),
                (visibleSize.height / 2) - (targetPos.y * targetScale)
            );

            auto moveTo = MoveTo::create(1.0f, layerPos);
            auto scaleTo = ScaleTo::create(1.0f, targetScale);
            auto spawn = Spawn::create(moveTo, scaleTo, nullptr);
            _gameLayer->runAction(EaseExponentialOut::create(spawn));
        }else {
        // 恢复逻辑：先拉回缩放，并在结束后重启动 Follow
        auto resetScale = ScaleTo::create(0.5f, 1.0f);
        auto callback = CallFunc::create([this]() {
            this->initCameraFollow(); // 重新启动 Tag 为 837 的动作
            });

        auto seq = Sequence::create(resetScale, callback, nullptr);
        seq->setTag(1001); // 同样设置 Tag
        _gameLayer->runAction(seq);
    }
}

void GameScene::returnToMapScene()
{
    CCLOG("Returning to map scene from: %s", getLevelName().c_str());

    // 关卡离开时缓存玩家进度：避免再次进图时等级/经验被重置
    if (_player)
    {
        if (auto saveManager = SaveManager::getInstance())
        {
            saveManager->cacheRuntimePlayerData(_player);
        }
    }

    auto mapScene = MapScene::createScene();
    if (!mapScene)
    {
        CCLOG("Error: Failed to create map scene");
        return;
    }

    auto transition = TransitionFade::create(SCENE_TRANSITION_DURATION, mapScene, Color3B::BLACK);
    Director::getInstance()->replaceScene(transition);
}

void GameScene::togglePauseMenu()
{
    if (_uiController)
    {
        _uiController->togglePauseMenu();
    }
}


void GameScene::setGamePaused(bool paused)
{
    if (_isPaused == paused)
    {
        return;
    }

    _isPaused = paused;
    GamePauseHelper::setWorldPaused(this, _gameLayer, paused, _cachedPhysicsAutoStep, _cachedPhysicsSpeed);
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

    if (key == "obscur" || key == "obscurmonster")
    {
        auto obscur = ObscurMonster::create();
        return obscur;
    }

    if (key == "trainingdummy" || key == "trainingdummymonster" || key == "dummy")
    {
        return TrainingDummyMonster::create();
    }

    CCLOG("Warning: Unknown monster type '%s'", monsterType.c_str());
    return nullptr;
}

bool GameScene::onContactBegin(PhysicsContact &contact)
{
    return CombatContactHelper::handleContactBegin(contact, _player, _inputController.get());
}

void GameScene::onContactSeparate(PhysicsContact &contact)
{
    CombatContactHelper::handleContactSeparate(contact, _inputController.get());
}

void GameScene::update(float dt)
{
    //-------------------------------------------------------------------------
    // 角色死亡：强制暂停并弹出选择菜单
    //-------------------------------------------------------------------------
    if (_player && _player->isDead())
    {
        if (_uiController && !_uiController->isDeathMenuShowing())
        {
            _uiController->showDeathMenu();
        }
        // 死亡状态下仅刷新 UI（世界已冻结）
        if (_uiController)
        {
            _uiController->update(dt);
        }
        return;
    }

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

        // 2. 新增：连战竞技场逻辑
        _levelMap->updateArenas(
            _player,
            _gameLayer,
            [this](const std::string& type) { return this->createMonsterByType(type); }
        );
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
