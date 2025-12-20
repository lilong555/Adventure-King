#pragma once
#include "Scenes/GameScene.h"
// ============================================================
// 起源之菇场景
// ============================================================

class OriginMushroomScene : public GameScene
{
public:
    /// @brief 创建起源之菇场景
    static cocos2d::Scene* createScene();
    /// @brief 初始化起源之菇场景
    virtual bool init() override;
    CREATE_FUNC(OriginMushroomScene);

    // 将此场景用到的所有资源注册到场景注册中心
    static void setupRegistry();
protected:
    /// @brief 关卡名
    virtual std::string getLevelName() const override { return "起源之菇"; }
    /// @brief 关卡配置
    virtual LevelConfig getLevelConfig() const override;

};
