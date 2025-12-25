/**
 * @file BossHealthBar.cpp
 * @brief Boss血条组件实现
 */

#include "BossHealthBar.h"
#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"

#include <algorithm>

USING_NS_CC;

BossHealthBar *BossHealthBar::create()
{
    BossHealthBar *ret = new (std::nothrow) BossHealthBar();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BossHealthBar::init()
{
    if (!Node::init())
    {
        return false;
    }

    // 创建容器
    _container = Node::create();
    this->addChild(_container);

    // 内容节点：用于受击反馈缩放/位移，避免与 show/hide 的容器动画互相干扰
    _content = Node::create();
    _container->addChild(_content);

    createBackground();
    createHealthBar();
    createBreakBar();
    createNameLabel();

    // 记录受击反馈的基准位置/缩放：后续无论连击触发多快，都必须回弹到这个“初始状态”
    _contentBasePos = _content->getPosition();
    _contentBaseScale = _content->getScale();

    // 初始隐藏
    _container->setVisible(false);
    _isVisible = false;

    return true;
}

void BossHealthBar::createBackground()
{
    _background = DrawNode::create();

    // 取消半透明黑色遮罩背景（避免遮挡游戏画面）
    float bgWidth = _barWidth + 40;
    float bgHeight = _barHeight + 50;

    // 绘制边框
    _background->drawRect(
        Vec2(-bgWidth / 2, -bgHeight / 2),
        Vec2(bgWidth / 2, bgHeight / 2),
        Color4F(0.6f, 0.2f, 0.2f, 1.0f));

    _content->addChild(_background, 0);
}

void BossHealthBar::createHealthBar()
{
    float barY = -5;

    // 血条背景
    _healthBarBg = DrawNode::create();
    _healthBarBg->drawSolidRect(
        Vec2(-_barWidth / 2, -_barHeight / 2),
        Vec2(_barWidth / 2, _barHeight / 2),
        Color4F(0.2f, 0.1f, 0.1f, 1.0f));
    _healthBarBg->setPosition(Vec2(0, barY));
    _content->addChild(_healthBarBg, 1);

    // 伤害延迟条
    _healthBarDamage = DrawNode::create();
    _healthBarDamage->setPosition(Vec2(0, barY));
    _content->addChild(_healthBarDamage, 2);

    // 血条填充
    _healthBarFill = DrawNode::create();
    _healthBarFill->drawSolidRect(
        Vec2(-_barWidth / 2, -_barHeight / 2),
        Vec2(_barWidth / 2, _barHeight / 2),
        Color4F(0.8f, 0.1f, 0.1f, 1.0f));
    _healthBarFill->setPosition(Vec2(0, barY));
    _content->addChild(_healthBarFill, 3);

    // 血条边框
    auto border = DrawNode::create();
    border->drawRect(
        Vec2(-_barWidth / 2, -_barHeight / 2),
        Vec2(_barWidth / 2, _barHeight / 2),
        Color4F(0.8f, 0.6f, 0.2f, 1.0f));
    border->setPosition(Vec2(0, barY));
    _content->addChild(border, 4);

    // HP数值标签
    _hpLabel = Label::createWithTTF("", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _hpLabel->setPosition(Vec2(0, barY));
    _hpLabel->setColor(Color3B::WHITE);
    _content->addChild(_hpLabel, 5);

    // 连击伤害标签：显示在血条右边（容差 1 秒）
    _comboDamageLabel = Label::createWithTTF("", "fonts/ZCOOLKuaiLe-Regular.ttf", 18);
    _comboDamageLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    _comboDamageLabel->setPosition(Vec2(_barWidth * 0.5f + 22.0f, barY));
    _comboDamageLabel->setColor(Color3B(255, 230, 140));
    _comboDamageLabel->enableOutline(Color4B::BLACK, 2);
    _comboDamageLabel->setVisible(false);
    _content->addChild(_comboDamageLabel, 5);
}

void BossHealthBar::createBreakBar()
{
    // 放在 Boss 血条下方：偏小的“击破条”
    const float barY = -5.0f;
    const float gapY = 6.0f;
    const float breakY = barY - (_barHeight * 0.5f) - gapY - (_breakBarHeight * 0.5f);

    _breakBarBg = DrawNode::create();
    _breakBarBg->drawSolidRect(
        Vec2(-_barWidth / 2, -_breakBarHeight / 2),
        Vec2(_barWidth / 2, _breakBarHeight / 2),
        Color4F(0.08f, 0.08f, 0.10f, 1.0f));
    _breakBarBg->setPosition(Vec2(0, breakY));
    _content->addChild(_breakBarBg, 1);

    _breakBarFill = DrawNode::create();
    _breakBarFill->setPosition(Vec2(0, breakY));
    _content->addChild(_breakBarFill, 2);

    _breakBarBorder = DrawNode::create();
    _breakBarBorder->drawRect(
        Vec2(-_barWidth / 2, -_breakBarHeight / 2),
        Vec2(_barWidth / 2, _breakBarHeight / 2),
        Color4F(0.55f, 0.65f, 0.95f, 1.0f));
    _breakBarBorder->setPosition(Vec2(0, breakY));
    _content->addChild(_breakBarBorder, 3);

    // 默认隐藏：只有 boss 支持击破条时才显示
    if (_breakBarBg) _breakBarBg->setVisible(false);
    if (_breakBarFill) _breakBarFill->setVisible(false);
    if (_breakBarBorder) _breakBarBorder->setVisible(false);
}

void BossHealthBar::createNameLabel()
{
    _nameLabel = Label::createWithTTF("BOSS", "fonts/ZCOOLKuaiLe-Regular.ttf", 24);
    _nameLabel->setPosition(Vec2(0, 25));
    _nameLabel->setColor(Color3B(255, 200, 100));
    _nameLabel->enableOutline(Color4B::BLACK, 2);
    _content->addChild(_nameLabel, 5);
}

void BossHealthBar::createPhaseIndicators(int phaseCount)
{
    // 清除旧的阶段指示器
    for (auto indicator : _phaseIndicators)
    {
        indicator->removeFromParent();
    }
    _phaseIndicators.clear();

    if (phaseCount <= 1)
        return;

    _phaseCount = phaseCount;

    // 创建阶段指示器（小圆点）
    float indicatorSize = 10.0f;
    float spacing = 15.0f;
    float totalWidth = (phaseCount - 1) * spacing;
    float startX = -totalWidth / 2;

    for (int i = 0; i < phaseCount; ++i)
    {
        auto indicator = DrawNode::create();

        // 当前阶段用实心圆，其他用空心圆
        if (i < _currentPhase)
        {
            indicator->drawSolidCircle(Vec2::ZERO, indicatorSize / 2, 0, 16, Color4F(1.0f, 0.8f, 0.2f, 1.0f));
        }
        else
        {
            indicator->drawCircle(Vec2::ZERO, indicatorSize / 2, 0, 16, false, Color4F(0.5f, 0.5f, 0.5f, 1.0f));
        }

        indicator->setPosition(Vec2(startX + i * spacing, -_barHeight / 2 - 15));
        _content->addChild(indicator, 5);
        _phaseIndicators.push_back(indicator);
    }
}

void BossHealthBar::bindBoss(CharacterBase *boss, const std::string &bossName, int phaseCount)
{
    _boss = boss;
    _bossName = bossName;
    _phaseCount = phaseCount;
    _currentPhase = 1;

    if (_nameLabel)
    {
        _nameLabel->setString(bossName);
    }

    if (_boss)
    {
        // 清空 Boss 缓存的“非 DOT 伤害”，避免刚绑定就触发连击/受击反馈
        _boss->consumePendingUiNonDotDamage();

        auto attr = _boss->getAttributeComponent();
        if (attr)
        {
            _lastHP = attr->getAttributeValue(AttributeType::MAX_HP);
            _damageBarHP = _lastHP;
        }
    }

    // 重置连击统计
    _comboDamageSum = 0.0;
    _comboWindowRemaining = 0.0f;
    _comboLastUpdateMs = 0;
    if (_comboDamageLabel)
    {
        _comboDamageLabel->setString("");
        _comboDamageLabel->setVisible(false);
    }

    createPhaseIndicators(phaseCount);
    updateDisplay();
}

void BossHealthBar::unbindBoss()
{
    _boss = nullptr;
    _comboDamageSum = 0.0;
    _comboWindowRemaining = 0.0f;
    _comboLastUpdateMs = 0;
    if (_comboDamageLabel)
    {
        _comboDamageLabel->setString("");
        _comboDamageLabel->setVisible(false);
    }
    hide();
}

void BossHealthBar::updateDisplay()
{
    if (!_boss)
        return;

    auto attr = _boss->getAttributeComponent();
    if (!attr)
        return;

    float maxHP = attr->getAttributeValue(AttributeType::MAX_HP);
    float currentHP = std::max(0.0f, _boss->getCurrentHP());

    // 连击窗口倒计时（容差 1 秒）
    const long long nowMs = utils::getTimeInMilliseconds();
    float dt = 0.0f;
    if (_comboLastUpdateMs > 0)
    {
        dt = static_cast<float>(nowMs - _comboLastUpdateMs) / 1000.0f;
        if (dt < 0.0f)
        {
            dt = 0.0f;
        }
    }
    _comboLastUpdateMs = nowMs;

    if (_comboWindowRemaining > 0.0f)
    {
        _comboWindowRemaining = std::max(0.0f, _comboWindowRemaining - std::max(0.0f, dt));
        if (_comboWindowRemaining <= 0.0f)
        {
            _comboDamageSum = 0.0;
            if (_comboDamageLabel)
            {
                _comboDamageLabel->setString("");
                _comboDamageLabel->setVisible(false);
            }
        }
    }

    // 仅在“非 DOT 伤害”时触发：连击累计 + 受击反馈动画
    // DOT 统一要求 causesHitStun=false，因此不会被 CharacterBase::consumePendingUiNonDotDamage 记录。
    const float nonDotDamage = std::max(0.0f, _boss->consumePendingUiNonDotDamage());
    if (nonDotDamage > 0.0f)
    {
        if (_comboWindowRemaining > 0.0f)
        {
            _comboDamageSum += nonDotDamage;
        }
        else
        {
            _comboDamageSum = nonDotDamage;
        }
        _comboWindowRemaining = COMBO_WINDOW_SECONDS;

        if (_comboDamageLabel)
        {
            _comboDamageLabel->setString(StringUtils::format("连击伤害 %.0f", _comboDamageSum));
            _comboDamageLabel->setVisible(true);
        }

        playHitAnimation();
    }

    // 检测HP变化
    if (currentHP > _lastHP)
    {
        // 恢复，直接更新
        _damageBarHP = currentHP;
    }
    _lastHP = currentHP;

    // 更新伤害延迟条
    if (_damageBarHP > currentHP)
    {
        _damageBarHP -= (maxHP * 0.02f); // 每帧减少2%最大血量
        if (_damageBarHP < currentHP)
        {
            _damageBarHP = currentHP;
        }
    }

    updateHealthBar(currentHP, maxHP);

    // 更新击破条（Boss 机制）：默认不显示，只有 max>0 时才渲染
    const int breakMax = _boss->getBreakMax();
    const int breakCurrent = _boss->getBreakMeter();
    updateBreakBar(breakCurrent, breakMax);
}

void BossHealthBar::updateHealthBar(float current, float max)
{
    float percent = max > 0 ? current / max : 0;
    float damagePercent = max > 0 ? _damageBarHP / max : 0;

    // 更新血条填充
    _healthBarFill->clear();
    if (percent > 0)
    {
        // Boss血条使用渐变红色
        Color4F hpColor;
        if (percent > 0.5f)
        {
            hpColor = Color4F(0.8f, 0.2f, 0.2f, 1.0f); // 深红
        }
        else if (percent > 0.25f)
        {
            hpColor = Color4F(0.9f, 0.4f, 0.1f, 1.0f); // 橙红
        }
        else
        {
            hpColor = Color4F(1.0f, 0.1f, 0.1f, 1.0f); // 亮红（危险）
        }

        float fillWidth = _barWidth * percent;
        _healthBarFill->drawSolidRect(
            Vec2(-_barWidth / 2, -_barHeight / 2),
            Vec2(-_barWidth / 2 + fillWidth, _barHeight / 2),
            hpColor);
    }

    // 更新伤害延迟条
    _healthBarDamage->clear();
    if (damagePercent > percent)
    {
        float currentWidth = _barWidth * percent;
        float damageWidth = _barWidth * damagePercent;
        _healthBarDamage->drawSolidRect(
            Vec2(-_barWidth / 2 + currentWidth, -_barHeight / 2),
            Vec2(-_barWidth / 2 + damageWidth, _barHeight / 2),
            Color4F(1.0f, 1.0f, 1.0f, 0.5f));
    }

    // 更新HP标签
    if (_hpLabel)
    {
        _hpLabel->setString(StringUtils::format("%.0f / %.0f", current, max));
    }
}

void BossHealthBar::updateBreakBar(int current, int max)
{
    const bool shouldShow = (max > 0);
    if (_breakBarBg) _breakBarBg->setVisible(shouldShow);
    if (_breakBarFill) _breakBarFill->setVisible(shouldShow);
    if (_breakBarBorder) _breakBarBorder->setVisible(shouldShow);

    if (!_breakBarFill || !shouldShow)
    {
        return;
    }

    const int clampedMax = std::max(1, max);
    const int clampedCur = std::max(0, std::min(current, clampedMax));
    const float percent = static_cast<float>(clampedCur) / static_cast<float>(clampedMax);

    _breakBarFill->clear();
    if (percent <= 0.0f)
    {
        return;
    }

    // 击破条颜色：偏蓝紫（与血条红色区分）
    const Color4F fillColor(0.30f, 0.55f, 1.00f, 1.0f);
    const float fillWidth = _barWidth * percent;
    _breakBarFill->drawSolidRect(
        Vec2(-_barWidth / 2, -_breakBarHeight / 2),
        Vec2(-_barWidth / 2 + fillWidth, _breakBarHeight / 2),
        fillColor);
}

void BossHealthBar::setCurrentPhase(int phase)
{
    if (phase == _currentPhase || phase < 1 || phase > _phaseCount)
        return;

    _currentPhase = phase;

    // 更新阶段指示器
    for (int i = 0; i < static_cast<int>(_phaseIndicators.size()); ++i)
    {
        auto indicator = _phaseIndicators[i];
        indicator->clear();

        if (i < _currentPhase)
        {
            indicator->drawSolidCircle(Vec2::ZERO, 5, 0, 16, Color4F(1.0f, 0.8f, 0.2f, 1.0f));
        }
        else
        {
            indicator->drawCircle(Vec2::ZERO, 5, 0, 16, false, Color4F(0.5f, 0.5f, 0.5f, 1.0f));
        }
    }

    playPhaseTransitionAnimation();
}

void BossHealthBar::show()
{
    if (_isVisible)
        return;

    _isVisible = true;
    _container->setVisible(true);

    // 从上方滑入动画
    _container->setPositionY(100);
    _container->setOpacity(0);

    auto moveDown = MoveTo::create(0.5f, Vec2(0, 0));
    auto fadeIn = FadeIn::create(0.3f);
    auto easeMove = EaseBackOut::create(moveDown);

    _container->runAction(Spawn::create(easeMove, fadeIn, nullptr));
}

void BossHealthBar::hide()
{
    if (!_isVisible)
        return;

    _isVisible = false;

    // 向上滑出动画
    auto moveUp = MoveBy::create(0.3f, Vec2(0, 100));
    auto fadeOut = FadeOut::create(0.3f);
    auto callback = CallFunc::create([this]()
                                     { _container->setVisible(false); });

    _container->runAction(Sequence::create(
        Spawn::create(moveUp, fadeOut, nullptr),
        callback,
        nullptr));
}

void BossHealthBar::playHitAnimation()
{
    if (!_content)
    {
        return;
    }

    // 受击反馈：向上渐进变大 -> 往下渐进变小 -> 恢复原样
    // 说明：只在“非 DOT 伤害”时触发（由 updateDisplay 中的 nonDotDamage 控制）
    static constexpr int HIT_ANIM_TAG = 9001;
    static constexpr float UP_SECONDS = 0.08f;
    static constexpr float DOWN_SECONDS = 0.12f;
    static constexpr float MOVE_Y = 10.0f;
    static constexpr float SCALE_UP = 1.06f;

    _content->stopActionByTag(HIT_ANIM_TAG);

    const Vec2 basePos = _contentBasePos;
    const float baseScale = _contentBaseScale;

    auto moveUp = EaseSineOut::create(MoveTo::create(UP_SECONDS, basePos + Vec2(0.0f, MOVE_Y)));
    auto scaleUp = EaseSineOut::create(ScaleTo::create(UP_SECONDS, baseScale * SCALE_UP));
    auto up = Spawn::create(moveUp, scaleUp, nullptr);

    auto moveDown = EaseSineIn::create(MoveTo::create(DOWN_SECONDS, basePos));
    auto scaleDown = EaseSineIn::create(ScaleTo::create(DOWN_SECONDS, baseScale));
    auto down = Spawn::create(moveDown, scaleDown, nullptr);

    auto seq = Sequence::create(up, down, nullptr);
    seq->setTag(HIT_ANIM_TAG);
    _content->runAction(seq);
}

void BossHealthBar::playPhaseTransitionAnimation()
{
    // 阶段转换时的特效
    // 血条闪烁
    auto blink1 = FadeTo::create(0.1f, 100);
    auto blink2 = FadeTo::create(0.1f, 255);
    auto blink3 = FadeTo::create(0.1f, 100);
    auto blink4 = FadeTo::create(0.1f, 255);

    _healthBarFill->runAction(Sequence::create(blink1, blink2, blink3, blink4, nullptr));

    // 名称标签放大缩小
    if (_nameLabel)
    {
        auto scaleUp = ScaleTo::create(0.2f, 1.3f);
        auto scaleDown = ScaleTo::create(0.2f, 1.0f);
        _nameLabel->runAction(Sequence::create(scaleUp, scaleDown, nullptr));
    }
}
