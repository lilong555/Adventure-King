#include "HomeScene.h"
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

    SceneRegistry::getInstance()->registerScene(MAP_ID, info);
}

LevelConfig HomeScene::getLevelConfig() const
{
    LevelConfig config;
    config.tmxMapPath = "Map/Home/home.tmx";
    // 参考起源之菇（mushroom）的背景方案：使用“背景序列”机制（这里是单张整图）
    config.backgroundSeriesPaths = { "Map/Home/home.png" };
    config.collisionLayerName = "collisions";
    config.bornLayerName = "born";
    config.gateLayerName = "gate";
    config.gravity = -1000.0f;
    config.enablePhysicsDebug = false;
    return config;
}

bool HomeScene::init()
{
    // 冒险王之家：使用 GameScene 的统一关卡加载流程（TMX/碰撞/玩家/输入/UI）
    LevelConfig config = getLevelConfig();
    if (!initWithPhysicsConfig(config))
    {
        return false;
    }

    // 播放背景音乐
    std::string musicFile = "Scene/MusicOfScene/Music_HomeScene.mp3";
    float musicVolume = GameConfig::UI::MainMenu::BGM_VOLUME;
    this->scheduleOnce(
        [musicFile, musicVolume](float dt)
        {
            MusicManager::getInstance()->playBGM(musicFile, true, musicVolume);
        },
        GameConfig::UI::MainMenu::BGM_DELAY_SECONDS, // 必须加一定延迟否则会被场景切换截断
        "PlayMusicAfterSceneChange");
    return true;
}
