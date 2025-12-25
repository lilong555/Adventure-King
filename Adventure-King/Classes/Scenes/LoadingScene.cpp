#include "Scenes/LoadingScene.h"
#include "Scenes/HelloWorldScene.h"
#include "Managers/SceneRegistry.h"
#include "Managers/MusicManager.h"
#include "Scenes/MapScene.h"
#include"Managers/MusicManager.h"
#include "Utils/ParticlePreloadHelper.h"
#include "Utils/ImeHelper.h"
#include "Configs/GameConfig.h"
#include "Configs/GameSceneConfig.h"

USING_NS_CC;

// 使用配置中的常量缩写
using namespace GameSceneConfig::UI::Loading;

// --- 静态创建方法 ---
Scene* LoadingScene::createScene(SceneID id) {
    auto scene = new (std::nothrow) LoadingScene();
    if (scene && scene->initWithSceneId(id)) {
        scene->autorelease();
        return scene;
    }
    CC_SAFE_DELETE(scene);
    return nullptr;
}

// --- 析构函数：清理异步回调和待切换场景 ---
LoadingScene::~LoadingScene() {
    if (!_callbackKey.empty()) {
        Director::getInstance()->getTextureCache()->unbindImageAsync(_callbackKey);
    }

    if (_pendingDestinationScene) {
        _pendingDestinationScene->release();
        _pendingDestinationScene = nullptr;
    }
}

// --- 初始化与 UI 布局 ---
bool LoadingScene::init() {
    if (!Scene::init()) return false;
    return true;
}

bool LoadingScene::initWithSceneId(SceneID id) {
    if (!init()) return false;

    _targetId = id;
    _callbackKey = StringUtils::format("LoadingScene_%p", this);

    // 1. 停止背景音乐，准备进入新环境
    MusicManager::getInstance()->stopBGM();

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 2. 绘制纯黑背景
    auto bg = LayerColor::create(Color4B(0, 0, 0, 255));
    addChild(bg);

    // 3. 创建加载提示文本
    _label = Label::createWithTTF("正在读取资源...", GameSceneConfig::Scene::DEFAULT_FONT_PATH, 28);
    if (_label) {
        _label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.55f));
        _label->setColor(Color3B(240, 240, 240));
        addChild(_label, 10);
    }

    // 4. 创建进度条 UI
    const float barWidth = visibleSize.width * BAR_WIDTH_RATIO;
    const float barX = origin.x + (visibleSize.width - barWidth) * 0.5f;
    const float barY = origin.y + BAR_BOTTOM_PADDING;

    _barBg = LayerColor::create(Color4B(40, 40, 40, 255), barWidth, BAR_HEIGHT);
    _barBg->setPosition(Vec2(barX, barY));
    addChild(_barBg, 10);

    _barFill = LayerColor::create(Color4B(180, 220, 255, 255), 0.0f, BAR_HEIGHT - BAR_FILL_PADDING * 2.0f);
    _barFill->setPosition(Vec2(barX + BAR_FILL_PADDING, barY + BAR_FILL_PADDING));
    addChild(_barFill, 11);

    // 5. 延迟一帧启动预加载，确保 UI 先显示出来
    this->scheduleOnce([this](float) { this->startPreload(); }, 0.0f, "StartPreloadLogic");

    return true;
}

// --- 核心预加载逻辑 ---
void LoadingScene::startPreload() {
    if (_finished) return;

    _finishScheduled = false;
    _paths = buildPreloadList(_targetId);
    _total = static_cast<int>(_paths.size());
    _loaded = 0;

    // 如果没有资源需要加载，直接结束
    if (_total <= 0) {
        _finishScheduled = true;
        this->scheduleOnce([this](float) { this->finishPreload(); }, 0.0f, "FinishImmediate");
        return;
    }

    auto textureCache = Director::getInstance()->getTextureCache();
    auto fileUtils = FileUtils::getInstance();

    std::vector<std::string> pendingPaths;
    for (const auto& path : _paths) {
        std::string fullPath = fileUtils->fullPathForFilename(path);
        // 如果资源不存在或已在内存中，直接计入完成
        if (fullPath.empty() || textureCache->getTextureForKey(fullPath)) {
            _loaded++;
            continue;
        }
        pendingPaths.push_back(path);
    }

    updateProgressUI();

    // 如果所有资源都在内存里了
    if (_loaded >= _total) {
        _finishScheduled = true;
        this->scheduleOnce([this](float) { this->finishPreload(); }, 0.0f, "FinishCached");
        return;
    }

    // 执行异步加载
    for (const auto& path : pendingPaths) {
        textureCache->addImageAsync(path, [this](Texture2D* tex) {
            this->onTextureLoaded(tex);
            }, _callbackKey);
    }
}

// --- 单个贴图加载回调 ---
void LoadingScene::onTextureLoaded(Texture2D* texture) {
    if (_finished) return;

    _loaded = std::min(_loaded + 1, _total);
    updateProgressUI();

    if (_loaded >= _total && !_finishScheduled) {
        _finishScheduled = true;
        // 延后一帧结束，确保 UI 更新完毕且避免回调死循环
        this->scheduleOnce([this](float) { this->finishPreload(); }, 0.0f, "DelayFinish");
    }
}

// --- 预加载全部完成后的清理与场景创建 ---
void LoadingScene::finishPreload() {
    if (_finished) return;
    _finished = true;

    if (_label) _label->setString("正在初始化关卡...");

    auto registry = SceneRegistry::getInstance();
    auto info = registry->getSceneInfo(_targetId);

    if (info) {
        // 1. 执行逻辑预热 (如生成动画缓存)
        if (info->onResourcesLoaded) {
            info->onResourcesLoaded();
        }

        // 2. 粒子系统通用预热
        ParticlePreloadHelper::preloadCommonParticles();

        // 3. 利用注册表工厂方法创建目标场景
        auto destScene = registry->createSceneInstance(_targetId);
        if (destScene) {
            destScene->retain(); // 手动引用计数加1，防止在切换前被销毁
            _pendingDestinationScene = destScene;
            tryReplacePendingScene();
            return;
        }
    }

    // 容错：如果失败，回到主菜单
    CCLOG("LoadingScene Error: SceneID %d creation failed!", (int)_targetId);
    auto fallback = HelloWorld::createScene();
    fallback->retain();
    _pendingDestinationScene = fallback;
    tryReplacePendingScene();
}

// --- 进度条刷新 ---
void LoadingScene::updateProgressUI() {
    float percent = (_total > 0) ? (static_cast<float>(_loaded) / _total) : 1.0f;

    if (_label) {
        _label->setString(StringUtils::format("加载中... (%d/%d)", _loaded, _total));
    }

    if (_barFill && _barBg) {
        float availableWidth = _barBg->getContentSize().width - BAR_FILL_PADDING * 2.0f;
        _barFill->setContentSize(Size(availableWidth * percent, _barFill->getContentSize().height));
    }
}

// --- 根据注册表构建资源列表 ---
std::vector<std::string> LoadingScene::buildPreloadList(SceneID id) const {
    std::vector<std::string> paths;

    // 通用资源：
    // 注意：这里不要引用不存在的占位图，否则会在启动/切场景时刷 “fullPathForFilename: No file found ...”。
    // 掉落物（血瓶/蓝瓶）：避免怪物首次掉落时才 IO 读取导致卡顿
    paths.emplace_back(GameConfig::DropItem::HP_SPRITE_PATH);
    paths.emplace_back(GameConfig::DropItem::MP_SPRITE_PATH);

    // 从注册表获取特定场景资源
    auto info = SceneRegistry::getInstance()->getSceneInfo(id);
    if (info) {
        paths.insert(paths.end(), info->imagePaths.begin(), info->imagePaths.end());
    }

    return paths;
}

// --- 安全切换场景：处理 TransitionScene 的冲突 ---
void LoadingScene::tryReplacePendingScene() {
    if (!_pendingDestinationScene) return;

    auto director = Director::getInstance();
    auto runningScene = director->getRunningScene();

    // 如果当前正在执行淡入淡出等过渡动画，则等待下一帧再试
    if (dynamic_cast<TransitionScene*>(runningScene)) {
        this->scheduleOnce([this](float) { this->tryReplacePendingScene(); }, 0.0f, "RetrySceneReplace");
        return;
    }

    // 执行最终替换
    director->replaceScene(_pendingDestinationScene);
    _pendingDestinationScene->release(); // 释放 retain
    _pendingDestinationScene = nullptr;
}

void LoadingScene::onEnter() {
    Scene::onEnter();
    ImeHelper::pushDisableIme(); // 加载期间禁止输入法干扰
}

void LoadingScene::onExit() {
    ImeHelper::popDisableIme();
    Scene::onExit();
}
