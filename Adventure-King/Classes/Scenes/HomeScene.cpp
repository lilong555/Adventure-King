#include "HomeScene.h"
#include "GameScene.h"
#include "Scenes/LevelMap.h"
#include "Managers/SceneTransitionManager.h"
#include "Managers/MusicManager.h"
#include "Managers/SceneRegistry.h"
#include "Configs/GameConfigs.h"

USING_NS_CC;

Scene *HomeScene::createScene()
{
    return HomeScene::create();
}

void HomeScene::setupRegistry()
{
    SceneInfo info;
    info.creator = []()
    { return HomeScene::createScene(); };

    // 资源列表：用于 LoadingScene 预加载，避免首次进图卡顿
    // 说明：TMX 会引用 tileset 图片，这里也一并预热到 TextureCache
    info.imagePaths = {
        "Map/Home/home.png",
        "Map/Home/HomeBackground_1.png",
        "Map/Origin_Mushroom/Env_Tree_Oak_Giant_Green.png",
    };

    SceneRegistry::getInstance()->registerScene(ID, info);
}

bool HomeScene::init() {
    // 1. 基础初始化
    if (!GameScene::init()) return false;

    // 2. 关卡特化配置
    LevelConfig config;
    config.tmxMapPath = "Scene/Backgrounds/HomeBackground_1.tmx";

    // 对应 Tiled 里的图层名
    config.collisionLayerName = "collisions"; // 包含地面和左右边界
    config.gateLayerName = "gates";           // 包含传送门区域
    config.bornLayerName = "collisions";      // 包含 PlayerSpawn 点

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
