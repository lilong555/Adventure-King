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

    virtual bool init(Mode mode,
                      PlayerCharacter *player,
                      const std::string &sceneName,
                      const cocos2d::Vec2 &playerPos);

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
    std::vector<cocos2d::Node *> _slotNodes; // 存档槽位节点

    LoadSuccessCallback _loadSuccessCallback = nullptr;

    const float TARGET_HEIGHT_RATIO = 0.7f; // 背景高度占屏幕比例

    // 初始化方法
    bool initBackground();
    bool initTitle();
    bool initSlots();
    bool initCloseButton();
    void layoutUI();

    // 槽位操作
    void onSlotClicked(int slotIndex);
    void onDeleteClicked(int slotIndex);

    // 确认对话框
    void showConfirmDialog(const std::string &message, const std::function<void()> &onConfirm);

    // 关闭按钮
    void onClose(cocos2d::Ref *sender);

    // 触摸事件
    virtual bool onTouchBegan(cocos2d::Touch *touch, cocos2d::Event *event) override;

    // 辅助方法
    cocos2d::Node *createSlotNode(int slotIndex, const SaveSlotData &slotData);
    std::string formatTimestamp(int64_t timestamp) const;
};
