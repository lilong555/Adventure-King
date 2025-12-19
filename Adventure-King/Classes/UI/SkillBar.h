/**
 * @file SkillBar.h
 * @brief 技能栏组件
 *
 * 显示玩家的主动技能槽位：
 * - 技能图标
 * - 冷却遮罩和倒计时
 * - 快捷键提示
 */

#pragma once

#include "cocos2d.h"
#include <vector>
#include <memory>

class PlayerCharacter;
struct ActiveSkill;

/**
 * @brief 单个技能槽位UI
 */
struct SkillSlotUI
{
    cocos2d::Node *container = nullptr;      // 槽位容器
    cocos2d::Sprite *icon = nullptr;         // 技能图标
    cocos2d::Sprite *iconBg = nullptr;       // 图标背景
    cocos2d::DrawNode *cooldownMask = nullptr; // 冷却遮罩
    cocos2d::Label *cooldownLabel = nullptr; // 冷却倒计时文字
    cocos2d::Label *hotkeyLabel = nullptr;   // 快捷键提示
    cocos2d::DrawNode *border = nullptr;     // 边框
    bool isEmpty = true;                     // 是否为空槽位
};

class SkillBar : public cocos2d::Node
{
public:
    /**
     * @brief 创建技能栏
     * @param slotCount 技能槽位数量
     * @return SkillBar 指针
     */
    static SkillBar *create(int slotCount = 4);

    /**
     * @brief 初始化技能栏节点
     */
    virtual bool init() override;

    /**
     * @brief 初始化技能槽位
     * @param slotCount 槽位数量
     */
    void initSlots(int slotCount);

    /**
     * @brief 绑定玩家角色
     * @param player 玩家角色指针
     */
    void bindPlayer(PlayerCharacter *player);

    /**
     * @brief 更新技能栏显示
     * 应在每帧调用以更新冷却显示
     */
    void updateDisplay();

    /**
     * @brief 设置槽位的技能图标
     * @param slotIndex 槽位索引
     * @param iconPath 图标路径
     */
    void setSlotIcon(size_t slotIndex, const std::string &iconPath);

    /**
     * @brief 设置槽位的快捷键提示
     * @param slotIndex 槽位索引
     * @param hotkey 快捷键文字（如 "E", "Q"）
     */
    void setSlotHotkey(size_t slotIndex, const std::string &hotkey);

    /**
     * @brief 播放技能使用动画
     * @param slotIndex 槽位索引
     */
    void playUseAnimation(size_t slotIndex);

    /**
     * @brief 设置技能栏布局方向
     * @param horizontal true为水平排列，false为垂直排列
     */
    void setHorizontalLayout(bool horizontal);

    /**
     * @brief 获取槽位数量
     */
    int getSlotCount() const { return static_cast<int>(_slots.size()); }

protected:
    /// @brief 创建单个技能槽位
    void createSlot(int index);
    /// @brief 更新槽位冷却显示
    void updateSlotCooldown(size_t index, float currentCD, float maxCD);
    /// @brief 设置槽位是否为空
    void updateSlotEmpty(size_t index, bool isEmpty);

protected:
    PlayerCharacter *_player = nullptr;
    std::vector<SkillSlotUI> _slots;

    // 配置参数
    float _slotSize = 50.0f;
    float _slotSpacing = 8.0f;
    bool _horizontalLayout = true;
    int _slotCount = 4;
};
