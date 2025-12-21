#include "MapScene.h"
#include "GameScene.h"
#include "DebugScene.h"
#include"Scenes/LevelScenes/OriginMushroomScene.h"
#include"Scenes/LevelScenes/MysteryForestScene.h"
#include "Character/Monster/Monsters/GoblinMonster.h"
#include "Character/Monster/Monsters/GobluMonster.h"
#include "Configs/GameConfigs.h"
#include "Scenes/LoadingScene.h"
#include <unordered_set>
USING_NS_CC;

// 统一的资源缺失提示
static void problemLoading(const char *filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}

Scene *MapScene::createScene()
{
    return MapScene::create();
}

MapScene::~MapScene()
{
    // 场景被销毁时解绑异步贴图回调，避免回调访问已释放的 MapScene
    if (!_preloadCallbackKey.empty())
    {
        if (auto cache = Director::getInstance()->getTextureCache())
        {
            cache->unbindImageAsync(_preloadCallbackKey);
        }
    }

    for (auto marker : _mapMarkers)
    {
        auto selectedImage = static_cast<std::string *>(marker->getUserData());
        delete selectedImage;
        marker->setUserData(nullptr);
    }
    _mapMarkers.clear();
}
MenuItemImage *MapScene::createMenuItem(
    const char *normal,
    const char *selected,
    const ccMenuCallback &callback)
{
    auto item = MenuItemImage::create(normal, selected, callback);
    return item;
}
bool MapScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    _preloadCallbackKey = StringUtils::format("MapScenePreload_%p", this);
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // ===============================================
    // 1. 初始化容器节点
    // ===============================================
    auto contentContainer = Node::create();
    contentContainer->setTag(TAG_CONTENT_CONTAINER);
    this->addChild(contentContainer, TAG_CONTENT_CONTAINER);
    contentContainer->setPosition(center);

    // ==========================================================
    // 2. 添加背景精灵
    // ==========================================================
    auto sprite = Sprite::create("Scene/Backgrounds/MapBackground.png");
    if (sprite == nullptr)
    {
        problemLoading("'Scene/Backgrounds/MapBackground.png'");
        return false;
    }

    sprite->setPosition(Vec2::ZERO);
    contentContainer->addChild(sprite, 0);

    Size textureSize = sprite->getContentSize();
    float scaleX = visibleSize.width / textureSize.width;
    float scaleY = visibleSize.height / textureSize.height;
    float scaleFactor = std::min(scaleX, scaleY);
    contentContainer->setScale(scaleFactor);

    // ==========================================================
    // 3. 初始化地图标记数据
    // ==========================================================
    Size backgroundSize = sprite->getContentSize();
    _markerInfos = {
        {"Scene/UI/mapselectItem_1.png", "Scene/UI/mapselectItem_1_selected.png", Vec2(backgroundSize.width / 20, backgroundSize.height / 2.7), 1, 0.32f, "起源之菇"},
        {"Scene/UI/mapselectItem_2.png", "Scene/UI/mapselectItem_2_selected.png", Vec2(backgroundSize.width / 8.46, backgroundSize.height / 8), 2, 0.32f, "神秘之森"},
        {"Scene/UI/PaintRoom.png", "Scene/UI/PaintRoomSelected.png", Vec2(-backgroundSize.width * 0.2f, backgroundSize.height * 0.2f), 99, 0.35f, "画室"},
    };

    // ==========================================================
    // 4. 创建并添加地标精灵
    // ==========================================================
    for (const auto &info : _markerInfos)
    {
        auto marker = Sprite::create(info.normalImage);
        if (!marker)
        {
            problemLoading(info.normalImage.c_str());
            continue;
        }

        marker->setPosition(info.position);
        marker->setTag(info.mapId);
        marker->setName(info.normalImage);
        marker->setUserData(new std::string(info.selectedImage));
        marker->setScale(info.scale);

        auto nameLabel = Label::createWithTTF(info.name, "fonts/ZCOOLKuaiLe-Regular.ttf", 48);
        if (nameLabel)
        {
            Size markerSize = marker->getContentSize();
            nameLabel->setPosition(Vec2(markerSize.width / 2, -nameLabel->getContentSize().height / 2 - 10));
            nameLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
            marker->addChild(nameLabel, 1);
        }

        contentContainer->addChild(marker, 1);
        _mapMarkers.push_back(marker);
    }

    // ==========================================================
    // 5. 添加鼠标事件监听器
    // ==========================================================
    auto mouseListener = EventListenerMouse::create();

    mouseListener->onMouseMove = [this](EventMouse *event)
    {
        if (_isTransitioning)
        {
            return;
        }

        Vec2 mousePos = Vec2(event->getCursorX(), event->getCursorY());

        for (auto marker : _mapMarkers)
        {
            Vec2 localPos = marker->convertToNodeSpace(mousePos);
            Rect rect = Rect(Vec2::ZERO, marker->getContentSize());

            std::string *selectedImage = static_cast<std::string *>(marker->getUserData());

            if (rect.containsPoint(localPos))
            {
                marker->setTexture(*selectedImage);
            }
            else
            {
                marker->setTexture(marker->getName());
            }
        }
    };

    mouseListener->onMouseDown = [this](EventMouse *event)
    {
        if (_isTransitioning)
        {
            return;
        }

        Vec2 mousePos = Vec2(event->getCursorX(), event->getCursorY());

        for (auto marker : _mapMarkers)
        {
            Vec2 localPos = marker->convertToNodeSpace(mousePos);
            Rect rect = Rect(Vec2::ZERO, marker->getContentSize());

            if (rect.containsPoint(localPos))
            {
                int mapId = marker->getTag();
                this->onMapMarkerClicked(mapId);
                break;
            }
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

    auto closeItem = MenuItemImage::create(
        "CloseNormal.png",
        "CloseSelected.png",
        CC_CALLBACK_1(MapScene::mapCloseCallback, this));

    if (closeItem)
    {
        float closeX = origin.x + visibleSize.width - closeItem->getContentSize().width / 2;
        float closeY = origin.y + visibleSize.height - closeItem->getContentSize().height / 2;
        closeItem->setPosition(Vec2(closeX, closeY));

        auto closeMenu = Menu::create(closeItem, nullptr);
        closeMenu->setPosition(Vec2::ZERO);
        closeMenu->setTag(TAG_MAP_MENU);
        this->addChild(closeMenu, TAG_MAP_MENU);
    }

    // 在地图选择界面期间后台预加载常用资源，减少“点击进入地图”和“首次刷怪”的卡顿
    startPreloadOriginMushroom(false);
    return true;
}

void MapScene::onMapMarkerClicked(int mapId)
{
    CCLOG("Clicked map: %d", mapId);
    enterMap(mapId);
}

void MapScene::mapCloseCallback(cocos2d::Ref *pSender)
{
    // 关闭地图时也主动解绑回调，避免回调在场景切换过程中触发造成不必要的逻辑执行
    if (!_preloadCallbackKey.empty())
    {
        if (auto cache = Director::getInstance()->getTextureCache())
        {
            // 注意：这里解绑的是 callbackKey（addImageAsync 第三个参数），不是文件路径
            cache->unbindImageAsync(_preloadCallbackKey);
        }
    }
    _originMushroomPreloading = false;

    cocos2d::Director::getInstance()->popScene();
}

cocos2d::Scene *MapScene::createDestinationScene(int mapId)
{
    Scene *scene = nullptr;
    switch (mapId)
    {
    case 1:
        scene = OriginMushroomScene::createScene();
        break;
    case 2:
        scene = MysteryForestScene::createScene();
        break;
    case 99: // 测试场景（画室）
        scene = DebugScene::createScene();
        break;
    default:
        CCLOG("Unknown mapId: %d", mapId);
        break;
    }

    if (!scene)
    {
        CCLOG("Failed to create scene for mapId: %d", mapId);
    }

    return scene;
}

void MapScene::enterMap(int mapId)
{
    if (_isTransitioning)
    {
        return;
    }
    _isTransitioning = true;

    // 如果目标关卡资源已在地图界面预热完毕，则直接进入关卡，避免多一次 LoadingScene 过渡
    // 目前仅起源之菇做了完整预加载（贴图 + 动画缓存预热）
    Scene* destinationScene = nullptr;
    if (mapId == 1 && _originMushroomAssetsReady)
    {
        destinationScene = createDestinationScene(mapId);
    }
    else
    {
        // 统一进入加载场景，加载完成后再切到目标关卡
        destinationScene = LoadingScene::createScene(mapId);
    }
    if (!destinationScene)
    {
        CCLOG("Failed to create destination scene for map: %d", mapId);
        _isTransitioning = false;
        return;
    }

    // 即将离开地图场景：解绑预加载回调，避免切换过程继续触发回调
    if (!_preloadCallbackKey.empty())
    {
        if (auto cache = Director::getInstance()->getTextureCache())
        {
            // 注意：这里解绑的是 callbackKey（addImageAsync 第三个参数），不是文件路径
            cache->unbindImageAsync(_preloadCallbackKey);
        }
    }
    _originMushroomPreloading = false;
    if (_preloadLabel)
    {
        _preloadLabel->setVisible(false);
    }

    auto director = Director::getInstance();
    director->popToRootScene();
    auto transition = TransitionFade::create(GameConfig::Scene::MENU_TRANSITION_DURATION, destinationScene, Color3B::BLACK);
    director->replaceScene(transition);
}

void MapScene::startPreloadOriginMushroom(bool showUI)
{
    if (_originMushroomAssetsReady)
    {
        return;
    }

    if (showUI)
    {
        if (!_preloadLabel)
        {
            auto visibleSize = Director::getInstance()->getVisibleSize();
            auto origin = Director::getInstance()->getVisibleOrigin();
            Vec2 pos(origin.x + visibleSize.width * 0.5f,
                     origin.y + visibleSize.height * 0.08f);
            _preloadLabel = Label::createWithTTF("加载中...", "fonts/ZCOOLKuaiLe-Regular.ttf", 28);
            if (_preloadLabel)
            {
                _preloadLabel->setPosition(pos);
                _preloadLabel->setColor(Color3B(240, 240, 240));
                addChild(_preloadLabel, 9999);
            }
        }
        if (_preloadLabel)
        {
            _preloadLabel->setVisible(true);
        }
    }

    if (_originMushroomPreloading)
    {
        updatePreloadLabel();
        return;
    }

    _originMushroomPreloading = true;
    _originMushroomFinishScheduled = false;
    _preloadLoaded = 0;

    std::unordered_set<std::string> uniq;
    std::vector<std::string> paths;
    paths.reserve(64);

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

    // 地图背景序列
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

    // UI：地标选中态贴图（避免第一次 hover 卡一下）
    for (const auto& info : _markerInfos)
    {
        addPath(info.selectedImage);
    }

    // 常用粒子贴图：避免首次受击/爆炸时加载造成卡顿
    addPath("Particle/particle_texture.png");

    // 哥布林资源（首刷会卡）：贴图先入 TextureCache
    addPath("Sprites/Enemies/Goblin/Goblin_idle.png");
    addPath("Sprites/Enemies/Goblin/Goblin_beattacked.png");
    for (int i = 1; i <= 4; ++i)
    {
        addPath(StringUtils::format("Sprites/Enemies/Goblin/Goblin_walk_%d.png", i));
        addPath(StringUtils::format("Sprites/Enemies/Goblin/Goblin_attack_%d.png", i));
    }

    // Boss（Goblu）资源：提前热身，避免第一次出现卡顿
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

    _preloadTotal = static_cast<int>(paths.size());
    updatePreloadLabel();

    if (_preloadTotal <= 0)
    {
        onOriginMushroomPreloadFinished();
        return;
    }

    auto textureCache = Director::getInstance()->getTextureCache();
    for (const auto& path : paths)
    {
        textureCache->addImageAsync(path,
                                    [this](Texture2D* texture)
                                    {
                                        this->onPreloadTextureLoaded(texture);
                                    },
                                    _preloadCallbackKey);
    }
}

void MapScene::onPreloadTextureLoaded(Texture2D* /*texture*/)
{
    if (!_originMushroomPreloading)
    {
        return;
    }

    _preloadLoaded = std::min(_preloadLoaded + 1, _preloadTotal);
    updatePreloadLabel();

    if (_preloadLoaded >= _preloadTotal && !_originMushroomFinishScheduled)
    {
        _originMushroomFinishScheduled = true;
        // 避免在 addImageAsync 的同步回调路径里直接触发换场景，延后一帧执行
        runAction(Sequence::create(DelayTime::create(0.0f),
                                   CallFunc::create([this]()
                                                    { this->onOriginMushroomPreloadFinished(); }),
                                   nullptr));
    }
}

void MapScene::onOriginMushroomPreloadFinished()
{
    _originMushroomAssetsReady = true;
    _originMushroomPreloading = false;
    _originMushroomFinishScheduled = false;

    if (_preloadLabel)
    {
        _preloadLabel->setVisible(false);
    }

    // 预热动画缓存：贴图已进 TextureCache，这里主要是填充 AnimationCache
    //GoblinMonster::preloadResources();
    //GobluMonster::preloadResources();
}

void MapScene::updatePreloadLabel()
{
    if (!_preloadLabel || !_preloadLabel->isVisible())
    {
        return;
    }

    if (_preloadTotal <= 0)
    {
        _preloadLabel->setString("加载中...");
        return;
    }
    _preloadLabel->setString(StringUtils::format("加载中... (%d/%d)", _preloadLoaded, _preloadTotal));
}
