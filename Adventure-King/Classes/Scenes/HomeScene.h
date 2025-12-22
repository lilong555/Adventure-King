#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "GameScene.h"

class HomeScene : public GameScene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;

    // 必须实现的纯虚函数
    virtual std::string getLevelName() const override { return "HomeScene"; }

    // “家”不需要刷怪
    virtual float getEnemySpawnViewDistance() const override { return 0.0f; }

    CREATE_FUNC(HomeScene);
    void menuReturnCallback(Ref* pSender);
};

#endif
