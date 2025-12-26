#pragma once

#include "cocos2d.h"
#include "Save/SaveData.h"
#include <functional>

class PlayerCharacter;

/**
 * 存档菜单层
 * 支持保存和加载游戏存档
 */
class SaveMenuLayer : public cocos2d::Layer
{
public:
    // 菜单模式
    enum class Mode
    {
        SAVE, // 保存模式
        LOAD  // 加载模式
    };

    /**
     * 创建存档菜单层
     * @param mode 菜单模式（SAVE 或 LOAD）
     * @param player 玩家角色指针（仅在 SAVE 模式下需要）
     * @param sceneName 当前场景名称（仅在 SAVE 模式下需要）
     * @param playerPos 玩家位置（仅在 SAVE 模式下需要）
     */
    static SaveMenuLayer *create(Mode mode,
                                  PlayerCharacter *player = nullptr,
                                  const std::string &sceneName = "",
                                  const cocos2d::Vec2 &playerPos = cocos2d::Vec2::ZERO);

    /**
     * @brief 初始化存档菜单
     */
    virtual bool init(Mode mode,
                      PlayerCharacter *player,
                      const std::string &sceneName,
                      const cocos2d::Vec2 &playerPos);

    // 关闭回调（用于在 GameScene 中恢复暂停菜单/暂停状态）
    using CloseCallback = std::function<void()>;
    void setCloseCallback(const CloseCallback &callback) { _closeCallback = callback; }

    virtual void onExit() override;

    // 设置加载成功回调（仅在 LOAD 模式下使用）
    using LoadSuccessCallback = std::function<void(const SaveSlotData &)>;
    void setLoadSuccessCallback(const LoadSuccessCallback &callback) { _loadSuccessCallback = callback; }

private:
    SaveMenuLayer() = default;

    Mode _mode = Mode::SAVE;
    PlayerCharacter *_player = nullptr;
    std::string _sceneName;
    cocos2d::Vec2 _playerPos;

    cocos2d::Sprite *_background = nullptr;
    cocos2d::Label *_titleLabel = nullptr;
    cocos2d::Label *_cloudStatusLabel = nullptr;
    std::vector<cocos2d::Node *> _slotNodes; // 存档槽位节点

    LoadSuccessCallback _loadSuccessCallback = nullptr;
    CloseCallback _closeCallback = nullptr;

    // 初始化方法
    /// @brief 初始化背景图
    bool initBackground();
    /// @brief 初始化标题
    bool initTitle();
    /// @brief 初始化存档槽位
    bool initSlots();
    /// @brief 初始化关闭按钮
    bool initCloseButton();
    /// @brief 初始化云同步按钮/状态
    bool initCloudControls();
    /// @brief 刷新云端状态文本（不重建节点）
    void refreshCloudStatusLabel();
    /// @brief 布局 UI 元素
    void layoutUI();

    // 槽位操作
    /// @brief 点击存档槽位
    void onSlotClicked(int slotIndex);
    /// @brief 点击云端按钮（云存/云读）
    void onCloudClicked(int slotIndex);
    /// @brief 点击云同步（不自动加载）
    void onCloudSyncClicked();
    /// @brief 点击删除存档
    void onDeleteClicked(int slotIndex);

    // 确认对话框
    /// @brief 弹出确认对话框
    void showConfirmDialog(const std::string &message, const std::function<void()> &onConfirm);

    // 关闭按钮
    /// @brief 关闭存档菜单
    void onClose(cocos2d::Ref *sender);

    // 触摸事件
    /// @brief 吞噬触摸，保持模态
    virtual bool onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *event) override;

    // 辅助方法
    /// @brief 创建单个槽位节点
    cocos2d::Node *createSlotNode(int slotIndex, const SaveSlotData &slotData);
    /// @brief 格式化时间戳
    std::string formatTimestamp(int64_t timestamp) const;
    /// @brief 重新刷新槽位列表
    void reloadSlots();
};
