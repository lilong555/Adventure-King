#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "GameScene.h"


class HomeScene : public GameScene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;

    // “家”不需要刷怪
    virtual float getEnemySpawnViewDistance() const override { return 0.0f; }

    CREATE_FUNC(HomeScene);
    void menuReturnCallback(Ref* pSender);

    // HomeScene 在 LoadingScene/SceneRegistry 中使用的地图 ID
    //static constexpr int MAP_ID = 3;
    static constexpr SceneID ID = SceneID::HOME;
    // 注册到场景注册表（供 LoadingScene 预加载使用）
    static void setupRegistry();

protected:
    // 关卡名（用于 UI 显示与存档标识）
    virtual std::string getLevelName() const override { return "冒险王之家"; }
};

#endif
