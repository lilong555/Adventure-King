//createScene(mapId)
//↓
//init()
//↓
//initWithMapId(mapId)
//↓
//startPreload()
//↓
//onTextureLoaded()   ← 多次回调
//↓
//finishPreload()     ← 触发 SceneRegistry 的 onResourcesLoaded + creator
//↓
//切换到真正关卡
#pragma once
#include "cocos2d.h"
#include "Configs/GameSceneConfig.h" // 确保包含 SceneID
#include <string>
#include <vector>

class LoadingScene : public cocos2d::Scene
{
public:
    // 统一使用 SceneID
    static cocos2d::Scene* createScene(SceneID id);

    bool init() override;
    void onEnter() override;
    void onExit() override;
    ~LoadingScene() override;

private:
    // 基础初始化
    bool initWithSceneId(SceneID id);

    // 加载核心流程
    void startPreload();
    void onTextureLoaded(cocos2d::Texture2D* texture);
    void finishPreload();

    // 场景切换逻辑
    void tryReplacePendingScene();
    void updateProgressUI();

    // 数据驱动方法
    std::vector<std::string> buildPreloadList(SceneID id)const;

    // UI 组件
    cocos2d::LayerColor* _barBg = nullptr;
    cocos2d::LayerColor* _barFill = nullptr;
    cocos2d::Label* _label = nullptr;

    // 状态管理
    SceneID _targetId = SceneID::NONE; // 统一名称
    std::vector<std::string> _paths;
    int _total = 0;
    int _loaded = 0;
    bool _finished = false;
    bool _finishScheduled = false;

    std::string _callbackKey;
    cocos2d::Scene* _pendingDestinationScene = nullptr;
};
