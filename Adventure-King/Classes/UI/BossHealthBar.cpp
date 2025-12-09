/**
 * @file BossHealthBar.cpp
 * @brief Boss血条组件实现
 */

#include "BossHealthBar.h"
#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"

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

    createBackground();
    createHealthBar();
    createNameLabel();

    // 初始隐藏
    _container->setVisible(false);
    _isVisible = false;

    return true;
}

void BossHealthBar::createBackground()
{
    _background = DrawNode::create();

    // 绘制半透明黑色背景
    float bgWidth = _barWidth + 40;
    float bgHeight = _barHeight + 50;

    _background->drawSolidRect(
        Vec2(-bgWidth / 2, -bgHeight / 2),
        Vec2(bgWidth / 2, bgHeight / 2),
        Color4F(0.0f, 0.0f, 0.0f, 0.7f));

    // 绘制边框
    _background->drawRect(
        Vec2(-bgWidth / 2, -bgHeight / 2),
        Vec2(bgWidth / 2, bgHeight / 2),
        Color4F(0.6f, 0.2f, 0.2f, 1.0f));

    _container->addChild(_background, 0);
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
    _container->addChild(_healthBarBg, 1);

    // 伤害延迟条
    _healthBarDamage = DrawNode::create();
    _healthBarDamage->setPosition(Vec2(0, barY));
    _container->addChild(_healthBarDamage, 2);

    // 血条填充
    _healthBarFill = DrawNode::create();
    _healthBarFill->drawSolidRect(
        Vec2(-_barWidth / 2, -_barHeight / 2),
        Vec2(_barWidth / 2, _barHeight / 2),
        Color4F(0.8f, 0.1f, 0.1f, 1.0f));
    _healthBarFill->setPosition(Vec2(0, barY));
    _container->addChild(_healthBarFill, 3);

    // 血条边框
    auto border = DrawNode::create();
    border->drawRect(
        Vec2(-_barWidth / 2, -_barHeight / 2),
        Vec2(_barWidth / 2, _barHeight / 2),
        Color4F(0.8f, 0.6f, 0.2f, 1.0f));
    border->setPosition(Vec2(0, barY));
    _container->addChild(border, 4);

    // HP数值标签
    _hpLabel = Label::createWithTTF("", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _hpLabel->setPosition(Vec2(0, barY));
    _hpLabel->setColor(Color3B::WHITE);
    _container->addChild(_hpLabel, 5);
}

void BossHealthBar::createNameLabel()
{
    _nameLabel = Label::createWithTTF("BOSS", "fonts/ZCOOLKuaiLe-Regular.ttf", 24);
    _nameLabel->setPosition(Vec2(0, 25));
    _nameLabel->setColor(Color3B(255, 200, 100));
    _nameLabel->enableOutline(Color4B::BLACK, 2);
    _container->addChild(_nameLabel, 5);
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
        _container->addChild(indicator, 5);
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
        auto attr = _boss->getAttributeComponent();
        if (attr)
        {
            _lastHP = attr->getAttributeValue(AttributeType::MAX_HP);
            _damageBarHP = _lastHP;
        }
    }

    createPhaseIndicators(phaseCount);
    updateDisplay();
}

void BossHealthBar::unbindBoss()
{
    _boss = nullptr;
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

    // 检测HP变化
    if (currentHP < _lastHP)
    {
        // 受伤，播放伤害动画
        playHitAnimation();
    }
    else if (currentHP > _lastHP)
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
    // 血条抖动效果
    auto shake1 = MoveBy::create(0.02f, Vec2(5, 0));
    auto shake2 = MoveBy::create(0.02f, Vec2(-10, 0));
    auto shake3 = MoveBy::create(0.02f, Vec2(10, 0));
    auto shake4 = MoveBy::create(0.02f, Vec2(-5, 0));

    _healthBarFill->runAction(Sequence::create(shake1, shake2, shake3, shake4, nullptr));

    // 边框闪红
    if (_background)
    {
        auto tintRed = TintTo::create(0.1f, 255, 100, 100);
        auto tintBack = TintTo::create(0.1f, 255, 255, 255);
        _background->runAction(Sequence::create(tintRed, tintBack, nullptr));
    }
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
