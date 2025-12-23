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
