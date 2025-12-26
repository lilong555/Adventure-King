#include "OriginMushroomScene.h"
#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/Monster/Monsters/GobluMonster.h"
#include"Character/Monster/Monsters/ObscurMonster.h"
#include"Configs/CharacterAssetConfig.h"
#include "Configs/GameConfig.h"
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
    config.backgroundSeriesPaths.reserve(GameSceneConfig::Map::OriginMushroom::BACKGROUND_COUNT);
    for (int i = 0; i < GameSceneConfig::Map::OriginMushroom::BACKGROUND_COUNT; ++i)
    {
        config.backgroundSeriesPaths.emplace_back(
            StringUtils::format("%s%02d.png",
                GameSceneConfig::Map::OriginMushroom::BACKGROUND_PREFIX,
                i));
    }
    config.playerSpritePath = GameSceneConfig::Scene::DEFAULT_PLAYER_SPRITE;
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
    info.sceneName = "起源之菇";

    // 2. 准备资源路径列表
    // 第一步：放入所有固定的、不需要拼接的字符串
    std::vector<std::string> paths = {
        // --- 公共资源 ---
        // 粒子特效使用 plist 内嵌纹理，不需要预加载 particle_texture.png

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
        "Sprites/Enemies/Goblu/Goblu.png",

        // --- 胜利横幅单图 ---
            "Scene/UI/ClearBanner.png"
    };

    // 第二步：追加动态生成的序列帧路径 (原逻辑迁移至此)

    // 背景序列
    for (int i = 0; i < GameSceneConfig::Map::OriginMushroom::BACKGROUND_COUNT; ++i)
    {
        paths.push_back(StringUtils::format("%s%02d.png", GameSceneConfig::Map::OriginMushroom::BACKGROUND_PREFIX, i));
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

    // Goblu 击破：倒地/起身 (1-3)
    for (int i = 1; i <= 3; ++i)
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_fall_%d.png", i));
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_rise_%d.png", i));
    }

    // 加载战士素材
    auto warriorPaths = AssetRes::getSelectedRolePaths(CharacterRole::WARRIOR);
    paths.insert(paths.end(), warriorPaths.begin(), warriorPaths.end());

    // 加载法师素材
    auto magePaths = AssetRes::getSelectedRolePaths(CharacterRole::MAGE);
    paths.insert(paths.end(), magePaths.begin(), magePaths.end());

    // 加载刺客素材
    auto assassinPaths = AssetRes::getSelectedRolePaths(CharacterRole::ASSASSIN);
    paths.insert(paths.end(), assassinPaths.begin(), assassinPaths.end());

    //通过接口获取 Obscur 预加载资源路径并合并
    auto obscurPaths = ObscurMonster::getPreloadResourcePaths();
    paths.insert(paths.end(), obscurPaths.begin(), obscurPaths.end());

    //资源去重
    std::sort(paths.begin(), paths.end());
    paths.erase(std::unique(paths.begin(), paths.end()), paths.end());

    // 将整理好的 vector 赋值给 info
    info.imagePaths = paths;

    // 贴图加载完毕后预热 AnimationCache：补齐 Obscur 的动态动作（attack/useice/ice）
    // 避免首次刷出怪物时出现“动作缺失/首帧卡顿”
    info.onResourcesLoaded = []()
    {
        GoblinMonster::preloadResources();
        ObscurMonster::preloadResources();
    };

    // 注册到管理器 (ID = 1)
    SceneRegistry::getInstance()->registerScene(SceneID::LEVEL_ORIGIN_MUSHROOM, info);
}
