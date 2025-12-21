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
    config.tmxMapPath = "";
    config.backgroundPath = "";
    config.gravity = -800.0f;
    config.enablePhysicsDebug = false;
    return config;
}

bool MysteryForestScene::init()
{
    if (!GameScene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    auto titleLabel = Label::createWithTTF(getLevelName(), GameConfig::Scene::DEFAULT_FONT_PATH, 72);
    if (titleLabel)
    {
        titleLabel->setPosition(center);
        titleLabel->setColor(Color3B::WHITE);
        addChild(titleLabel, 1);
    }

    auto hintLabel = Label::createWithTTF("Click map button to return", GameConfig::Scene::DEFAULT_FONT_PATH, 32);
    if (hintLabel)
    {
        hintLabel->setPosition(Vec2(center.x, center.y - 80));
        hintLabel->setColor(Color3B(200, 200, 200));
        addChild(hintLabel, 1);
    }

    initUIController();

    CCLOG("MysteryForestScene initialized (placeholder)");
    return true;
}
