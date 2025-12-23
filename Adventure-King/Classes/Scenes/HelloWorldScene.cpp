#include "HelloWorldScene.h"
#include "HomeScene.h"
#include "MapScene.h"
#include "GameScene.h"
#include "Character/Player/PlayerCharacter.h"
#include "Scenes/LoadingScene.h"
#include "Scenes/LevelScenes/OriginMushroomScene.h"
#include "Scenes/LevelScenes/MysteryForestScene.h"
#include "Scenes/Layers/SaveMenuLayer.h"
#include "Scenes/Layers/SetMenuLayer.h"
#include"Utils/ParticlePreloadHelper.h"
#include "Managers/MusicManager.h"
#include "audio/include/AudioEngine.h"
#include "Configs/GameConfigs.h"
#include"Managers/SceneRegistry.h"
#include "Save/SaveData.h"
#include "Save/SaveManager.h"
#include "Configs/PlayerRoleConfig.h"

USING_NS_CC;

Scene *HelloWorld::createScene()
{
    return HelloWorld::create();
}

// 当文件不存在时，打印有用的错误消息而不是段错误。
// 统一的资源缺失提示
static void problemLoading(const char *filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}
MenuItemImage *HelloWorld::createMenuItem(
    const char *normal,
    const char *selected,
    const ccMenuCallback &callback)
{
    auto item = MenuItemImage::create(normal, selected, callback);
    if (!item || item->getContentSize().width <= 0 || item->getContentSize().height <= 0)
    {
        problemLoading(normal);
    }
    return item;
}

// 初始化实例
bool HelloWorld::init()
{
    // 1. super init first
    if (!Scene::init())
    {
        return false;
    }
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 默认职业：如果会话中已选择过，则沿用
    if (auto saveManager = SaveManager::getInstance())
    {
        if (saveManager->hasSessionSelectedRole())
        {
            _selectedRole = saveManager->getSessionSelectedRole();
        }
    }

    // ==========================================================
    // 布局常量和参考点定义
    // ==========================================================
    // 屏幕中心点
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // 按钮之间的水平间距
    const float buttonHorizontalSpacing = GameSceneConfig::UI::MainMenu::BUTTON_HORIZONTAL_SPACING;
    const float subMenuYMultiplier = GameSceneConfig::UI::MainMenu::SUB_MENU_Y_MULTIPLIER;
    const float menuOffsetYDivisor = GameSceneConfig::UI::MainMenu::MENU_OFFSET_Y_DIVISOR;
    const int contentZOrder = GameSceneConfig::UI::MainMenu::CONTENT_Z_ORDER;
    const int menuZOrder = GameSceneConfig::UI::MainMenu::MENU_Z_ORDER;

    // ===============================================
    // 内容容器节点，用于统一缩放和定位
    // ===============================================
    // 创建一个父级容器节点
    auto contentContainer = Node::create();
    contentContainer->setTag(TAG_CONTENT_CONTAINER);
    // 将容器节点添加到场景中
    this->addChild(contentContainer, contentZOrder); // 确保 z-order 高于背景 (背景 z=0)
    // 将容器节点定位
    contentContainer->setPosition(center);

    // ==========================================================
    // 2. 菜单项创建和错误检查 (使用 Lambda 简化代码)
    // ==========================================================

    //// 退出按钮 (右下角)
    // auto closeItem = createMenuItem(
    //     "Scene/UI/CloseNormal.png",
    //     "Scene/UI/CloseSelected.png",
    //     CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));

    // 开始按钮 (主菜单中心)
    auto StartItem = createMenuItem(
        "Scene/UI/StartItemNormal.png",
        "Scene/UI/StartItemSelect.png",
        CC_CALLBACK_1(HelloWorld::menuStartCallback, this));

    // 设置按钮 (左侧)
    auto SetItem = createMenuItem(
        "Scene/UI/SetingNormal.png",
        "Scene/UI/SetingSelect.png",
        CC_CALLBACK_1(HelloWorld::menuSetCallback, this));

    // 地图按钮 (中央下方)
    auto MapItem = createMenuItem(
        "Scene/UI/MapNormal.png",
        "Scene/UI/MapSelect.png",
        CC_CALLBACK_1(HelloWorld::menuMapCallback, this));

    // 存档按钮 (右侧)
    auto SaveItem = createMenuItem(
        "Scene/UI/SaveNormal.png",
        "Scene/UI/SaveSelect.png",
        CC_CALLBACK_1(HelloWorld::menuSaveCallback, this));

    // ==========================================================
    // 3. 统一设置按钮位置
    // ==========================================================

    // StartItem 定位到屏幕中心
    if (StartItem)
    {
        StartItem->setPosition(Vec2::ZERO);
    }

    // 下方按钮组的 Y 坐标
    float sub_menu_y = -subMenuYMultiplier * StartItem->getContentSize().height;

    if (MapItem)
    {
        // MapItem 在 StartItem 下方居中
        MapItem->setPosition(Vec2(0, sub_menu_y));
    }

    if (SetItem)
    {
        // SetItem 在 MapItem 左侧
        SetItem->setPosition(Vec2(-buttonHorizontalSpacing, sub_menu_y));
    }

    if (SaveItem)
    {
        // SaveItem 在 MapItem 右侧
        SaveItem->setPosition(Vec2(buttonHorizontalSpacing, sub_menu_y));
    }

    // ==========================================================
    // 4. 创建菜单并添加
    // ==========================================================

    auto menu = Menu::create(StartItem, SetItem, SaveItem, MapItem, NULL);

    // --- 核心设置 ---
    //menu->ignoreAnchorPointForPosition(false);
    //menu->setAnchorPoint(Vec2(0.5f, 0.5f));    // 设置缩放中心为中点
    menu->setContentSize(Size::ZERO);          // 强制内容大小为0，使锚点与 (0,0) 重合
    menu->setScale(2.0f);                      // 整体缩放

    menu->setPosition(Vec2(0,0));

    contentContainer->addChild(menu, menuZOrder);

    // ==========================================================
    // 5. 添加背景精灵
    // ==========================================================

    auto sprite = Sprite::create("Scene/Backgrounds/startMenu.png");
    if (sprite == nullptr)
    {
        problemLoading("'Scene/Backgrounds/startMenu.png'");
    }
    else
    {
        sprite->setPosition(Vec2::ZERO);
        contentContainer->addChild(sprite, 0);

        // 缩放整个内容容器以适应屏幕
        Size textureSize = sprite->getContentSize();
        float scaleX = visibleSize.width / textureSize.width;
        float scaleY = visibleSize.height / textureSize.height;
        float scaleFactor = std::min(scaleX, scaleY);

        // 将相同的缩放比例应用到 X 和 Y 轴
        contentContainer->setScale(scaleFactor);
    }

    // ==========================================================
    // 5.1 角落装饰精灵（放在背景之上，但不遮挡菜单按钮）
    // ==========================================================
    const int cornerSpriteZOrder = menuZOrder - 1; // 背景 z=0，菜单 z=1

    // 以主菜单背景为参考，定位到四角（contentContainer 的坐标原点在中心）
    const Size referenceSize = (sprite != nullptr) ? sprite->getContentSize() : visibleSize;
    const Vec2 halfSize(referenceSize.width * 0.5f, referenceSize.height * 0.5f);

    // 角落装饰的轻微摇晃/漂浮效果（注意：装饰仅用于表现，不参与任何交互）
    auto addCornerWobbleEffect = [](Node *target,
                                    const Vec2 &moveOffset,
                                    float rotateAmplitude,
                                    float moveDuration,
                                    float rotateDuration)
    {
        if (!target)
        {
            return;
        }

        target->setRotation(0.0f);

        // 轻微漂浮（建议传入“向内”的偏移，避免角落元素漂到屏幕外导致看不出来在动）
        auto moveAction = RepeatForever::create(Sequence::create(
            EaseSineInOut::create(MoveBy::create(moveDuration, moveOffset)),
            EaseSineInOut::create(MoveBy::create(moveDuration, -moveOffset)),
            nullptr));

        // 轻微左右摇摆（基于锚点旋转，形成“摇晃”观感）
        auto rotateAction = RepeatForever::create(Sequence::create(
            EaseSineInOut::create(RotateTo::create(rotateDuration, rotateAmplitude)),
            EaseSineInOut::create(RotateTo::create(rotateDuration, -rotateAmplitude)),
            nullptr));

        target->runAction(moveAction);
        target->runAction(rotateAction);
    };

    // backman：左下角
    auto backman = Sprite::create("Scene/Backgrounds/backman.png");
    if (backman == nullptr)
    {
        problemLoading("Scene/Backgrounds/backman.png");
    }
    else
    {
        backman->setAnchorPoint(Vec2(0.0f, 0.0f));
        backman->setPosition(Vec2(-halfSize.x, -halfSize.y + visibleSize.height * 0.1f)); // 微调一点点位置
        backman->setScale(2.0f);
        contentContainer->addChild(backman, cornerSpriteZOrder);
        // backman 比较大，旋转幅度要小一些，否则会“晃得太厉害”
        addCornerWobbleEffect(backman, Vec2(0.0f, 4.0f), 0.6f, 1.3f, 1.0f);
    }

    // dragon1：右下角
    auto dragon1 = Sprite::create("Scene/Backgrounds/dragon1.png");
    if (dragon1 == nullptr)
    {
        problemLoading("Scene/Backgrounds/dragon1.png");
    }
    else
    {
        dragon1->setAnchorPoint(Vec2(1.0f, 0.0f));
        dragon1->setPosition(Vec2(halfSize.x, -halfSize.y));
        dragon1->setScale(2.0f);
        contentContainer->addChild(dragon1, cornerSpriteZOrder);
        // 右下角：向左上（向内）漂浮，避免漂到屏幕外导致“看起来没动”
        addCornerWobbleEffect(dragon1, Vec2(-5.0f, 4.0f), 1.0f, 1.2f, 0.9f);
    }

    // dragon2：右上角
    auto dragon2 = Sprite::create("Scene/Backgrounds/dragon2.png");
    if (dragon2 == nullptr)
    {
        problemLoading("Scene/Backgrounds/dragon2.png");
    }
    else
    {
        dragon2->setAnchorPoint(Vec2(1.0f, 1.0f));
        dragon2->setPosition(Vec2(halfSize.x, halfSize.y));
        dragon2->setScale(2.0f);
        contentContainer->addChild(dragon2, cornerSpriteZOrder);
        // 右上角：向左下（向内）漂浮，避免漂到屏幕外导致“看起来没动”
        addCornerWobbleEffect(dragon2, Vec2(-5.0f, -4.0f), 1.0f, 1.15f, 0.85f);

        // dragon2 左下角火焰粒子（作为 dragon2 子节点，保证跟随摇晃且不遮挡菜单按钮）
        auto dragonFire = ParticleSystemQuad::create("Particle/par_dragon_fire.plist");
        if (!dragonFire)
        {
            problemLoading("Particle/par_dragon_fire.plist");
        }
        else
        {
            dragonFire->setAnchorPoint(Vec2(0.0f, 0.0f));
            dragonFire->setPosition(Vec2(80.0f, 85.0f)); // dragon2 的左下角
            dragonFire->setPositionType(ParticleSystem::PositionType::GROUPED);
            dragonFire->setScale(0.35f);
            dragonFire->resetSystem();
            dragon2->addChild(dragonFire, 1);
        }
    }

    // ==========================================================
    // 6. 右侧火星粒子效果
    // ==========================================================
    const float particleMarginX = 40.0f;
    const float particleBaseX = origin.x + visibleSize.width - particleMarginX;
    const float particleYPositions[] = {
        origin.y + visibleSize.height * 0.25f,
        origin.y + visibleSize.height * 0.5f,
        origin.y + visibleSize.height * 0.75f};
    for (float y : particleYPositions)
    {
        auto particleSystem = ParticleSystemQuad::create("Particle/par_warfire.plist");
        if (!particleSystem)
        {
            problemLoading("Particle/par_warfire.plist");
            continue;
        }
        particleSystem->setScale(0.7f);
        particleSystem->setPosition(Vec2(particleBaseX, y));
        particleSystem->setPositionType(ParticleSystem::PositionType::FREE);
        particleSystem->resetSystem();
        this->addChild(particleSystem, contentZOrder + 1);
    }

    // ==========================================================
    // 7. 职业选择提示（不影响主菜单布局）
    // ==========================================================
    _roleHintLabel = Label::createWithTTF("", GameSceneConfig::Scene::DEFAULT_FONT_PATH, 22);
    if (_roleHintLabel)
    {
        _roleHintLabel->setAnchorPoint(Vec2(0.0f, 0.0f));
        _roleHintLabel->setPosition(Vec2(origin.x + 18.0f, origin.y + 18.0f));
        _roleHintLabel->setColor(Color3B(230, 230, 230));
        this->addChild(_roleHintLabel, GameSceneConfig::UI::Z_ORDER);
        updateRoleHintLabel();
    }

    // 1/2/3 快捷切换职业（战士/刺客/法师）
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event*)
    {
        CharacterRole newRole = _selectedRole;
        if (keyCode == EventKeyboard::KeyCode::KEY_1)
        {
            newRole = CharacterRole::WARRIOR;
        }
        else if (keyCode == EventKeyboard::KeyCode::KEY_2)
        {
            newRole = CharacterRole::ASSASSIN;
        }
        else if (keyCode == EventKeyboard::KeyCode::KEY_3)
        {
            newRole = CharacterRole::MAGE;
        }

        if (newRole != _selectedRole)
        {
            _selectedRole = newRole;
            updateRoleHintLabel();
            if (auto saveManager = SaveManager::getInstance())
            {
                saveManager->setSessionSelectedRole(_selectedRole);
            }
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    std::string musicFile = "Scene/MusicOfScene/Music_HelloWorldScene.mp3";
    float musicVolume = GameSceneConfig::UI::MainMenu::BGM_VOLUME;
    this->scheduleOnce(
        [musicFile, musicVolume](float dt)
        {
            MusicManager::getInstance()->playBGM(musicFile, true, musicVolume);
        },
        GameSceneConfig::UI::MainMenu::BGM_DELAY_SECONDS, // 必须加一定延迟否则会被场景切换截断
        "PlayMusicAfterSceneChange");
    return true;
}

void HelloWorld::updateRoleHintLabel()
{
    if (!_roleHintLabel)
    {
        return;
    }

    _roleHintLabel->setString(StringUtils::format("当前职业：%s（按 1 战士 / 2 刺客 / 3 法师）",
        PlayerRoleConfig::getDisplayName(_selectedRole)));
}

void HelloWorld::menuCloseCallback(Ref *pSender)
{
    // 关闭cocos2d - x游戏场景并退出应用程序
    Director::getInstance()->end();

    /*若要在不退出应用的情况下返回到原生 iOS 屏幕（如果存在），请不要使用上面给出的 Director::getInstance()->end()，而是触发在 RootViewController.mm 中创建的自定义事件，如下所示*/

    // EventCustom customEndEvent("game_scene_close_event");
    //_eventDispatcher->dispatchEvent(&customEndEvent);
}

void HelloWorld::menuStartCallback(Ref* pSender)
{
    // 1. [最佳实践] 禁用按钮防止“连点”
    // 在异步加载场景前，防止用户多次点击导致创建多个 LoadingScene
    if (pSender) {
        static_cast<MenuItem*>(pSender)->setEnabled(false);
    }

    // 新开局：清空运行时数据，避免“上一局的等级/装备”带入
    if (auto saveManager = SaveManager::getInstance())
    {
        saveManager->clearRuntimePlayerData();
        saveManager->clearRuntimePlayerPosition();
        saveManager->setSessionSelectedRole(_selectedRole);
    }

    // 2. 使用强类型 SceneID 替换 HomeScene::MAP_ID (int)
    // 确保 LoadingScene::createScene 的参数也已经改成了 SceneID 类型
    auto newScene = LoadingScene::createScene(SceneID::HOME);

    // 3. 执行切换
    if (newScene) {
        // 使用 replaceScene 释放主菜单资源
        Director::getInstance()->replaceScene(newScene);
    }
    else {
        CCLOG("Error: Failed to create LoadingScene for SceneID::HOME");
        // 如果创建失败，记得恢复按钮点击
        if (pSender) static_cast<MenuItem*>(pSender)->setEnabled(true);
    }
}

void HelloWorld::menuSaveCallback(Ref *pSender)
{
    // 主菜单中只能加载游戏，不能保存
    auto saveMenu = SaveMenuLayer::create(SaveMenuLayer::Mode::LOAD);

    // 设置加载成功回调
    saveMenu->setLoadSuccessCallback([this](const SaveSlotData &saveData)
                                     {
        CCLOG("HelloWorld - 加载存档成功，场景: %s", saveData.progressData.currentSceneName.c_str());

        // 根据存档的场景名称创建对应的场景
        Scene* targetScene = nullptr;
        const std::string& sceneName = saveData.progressData.currentSceneName;

        if (sceneName == "起源之菇")
        {
            targetScene = OriginMushroomScene::createScene();
        }
        else if (sceneName == "神秘之森")
        {
            targetScene = MysteryForestScene::createScene();
        }
        else if (sceneName == "冒险王之家")
        {
            targetScene = HomeScene::createScene();
        }
        else
        {
            CCLOG("HelloWorld - 未知的场景名称: %s，跳转到地图选择", sceneName.c_str());
            targetScene = MapScene::createScene();
        }

        if (targetScene)
        {
            // 获取 GameScene 并应用存档数据
            auto gameScene = dynamic_cast<GameScene*>(targetScene);
            if (gameScene)
            {
                // 在场景初始化后应用玩家数据
                // 使用 scheduleOnce 延迟执行，确保场景完全初始化
                auto saveManager = SaveManager::getInstance();
                auto playerData = saveData.playerData;
                auto playerPos = Vec2(saveData.progressData.playerPosX, saveData.progressData.playerPosY);

                // 同步运行时数据：保证新场景创建玩家时即可拿到正确的等级/经验等（避免先用旧数据刷怪/显示）
                if (saveManager)
                {
                    saveManager->setRuntimePlayerData(playerData);
                }

                gameScene->scheduleOnce([saveManager, playerData, playerPos](float dt) {
                    auto currentScene = Director::getInstance()->getRunningScene();
                    auto currentGameScene = dynamic_cast<GameScene*>(currentScene);
                    if (currentGameScene)
                    {
                        auto player = currentGameScene->getPlayer();
                        if (player)
                        {
                            // 应用玩家数据
                            saveManager->applyPlayerData(player, playerData);
                            // 设置玩家位置
                            player->setPosition(playerPos);
                            CCLOG("HelloWorld - 玩家数据已恢复，位置: (%.1f, %.1f)", playerPos.x, playerPos.y);
                        }
                    }
                }, 0.1f, "apply_save_data");
            }

            // 切换到目标场景
            auto transition = TransitionFade::create(GameSceneConfig::Scene::TRANSITION_DURATION, targetScene, Color3B::BLACK);
            Director::getInstance()->replaceScene(transition);
        }
        else
        {
            CCLOG("HelloWorld - 创建目标场景失败");
        } });

    // 直接添加到场景而不是 contentContainer，避免受容器缩放影响
    this->addChild(saveMenu, GameSceneConfig::UI::Z_ORDER);
}

void HelloWorld::menuMapCallback(Ref *pSender)
{
    auto mapScene = MapScene::createScene();
    if (!mapScene)
    {
        CCLOG("Error: Failed to create MapScene.");
        return;
    }

    auto transition = TransitionFade::create(GameSceneConfig::Scene::MENU_TRANSITION_DURATION, mapScene, Color3B::BLACK);
    Director::getInstance()->pushScene(transition);
}
void HelloWorld::menuSetCallback(Ref *pSender)
{
    auto setMenu = SettingMenuLayer::create();

    // 直接添加到场景而不是 contentContainer，避免受容器缩放影响
    this->addChild(setMenu, GameSceneConfig::UI::Z_ORDER);
}
std::vector<std::string> HelloWorld::getPreloadResourcePaths() {
    return {
        // --- 核心背景 ---
        "Scene/Backgrounds/startMenu.png",
        "Scene/Backgrounds/backman.png",
        "Scene/Backgrounds/dragon1.png",
        "Scene/Backgrounds/dragon2.png",

        // --- 菜单按钮 UI ---
        "Scene/UI/StartItemNormal.png",
        "Scene/UI/StartItemSelect.png",
        "Scene/UI/SetingNormal.png",
        "Scene/UI/SetingSelect.png",
        "Scene/UI/MapNormal.png",
        "Scene/UI/MapSelect.png",
        "Scene/UI/SaveNormal.png",
        "Scene/UI/SaveSelect.png",
        "Scene/UI/CloseNormal.png",     // 虽然被注释，建议预载
        "Scene/UI/CloseSelected.png"
    };
}

//此函数可以作为模板，注册主菜单场景到全局场景注册中心
void HelloWorld::setupRegistry()
{
    SceneInfo info;

    info.creator = []() { return HelloWorld::createScene(); };
    info.imagePaths = HelloWorld::getPreloadResourcePaths();

    // 利用你的 Helper 进行预热
    info.onResourcesLoaded = []() {
        CCLOG("HelloWorld: Pre-warming particles via Helper...");

        // 1. 预热通用粒子（伤害、升级等，防止战斗卡顿）
        ParticlePreloadHelper::preloadCommonParticles();

        // 2. 预热主菜单特有粒子（如果有不在 Common 列表里的）
        // ParticlePreloadHelper::preloadParticlePlists({"Particle/special_menu_effect.plist"});

        // 3. 预热音频（把 AudioEngine 的首次初始化/解码开销挪到 LoadingScene，避免主菜单粒子“卡住一秒”）
        cocos2d::experimental::AudioEngine::preload("Scene/MusicOfScene/Music_HelloWorldScene.mp3");
        cocos2d::experimental::AudioEngine::preload("Scene/MusicOfScene/Music_HomeScene.mp3");
        };

    SceneRegistry::getInstance()->registerScene(SceneID::HELLO_WORLD, info);
}
