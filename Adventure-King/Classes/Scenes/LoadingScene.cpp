#include "Scenes/LoadingScene.h"
#include "Managers/SceneRegistry.h"
#include "Scenes/DebugScene.h"
#include "Scenes/HomeScene.h"
#include "Scenes/MapScene.h"
#include"Scenes/LevelScenes/OriginMushroomScene.h"
#include"Scenes/LevelScenes/MysteryForestScene.h"
#include"Managers/MusicManager.h"
#include "Utils/ParticlePreloadHelper.h"
#include "Utils/ImeHelper.h"
#include "2d/CCTransition.h"
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

    if (_pendingDestinationScene)
    {
        _pendingDestinationScene->release();
        _pendingDestinationScene = nullptr;
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

void LoadingScene::onEnter()
{
    Scene::onEnter();
    ImeHelper::pushDisableIme();
}

void LoadingScene::onExit()
{
    ImeHelper::popDisableIme();
    Scene::onExit();
}

bool LoadingScene::initWithMapId(int mapId)
{
    if (!init())
    {
        return false;
    }
    MusicManager::getInstance()->stopBGM();
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
    if (_finished) return;
    _finished = true;

    // 粒子预热：使用 plist 内嵌纹理的粒子首次触发会解码/上传贴图，提前在加载阶段完成
    if (_label)
    {
        _label->setString("粒子预热中...");
    }
    ParticlePreloadHelper::preloadCommonParticles();

    auto info = SceneRegistry::getInstance()->getSceneInfo(_mapId);
    Scene* destinationScene = nullptr;

    if (info) {
        // 1. 执行注册好的预热回调 (AnimationCache 等)
        if (info->onResourcesLoaded) {
            info->onResourcesLoaded();
        }

        // 2. 使用工厂方法创建目标场景
        if (info->creator) {
            destinationScene = info->creator();
        }
    }

    if (destinationScene)
    {
        // 延迟到 TransitionScene 结束后再切，避免嵌套 replaceScene 导致崩溃
        if (_pendingDestinationScene)
        {
            _pendingDestinationScene->release();
        }
        _pendingDestinationScene = destinationScene;
        _pendingDestinationScene->retain();
        tryReplacePendingScene();
        return;
    }

    // 容错处理：如果注册表中没找到，回退到默认
    if (!info)
    {
        CCLOG("Error: MapID %d not found in registry!", _mapId);
    }
    else
    {
        CCLOG("Error: MapID %d create scene failed (creator is %s)", _mapId, info->creator ? "set" : "null");
    }

    auto fallback = MapScene::createScene();
    if (fallback)
    {
        if (_pendingDestinationScene)
        {
            _pendingDestinationScene->release();
        }
        _pendingDestinationScene = fallback;
        _pendingDestinationScene->retain();
        tryReplacePendingScene();
    }
}

void LoadingScene::tryReplacePendingScene()
{
    if (!_pendingDestinationScene)
    {
        return;
    }

    auto director = Director::getInstance();
    auto running = director ? director->getRunningScene() : nullptr;
    if (running && dynamic_cast<TransitionScene*>(running))
    {
        // 仍处于过渡场景中（比如从 MapScene 淡入 LoadingScene），延后一帧再尝试切换
        scheduleOnce([this](float)
                     { this->tryReplacePendingScene(); },
                     0.0f,
                     "TryReplacePendingScene");
        return;
    }

    director->replaceScene(_pendingDestinationScene);
    _pendingDestinationScene->release();
    _pendingDestinationScene = nullptr;
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
    std::vector<std::string> paths;

    // 1. 加载所有场景通用的基础资源 (UI 等)
    paths.push_back("Scene/Backgrounds/MapBackground.png");
    // 粒子特效使用 plist 内嵌纹理，不需要预加载 particle_texture.png

    // 2. 从注册表中获取该 ID 特有的资源
    auto info = SceneRegistry::getInstance()->getSceneInfo(mapId);
    if (info) {
        // 将特定场景的资源合并进来
        paths.insert(paths.end(), info->imagePaths.begin(), info->imagePaths.end());
    }

    return paths;
}

Scene* LoadingScene::createDestinationScene(int mapId) const
{
    Scene* scene = nullptr;
    switch (mapId)
    {
    case HomeScene::MAP_ID:
        scene = HomeScene::createScene();
        break;
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
