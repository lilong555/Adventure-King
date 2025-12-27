#include "UI/BlessingNpcLayer.h"
#include "AI/AiBlessingService.h"
#include "Character/Player/PlayerCharacter.h"
#include "Utils/ClipboardHelper.h"
#include "Utils/ImeHelper.h"

#include <cmath>
#include <algorithm>

USING_NS_CC;

BlessingNpcLayer *BlessingNpcLayer::create()
{
    auto ret = new (std::nothrow) BlessingNpcLayer();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BlessingNpcLayer::init()
{
    if (!LayerColor::initWithColor(Color4B(0, 0, 0, 160)))
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    const Vec2 center(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f);

    // 吞噬触摸，避免穿透到游戏层
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this](Touch *, Event *) {
        return _showing; // 仅显示时拦截触摸，避免隐藏状态误吞输入
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    // ESC 关闭
    auto getActiveField = [this]() -> ui::TextField * {
        ui::TextField *fields[] = {_urlField, _apiKeyField, _modelField, _promptField};
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
        // 常见场景：从浏览器/终端复制时带换行，直接粘贴会导致请求失败
        s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
        s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
        return s;
    };

    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = [this, getActiveField, sanitizeClipboard](EventKeyboard::KeyCode keyCode, Event *event) {
        if (!_showing)
        {
            return;
        }
        if (event)
        {
            // 弹窗显示时吞掉按键，避免穿透到游戏输入
            event->stopPropagation();
        }

        if (keyCode == EventKeyboard::KeyCode::KEY_CTRL || keyCode == EventKeyboard::KeyCode::KEY_RIGHT_CTRL)
        {
            _ctrlDown = true;
            return;
        }
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            onCloseClicked();
            return;
        }

        // Ctrl+V / Ctrl+C：解决 ui::TextField 默认不支持复制粘贴的问题
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
                    if ((int)clip.size() > remain)
                    {
                        clip = clip.substr(0, (size_t)remain);
                    }
                }

                field->setString(field->getString() + clip);
                field->setCursorPosition((std::size_t)field->getStringLength());
            }
            else // KEY_C
            {
                ClipboardHelper::setText(field->getString());
                setMessage("已复制当前输入框内容到剪贴板。", Color4B(120, 220, 120, 255));
            }
        }
    };
    keyListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event) {
        if (!_showing)
        {
            return;
        }
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

    const float panelW = std::min(920.0f, visibleSize.width * 0.92f);
    const float panelH = std::min(640.0f, visibleSize.height * 0.90f);
    _panel = LayerColor::create(Color4B(35, 35, 35, 235), panelW, panelH);
    // 面板整体靠右，为左侧“步骤说明”留空间（避免挤在面板内）
    _panel->setPosition(Vec2(origin.x + visibleSize.width - panelW - 20.0f, center.y - panelH * 0.5f));
    this->addChild(_panel, 1);

    // 左侧步骤说明（屏幕左边，不堆在面板里）
    const float guideRightEdge = _panel->getPositionX() - 18.0f;
    const float guideW = std::min(360.0f, std::max(240.0f, guideRightEdge - origin.x - 20.0f));
    _guideLabel = Label::createWithTTF(
        "步骤1：填写 baseUrl / apiKey\n"
        "步骤2：点击【确认】生成问题\n"
        "步骤3：在对话框输入回答\n"
        "步骤4：点击【提交回答】获得赐福（覆盖旧赐福）\n"
        "提示：本页面已恢复中文输入法，可直接输入中文",
        "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
        20);
    _guideLabel->setAnchorPoint(Vec2(0.0f, 1.0f));
    _guideLabel->setPosition(Vec2(origin.x + 20.0f, origin.y + visibleSize.height - 22.0f));
    _guideLabel->setTextColor(Color4B(220, 220, 220, 255));
    _guideLabel->setDimensions(guideW, visibleSize.height * 0.55f);
    _guideLabel->setHorizontalAlignment(TextHAlignment::LEFT);
    _guideLabel->setVerticalAlignment(TextVAlignment::TOP);
    this->addChild(_guideLabel, 2);

    // 标题
    auto title = Label::createWithTTF("赐福 NPC（展示接口）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 30);
    title->setPosition(Vec2(panelW * 0.5f, panelH - 42.0f));
    _panel->addChild(title);

    // 提示信息
    _messageLabel = Label::createWithTTF(
        "提示：输入框支持 Ctrl+V 粘贴 / Ctrl+C 复制",
        "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
        18);
    _messageLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
    _messageLabel->setPosition(Vec2(panelW * 0.5f, panelH - 82.0f));
    _messageLabel->setTextColor(Color4B(200, 200, 200, 255));
    _panel->addChild(_messageLabel);

    const float leftX = 70.0f;
    const float fieldX = 230.0f;
    const float rowH = 56.0f;
    const float startY = panelH - 160.0f;

    auto addRowLabel = [&](const std::string &text, float y) {
        auto label = Label::createWithTTF(text, "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
        label->setAnchorPoint(Vec2(0.0f, 0.5f));
        label->setPosition(Vec2(leftX, y));
        _panel->addChild(label);
    };

    auto styleTextField = [&](ui::TextField *field, float y, int maxLen, bool password = false) {
        field->setAnchorPoint(Vec2(0.0f, 0.5f));
        field->setPosition(Vec2(fieldX, y));
        field->setTextColor(Color4B(240, 240, 240, 255));
        field->setPlaceHolderColor(Color4B(160, 160, 160, 255));
        field->setMaxLengthEnabled(true);
        field->setMaxLength(maxLen);
        field->setCursorEnabled(true);
        field->setCursorChar('|');
        if (password)
        {
            field->setPasswordEnabled(true);
            field->setPasswordStyleText("*");
        }
        _panel->addChild(field);

        auto underline = LayerColor::create(Color4B(90, 90, 90, 255), panelW - fieldX - 50.0f, 2.0f);
        underline->setPosition(Vec2(fieldX, y - 18.0f));
        _panel->addChild(underline);
    };

    // baseUrl
    addRowLabel("baseUrl", startY);
    // 公共服务默认URL：https://elysiver.h-e.top（来源：https://linux.do/t/topic/1175087/27）
    _urlField = ui::TextField::create("例如 https://elysiver.h-e.top", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    _urlField->setString("https://elysiver.h-e.top");
    styleTextField(_urlField, startY, 220);

    // apiKey
    addRowLabel("apiKey", startY - rowH);
    _apiKeyField = ui::TextField::create("OpenAI Bearer Token（展示阶段手填）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    styleTextField(_apiKeyField, startY - rowH, 240, true);

    // model
    addRowLabel("model", startY - rowH * 2);
    _modelField = ui::TextField::create("可选（默认 gemini-3-flash-preview）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    _modelField->setString("");
    styleTextField(_modelField, startY - rowH * 2, 80);

    // 对话框：用于展示 NPC 问题与玩家回答（确认后才需要输入回答）
    const float dialogW = panelW - 100.0f;
    const float dialogH = 240.0f;
    _dialogPanel = LayerColor::create(Color4B(20, 20, 20, 220), dialogW, dialogH);
    _dialogPanel->setPosition(Vec2((panelW - dialogW) * 0.5f, 118.0f));
    _panel->addChild(_dialogPanel, 2);

    auto dialogTitle = Label::createWithTTF("对话框", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    dialogTitle->setAnchorPoint(Vec2(0.0f, 1.0f));
    dialogTitle->setPosition(Vec2(18.0f, dialogH - 14.0f));
    dialogTitle->setTextColor(Color4B(220, 220, 220, 255));
    _dialogPanel->addChild(dialogTitle);

    _dialogNpcLabel = Label::createWithTTF(
        "点击【确认】后，NPC 会提出考验冒险决心的问题。",
        "fonts/NotoSansSC/NotoSansSC-Regular.ttf",
        18);
    _dialogNpcLabel->setAnchorPoint(Vec2(0.0f, 1.0f));
    _dialogNpcLabel->setPosition(Vec2(18.0f, dialogH - 46.0f));
    _dialogNpcLabel->setTextColor(Color4B(200, 200, 200, 255));
    _dialogNpcLabel->setDimensions(dialogW - 36.0f, dialogH - 110.0f);
    _dialogNpcLabel->setHorizontalAlignment(TextHAlignment::LEFT);
    _dialogNpcLabel->setVerticalAlignment(TextVAlignment::TOP);
    _dialogPanel->addChild(_dialogNpcLabel);

    auto answerLabel = Label::createWithTTF("你的回答：", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 20);
    answerLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    answerLabel->setPosition(Vec2(18.0f, 38.0f));
    _dialogPanel->addChild(answerLabel);

    _promptField = ui::TextField::create("先点击【确认】生成问题", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 20);
    _promptField->setAnchorPoint(Vec2(0.0f, 0.5f));
    _promptField->setPosition(Vec2(118.0f, 38.0f));
    _promptField->setTextColor(Color4B(240, 240, 240, 255));
    _promptField->setPlaceHolderColor(Color4B(160, 160, 160, 255));
    _promptField->setMaxLengthEnabled(true);
    _promptField->setMaxLength(400);
    _promptField->setCursorEnabled(true);
    _promptField->setCursorChar('|');
    _dialogPanel->addChild(_promptField);

    auto answerUnderline = LayerColor::create(Color4B(90, 90, 90, 255), dialogW - 118.0f - 18.0f, 2.0f);
    answerUnderline->setPosition(Vec2(118.0f, 20.0f));
    _dialogPanel->addChild(answerUnderline);

    // 按钮
    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    _panel->addChild(menu, 10);

    _saveItem = MenuItemLabel::create(
        Label::createWithTTF("确认", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [this](Ref *) { onSaveConfigClicked(); });
    _saveItem->setPosition(Vec2(panelW * 0.25f, 58.0f));
    menu->addChild(_saveItem);

    _requestItem = MenuItemLabel::create(
        Label::createWithTTF("提交回答", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [this](Ref *) { onRequestBlessingClicked(); });
    _requestItem->setPosition(Vec2(panelW * 0.45f, 58.0f));
    menu->addChild(_requestItem);

    _clearItem = MenuItemLabel::create(
        Label::createWithTTF("清空赐福", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [this](Ref *) { onClearBlessingClicked(); });
    _clearItem->setPosition(Vec2(panelW * 0.65f, 58.0f));
    menu->addChild(_clearItem);

    _closeItem = MenuItemLabel::create(
        Label::createWithTTF("关闭", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [this](Ref *) { onCloseClicked(); });
    _closeItem->setPosition(Vec2(panelW * 0.83f, 58.0f));
    menu->addChild(_closeItem);

    setBusy(false);
    this->setVisible(false);
    _showing = false;

    // 如果已有运行时配置，回填到输入框（便于反复测试）
    auto cfg = AiBlessingService::getInstance()->getRuntimeConfig();
    if (!cfg.baseUrl.empty() && _urlField)
    {
        _urlField->setString(cfg.baseUrl);
    }
    if (!cfg.apiKey.empty() && _apiKeyField)
    {
        _apiKeyField->setString(cfg.apiKey);
    }
    if (!cfg.model.empty() && _modelField)
    {
        _modelField->setString(cfg.model);
    }

    return true;
}

void BlessingNpcLayer::onExit()
{
    // 兜底：若弹窗仍处于“恢复 IME”状态就被销毁，先把计数补回，避免 GameScene::onExit 再 pop 导致不匹配
    if (_imeRestored)
    {
        ImeHelper::pushDisableIme();
        _imeRestored = false;
    }
    LayerColor::onExit();
}

void BlessingNpcLayer::bindPlayer(PlayerCharacter *player)
{
    _player = player;
}

void BlessingNpcLayer::show()
{
    if (_showing)
    {
        return;
    }
    _showing = true;

    // 进入赐福页面时恢复输入法（允许中文输入）
    if (!_imeRestored)
    {
        ImeHelper::popDisableIme();
        _imeRestored = true;
    }

    setVisible(true);
    setBusy(false);
    _waitingForAnswer = false;
    _cachedQuestions.clear();
    if (_requestItem)
        _requestItem->setEnabled(false);
    if (_promptField)
    {
        _promptField->setPlaceHolder("先点击【确认】生成问题");
        _promptField->setString("");
    }
    if (_dialogNpcLabel)
    {
        _dialogNpcLabel->setString("点击【确认】后，NPC 会提出考验冒险决心的问题。");
    }

    // 淡入
    _panel->setOpacity(0);
    _panel->runAction(FadeIn::create(0.15f));
}

void BlessingNpcLayer::hide()
{
    if (!_showing)
    {
        return;
    }
    _showing = false;

    // 退出赐福页面时重新禁用输入法，避免抢按键
    if (_imeRestored)
    {
        ImeHelper::pushDisableIme();
        _imeRestored = false;
    }

    auto fadeOut = FadeOut::create(0.15f);
    auto cb = CallFunc::create([this]() {
        this->setVisible(false);
    });
    _panel->runAction(Sequence::create(fadeOut, cb, nullptr));
}

void BlessingNpcLayer::setBusy(bool busy)
{
    _busy = busy;
    if (_saveItem)
        _saveItem->setEnabled(!busy);
    if (_requestItem)
        _requestItem->setEnabled(!busy && _waitingForAnswer);
    if (_clearItem)
        _clearItem->setEnabled(!busy);
    if (_closeItem)
        _closeItem->setEnabled(!busy);
}

void BlessingNpcLayer::setMessage(const std::string &msg, const Color4B &color)
{
    if (!_messageLabel)
    {
        return;
    }
    _messageLabel->setString(msg);
    _messageLabel->setTextColor(color);
}

std::string BlessingNpcLayer::getBaseUrl() const
{
    return _urlField ? _urlField->getString() : "";
}

std::string BlessingNpcLayer::getApiKey() const
{
    return _apiKeyField ? _apiKeyField->getString() : "";
}

std::string BlessingNpcLayer::getModel() const
{
    return _modelField ? _modelField->getString() : "";
}

std::string BlessingNpcLayer::getUserPrompt() const
{
    return _promptField ? _promptField->getString() : "";
}

void BlessingNpcLayer::onSaveConfigClicked()
{
    if (_busy)
    {
        return;
    }
    if (!_player)
    {
        setMessage("确认失败：玩家不存在", Color4B(220, 120, 120, 255));
        return;
    }

    AiBlessingService::Config cfg;
    cfg.baseUrl = getBaseUrl();
    cfg.apiKey = getApiKey();
    cfg.model = getModel();

    AiBlessingService::getInstance()->setRuntimeConfig(cfg);

    std::string hint;
    if (!AiBlessingService::getInstance()->isConfigured(&hint))
    {
        setMessage(hint, Color4B(220, 120, 120, 255));
        return;
    }

    // 确认后立刻生成问题（用户只需回答一次即可拿到赐福）
    _waitingForAnswer = false;
    _cachedQuestions.clear();
    if (_requestItem)
        _requestItem->setEnabled(false);

    const std::string userPrompt = "";
    setBusy(true);
    setMessage("赐福考验开始：正在生成问题...", Color4B(200, 200, 200, 255));
    if (_dialogNpcLabel)
    {
        _dialogNpcLabel->setString("正在生成问题...");
    }

    // 防御性：请求期间保持节点存活，避免场景切换导致回调访问已释放对象
    this->retain();

    AiBlessingService::getInstance()->requestChallengeQuestions(
        userPrompt,
        [this](bool ok, const std::string &npcQuestions, const std::string &err) {
            if (!this->getParent())
            {
                this->release();
                return;
            }

            if (!ok)
            {
                setBusy(false);
                setMessage(err.empty() ? "生成问题失败" : err, Color4B(220, 120, 120, 255));
                this->release();
                return;
            }

            _cachedQuestions = npcQuestions;
            _waitingForAnswer = true;

            if (_promptField)
            {
                _promptField->setPlaceHolder("请在此输入你的回答（只需一次）");
                _promptField->setString("");
            }

            if (_requestItem)
                _requestItem->setEnabled(true);

            if (_dialogNpcLabel)
            {
                _dialogNpcLabel->setString("赐福考验：\n" + npcQuestions);
            }
            setMessage("问题已生成：请在对话框输入回答，然后点击【提交回答】。", Color4B(200, 200, 200, 255));
            setBusy(false);
            this->release();
        });
}

void BlessingNpcLayer::onRequestBlessingClicked()
{
    if (_busy)
    {
        return;
    }
    if (!_player)
    {
        setMessage("请求失败：玩家不存在", Color4B(220, 120, 120, 255));
        return;
    }

    if (!_waitingForAnswer)
    {
        setMessage("请先点击【确认】生成考验问题。", Color4B(220, 180, 120, 255));
        return;
    }

    // 每次点击都以输入框为准更新运行时配置（允许玩家在回答前微调 baseUrl/model）
    AiBlessingService::Config cfg;
    cfg.baseUrl = getBaseUrl();
    cfg.apiKey = getApiKey();
    cfg.model = getModel();
    AiBlessingService::getInstance()->setRuntimeConfig(cfg);

    std::string hint;
    if (!AiBlessingService::getInstance()->isConfigured(&hint))
    {
        setMessage(hint, Color4B(220, 120, 120, 255));
        return;
    }

    const std::string answer = getUserPrompt();
    if (answer.empty())
    {
        setMessage("请先在对话框输入你的回答。", Color4B(220, 180, 120, 255));
        return;
    }

    setBusy(true);
    setMessage("正在分析你的回答并赐福...", Color4B(200, 200, 200, 255));

    // 防御性：请求期间保持节点存活，避免场景切换导致回调访问已释放对象
    this->retain();

    AiBlessingService::getInstance()->requestBlessingFromDialogue(
        _cachedQuestions,
        answer,
        [this](bool ok, const std::string &npcText, const Attributes &bonus, const std::string &err) {
            if (!this->getParent())
            {
                this->release();
                return;
            }

            if (!ok)
            {
                setBusy(false);
                setMessage(err.empty() ? "赐福失败" : err, Color4B(220, 120, 120, 255));
                this->release();
                return;
            }

            if (_player)
            {
                _player->applyAiBlessingBonus(bonus);
            }

            std::string summary = buildBlessingSummary(bonus);
            if (_dialogNpcLabel)
            {
                std::string msg = npcText;
                if (!summary.empty())
                {
                    msg += "\n赐福：" + summary + "\n（已覆盖旧赐福）";
                }
                _dialogNpcLabel->setString(msg);
            }
            setMessage("赐福完成（已覆盖旧赐福）", Color4B(120, 220, 120, 255));

            // 一次对话完成后回到初始状态，允许再次发起新的考验（再次点击【确认】）
            _waitingForAnswer = false;
            _cachedQuestions.clear();
            if (_requestItem)
                _requestItem->setEnabled(false);
            if (_promptField)
            {
                _promptField->setPlaceHolder("先点击【确认】生成问题");
                _promptField->setString("");
            }

            setBusy(false);
            this->release();
        });
}

void BlessingNpcLayer::onClearBlessingClicked()
{
    if (_busy)
    {
        return;
    }
    if (!_player)
    {
        setMessage("清空失败：玩家不存在", Color4B(220, 120, 120, 255));
        return;
    }
    _player->clearAiBlessingBonus();
    _waitingForAnswer = false;
    _cachedQuestions.clear();
    if (_requestItem)
        _requestItem->setEnabled(false);
    if (_promptField)
    {
        _promptField->setPlaceHolder("先点击【确认】生成问题");
        _promptField->setString("");
    }
    if (_dialogNpcLabel)
    {
        _dialogNpcLabel->setString("已清空赐福。点击【确认】开始新的考验。");
    }
    setMessage("已清空赐福（对话已重置）", Color4B(200, 200, 200, 255));
}

void BlessingNpcLayer::onCloseClicked()
{
    if (_busy)
    {
        return;
    }
    hide();
    if (_closeCallback)
    {
        _closeCallback();
    }
}

std::string BlessingNpcLayer::buildBlessingSummary(const Attributes &bonus) const
{
    if (bonus.values.empty())
    {
        return "";
    }

    auto nameOf = [](AttributeType type) -> const char * {
        switch (type)
        {
        case AttributeType::MAX_HP:
            return "生命";
        case AttributeType::MAX_MP:
            return "法力";
        case AttributeType::STRENGTH:
            return "力量";
        case AttributeType::DEFENSE:
            return "防御";
        case AttributeType::MOVE_SPEED:
            return "速度";
        case AttributeType::CRITICAL_RATE:
            return "暴击率";
        case AttributeType::ATTACKINTERVAL:
            return "攻速";
        default:
            return "属性";
        }
    };

    std::string out;
    for (std::map<AttributeType, float>::const_iterator it = bonus.values.begin(); it != bonus.values.end(); ++it)
    {
        if (!out.empty())
        {
            out += "  ";
        }
        float v = it->second;
        if (it->first == AttributeType::CRITICAL_RATE)
        {
            // 0.05 -> 5%
            int pct = static_cast<int>(std::round(v * 100.0f));
            out += StringUtils::format("%s+%d%%", nameOf(it->first), pct);
        }
        else
        {
            out += StringUtils::format("%s+%.0f", nameOf(it->first), v);
        }
    }
    return out;
}
