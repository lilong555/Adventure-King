#include "Scenes/Layers/CloudAuthLayer.h"
#include "Save/Cloud/CloudSyncService.h"
#include "Utils/ClipboardHelper.h"

#include <algorithm>

USING_NS_CC;

CloudAuthLayer *CloudAuthLayer::create(const DoneCallback &cb)
{
    auto ret = new (std::nothrow) CloudAuthLayer();
    if (ret && ret->init(cb))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool CloudAuthLayer::init(const DoneCallback &cb)
{
    if (!LayerColor::initWithColor(Color4B(0, 0, 0, 160)))
    {
        return false;
    }

    _doneCallback = cb;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    const Vec2 center(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f);

    // 吞噬触摸，避免穿透到主菜单
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [](Touch *, Event *) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    // ESC 关闭
    auto getActiveField = [this]() -> ui::TextField * {
        ui::TextField *fields[] = {_urlField, _userField, _passField};
        for (auto *f : fields)
        {
            if (f && f->getAttachWithIME())
            {
                return f;
            }
        }
        for (auto *f : fields)
        {
            if (f && f->isFocused())
            {
                return f;
            }
        }
        return nullptr;
    };

    auto sanitizeClipboard = [](std::string s) {
        s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
        s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
        return s;
    };
    auto utf8PrefixByChars = [](const std::string &s, int maxChars) -> std::string {
        // 以“字符数”截断 UTF-8 字符串，避免按字节截断导致乱码/非法 UTF-8
        if (maxChars <= 0)
        {
            return std::string();
        }

        std::size_t i = 0;
        int chars = 0;
        const std::size_t n = s.size();

        while (i < n && chars < maxChars)
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            std::size_t charLen = 1;

            if ((c & 0x80) == 0x00)
            {
                charLen = 1; // ASCII
            }
            else if ((c & 0xE0) == 0xC0)
            {
                charLen = 2; // 2-byte sequence
            }
            else if ((c & 0xF0) == 0xE0)
            {
                charLen = 3; // 3-byte sequence
            }
            else if ((c & 0xF8) == 0xF0)
            {
                charLen = 4; // 4-byte sequence
            }
            else
            {
                // 非法 UTF-8 首字节：停止，避免输出非法序列
                break;
            }

            if (i + charLen > n)
            {
                // 末尾字符不完整：不包含它
                break;
            }

            i += charLen;
            ++chars;
        }

        return s.substr(0, i);
    };

    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = [this, getActiveField, sanitizeClipboard, utf8PrefixByChars](EventKeyboard::KeyCode keyCode, Event *event) {
        if (event)
        {
            // 弹窗显示时吞掉按键，避免穿透到主菜单快捷键
            event->stopPropagation();
        }

        if (keyCode == EventKeyboard::KeyCode::KEY_CTRL || keyCode == EventKeyboard::KeyCode::KEY_RIGHT_CTRL)
        {
            _ctrlDown = true;
            return;
        }
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            onCancelClicked();
            return;
        }

        if (_ctrlDown && (keyCode == EventKeyboard::KeyCode::KEY_V || keyCode == EventKeyboard::KeyCode::KEY_C))
        {
            auto *field = getActiveField();
            if (!field)
            {
                setMessage("请先点击输入框（让光标出现），再使用 Ctrl+V / Ctrl+C。", Color4B(220, 180, 120, 255));
                return;
            }

            if (keyCode == EventKeyboard::KeyCode::KEY_V)
            {
                std::string clip = sanitizeClipboard(ClipboardHelper::getText());
                if (clip.empty())
                {
                    setMessage("剪贴板为空，无法粘贴。", Color4B(220, 120, 120, 255));
                    return;
                }

                if (field->isMaxLengthEnabled())
                {
                    const int remain = field->getMaxLength() - field->getStringLength();
                    if (remain <= 0)
                    {
                        return;
                    }
                    clip = utf8PrefixByChars(clip, remain);
                }

                field->setString(field->getString() + clip);
                field->setCursorPosition((std::size_t)field->getStringLength());
            }
            else
            {
                ClipboardHelper::setText(field->getString());
                setMessage("已复制当前输入框内容到剪贴板。", Color4B(120, 220, 120, 255));
            }
        }
    };
    keyListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event) {
        if (event)
        {
            event->stopPropagation();
        }
        if (keyCode == EventKeyboard::KeyCode::KEY_CTRL || keyCode == EventKeyboard::KeyCode::KEY_RIGHT_CTRL)
        {
            _ctrlDown = false;
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    // 面板
    const float panelW = std::min(700.0f, visibleSize.width * 0.9f);
    const float panelH = std::min(420.0f, visibleSize.height * 0.8f);
    _panel = LayerColor::create(Color4B(35, 35, 35, 230), panelW, panelH);
    _panel->setPosition(Vec2(center.x - panelW * 0.5f, center.y - panelH * 0.5f));
    this->addChild(_panel, 1);

    // 标题
    auto title = Label::createWithTTF("登录 / 注册", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 30);
    title->setPosition(Vec2(panelW * 0.5f, panelH - 48.0f));
    _panel->addChild(title);

    // 提示信息
    _messageLabel = Label::createWithTTF("提示：游客模式将禁用云存功能（用户名3-32位；密码6-64位）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 18);
    _messageLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
    _messageLabel->setPosition(Vec2(panelW * 0.5f, panelH - 88.0f));
    _messageLabel->setTextColor(Color4B(200, 200, 200, 255));
    _panel->addChild(_messageLabel);

    const float leftX = 90.0f;
    const float fieldX = 210.0f;
    const float rowH = 60.0f;
    const float startY = panelH - 150.0f;

    auto addRowLabel = [&](const std::string &text, float y) {
        auto label = Label::createWithTTF(text, "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
        label->setAnchorPoint(Vec2(0.0f, 0.5f));
        label->setPosition(Vec2(leftX, y));
        _panel->addChild(label);
    };

    auto styleTextField = [&](ui::TextField *field, float y, int maxLen) {
        field->setAnchorPoint(Vec2(0.0f, 0.5f));
        field->setPosition(Vec2(fieldX, y));
        field->setTextColor(Color4B(240, 240, 240, 255));
        field->setPlaceHolderColor(Color4B(160, 160, 160, 255));
        field->setMaxLengthEnabled(true);
        field->setMaxLength(maxLen);
        field->setCursorEnabled(true);
        field->setCursorChar('|');
        _panel->addChild(field);

        // 输入框底线
        auto underline = LayerColor::create(Color4B(90, 90, 90, 255), panelW - fieldX - 60.0f, 2.0f);
        underline->setPosition(Vec2(fieldX, y - 18.0f));
        _panel->addChild(underline);
    };

    // URL
    addRowLabel("服务地址", startY);
    // 默认用 127.0.0.1 强制走 IPv4，避免 Windows 下 localhost 解析到 IPv6/其它本机服务导致偶发 404/401
    // 默认端口使用 5174：5173 常被前端工具（例如 Vite）占用，容易产生端口冲突
    _urlField = ui::TextField::create("http://127.0.0.1:5174", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    _urlField->setString("http://127.0.0.1:5174");
    styleTextField(_urlField, startY, 200);

    // 用户名
    addRowLabel("用户名", startY - rowH);
    _userField = ui::TextField::create("3-32位字母/数字/下划线", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    styleTextField(_userField, startY - rowH, 32);

    // 密码
    addRowLabel("密码", startY - rowH * 2);
    _passField = ui::TextField::create("请输入密码", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    _passField->setPasswordEnabled(true);
    _passField->setPasswordStyleText("*");
    styleTextField(_passField, startY - rowH * 2, 64);

    // 按钮
    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    _panel->addChild(menu, 10);

    _loginItem = MenuItemLabel::create(
        Label::createWithTTF("登录", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 26),
        [this](Ref *) { onLoginClicked(); });
    _loginItem->setPosition(Vec2(panelW * 0.32f, 58.0f));
    menu->addChild(_loginItem);

    _registerItem = MenuItemLabel::create(
        Label::createWithTTF("注册", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 26),
        [this](Ref *) { onRegisterClicked(); });
    _registerItem->setPosition(Vec2(panelW * 0.5f, 58.0f));
    menu->addChild(_registerItem);

    _cancelItem = MenuItemLabel::create(
        Label::createWithTTF("取消", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 26),
        [this](Ref *) { onCancelClicked(); });
    _cancelItem->setPosition(Vec2(panelW * 0.68f, 58.0f));
    menu->addChild(_cancelItem);

    setBusy(false);
    return true;
}

void CloudAuthLayer::setBusy(bool busy)
{
    _busy = busy;
    if (_loginItem)
        _loginItem->setEnabled(!busy);
    if (_registerItem)
        _registerItem->setEnabled(!busy);
    if (_cancelItem)
        _cancelItem->setEnabled(!busy);
}

void CloudAuthLayer::setMessage(const std::string &msg, const Color4B &color)
{
    if (!_messageLabel)
    {
        return;
    }
    _messageLabel->setString(msg);
    _messageLabel->setTextColor(color);
}

std::string CloudAuthLayer::getUrl() const
{
    return _urlField ? _urlField->getString() : "";
}

std::string CloudAuthLayer::getUsername() const
{
    return _userField ? _userField->getString() : "";
}

std::string CloudAuthLayer::getPassword() const
{
    return _passField ? _passField->getString() : "";
}

void CloudAuthLayer::onLoginClicked()
{
    startAuthRequest(false);
}

void CloudAuthLayer::onRegisterClicked()
{
    startAuthRequest(true);
}

void CloudAuthLayer::startAuthRequest(bool isRegister)
{
    if (_busy)
    {
        return;
    }

    const std::string url = getUrl();
    const std::string user = getUsername();
    const std::string pass = getPassword();

    setBusy(true);
    setMessage(isRegister ? "正在注册..." : "正在登录...", Color4B(200, 200, 200, 255));

    auto cloud = CloudSyncService::getInstance();
    auto done = [this](bool ok, const std::string &msg) {
        setBusy(false);
        setMessage(msg, ok ? Color4B(120, 220, 120, 255) : Color4B(220, 120, 120, 255));
        if (ok && _doneCallback)
        {
            // 安全性：登录成功后清空密码输入框，避免后续误操作/截图泄露
            if (_passField)
            {
                _passField->setString("");
            }
            _doneCallback(true, msg);
        }
    };

    if (isRegister)
    {
        cloud->registerAndLogin(url, user, pass, done);
    }
    else
    {
        cloud->login(url, user, pass, done);
    }
}

void CloudAuthLayer::onCancelClicked()
{
    if (_busy)
    {
        return;
    }

    if (_doneCallback)
    {
        _doneCallback(false, "已取消");
    }
}
