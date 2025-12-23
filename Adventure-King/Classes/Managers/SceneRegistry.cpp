#include "Managers/SceneRegistry.h"

// 如果你需要打印日志，可以包含 cocos2d.h
// #include "cocos2d.h"

SceneRegistry* SceneRegistry::getInstance()
{
    // C++11 局部静态变量：
    // 1. 它是线程安全的（在初始化时）。
    // 2. 它不需要手动 delete，程序结束时会自动析构。
    // 3. 这种写法不需要在 .h 文件里声明 private static 成员变量。
    static SceneRegistry instance;
    return &instance;
}

void SceneRegistry::registerScene(int mapId, const SceneInfo& info)
{
    // 将 info 存入 map。
    // 如果 mapId 已经存在，operator[] 会覆盖旧的数据，这通常是我们期望的行为（允许更新注册信息）。
    _registry[mapId] = info;

    // 可选：添加日志方便调试
    // cocos2d::log("SceneRegistry: Registered MapID %d", mapId);
}

const SceneInfo* SceneRegistry::getSceneInfo(int mapId) const
{
    // 在 map 中查找 mapId
    auto it = _registry.find(mapId);

    // 如果找到了 (迭代器不等于 end())
    if (it != _registry.end())
    {
        // 返回值的地址 (&it->second)
        // 注意：这里返回指针而不是引用或拷贝，是为了能返回 nullptr 表示“未找到”
        return &(it->second);
    }

    // 未找到，返回空指针
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

int SceneRegistry::getMapIdBySceneName(const std::string& sceneName) const
{
    if (sceneName.empty())
    {
        return -1;
    }

    for (const auto& kv : _registry)
    {
        if (kv.second.sceneName == sceneName)
        {
            return kv.first;
        }
    }
    return -1;
}
