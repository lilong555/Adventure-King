
#include "AppDelegate.h"
#include"Scenes/LevelScenes/OriginMushroomScene.h"
#include"Scenes/LevelScenes/MysteryForestScene.h"
#include "Scenes/HomeScene.h"
#include "Scenes/DebugScene.h"
#include "Scenes/LoadingScene.h"
#include "Scenes/HelloWorldScene.h"
#include "Configs/GameConfigs.h"
#include <cstdlib>

#if USE_AUDIO_ENGINE && USE_SIMPLE_AUDIO_ENGINE
#error "Don't use AudioEngine and SimpleAudioEngine at the same time. Please just select one in your game!"
#endif

#if USE_AUDIO_ENGINE
#include "audio/include/AudioEngine.h"
using namespace cocos2d::experimental;
#elif USE_SIMPLE_AUDIO_ENGINE
#include "audio/include/SimpleAudioEngine.h"
using namespace CocosDenshion;
#endif

USING_NS_CC;

static const auto &designResolutionSize = GameConfig::App::DESIGN_RESOLUTION_SIZE;
static const auto &smallResolutionSize = GameConfig::App::SMALL_RESOLUTION_SIZE;
static const auto &mediumResolutionSize = GameConfig::App::MEDIUM_RESOLUTION_SIZE;
static const auto &largeResolutionSize = GameConfig::App::LARGE_RESOLUTION_SIZE;

AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate()
{
#if USE_AUDIO_ENGINE
    AudioEngine::end();
#elif USE_SIMPLE_AUDIO_ENGINE
    SimpleAudioEngine::end();
#endif
}

// if you want a different context, modify the value of glContextAttrs
// it will affect all platforms
void AppDelegate::initGLContextAttrs()
{
    // set OpenGL context attributes: red,green,blue,alpha,depth,stencil,multisamplesCount
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8, 0};

    GLView::setGLContextAttrs(glContextAttrs);
}

// 包管理器注册入口（保持默认实现即可）
static int register_all_packages()
{
    return 0; // flag for packages manager
}

bool AppDelegate::applicationDidFinishLaunching()
{
    // initialize director
    auto director = Director::getInstance();
    auto glview = director->getOpenGLView();
    if (!glview)
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
        glview = GLViewImpl::createWithRect("Adventure-King", cocos2d::Rect(0, 0, designResolutionSize.width, designResolutionSize.height));
#else
        glview = GLViewImpl::create("Adventure-King");
#endif
        director->setOpenGLView(glview);
    }

    // turn on display FPS
    director->setDisplayStats(GameConfig::App::SHOW_FPS);

    // set FPS. the default value is 1.0/60 if you don't call this
    // 开发默认使用 144FPS 便于观察波动；可通过环境变量 ADVENTURE_KING_FPS 覆盖
    float targetFps = GameConfig::App::DEFAULT_FPS;
    if (const char *fpsEnv = std::getenv(GameConfig::App::FPS_ENV_NAME))
    {
        const int fps = std::atoi(fpsEnv);
        if (fps > 0)
        {
            const int cappedFps = (fps > GameConfig::App::MAX_FPS) ? GameConfig::App::MAX_FPS : fps;
            targetFps = static_cast<float>(cappedFps);
        }
    }
    director->setAnimationInterval(1.0f / targetFps);

    // Set the design resolution
    glview->setDesignResolutionSize(designResolutionSize.width, designResolutionSize.height, ResolutionPolicy::SHOW_ALL);
    auto frameSize = glview->getFrameSize();
    // if the frame's height is larger than the height of medium size.
    if (frameSize.height > mediumResolutionSize.height)
    {
        director->setContentScaleFactor(MIN(largeResolutionSize.height / designResolutionSize.height, largeResolutionSize.width / designResolutionSize.width));
    }
    // if the frame's height is larger than the height of small size.
    else if (frameSize.height > smallResolutionSize.height)
    {
        director->setContentScaleFactor(MIN(mediumResolutionSize.height / designResolutionSize.height, mediumResolutionSize.width / designResolutionSize.width));
    }
    // if the frame's height is smaller than the height of medium size.
    else
    {
        director->setContentScaleFactor(MIN(smallResolutionSize.height / designResolutionSize.height, smallResolutionSize.width / designResolutionSize.width));
    }

    // 注意在这里创建游戏资源的注册表
    HelloWorld::setupRegistry();
    OriginMushroomScene::setupRegistry();
    MysteryForestScene::setupRegistry();
    HomeScene::setupRegistry();
    DebugScene::setupRegistry();
    

    register_all_packages();

    // create a scene. it's an autorelease object
    auto scene = LoadingScene::createScene(SceneID::HELLO_WORLD);

    director->runWithScene(scene);

    return true;
}

// This function will be called when the app is inactive. Note, when receiving a phone call it is invoked.
void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();

#if USE_AUDIO_ENGINE
    AudioEngine::pauseAll();
#elif USE_SIMPLE_AUDIO_ENGINE
    SimpleAudioEngine::getInstance()->pauseBackgroundMusic();
    SimpleAudioEngine::getInstance()->pauseAllEffects();
#endif
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->startAnimation();

#if USE_AUDIO_ENGINE
    AudioEngine::resumeAll();
#elif USE_SIMPLE_AUDIO_ENGINE
    SimpleAudioEngine::getInstance()->resumeBackgroundMusic();
    SimpleAudioEngine::getInstance()->resumeAllEffects();
#endif
}
