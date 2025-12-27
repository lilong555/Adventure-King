#include "HomeScene.h"
#include "GameScene.h"
#include "GameUI.h"
#include "Scenes/LevelMap.h"
#include "Managers/SceneTransitionManager.h"
#include "Managers/MusicManager.h"
#include "Managers/SceneRegistry.h"
#include "Configs/GameConfig.h"
#include "Character/Player/PlayerCharacter.h"

USING_NS_CC;

namespace
{
    constexpr float BLESSING_NPC_INTERACT_DISTANCE = 220.0f;
    // 说明：该 NPC 原图尺寸 1521x2670，非常大，需缩放到合理大小
    constexpr float BLESSING_NPC_SPRITE_SCALE = 0.08f;
} // namespace

Scene *HomeScene::createScene()
{
    return HomeScene::create();
}

void HomeScene::setupRegistry()
{
    SceneInfo info;
    info.creator = []()
    { return HomeScene::createScene(); };
    info.sceneName = "冒险王之家";

    // 资源列表：用于 LoadingScene 预加载，避免首次进图卡顿
    // 说明：TMX 会引用 tileset 图片，这里也一并预热到 TextureCache
    info.imagePaths = {
        "Map/Home/home.png",
        "Map/Home/HomeBackground_1.png",
        "Map/Origin_Mushroom/Env_Tree_Oak_Giant_Green.png",
        "Sprites/Characters/Npc/spr_shutouj.png",
    };

    SceneRegistry::getInstance()->registerScene(ID, info);
}

bool HomeScene::init() {
    // 1. 基础初始化
    if (!GameScene::init()) return false;

    // 2. 关卡特化配置
    LevelConfig config;
    config.tmxMapPath = "Map/Home/home.tmx";
    config.playerSpritePath = GameSceneConfig::Scene::DEFAULT_PLAYER_SPRITE;

    // 对应 Tiled 里的图层名
    config.collisionLayerName = "collisions"; // 包含地面和左右边界
    config.gateLayerName = "gate";            // 包含传送门区域
    config.bornLayerName = "born";            // 包含 PlayerSpawn 点

    // 物理参数
    config.gravity = -980.0f;               // 标准重力
    config.enablePhysicsDebug = true;       // 调试模式：显示红色碰撞框

    // 3. 调用基类模板方法完成流水线初始化
    if (!initWithPhysicsConfig(config)) {
        CCLOG("HomeScene - Failed to init with physics config");
        return false;
    }

    //实现地图缩放
    if (_levelMap) {
        // 1. 获取地图的原始 TMX 对象
        auto tiledMap = _levelMap->getTileMap();
        if (tiledMap) {
            auto visibleSize = Director::getInstance()->getVisibleSize();
            auto mapSize = tiledMap->getContentSize();

            // 2. 计算缩放比（以高度为基准铺满屏幕）
            float scaleY = visibleSize.height / mapSize.height;

            // 3. 应用缩放
            // 注意：为了保持比例，X 和 Y 通常使用相同的缩放值
            _gameLayer->setScale(scaleY);

            // 4. 更新地图内容大小，确保相机 Follow 边界正确
            //_gameLayer->setContentSize(Size(mapSize.width * scaleY, mapSize.height * scaleY));
        }
    }

    // 赐福入口 NPC（在 Home 地图点位生成）
    initBlessingNpc();

    // 4.播放背景音乐
    std::string musicFile = "Scene/MusicOfScene/Music_HomeScene.mp3";
    float musicVolume = GameSceneConfig::UI::MainMenu::BGM_VOLUME;
    this->scheduleOnce(
        [musicFile, musicVolume](float dt)
        {
            MusicManager::getInstance()->playBGM(musicFile, true, musicVolume);
        },
        0.5f, //必须加一定延迟否则会被场景切换截断
        "PlayMusicAfterSceneChange"
    );
    CCLOG("HomeScene - Initialized successfully");
    return true;
}

void HomeScene::onExit()
{
    // 清理回调，避免场景退出后仍有输入事件触发回调导致访问已释放对象
    if (_inputController)
    {
        _inputController->setNpcQuery({});
        _inputController->setNpcInteract({});
    }
    if (_uiController)
    {
        _uiController->setNpcHintQuery({});
    }

    GameScene::onExit();
}

void HomeScene::initBlessingNpc()
{
    _blessingNpcSprite = nullptr;
    if (!_levelMap || !_gameLayer)
    {
        return;
    }

    auto tiledMap = _levelMap->getTileMap();
    if (!tiledMap)
    {
        return;
    }

    auto group = tiledMap->getObjectGroup("npc");
    if (!group)
    {
        CCLOG("HomeScene - 未找到 objectgroup: npc");
        return;
    }

    Vec2 npcPos = Vec2::ZERO;
    bool found = false;
    auto objects = group->getObjects();
    for (const auto &obj : objects)
    {
        auto dict = obj.asValueMap();
        if (dict["name"].asString() != "npc")
        {
            continue;
        }
        npcPos = Vec2(dict["x"].asFloat(), dict["y"].asFloat());
        found = true;
        break;
    }

    if (!found)
    {
        CCLOG("HomeScene - npc 组内未找到 name=npc 的点位");
        return;
    }

    auto sprite = Sprite::create("Sprites/Characters/Npc/spr_shutouj.png");
    if (!sprite)
    {
        CCLOG("HomeScene - 创建赐福 NPC 失败：spr_shutouj.png");
        return;
    }

    sprite->setAnchorPoint(Vec2(0.5f, 0.0f)); // 脚下锚点，贴地更直观
    sprite->setPosition(npcPos);
    sprite->setScale(BLESSING_NPC_SPRITE_SCALE);
    _gameLayer->addChild(sprite, PLAYER_Z_ORDER - 1);
    _blessingNpcSprite = sprite;

    // W 键交互：靠近 NPC 时进入赐福（优先级高于门区与跳跃）
    if (_inputController)
    {
        _inputController->setNpcQuery([this]() { return this->isPlayerAtBlessingNpc(); });
        _inputController->setNpcInteract([this]() {
            if (!_uiController)
            {
                return;
            }

            auto ui = _uiController->getGameUI();
            if (!ui || ui->isBlessingNpcShowing())
            {
                return;
            }

            // 进入赐福时暂停世界（Home 也可能有玩家移动/跳跃）
            setGamePaused(true);
            ui->showBlessingNpc();
        });
    }

    // UI 提示：靠近 NPC 显示“按W进入赐福”
    if (_uiController)
    {
        _uiController->setNpcHintQuery([this]() { return this->isPlayerAtBlessingNpc(); });
    }
}

bool HomeScene::isPlayerAtBlessingNpc() const
{
    if (!_player || !_blessingNpcSprite)
    {
        return false;
    }

    const Vec2 playerPos = _player->getPosition();
    const Vec2 npcPos = _blessingNpcSprite->getPosition();
    return playerPos.distance(npcPos) <= BLESSING_NPC_INTERACT_DISTANCE;
}
