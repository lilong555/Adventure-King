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

    /**
     * @brief 初始化 Boss 血条节点
     */
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
    /// @brief 创建背景容器
    void createBackground();
    /// @brief 创建血条（含底/填充/伤害条）
    void createHealthBar();
    /// @brief 创建击破条（Boss 机制）
    void createBreakBar();
    /// @brief 创建 Boss 名称文本
    void createNameLabel();
    /// @brief 创建阶段指示器
    void createPhaseIndicators(int phaseCount);
    /// @brief 更新血条填充显示
    void updateHealthBar(float current, float max);
    /// @brief 更新击破条填充显示
    void updateBreakBar(int current, int max);

protected:
    CharacterBase *_boss = nullptr;
    std::string _bossName;

    // UI元素
    cocos2d::Node *_container = nullptr;
    cocos2d::Node *_content = nullptr; // show/hide 动画不影响受击反馈，便于分层控制
    cocos2d::Vec2 _contentBasePos = cocos2d::Vec2::ZERO; // 受击反馈回弹的固定基准位置（避免连击导致漂移）
    float _contentBaseScale = 1.0f;                      // 受击反馈回弹的固定基准缩放（避免连击导致累计变大）
    cocos2d::DrawNode *_background = nullptr;
    cocos2d::DrawNode *_healthBarBg = nullptr;
    cocos2d::DrawNode *_healthBarFill = nullptr;
    cocos2d::DrawNode *_healthBarDamage = nullptr;
    cocos2d::DrawNode *_breakBarBg = nullptr;
    cocos2d::DrawNode *_breakBarFill = nullptr;
    cocos2d::DrawNode *_breakBarBorder = nullptr;
    cocos2d::Label *_nameLabel = nullptr;
    cocos2d::Label *_hpLabel = nullptr;
    cocos2d::Label *_comboDamageLabel = nullptr; // 连击伤害（1秒窗口）
    std::vector<cocos2d::DrawNode *> _phaseIndicators;

    // 配置参数
    float _barWidth = 500.0f;
    float _barHeight = 25.0f;
    float _breakBarHeight = 10.0f;
    int _phaseCount = 1;
    int _currentPhase = 1;

    // 动画相关
    float _lastHP = 0.0f;
    float _damageBarHP = 0.0f;
    bool _isVisible = false;

    // 连击伤害统计（容差 1 秒）：窗口内累计“非 DOT 伤害”
    static constexpr float COMBO_WINDOW_SECONDS = 1.0f;
    float _comboWindowRemaining = 0.0f;
    double _comboDamageSum = 0.0;
    long long _comboLastUpdateMs = 0; // 用真实时间计算窗口衰减（UI 有节流，不保证每帧更新）
};
