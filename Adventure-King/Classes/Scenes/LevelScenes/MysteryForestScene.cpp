#include "MysteryForestScene.h"
#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/Monster/Monsters/GobluMonster.h"
#include"Character/Monster/Monsters/ObscurMonster.h"
#include"Configs/CharacterAssetConfig.h"
#include"Save/SaveManager.h"
#include "Configs/GameConfig.h"
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
    config.playerSpritePath = GameSceneConfig::Scene::DEFAULT_PLAYER_SPRITE;
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
    config.enablePhysicsDebug = true;
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

    // 1. 设置场景创建器与显示名称
    info.creator = []() {
        return MysteryForestScene::createScene();
        };
    info.sceneName = "神秘之森";

    // 2. 准备资源路径列表
    std::vector<std::string> paths = {
        // --- 神秘之森专属：地牢 Tileset 静态贴图 ---
        "Map/dungeon/1.png",
        "Map/dungeon/2.png",
        "Map/dungeon/3.png",
        "Map/dungeon/4.png",

        // --- 共有怪物静态单图 ---
        "Sprites/Enemies/Goblin/Goblin_idle.png",
        "Sprites/Enemies/Goblin/Goblin_beattacked.png",
        "Sprites/Enemies/Goblu/Goblu.png"
    };

    // 3. 追加动态生成的怪物序列帧 (同步 OriginMushroom 的逻辑)

    // 哥布林行走与攻击 (1-4)
    for (int i = 1; i <= 4; ++i)
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblin/Goblin_walk_%d.png", i));
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblin/Goblin_attack_%d.png", i));
    }

    // Goblu 行走、攻击与死亡动画
    for (int i = 1; i <= 4; ++i)
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_walk_%d.png", i));
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_attack_%02d.png", i));
    }
    for (int i = 11; i <= 15; ++i) // 额外攻击帧
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_attack_%02d.png", i));
    }
    for (int i = 1; i <= 6; ++i) // 死亡动画
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_death_%d.png", i));
    }

    // Goblu 击破：倒地/起身 (1-3)
    for (int i = 1; i <= 3; ++i)
    {
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_fall_%d.png", i));
        paths.push_back(StringUtils::format("Sprites/Enemies/Goblu/Goblu_rise_%d.png", i));
    }

    // 4. 通过静态接口合并角色/怪物资源路径

// 获取当前选择的职业
    CharacterRole currentRole = CharacterRole::MAGE; // 给个默认值
    auto saveManager = SaveManager::getInstance();
    if (saveManager && saveManager->hasSessionSelectedRole()) {
        // 从全局单例中读取在 HelloWorld 界面选好的职业
        currentRole = saveManager->getSessionSelectedRole();
    }

    // 3. 调用第一步定义的辅助函数
    auto playerPaths = AssetRes::getSelectedRolePaths(currentRole);
    paths.insert(paths.end(), playerPaths.begin(), playerPaths.end());

    // 获取 ObscurMonster (小怪) 预加载资源路径
    auto obscurPaths = ObscurMonster::getPreloadResourcePaths();
    paths.insert(paths.end(), obscurPaths.begin(), obscurPaths.end());

    // 5. 资源去重与排序（防止重复加载同一张图）
    std::sort(paths.begin(), paths.end());
    paths.erase(std::unique(paths.begin(), paths.end()), paths.end());

    // 赋值最终资源列表
    info.imagePaths = paths;

    // 6. 贴图预加载后的回调：预热 AnimationCache
    // 确保连战刷怪时不会因为生成动画对象而掉帧
    info.onResourcesLoaded = []()
        {
            GoblinMonster::preloadResources();
            ObscurMonster::preloadResources();
            // 如果 Goblu 也有 preloadResources 接口，建议在此一并调用
        };

    // 7. 注册到管理器 (使用枚举 ID)
    SceneRegistry::getInstance()->registerScene(SceneID::LEVEL_MYSTERY_FOREST, info);
}
