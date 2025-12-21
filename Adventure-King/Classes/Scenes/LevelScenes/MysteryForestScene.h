#pragma once
#include "Scenes/GameScene.h"
// ============================================================
// 神秘之森场景
// ============================================================

class MysteryForestScene : public GameScene
{
public:
    /// @brief 创建神秘之森场景
    static cocos2d::Scene* createScene();
    /// @brief 初始化神秘之森场景
    virtual bool init() override;
    CREATE_FUNC(MysteryForestScene);
    // 将此场景用到的所有资源注册到场景注册中心
    static void setupRegistry();
protected:
    /// @brief 关卡名
    virtual std::string getLevelName() const override { return "神秘之森"; }
    /// @brief 关卡配置
    virtual LevelConfig getLevelConfig() const override;
};
