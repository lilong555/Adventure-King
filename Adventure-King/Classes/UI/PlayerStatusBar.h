/**
 * @file PlayerStatusBar.h
 * @brief 玩家状态栏组件
 *
 * 显示玩家的核心状态信息：
 * - HP 血量条（红色）
 * - MP 魔法条（蓝色）
 * - EXP 经验条（黄色）
 * - 等级显示
 */

#pragma once

#include "cocos2d.h"

class PlayerCharacter;

class PlayerStatusBar : public cocos2d::Node
{
public:
    /**
     * @brief 创建玩家状态栏
     * @return PlayerStatusBar 指针
     */
    static PlayerStatusBar *create();

    /**
     * @brief 初始化状态栏节点与子组件
     */
    virtual bool init() override;

    /**
     * @brief 绑定玩家角色
     * @param player 玩家角色指针
     */
    void bindPlayer(PlayerCharacter *player);

    /**
     * @brief 更新状态栏显示
     * 应在每帧调用以保持数据同步
     */
    void updateDisplay();

    /**
     * @brief 设置状态栏位置（相对于屏幕）
     * @param position 位置
     */
    void setBarPosition(const cocos2d::Vec2 &position);

    /**
     * @brief 设置状态栏缩放
     * @param scale 缩放比例
     */
    void setBarScale(float scale);

    /**
     * @brief 显示/隐藏经验条
     * @param visible 是否显示
     */
    void setExpBarVisible(bool visible);

protected:
    // 创建各类 UI 元素
    /// @brief 创建 HP 条
    void createHPBar();
    /// @brief 创建 MP 条
    void createMPBar();
    /// @brief 创建经验条
    void createExpBar();
    /// @brief 创建等级标签
    void createLevelLabel();

    // 更新各个进度条
    /// @brief 更新 HP 条显示
    void updateHPBar(float current, float max);
    /// @brief 更新 MP 条显示
    void updateMPBar(float current, float max);
    /// @brief 更新经验条显示
    void updateExpBar(int current, int required);
    /// @brief 更新等级文本
    void updateLevelLabel(int level);

    // 播放血量变化动画
    /// @brief 播放伤害延迟条动画
    void playHPChangeAnimation(float oldHP, float newHP);

protected:
    PlayerCharacter *_player = nullptr;

    // HP 条
    cocos2d::DrawNode *_hpBarBg = nullptr;
    cocos2d::DrawNode *_hpBarFill = nullptr;
    cocos2d::DrawNode *_hpBarDamage = nullptr; // 伤害延迟条（白色）
    cocos2d::Label *_hpLabel = nullptr;

    // MP 条
    cocos2d::DrawNode *_mpBarBg = nullptr;
    cocos2d::DrawNode *_mpBarFill = nullptr;
    cocos2d::Label *_mpLabel = nullptr;

    // 经验条
    cocos2d::DrawNode *_expBarBg = nullptr;
    cocos2d::DrawNode *_expBarFill = nullptr;
    cocos2d::Label *_expLabel = nullptr;
    bool _expBarVisible = true;

    // 等级显示
    cocos2d::Label *_levelLabel = nullptr;

    // 配置参数
    float _barWidth = 200.0f;
    float _barHeight = 18.0f;
    float _barSpacing = 6.0f;
    float _expBarHeight = 10.0f;

    // 缓存上一帧的HP值（用于伤害动画）
    float _lastHP = 0.0f;
    float _damageBarHP = 0.0f;
};
