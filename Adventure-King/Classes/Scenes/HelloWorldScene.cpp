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
#include "Scenes/Layers/CloudAuthLayer.h"
#include"Utils/ParticlePreloadHelper.h"
#include"Utils/ImeHelper.h"
#include "Managers/MusicManager.h"
#include "audio/include/AudioEngine.h"
#include"Configs/GameSceneConfig.h"
#include"Managers/SceneRegistry.h"
#include "Save/SaveData.h"
#include "Save/SaveManager.h"
#include "Save/Cloud/CloudSyncService.h"
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

    if (SetItem)
    {
        // SetItem 在下方按钮组左侧
        SetItem->setPosition(Vec2(-buttonHorizontalSpacing * 0.5f, sub_menu_y));
    }

    if (SaveItem)
    {
        // SaveItem 在下方按钮组右侧
        SaveItem->setPosition(Vec2(buttonHorizontalSpacing * 0.5f, sub_menu_y));
    }

    // ==========================================================
    // 4. 创建菜单并添加
    // ==========================================================

    auto menu = Menu::create(StartItem, SetItem, SaveItem, NULL);

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

    // ==========================================================
    // 7.1 云端账号入口：游客登录 / 登录注册
    // ==========================================================
    // 注意：这组 UI 直接挂到场景上，避免受 contentContainer 缩放影响
    {
        _cloudAccountLabel = Label::createWithTTF("", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 20);
        if (_cloudAccountLabel)
        {
            _cloudAccountLabel->setAnchorPoint(Vec2(0.0f, 1.0f));
            _cloudAccountLabel->setPosition(Vec2(origin.x + 18.0f, origin.y + visibleSize.height - 18.0f));
            this->addChild(_cloudAccountLabel, GameSceneConfig::UI::Z_ORDER);
            updateCloudAccountLabel();
        }

        auto cloudMenu = Menu::create();
        cloudMenu->setPosition(Vec2::ZERO);
        this->addChild(cloudMenu, GameSceneConfig::UI::Z_ORDER);

        auto guestItem = MenuItemLabel::create(
            Label::createWithTTF("游客登录（禁用云存）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22),
            [this](Ref *) {
                auto cloud = CloudSyncService::getInstance();
                cloud->setGuestMode(true);
                updateCloudAccountLabel();
            });
        guestItem->setAnchorPoint(Vec2(0.0f, 1.0f));
        guestItem->setPosition(Vec2(origin.x + 18.0f, origin.y + visibleSize.height - 48.0f));
        cloudMenu->addChild(guestItem);

        auto loginItem = MenuItemLabel::create(
            Label::createWithTTF("登录/注册", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22),
            [this](Ref *) {
                showCloudAuthLayer();
            });
        loginItem->setAnchorPoint(Vec2(0.0f, 1.0f));
        loginItem->setPosition(Vec2(origin.x + 18.0f, origin.y + visibleSize.height - 78.0f));
        cloudMenu->addChild(loginItem);
    }

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

    _roleHintLabel->setString(StringUtils::format("当前职业：%s（开始游戏后可重新选择）",
        PlayerRoleConfig::getDisplayName(_selectedRole)));
}

void HelloWorld::updateCloudAccountLabel()
{
    if (!_cloudAccountLabel)
    {
        return;
    }

    auto cloud = CloudSyncService::getInstance();
    std::string text;
    Color4B color(220, 120, 120, 255);

    if (cloud->isGuestMode())
    {
        text = "账号：游客（云存禁用）";
        color = Color4B(210, 200, 120, 255);
    }
    else if (cloud->isConfigured())
    {
        const std::string user = cloud->getActiveUsername();
        text = user.empty() ? "账号：已登录" : ("账号：" + user);
        color = Color4B(120, 220, 120, 255);
    }
    else
    {
        text = "账号：未登录（云存不可用）";
    }

    _cloudAccountLabel->setString(text);
    _cloudAccountLabel->setTextColor(color);
}

void HelloWorld::showCloudAuthLayer()
{
    if (_cloudAuthLayer)
    {
        return;
    }

    _cloudAuthLayer = CloudAuthLayer::create([this](bool ok, const std::string & /*message*/) {
        // 无论成功/取消，都刷新一次账号状态（成功后 CloudSyncService 已写入运行时账号）
        updateCloudAccountLabel();
        hideCloudAuthLayer();
    });
    if (!_cloudAuthLayer)
    {
        CCLOG("HelloWorld - 创建 CloudAuthLayer 失败");
        return;
    }

    this->addChild(_cloudAuthLayer, GameSceneConfig::UI::Z_ORDER + 50);
}

void HelloWorld::hideCloudAuthLayer()
{
    if (!_cloudAuthLayer)
    {
        return;
    }

    _cloudAuthLayer->removeFromParent();
    _cloudAuthLayer = nullptr;
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
    // 点击“开始游戏”强制先选择一个存档槽位：
    // - 空槽位：新开游戏（进入职业选择）
    // - 非空槽位：读取存档并直接进入对应关卡
    auto startMenuItem = dynamic_cast<MenuItem*>(pSender);
    if (!startMenuItem)
    {
        CCLOG("HelloWorld::menuStartCallback - pSender 不是 MenuItem，忽略");
        return;
    }

    // 防止连点弹出多个菜单
    startMenuItem->setEnabled(false);

    auto saveMenu = SaveMenuLayer::create(SaveMenuLayer::Mode::START);
    if (!saveMenu)
    {
        CCLOG("HelloWorld::menuStartCallback - 创建存档槽位选择菜单失败");
        startMenuItem->setEnabled(true);
        return;
    }

    // 关闭（取消）时恢复按钮可点
    saveMenu->setCloseCallback([startMenuItem]() {
        if (startMenuItem)
        {
            startMenuItem->setEnabled(true);
        }
    });

    saveMenu->setStartSlotCallback([this, startMenuItem](int /*slotIndex*/, bool hasSave, const SaveSlotData &saveData)
                                   {
        // 选中槽位后，菜单会被移除，按钮可由 closeCallback 统一恢复

        if (!hasSave)
        {
            // 新开局：进入职业选择（职业选择确认后进入 HOME）
            this->showRoleSelectLayer(startMenuItem);
            return;
        }

        // 读取存档：统一走 LoadingScene（与 MapScene/关卡内读档一致）
        const std::string &sceneName = saveData.progressData.currentSceneName;
        auto registry = SceneRegistry::getInstance();
        SceneID targetID = registry ? registry->getSceneIDByName(sceneName) : SceneID::NONE;

        if (targetID == SceneID::NONE)
        {
            CCLOG("HelloWorld - 读档失败：注册表中不存在场景 [%s]，跳转到地图选择", sceneName.c_str());
            auto mapScene = MapScene::createScene();
            if (mapScene)
            {
                auto transition = TransitionFade::create(GameSceneConfig::Scene::TRANSITION_DURATION, mapScene, Color3B::BLACK);
                Director::getInstance()->replaceScene(transition);
            }
            return;
        }

        auto saveManager = SaveManager::getInstance();
        if (saveManager)
        {
            saveManager->setRuntimePlayerData(saveData.playerData);
            saveManager->setRuntimePlayerPosition(Vec2(saveData.progressData.playerPosX, saveData.progressData.playerPosY));
            saveManager->setRuntimeProgressData(saveData.progressData);
        }

        auto loadingScene = LoadingScene::createScene(targetID);
        if (!loadingScene)
        {
            CCLOG("HelloWorld - 创建 LoadingScene 失败，跳转到地图选择");
            auto mapScene = MapScene::createScene();
            if (mapScene)
            {
                auto transition = TransitionFade::create(GameSceneConfig::Scene::TRANSITION_DURATION, mapScene, Color3B::BLACK);
                Director::getInstance()->replaceScene(transition);
            }
            return;
        }

        auto transition = TransitionFade::create(GameSceneConfig::Scene::TRANSITION_DURATION, loadingScene, Color3B::BLACK);
        Director::getInstance()->replaceScene(transition);
    });

    // 直接添加到场景而不是 contentContainer，避免受容器缩放影响
    this->addChild(saveMenu, GameSceneConfig::UI::Z_ORDER);
}

void HelloWorld::menuSaveCallback(Ref *pSender)
{
    // 主菜单中只能加载游戏，不能保存
    auto saveMenu = SaveMenuLayer::create(SaveMenuLayer::Mode::LOAD);

    // 设置加载成功回调
    saveMenu->setLoadSuccessCallback([this](const SaveSlotData &saveData)
                                     {
        CCLOG("HelloWorld - 加载存档成功，场景: %s", saveData.progressData.currentSceneName.c_str());

        // 注意：主菜单读档不能先 createScene 再 setRuntimeXXX，
        // 否则 GameScene::initPlayer 读取不到 runtime 数据，会导致“读档仍是 Lv.1 + 出生点”。
        // 统一走 LoadingScene（与 MapScene/关卡内读档一致）：
        // 1) 先写入 SaveManager 的 runtime 数据
        // 2) 再进入 LoadingScene，由 LoadingScene 创建目标关卡并自动恢复
        const std::string& sceneName = saveData.progressData.currentSceneName;
        auto registry = SceneRegistry::getInstance();
        SceneID targetID = registry ? registry->getSceneIDByName(sceneName) : SceneID::NONE;

        if (targetID == SceneID::NONE)
        {
            CCLOG("HelloWorld - 读档失败：注册表中不存在场景 [%s]，跳转到地图选择", sceneName.c_str());
            auto mapScene = MapScene::createScene();
            if (mapScene)
            {
                auto transition = TransitionFade::create(GameSceneConfig::Scene::TRANSITION_DURATION, mapScene, Color3B::BLACK);
                Director::getInstance()->replaceScene(transition);
            }
            return;
        }

        auto saveManager = SaveManager::getInstance();
        if (saveManager)
        {
            saveManager->setRuntimePlayerData(saveData.playerData);
            saveManager->setRuntimePlayerPosition(Vec2(saveData.progressData.playerPosX, saveData.progressData.playerPosY));
            saveManager->setRuntimeProgressData(saveData.progressData);
        }

        auto loadingScene = LoadingScene::createScene(targetID);
        if (!loadingScene)
        {
            CCLOG("HelloWorld - 创建 LoadingScene 失败，跳转到地图选择");
            auto mapScene = MapScene::createScene();
            if (mapScene)
            {
                auto transition = TransitionFade::create(GameSceneConfig::Scene::TRANSITION_DURATION, mapScene, Color3B::BLACK);
                Director::getInstance()->replaceScene(transition);
            }
            return;
        }

        auto transition = TransitionFade::create(GameSceneConfig::Scene::TRANSITION_DURATION, loadingScene, Color3B::BLACK);
        Director::getInstance()->replaceScene(transition);
    });

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
    Director::getInstance()->replaceScene(transition);
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
        // 关闭按钮（MapScene 复用）
        "Scene/UI/CloseSaveMenu.png",
        "Scene/UI/CloseSaveMenuSelected.png",

        // 职业选择预览（点击开始后会用到，避免首次弹窗卡一下）
        "Sprites/Characters/Player/man/default/spr_man_run.png",
        "Sprites/Characters/Player/maaer/default/spr_maaer_run_1.png",
        "Sprites/Characters/Player/Klee/default/spr_klee_run.png",
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

// ==========================================================
// 职业选择弹窗
// ==========================================================

void HelloWorld::showRoleSelectLayer(MenuItem* startMenuItem)
{
    using namespace GameSceneConfig::UI::RoleSelectLayer;
    if (_roleSelectLayer)
    {
        return;
    }

    _pendingStartMenuItem = startMenuItem;
    if (_pendingStartMenuItem)
    {
        _pendingStartMenuItem->setEnabled(false); // 避免连点弹出多个弹窗
    }

    const auto visibleSize = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();
    const Vec2 center(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f);



    _roleSelectLayer = LayerColor::create(Color4B(0, 0, 0, kOverlayAlpha));
    _roleSelectLayer->setContentSize(visibleSize);
    _roleSelectLayer->setIgnoreAnchorPointForPosition(false);
    _roleSelectLayer->setAnchorPoint(Vec2::ZERO);
    _roleSelectLayer->setPosition(origin);
    this->addChild(_roleSelectLayer, GameSceneConfig::UI::Z_ORDER + 10);

    // 吞掉触摸，避免点到背后的主菜单按钮
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, _roleSelectLayer);

    // 弹窗面板（纯色，避免依赖额外资源）
    const float panelW = std::min(visibleSize.width * kPanelWidthRatio, kMaxPanelWidth);
    const float panelH = std::min(visibleSize.height * kPanelHeightRatio, kMaxPanelHeight);
    auto panel = LayerColor::create(Color4B(30, 30, 40, 235), panelW, panelH);
    panel->setIgnoreAnchorPointForPosition(false);
    panel->setAnchorPoint(Vec2(0.5f, 0.5f));
    panel->setPosition(center);
    _roleSelectLayer->addChild(panel, 1);

    // 标题
    auto title = Label::createWithTTF("选择角色", GameSceneConfig::Scene::DEFAULT_FONT_PATH, 34);
    if (title)
    {
        title->setAnchorPoint(Vec2(0.5f, 1.0f));
        title->setPosition(Vec2(panelW * 0.5f, panelH - kTitleTopPadding));
        title->setColor(Color3B(240, 240, 240));
        panel->addChild(title, 2);
    }

    // 预览区域（右侧）
    _rolePreviewTargetHeight = panelH * kPreviewHeightRatio;
    refreshRolePreview();
    if (_rolePreviewSprite)
    {
        // 预览 sprite 作为 panel 子节点，方便用 panel 尺寸做布局
        _rolePreviewSprite->removeFromParent();
        panel->addChild(_rolePreviewSprite, 2);

        _rolePreviewSprite->setAnchorPoint(Vec2(0.5f, 0.0f));
        _rolePreviewSprite->setPosition(Vec2(panelW * kPreviewXRatio, panelH * kPreviewBottomRatio));
    }

    // 左侧：职业按钮
    const float roleListX = panelW * kRoleListXRatio;
    auto makeRoleItem = [this, roleListX](CharacterRole role, float y, const Color3B& color)
    {
        auto label = Label::createWithTTF(PlayerRoleConfig::getDisplayName(role),
                                          GameSceneConfig::Scene::DEFAULT_FONT_PATH,
                                          28);
        if (label)
        {
            label->setColor(color);
        }

        auto item = MenuItemLabel::create(label, [this, role](Ref*)
        {
            _selectedRole = role;
            updateRoleHintLabel();
            refreshRolePreview();
        });

        if (item)
        {
            item->setAnchorPoint(Vec2(0.0f, 0.5f));
            item->setPosition(Vec2(roleListX, y));
        }
        return item;
    };

    const float listTop = panelH * kRoleListTopRatio;
    const float listGap = kRoleListGap;
    auto warriorItem = makeRoleItem(CharacterRole::WARRIOR, listTop, Color3B(255, 235, 190));
    auto assassinItem = makeRoleItem(CharacterRole::ASSASSIN, listTop - listGap, Color3B(190, 235, 255));
    auto mageItem = makeRoleItem(CharacterRole::MAGE, listTop - listGap * 2.0f, Color3B(210, 200, 255));

    // 底部按钮：开始/返回
    auto startLabel = Label::createWithTTF("开始", GameSceneConfig::Scene::DEFAULT_FONT_PATH, 30);
    auto backLabel = Label::createWithTTF("返回", GameSceneConfig::Scene::DEFAULT_FONT_PATH, 30);
    auto confirmStartItem = MenuItemLabel::create(startLabel, [this](Ref*) { startGameWithSelectedRole(); });
    auto cancelBackItem = MenuItemLabel::create(backLabel, [this](Ref*) { hideRoleSelectLayer(true); });
    if (confirmStartItem) confirmStartItem->setPosition(Vec2(panelW * kConfirmXRatio, panelH * kActionYRatio));
    if (cancelBackItem) cancelBackItem->setPosition(Vec2(panelW * kCancelXRatio, panelH * kActionYRatio));

    auto menu = Menu::create(warriorItem, assassinItem, mageItem, confirmStartItem, cancelBackItem, nullptr);
    if (menu)
    {
        menu->setPosition(Vec2::ZERO);
        panel->addChild(menu, 3);
    }

    // Esc 快捷关闭
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event*)
    {
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            hideRoleSelectLayer(true);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, _roleSelectLayer);
}

void HelloWorld::hideRoleSelectLayer(bool restoreStartButton)
{
    if (_roleSelectLayer)
    {
        // 显式清理事件监听，避免潜在的悬挂监听/重复注册问题
        _eventDispatcher->removeEventListenersForTarget(_roleSelectLayer);
        _roleSelectLayer->removeFromParent();
        _roleSelectLayer = nullptr;
    }
    // 弹窗销毁后，预览节点也会随之销毁，这里清空指针避免悬挂引用
    _rolePreviewSprite = nullptr;
    _rolePreviewTargetHeight = 0.0f;

    if (restoreStartButton && _pendingStartMenuItem)
    {
        _pendingStartMenuItem->setEnabled(true);
    }
    _pendingStartMenuItem = nullptr;
}

void HelloWorld::refreshRolePreview()
{
    const char* path = PlayerRoleConfig::getDefaultSpritePath(_selectedRole);
    if (!path)
    {
        return;
    }

    // 预览 sprite 只用于 UI 展示，直接按文件路径加载即可
    auto sprite = Sprite::create(path);
    if (!sprite)
    {
        CCLOG("HelloWorld: 预览角色贴图加载失败：%s", path);
        return;
    }

    Node* parent = nullptr;
    int zOrder = 0;
    Vec2 pos = Vec2::ZERO;
    Vec2 anchor = Vec2(0.5f, 0.0f);
    if (_rolePreviewSprite)
    {
        // 如果已有预览：保留父节点/位置/锚点/层级，只替换贴图
        parent = _rolePreviewSprite->getParent();
        zOrder = _rolePreviewSprite->getLocalZOrder();
        pos = _rolePreviewSprite->getPosition();
        anchor = _rolePreviewSprite->getAnchorPoint();
        _rolePreviewSprite->removeFromParent();
        _rolePreviewSprite = nullptr;
    }

    _rolePreviewSprite = sprite;
    _rolePreviewSprite->setAnchorPoint(anchor);
    _rolePreviewSprite->setPosition(pos);
    // 缩放统一以目标展示高度为准（由 showRoleSelectLayer 设置），避免“第一次/后续刷新”行为不一致
    if (_rolePreviewTargetHeight > 0.0f)
    {
        const float srcH = std::max(1.0f, _rolePreviewSprite->getContentSize().height);
        const float newScale = _rolePreviewTargetHeight / srcH;
        _rolePreviewSprite->setScale(newScale);
    }

    if (parent)
    {
        parent->addChild(_rolePreviewSprite, zOrder);
    }
}

void HelloWorld::startGameWithSelectedRole()
{
    // 防止重复触发
    if (_pendingStartMenuItem)
    {
        _pendingStartMenuItem->setEnabled(false);
    }

    // 新开局：设置会话职业（内部会清空运行时数据/位置）
    if (auto saveManager = SaveManager::getInstance())
    {
        saveManager->setSessionSelectedRole(_selectedRole);
    }

    // 进入 HOME（通过 LoadingScene 走统一的预加载机制）
    auto newScene = LoadingScene::createScene(SceneID::HOME);
    if (newScene)
    {
        Director::getInstance()->replaceScene(newScene);
        return;
    }

    CCLOG("Error: Failed to create LoadingScene for SceneID::HOME");
    hideRoleSelectLayer(true);
}
void HelloWorld::onEnter()
{
    Scene::onEnter(); // 必须先调用父类的实现

    // 进入主菜单时禁用输入法，避免中文输入法抢占键盘输入影响快捷键操作
    ImeHelper::pushDisableIme();
    CCLOG("HelloWorld: Entered scene, IME disabled.");
}

void HelloWorld::onExit()
{
    // 离开主菜单时执行 pop，如果此时没有其他地方持有禁用请求，输入法将恢复
    ImeHelper::popDisableIme();
    CCLOG("HelloWorld: Exited scene, IME counter popped.");

    Scene::onExit(); // 建议在逻辑处理完后调用父类实现
}
