#include "UI/BlessingNpcLayer.h"
#include "AI/AiBlessingService.h"
#include "Character/Player/PlayerCharacter.h"

#include <cmath>

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
    auto keyListener = EventListenerKeyboard::create();
    keyListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event *) {
        if (!_showing)
        {
            return;
        }
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            onCloseClicked();
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);

    const float panelW = std::min(920.0f, visibleSize.width * 0.92f);
    const float panelH = std::min(520.0f, visibleSize.height * 0.86f);
    _panel = LayerColor::create(Color4B(35, 35, 35, 235), panelW, panelH);
    _panel->setPosition(Vec2(center.x - panelW * 0.5f, center.y - panelH * 0.5f));
    this->addChild(_panel, 1);

    // 标题
    auto title = Label::createWithTTF("赐福 NPC（展示接口）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 30);
    title->setPosition(Vec2(panelW * 0.5f, panelH - 42.0f));
    _panel->addChild(title);

    // 提示信息
    _messageLabel = Label::createWithTTF("提示：在此填写 baseUrl + apiKey，然后点击“请求赐福”（会覆盖旧赐福）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 18);
    _messageLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
    _messageLabel->setPosition(Vec2(panelW * 0.5f, panelH - 82.0f));
    _messageLabel->setTextColor(Color4B(200, 200, 200, 255));
    _panel->addChild(_messageLabel);

    const float leftX = 70.0f;
    const float fieldX = 230.0f;
    const float rowH = 56.0f;
    const float startY = panelH - 140.0f;

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
    _urlField = ui::TextField::create("例如 http://127.0.0.1:8000", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    _urlField->setString("http://127.0.0.1:8000");
    styleTextField(_urlField, startY, 220);

    // apiKey
    addRowLabel("apiKey", startY - rowH);
    _apiKeyField = ui::TextField::create("OpenAI Bearer Token（展示阶段手填）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    styleTextField(_apiKeyField, startY - rowH, 240, true);

    // model
    addRowLabel("model", startY - rowH * 2);
    _modelField = ui::TextField::create("可选（默认 gpt-4o-mini）", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    _modelField->setString("");
    styleTextField(_modelField, startY - rowH * 2, 80);

    // prompt
    addRowLabel("对话", startY - rowH * 3);
    _promptField = ui::TextField::create("例如：请赐予我新的赐福", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 22);
    _promptField->setString("请赐予我新的赐福（会覆盖旧赐福）。");
    styleTextField(_promptField, startY - rowH * 3, 200);

    // 按钮
    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    _panel->addChild(menu, 10);

    _saveItem = MenuItemLabel::create(
        Label::createWithTTF("保存配置", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
        [this](Ref *) { onSaveConfigClicked(); });
    _saveItem->setPosition(Vec2(panelW * 0.25f, 58.0f));
    menu->addChild(_saveItem);

    _requestItem = MenuItemLabel::create(
        Label::createWithTTF("请求赐福", "fonts/NotoSansSC/NotoSansSC-Regular.ttf", 24),
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
    setVisible(true);
    setBusy(false);

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
        _requestItem->setEnabled(!busy);
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

    setMessage("AI 配置已保存（仅本次运行有效）", Color4B(120, 220, 120, 255));
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

    std::string hint;
    if (!AiBlessingService::getInstance()->isConfigured(&hint))
    {
        setMessage(hint, Color4B(220, 120, 120, 255));
        return;
    }

    setBusy(true);
    setMessage("正在请求赐福...", Color4B(200, 200, 200, 255));

    const std::string userPrompt = getUserPrompt();

    // 防御性：请求期间保持节点存活，避免场景切换导致回调访问已释放对象
    this->retain();

    AiBlessingService::getInstance()->requestBlessing(userPrompt,
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
                                                          std::string msg = npcText;
                                                          if (!summary.empty())
                                                          {
                                                              msg += "\n赐福：" + summary + "\n（已覆盖旧赐福）";
                                                          }
                                                          setMessage(msg, Color4B(120, 220, 120, 255));
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
    setMessage("已清空赐福", Color4B(200, 200, 200, 255));
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
