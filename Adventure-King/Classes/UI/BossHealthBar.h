/**
 * @file BossHealthBar.h
 * @brief Boss血条组件
 *
 * 显示Boss的血量信息：
 * - 大型血条显示在屏幕顶部
 * - Boss名称
 * - 阶段指示器（支持多阶段Boss）
 * - 血量变化动画
 */

#pragma once

#include "cocos2d.h"

class CharacterBase;

class BossHealthBar : public cocos2d::Node
{
public:
    /**
     * @brief 创建Boss血条
     * @return BossHealthBar 指针
     */
    static BossHealthBar *create();

    virtual bool init() override;

    /**
     * @brief 绑定Boss角色
     * @param boss Boss角色指针
     * @param bossName Boss名称
     * @param phaseCount 阶段数量（默认1）
     */
    void bindBoss(CharacterBase *boss, const std::string &bossName, int phaseCount = 1);

    /**
     * @brief 解绑Boss（Boss死亡时调用）
     */
    void unbindBoss();

    /**
     * @brief 更新血条显示
     */
    void updateDisplay();

    /**
     * @brief 设置当前阶段
     * @param phase 阶段编号（从1开始）
     */
    void setCurrentPhase(int phase);

    /**
     * @brief 显示Boss血条（带动画）
     */
    void show();

    /**
     * @brief 隐藏Boss血条（带动画）
     */
    void hide();

    /**
     * @brief 播放受击动画
     */
    void playHitAnimation();

    /**
     * @brief 播放阶段转换动画
     */
    void playPhaseTransitionAnimation();

protected:
    void createBackground();
    void createHealthBar();
    void createNameLabel();
    void createPhaseIndicators(int phaseCount);
    void updateHealthBar(float current, float max);

protected:
    CharacterBase *_boss = nullptr;
    std::string _bossName;

    // UI元素
    cocos2d::Node *_container = nullptr;
    cocos2d::DrawNode *_background = nullptr;
    cocos2d::DrawNode *_healthBarBg = nullptr;
    cocos2d::DrawNode *_healthBarFill = nullptr;
    cocos2d::DrawNode *_healthBarDamage = nullptr;
    cocos2d::Label *_nameLabel = nullptr;
    cocos2d::Label *_hpLabel = nullptr;
    std::vector<cocos2d::DrawNode *> _phaseIndicators;

    // 配置参数
    float _barWidth = 500.0f;
    float _barHeight = 25.0f;
    int _phaseCount = 1;
    int _currentPhase = 1;

    // 动画相关
    float _lastHP = 0.0f;
    float _damageBarHP = 0.0f;
    bool _isVisible = false;
};
