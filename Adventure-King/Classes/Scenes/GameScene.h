/**
 * @file GameScene.h
 * @brief 游戏关卡场景基类
 *
 * 定义了所有游戏关卡场景的通用功能，包括：
 * - 地图按钮（返回地图选择界面）
 * - 场景基础布局
 */

#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"

class GameScene : public cocos2d::Scene
{
public:
    virtual bool init() override;
    static cocos2d::Scene* createScene();
    // 节点标签枚举
    enum NodeTags
    {
        TAG_MAP_BUTTON = 100,
        TAG_BACKGROUND = 101,
    };

protected:
    /**
     * @brief 创建地图按钮（左上角）
     * 点击后返回地图选择界面
     */
    void createMapButton();

    /**
     * @brief 地图按钮回调
     * @param pSender 发送者
     */
    void onMapButtonClicked(cocos2d::Ref *pSender);

    /**
     * @brief 设置场景背景（子类可重写）
     * @param backgroundPath 背景图片路径
     */
    virtual void setupBackground(const std::string &backgroundPath);

    /**
     * @brief 获取关卡名称（子类需重写）
     * @return 关卡名称字符串
     */
    virtual std::string getLevelName() const = 0;
};

// ============================================================
// 起源之菇场景
// ============================================================
class OriginMushroomScene : public GameScene
{
public:
    static cocos2d::Scene *createScene();
    virtual bool init() override;
    CREATE_FUNC(OriginMushroomScene);

protected:
    virtual std::string getLevelName() const override { return "起源之菇"; }
};

// ============================================================
// 神秘之森场景
// ============================================================
class MysteryForestScene : public GameScene
{
public:
    static cocos2d::Scene *createScene();
    virtual bool init() override;
    CREATE_FUNC(MysteryForestScene);

protected:
    virtual std::string getLevelName() const override { return "神秘之森"; }
};

#endif // __GAME_SCENE_H__
