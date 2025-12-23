#include "Managers/SceneRegistry.h"

SceneRegistry* SceneRegistry::getInstance()
{
    static SceneRegistry instance;
    return &instance;
}

void SceneRegistry::registerScene(SceneID id, const SceneInfo& info)
{
    // 插入或覆盖场景信息
    _registry[id] = info;
    cocos2d::log("SceneRegistry: Registered SceneID %d", static_cast<int>(id));
}

const SceneInfo* SceneRegistry::getSceneInfo(SceneID id) const
{
    auto it = _registry.find(id);
    if (it != _registry.end())
    {
        return &(it->second);
    }
    return nullptr;
}

cocos2d::Scene* SceneRegistry::createSceneInstance(SceneID id)
{
    auto it = _registry.find(id);
    if (it != _registry.end() && it->second.creator)
    {
        // 调用 SceneInfo 中存储的工厂函数
        return it->second.creator();
    }

    CCLOG("Error: SceneRegistry - No creator found for SceneID %d", static_cast<int>(id));
    return nullptr;
}

const SceneInfo* SceneRegistry::getSceneInfoBySceneName(const std::string& sceneName) const
{
    if (sceneName.empty())
    {
        return nullptr;
    }

    for (const auto& kv : _registry)
    {
        if (kv.second.sceneName == sceneName)
        {
            return &(kv.second);
        }
    }
    return nullptr;
}

SceneID SceneRegistry::getSceneIDByName(const std::string& sceneName) const
{

    // 1. 基础防御
    if (sceneName.empty())
    {
        return SceneID::NONE;
    }

    // 2. 遍历注册表进行反向查找
    // 使用 C++17 结构化绑定 [id, info] 代替 kv.first/kv.second
    for (const auto& [id, info] : _registry)
    {
        if (info.sceneName == sceneName)
        {
            return id;
        }
    }

    // 3. 容错处理：未找到时记录日志并返回 NONE
    CCLOG("SceneRegistry: No SceneID found for name '%s'", sceneName.c_str());
    return SceneID::NONE;
}
