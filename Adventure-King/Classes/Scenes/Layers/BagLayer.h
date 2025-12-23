#pragma once
//背包的页面建议放在这里
#pragma once
#ifndef __BAG_LAYER_H__
#define __BAG_LAYER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h" // 包含所有 UI 组件（ScrollView, Button 等）

class BagLayer : public cocos2d::Layer
{
public:
    static BagLayer* create();
    virtual bool init() override;

private:
    // --- 1. 模块化初始化 ---
    bool initBackground();
    bool initTitle();
    bool initScrollView();    // 背包核心：滚动容器
    bool initCloseButton();
    bool initItemDetails();   // 侧边栏：点击道具后显示的详细信息

    // --- 2. 逻辑处理 ---
    void refreshInventory();   // 核心：根据数据源刷新背包格子
    void onItemSelected(cocos2d::Ref* sender, int itemID);
    void onClose(cocos2d::Ref* sender);

    // --- 3. 模态拦截 ---
    virtual bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event) override;

    // --- 4. 成员变量 ---
    cocos2d::ui::ScrollView* _scrollView; // 用于放置道具格子
    cocos2d::Label* _detailNameLabel;
    cocos2d::Label* _detailDescLabel;
    cocos2d::Sprite* _detailIcon;
};

#endif
