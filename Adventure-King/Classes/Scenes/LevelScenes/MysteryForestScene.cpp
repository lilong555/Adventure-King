#include "MysteryForestScene.h"
#include "Configs/GameConfigs.h"
#include "Managers/SceneRegistry.h"
USING_NS_CC;
// ============================================================
// MysteryForestScene 实现（神秘之森）
// ============================================================

Scene* MysteryForestScene::createScene()
{
    return MysteryForestScene::create();
}

LevelConfig MysteryForestScene::getLevelConfig() const
{
    LevelConfig config;
    config.tmxMapPath = "Map/dungeon/dungeon.tmx";
    config.playerSpritePath = GameConfig::Scene::DEFAULT_PLAYER_SPRITE;
    // 神秘之森：背景由 1~4 四张图从左到右拼接（不重叠）
    config.backgroundSeriesPaths = {
        "Map/dungeon/1.png",
        "Map/dungeon/2.png",
        "Map/dungeon/3.png",
        "Map/dungeon/4.png",
    };
    config.collisionLayerName = "collisions";
    config.bornLayerName = "born";
    config.gateLayerName = "gate";
    config.gravity = -1000.0f;
    config.enablePhysicsDebug = false;
    return config;
}

bool MysteryForestScene::init()
{
    LevelConfig config = getLevelConfig();
    if (!initWithPhysicsConfig(config))
    {
        return false;
    }
    CCLOG("MysteryForestScene initialized");
    return true;
}

void MysteryForestScene::setupRegistry()
{
    SceneInfo info;

    // 1. 设置创建器
    info.creator = []()
    { return MysteryForestScene::createScene(); };
    info.sceneName = "神秘之森";

    // 2. 资源列表（神秘之森：地牢 tileset）
    info.imagePaths = {
        // 粒子特效使用 plist 内嵌纹理，不需要预加载 particle_texture.png
        "Map/dungeon/1.png",
        "Map/dungeon/2.png",
        "Map/dungeon/3.png",
        "Map/dungeon/4.png",
    };

    SceneRegistry::getInstance()->registerScene(2, info);
}
