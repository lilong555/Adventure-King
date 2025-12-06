/**
 * @file GameUI.h
 * @brief 游戏内 UI 层
 *
 * 管理游戏过程中的 UI 元素，包括：
 * - 地图按钮
 * - 交互提示
 * - 其他 HUD 元素
 *
 * UI 层不受相机跟随影响，始终显示在屏幕固定位置
 */

#ifndef __GAME_UI_H__
#define __GAME_UI_H__

#include "cocos2d.h"

class GameUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建 GameUI 实例
     * @return GameUI 指针
     */
    static GameUI *create();

    /**
     * @brief 初始化
     */
    virtual bool init() override;

    /**
     * @brief 设置地图按钮回调
     * @param callback 点击回调函数
     */
    void setMapButtonCallback(const std::function<void()> &callback);

    /**
     * @brief 显示交互提示（如 "按 W 进入传送门"）
     * @param message 提示信息
     */
    void showInteractionHint(const std::string &message);

    /**
     * @brief 隐藏交互提示
     */
    void hideInteractionHint();

    /**
     * @brief 设置关卡名称
     * @param name 关卡名称
     */
    void setLevelName(const std::string &name);

    /**
     * @brief 更新 UI 位置（跟随相机）
     * @param cameraOffset 相机偏移量（场景位置的负值）
     */
    void updatePosition(const cocos2d::Vec2 &cameraOffset);

protected:
    // 地图按钮
    cocos2d::MenuItemImage *_mapButton = nullptr;
    cocos2d::Menu *_mapMenu = nullptr;

    // 交互提示标签
    cocos2d::Label *_interactionHint = nullptr;

    // 关卡名称标签
    cocos2d::Label *_levelNameLabel = nullptr;

    // 地图按钮回调
    std::function<void()> _mapButtonCallback;

    // UI 元素的相对位置（相对于屏幕）
    cocos2d::Vec2 _mapButtonPos;
    cocos2d::Vec2 _interactionHintPos;
    cocos2d::Vec2 _levelNamePos;

    /**
     * @brief 创建地图按钮
     */
    void createMapButton();

    /**
     * @brief 创建交互提示
     */
    void createInteractionHint();

    /**
     * @brief 创建关卡名称标签
     */
    void createLevelNameLabel();

    /**
     * @brief 地图按钮点击回调
     */
    void onMapButtonClicked(cocos2d::Ref *sender);
};

#endif // __GAME_UI_H__
