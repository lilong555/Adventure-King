#include "OriginMushroomScene.h"
#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/Monster/Monsters/GobluMonster.h"
#include "Configs/GameConfigs.h"
#include "Managers/SceneRegistry.h"
USING_NS_CC;
// ============================================================
// OriginMushroomScene 实现（起源之菇）
// ============================================================

Scene* OriginMushroomScene::createScene()
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
    config.playerSpritePath = GameConfig::Scene::DEFAULT_PLAYER_SPRITE;
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
void OriginMushroomScene::setupRegistry()
{
    SceneInfo info;

    // 1. 设置创建器
    info.creator = []() {
        return OriginMushroomScene::createScene();
        };

    // 2. 准备资源路径列表
    // 第一步：放入所有固定的、不需要拼接的字符串
    std::vector<std::string> paths = {
        // --- 公共资源 ---
        "Particle/particle_texture.png",

        // --- 地图选择 UI (防首次卡顿) ---
        "Scene/Backgrounds/MapBackground.png",
        "Scene/UI/mapselectItem_1_selected.png",
        "Scene/UI/mapselectItem_2_selected.png",

        // --- 地图 Tileset 静态贴图 ---
        "Map/Origin_Mushroom/Env_Tree_Oak_Giant_Green.png",
        "Map/Origin_Mushroom/s1.png",
        "Map/Origin_Mushroom/s2.png",
        "Map/Origin_Mushroom/s3.png",
        "Map/Origin_Mushroom/s4.png",

        // --- 怪物静态单图 ---
        "Sprites/Enemies/Goblin/Goblin_idle.png",
        "Sprites/Enemies/Goblin/Goblin_beattacked.png",
        "Sprites/Enemies/Goblu/Goblu.png"
    };

    // 第二步：追加动态生成的序列帧路径 (原逻辑迁移至此)

    // 背景序列
    for (int i = 0; i < GameConfig::Map::OriginMushroom::BACKGROUND_COUNT; ++i)
    {
        paths.push_back(StringUtils::format("%s%02d.png", GameConfig::Map::OriginMushroom::BACKGROUND_PREFIX, i));
    }

    // 哥布林行走/攻击 (1-4)
    for (int i = 1; i <= 4; ++i)
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblin/Goblin_walk_%d.png", i));
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblin/Goblin_attack_%d.png", i));
    }

    // Goblu 行走/攻击 (1-4)
    for (int i = 1; i <= 4; ++i)
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_walk_%d.png", i));
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_attack_%02d.png", i));
    }

    // Goblu 额外攻击帧 (11-15)
    for (int i = 11; i <= 15; ++i)
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_attack_%02d.png", i));
    }

    // Goblu 死亡动画 (1-6)
    for (int i = 1; i <= 6; ++i)
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_death_%d.png", i));
    }

    // 将整理好的 vector 赋值给 info
    info.imagePaths = paths;

    //// 3. 设置预热回调 (动画缓存等)
    //info.onResourcesLoaded = []() {
    //    GoblinMonster::preloadResources();
    //    GobluMonster::preloadResources();
    //    };

    // 注册到管理器 (ID = 1)
    SceneRegistry::getInstance()->registerScene(1, info);
}
