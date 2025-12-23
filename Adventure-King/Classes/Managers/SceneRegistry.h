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
    // 0. 场景名（用于存档中的 currentSceneName 反查 mapId/creator）
    std::string sceneName;
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

    // 通过场景名查找场景信息（用于读档等场景恢复）
    const SceneInfo* getSceneInfoBySceneName(const std::string& sceneName) const;

    // 通过场景名反查 mapId（找不到返回 -1）
    int getMapIdBySceneName(const std::string& sceneName) const;

private:
    std::map<int, SceneInfo> _registry;
};

#endif
