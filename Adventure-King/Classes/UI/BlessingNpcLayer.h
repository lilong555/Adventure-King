#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <functional>
#include <string>

class PlayerCharacter;
struct Attributes;

/**
 * @brief 赐福 NPC 弹窗（展示阶段：用于验证 OpenAI 兼容接口 + 覆盖式属性 Buff）
 *
 * - baseUrl / apiKey 由玩家在游戏内填写（不写死）
 * - 点击“请求赐福”会向 OpenAI 格式接口发送 chat/completions
 * - 返回的赐福属性会覆盖旧赐福（只保留一个 Buff）
 */
class BlessingNpcLayer : public cocos2d::LayerColor
{
public:
    using CloseCallback = std::function<void()>;

    static BlessingNpcLayer *create();
    bool init() override;
    void onExit() override;

    void bindPlayer(PlayerCharacter *player);
    void setCloseCallback(const CloseCallback &cb) { _closeCallback = cb; }

    void show();
    void hide();
    bool isShowing() const { return _showing; }

private:
    BlessingNpcLayer() = default;

    void setBusy(bool busy);
    void setMessage(const std::string &msg, const cocos2d::Color4B &color = cocos2d::Color4B::WHITE);

    std::string getBaseUrl() const;
    std::string getApiKey() const;
    std::string getModel() const;
    std::string getUserPrompt() const;

    void onSaveConfigClicked();
    void onRequestBlessingClicked();
    void onClearBlessingClicked();
    void onCloseClicked();

    std::string buildBlessingSummary(const Attributes &bonus) const;

    PlayerCharacter *_player = nullptr;

    cocos2d::LayerColor *_panel = nullptr;
    cocos2d::Label *_guideLabel = nullptr; // 左侧步骤说明（避免挤在面板内）
    cocos2d::Label *_messageLabel = nullptr;

    cocos2d::ui::TextField *_urlField = nullptr;
    cocos2d::ui::TextField *_apiKeyField = nullptr;
    cocos2d::ui::TextField *_modelField = nullptr;

    // 对话框：展示 NPC 问题与玩家回答
    cocos2d::LayerColor *_dialogPanel = nullptr;
    cocos2d::Label *_dialogNpcLabel = nullptr;
    cocos2d::ui::TextField *_promptField = nullptr; // 复用为“回答输入框”

    cocos2d::MenuItemLabel *_saveItem = nullptr;
    cocos2d::MenuItemLabel *_requestItem = nullptr;
    cocos2d::MenuItemLabel *_clearItem = nullptr;
    cocos2d::MenuItemLabel *_closeItem = nullptr;

    bool _busy = false;
    bool _showing = false;
    bool _ctrlDown = false;
    bool _imeRestored = false;

    bool _waitingForAnswer = false;
    std::string _cachedQuestions;

    CloseCallback _closeCallback = nullptr;
};
