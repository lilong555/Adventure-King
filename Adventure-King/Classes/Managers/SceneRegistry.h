#pragma once
#ifndef __SCENE_REGISTRY_H__
#define __SCENE_REGISTRY_H__
//LoadingScene可以作为SceneTransitionManager的加强版，但他存在耦合度过高的问题，
// 这里定义一个场景注册中心，供LoadingScene使用，从而降低耦合度
#include "cocos2d.h"
#include <functional>
#include <vector>
#include <map>
#include <string>

// 定义场景信息的结构体
struct SceneInfo {
    // 1. 如何创建这个场景？ (工厂方法)
    std::function<cocos2d::Scene* ()> creator;
    // 2. 这个场景需要加载哪些图片路径？
    std::vector<std::string> imagePaths;
    // 3. 加载完贴图后，还需要做什么额外的预热？(比如 AnimationCache)
    std::function<void()> onResourcesLoaded;
};

class SceneRegistry {
public:
    static SceneRegistry* getInstance();

    // 注册场景的方法
    void registerScene(int mapId, const SceneInfo& info);

    // 获取场景信息
    const SceneInfo* getSceneInfo(int mapId) const;

private:
    std::map<int, SceneInfo> _registry;
};

#endif
