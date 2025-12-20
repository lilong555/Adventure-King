#pragma once

#include "cocos2d.h"
#include <string>
#include <vector>

// 加载场景：用于进入关卡前预加载资源并显示进度条
class LoadingScene : public cocos2d::Scene
{
public:
    // 创建并开始加载指定地图（mapId 与 MapScene 的一致）
    static cocos2d::Scene* createScene(int mapId);

    bool init() override;
    ~LoadingScene() override;

private:
    bool initWithMapId(int mapId);

    // 启动预加载
    void startPreload();
    // 单个贴图加载完成回调
    void onTextureLoaded(cocos2d::Texture2D* texture);
    // 预加载全部完成
    void finishPreload();
    // 更新进度条与文本
    void updateProgressUI();

    // 根据 mapId 构建需要预加载的资源列表
    std::vector<std::string> buildPreloadList(int mapId) const;
    // 根据 mapId 创建目标场景
    cocos2d::Scene* createDestinationScene(int mapId) const;

    // UI
    cocos2d::LayerColor* _barBg = nullptr;
    cocos2d::LayerColor* _barFill = nullptr;
    cocos2d::Label* _label = nullptr;

    // 状态
    int _mapId = -1;
    std::vector<std::string> _paths;
    int _total = 0;
    int _loaded = 0;
    bool _finished = false;
    bool _finishScheduled = false;

    // 用于解绑 TextureCache 异步回调，避免回调访问已释放对象
    std::string _callbackKey;
};
