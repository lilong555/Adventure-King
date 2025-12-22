#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "Scenes/GameScene.h"

class HomeScene : public GameScene
{
public:
    // HomeScene 在 LoadingScene/SceneRegistry 中使用的地图 ID
    static constexpr int MAP_ID = 3;

    // 静态创建方法：用于生成场景实例
    static cocos2d::Scene *createScene();
    // 注册到场景注册表（供 LoadingScene 预加载使用）
    static void setupRegistry();

    // 初始化方法：用于设置场景内容
    virtual bool init() override;
    // 创建宏
    CREATE_FUNC(HomeScene);

protected:
    // 关卡名（用于 UI 显示与存档标识）
    virtual std::string getLevelName() const override { return "冒险王之家"; }
    // 关卡配置
    virtual LevelConfig getLevelConfig() const override;
};

#endif // __HOME_SCENE_H__
