#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <functional>
#include <string>

/**
 * @brief 云端账号登录/注册弹窗（主菜单使用）
 *
 * 设计原则：
 * - 会话级账号：不落盘（避免存储明文密码）
 * - 支持游客模式：由主菜单控制，游客模式下禁用云功能
 */
class CloudAuthLayer : public cocos2d::LayerColor
{
public:
    using DoneCallback = std::function<void(bool ok, const std::string &message)>;

    static CloudAuthLayer *create(const DoneCallback &cb);

    bool init(const DoneCallback &cb);

private:
    DoneCallback _doneCallback;

    cocos2d::LayerColor *_panel = nullptr;
    cocos2d::Label *_messageLabel = nullptr;

    cocos2d::ui::TextField *_urlField = nullptr;
    cocos2d::ui::TextField *_userField = nullptr;
    cocos2d::ui::TextField *_passField = nullptr;

    cocos2d::MenuItemLabel *_loginItem = nullptr;
    cocos2d::MenuItemLabel *_registerItem = nullptr;
    cocos2d::MenuItemLabel *_cancelItem = nullptr;

    bool _busy = false;
    bool _ctrlDown = false;

    void setBusy(bool busy);
    void setMessage(const std::string &msg, const cocos2d::Color4B &color = cocos2d::Color4B::WHITE);

    std::string getUrl() const;
    std::string getUsername() const;
    std::string getPassword() const;

    void startAuthRequest(bool isRegister);

    void onLoginClicked();
    void onRegisterClicked();
    void onCancelClicked();
};
