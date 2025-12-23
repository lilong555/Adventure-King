#pragma once
#ifndef __SCENE_REGISTRY_H__
#define __SCENE_REGISTRY_H__

#include "cocos2d.h"
#include "Configs/GameSceneConfig.h" // 必须包含 SceneID 定义
#include <functional>
#include <vector>
#include <map>
#include <string>

/**
 * @brief 场景注册信息
 */
struct SceneInfo {
    // 场景名称（用于 UI 显示与存档标识）
    std::string sceneName;
    // 场景创建工厂函数
    std::function<cocos2d::Scene* ()> creator;
    // 场景依赖的资源路径列表
    std::vector<std::string> imagePaths;
    // 资源加载完成后的回调（用于预热动画、缓存等）
    std::function<void()> onResourcesLoaded;
};

class SceneRegistry {
public:
    // Meyers Singleton: 线程安全且自动管理生命周期
    static SceneRegistry* getInstance();

    // 禁止拷贝和赋值
    SceneRegistry(const SceneRegistry&) = delete;
    SceneRegistry& operator=(const SceneRegistry&) = delete;

    /**
     * @brief 注册场景信息
     */
    void registerScene(SceneID id, const SceneInfo& info);

    /**
     * @brief 获取场景配置信息
     */
    const SceneInfo* getSceneInfo(SceneID id) const;

    /**
     * @brief 直接根据 ID 创建场景实例
     */
    cocos2d::Scene* createSceneInstance(SceneID id);

    // 通过场景名查找场景信息（用于读档等场景恢复）
    const SceneInfo* getSceneInfoBySceneName(const std::string& sceneName) const;

    // 通过场景名反查 mapId（找不到返回 -1）
    SceneID getSceneIDByName(const std::string& sceneName) const;

private:
    SceneRegistry() = default;

    // 统一使用 SceneID 作为键，SceneInfo 已经包含了所有信息
    std::map<SceneID, SceneInfo> _registry;
};

#endif
