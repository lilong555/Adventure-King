#include "Scenes/LoadingScene.h"

#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/Monster/Monsters/GobluMonster.h"
#include "Configs/GameConfigs.h"
#include "Scenes/DebugScene.h"
#include "Scenes/GameScene.h"
#include "Scenes/MapScene.h"
#include <algorithm>
#include <unordered_set>

USING_NS_CC;

namespace
{
    constexpr float kBarWidthRatio = 0.75f;
    constexpr float kBarHeight = 16.0f;
    constexpr float kBarBottomPadding = 22.0f;
    constexpr float kBarFillPadding = 2.0f;
}

Scene* LoadingScene::createScene(int mapId)
{
    auto scene = new (std::nothrow) LoadingScene();
    if (scene && scene->initWithMapId(mapId))
    {
        scene->autorelease();
        return scene;
    }
    CC_SAFE_DELETE(scene);
    return nullptr;
}

LoadingScene::~LoadingScene()
{
    if (!_callbackKey.empty())
    {
        if (auto cache = Director::getInstance()->getTextureCache())
        {
            cache->unbindImageAsync(_callbackKey);
        }
    }
}

bool LoadingScene::init()
{
    if (!Scene::init())
    {
        return false;
    }
    return true;
}

bool LoadingScene::initWithMapId(int mapId)
{
    if (!init())
    {
        return false;
    }

    _mapId = mapId;
    _callbackKey = StringUtils::format("LoadingScene_%p", this);

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 背景遮罩（简洁黑底）
    auto bg = LayerColor::create(Color4B(0, 0, 0, 255));
    addChild(bg);

    // 文本
    _label = Label::createWithTTF("加载中...", "fonts/ZCOOLKuaiLe-Regular.ttf", 28);
    if (_label)
    {
        _label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
                                 origin.y + visibleSize.height * 0.55f));
        _label->setColor(Color3B(240, 240, 240));
        addChild(_label, 10);
    }

    // 进度条（底部）
    const float barWidth = visibleSize.width * kBarWidthRatio;
    const float barX = origin.x + (visibleSize.width - barWidth) * 0.5f;
    const float barY = origin.y + kBarBottomPadding;

    _barBg = LayerColor::create(Color4B(40, 40, 40, 255), barWidth, kBarHeight);
    _barBg->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
    _barBg->setPosition(Vec2(barX, barY));
    addChild(_barBg, 10);

    const float fillWidth = std::max(0.0f, barWidth - kBarFillPadding * 2.0f);
    const float fillHeight = std::max(0.0f, kBarHeight - kBarFillPadding * 2.0f);
    _barFill = LayerColor::create(Color4B(180, 220, 255, 255), 0.0f, fillHeight);
    _barFill->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
    _barFill->setPosition(Vec2(barX + kBarFillPadding, barY + kBarFillPadding));
    addChild(_barFill, 11);

    // 第一帧开始加载（确保 UI 先显示出来）
    runAction(Sequence::create(DelayTime::create(0.0f),
                               CallFunc::create([this]()
                                                { this->startPreload(); }),
                               nullptr));
    return true;
}

void LoadingScene::startPreload()
{
    if (_finished)
    {
        return;
    }

    _finishScheduled = false;
    _paths = buildPreloadList(_mapId);
    _total = static_cast<int>(_paths.size());
    _loaded = 0;

    if (_total <= 0)
    {
        // 延后一帧执行，避免与 startPreload 的调用栈重入
        _finishScheduled = true;
        runAction(Sequence::create(DelayTime::create(0.0f),
                                   CallFunc::create([this]()
                                                    { this->finishPreload(); }),
                                   nullptr));
        return;
    }

    auto textureCache = Director::getInstance()->getTextureCache();
    auto fileUtils = FileUtils::getInstance();

    std::vector<std::string> pending;
    pending.reserve(_paths.size());

    // 已缓存/缺失资源直接计入完成，避免 addImageAsync 在已加载路径上走同步回调导致重入
    for (const auto& path : _paths)
    {
        const std::string fullPath = fileUtils ? fileUtils->fullPathForFilename(path) : "";
        if (fullPath.empty())
        {
            CCLOG("LoadingScene: 预加载资源缺失：%s", path.c_str());
            _loaded = std::min(_loaded + 1, _total);
            continue;
        }
        if (textureCache && textureCache->getTextureForKey(fullPath))
        {
            _loaded = std::min(_loaded + 1, _total);
            continue;
        }
        pending.push_back(path);
    }

    updateProgressUI();
    if (_loaded >= _total)
    {
        _finishScheduled = true;
        runAction(Sequence::create(DelayTime::create(0.0f),
                                   CallFunc::create([this]()
                                                    { this->finishPreload(); }),
                                   nullptr));
        return;
    }

    for (const auto& path : pending)
    {
        textureCache->addImageAsync(path,
                                    [this](Texture2D* texture)
                                    {
                                        this->onTextureLoaded(texture);
                                    },
                                    _callbackKey);
    }
}

void LoadingScene::onTextureLoaded(Texture2D* /*texture*/)
{
    if (_finished)
    {
        return;
    }

    _loaded = std::min(_loaded + 1, _total);
    updateProgressUI();

    if (_loaded >= _total && !_finishScheduled)
    {
        _finishScheduled = true;
        // 避免在 addImageAsync 的同步回调路径里直接触发换场景，延后一帧执行
        runAction(Sequence::create(DelayTime::create(0.0f),
                                   CallFunc::create([this]()
                                                    { this->finishPreload(); }),
                                   nullptr));
    }
}

void LoadingScene::finishPreload()
{
    if (_finished)
    {
        return;
    }
    _finished = true;

    // 预热动画缓存：贴图已进 TextureCache，这里主要是填充 AnimationCache
    GoblinMonster::preloadResources();
    GobluMonster::preloadResources();

    // 让进度条先到 100%，再切场景（避免最后一帧看起来停在 99%）
    _loaded = _total;
    updateProgressUI();

    auto destinationScene = createDestinationScene(_mapId);
    if (!destinationScene)
    {
        CCLOG("LoadingScene: 创建目标场景失败，mapId=%d", _mapId);
        // 回退到地图场景
        destinationScene = MapScene::createScene();
    }

    auto transition = TransitionFade::create(GameConfig::Scene::MENU_TRANSITION_DURATION,
                                             destinationScene,
                                             Color3B::BLACK);
    Director::getInstance()->replaceScene(transition);
}

void LoadingScene::updateProgressUI()
{
    float percent = 0.0f;
    if (_total > 0)
    {
        percent = 100.0f * static_cast<float>(_loaded) / static_cast<float>(_total);
        percent = std::max(0.0f, std::min(100.0f, percent));
    }

    if (_label)
    {
        if (_total > 0)
        {
            _label->setString(StringUtils::format("加载中... (%d/%d)", _loaded, _total));
        }
        else
        {
            _label->setString("加载中...");
        }
    }

    if (_barBg && _barFill)
    {
        const float totalWidth = _barBg->getContentSize().width - kBarFillPadding * 2.0f;
        const float fillWidth = std::max(0.0f, totalWidth * (percent / 100.0f));
        _barFill->setContentSize(Size(fillWidth, _barFill->getContentSize().height));
    }
}

std::vector<std::string> LoadingScene::buildPreloadList(int mapId) const
{
    std::unordered_set<std::string> uniq;
    std::vector<std::string> paths;
    paths.reserve(96);

    auto addPath = [&uniq, &paths](const std::string& path)
    {
        if (path.empty())
        {
            return;
        }
        if (uniq.insert(path).second)
        {
            paths.push_back(path);
        }
    };

    // 公共资源：粒子贴图等
    addPath("Particle/particle_texture.png");

    // 地图选择 UI：避免第一次 hover/点击时再加载
    addPath("Scene/Backgrounds/MapBackground.png");
    addPath("Scene/UI/mapselectItem_1_selected.png");
    addPath("Scene/UI/mapselectItem_2_selected.png");

    if (mapId == 1)
    {
        // 起源之菇背景序列
        for (int i = 0; i < GameConfig::Map::OriginMushroom::BACKGROUND_COUNT; ++i)
        {
            addPath(StringUtils::format("%s%02d.png", GameConfig::Map::OriginMushroom::BACKGROUND_PREFIX, i));
        }

        // TMX tileset 贴图（Origin_Mushroom.tmx 引用的 tsx）
        addPath("Map/Origin_Mushroom/Env_Tree_Oak_Giant_Green.png");
        addPath("Map/Origin_Mushroom/s1.png");
        addPath("Map/Origin_Mushroom/s2.png");
        addPath("Map/Origin_Mushroom/s3.png");
        addPath("Map/Origin_Mushroom/s4.png");

        // 哥布林：首刷卡顿主要来自贴图首次解码 + 动画缓存创建
        addPath("Sprites/Enemies/Goblin/Goblin_idle.png");
        addPath("Sprites/Enemies/Goblin/Goblin_beattacked.png");
        for (int i = 1; i <= 4; ++i)
        {
            addPath(StringUtils::format("Sprites/Enemies/Goblin/Goblin_walk_%d.png", i));
            addPath(StringUtils::format("Sprites/Enemies/Goblin/Goblin_attack_%d.png", i));
        }

        // Goblu：同样提前热身
        addPath("Sprites/Enemies/Goblu/Goblu.png");
        for (int i = 1; i <= 4; ++i)
        {
            addPath(StringUtils::format("Sprites/Enemies/Goblu/Goblu_walk_%d.png", i));
            addPath(StringUtils::format("Sprites/Enemies/Goblu/Goblu_attack_%02d.png", i));
        }
        for (int i = 11; i <= 15; ++i)
        {
            addPath(StringUtils::format("Sprites/Enemies/Goblu/Goblu_attack_%02d.png", i));
        }

        // Goblu 死亡动画：避免首次死亡时加载卡顿
        for (int i = 1; i <= 6; ++i)
        {
            addPath(StringUtils::format("Sprites/Enemies/Goblu/Goblu_death_%d.png", i));
        }
    }

    // 其它 mapId 目前多为占位场景，可在后续按需补充
    return paths;
}

Scene* LoadingScene::createDestinationScene(int mapId) const
{
    Scene* scene = nullptr;
    switch (mapId)
    {
    case 1:
        scene = OriginMushroomScene::createScene();
        break;
    case 2:
        scene = MysteryForestScene::createScene();
        break;
    case 99:
        scene = DebugScene::createScene();
        break;
    default:
        break;
    }
    return scene;
}
