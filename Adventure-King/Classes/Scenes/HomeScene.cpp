#include "HomeScene.h"
#include "Managers/MusicManager.h"
#include "Configs/GameConfigs.h"

USING_NS_CC;

Scene *HomeScene::createScene()
{
    return HomeScene::create();
}

LevelConfig HomeScene::getLevelConfig() const
{
    LevelConfig config;
    config.tmxMapPath = "Map/Home/home.tmx";
    // 使用 Home 的整图背景（与 TMX 像素尺寸一致：3200x1440）
    config.backgroundPath = "Map/Home/home.png";
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
