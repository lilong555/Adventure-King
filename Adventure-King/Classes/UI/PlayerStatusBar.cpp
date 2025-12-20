/**
 * @file PlayerStatusBar.cpp
 * @brief 玩家状态栏组件实现
 */

#include "PlayerStatusBar.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"

USING_NS_CC;

PlayerStatusBar *PlayerStatusBar::create()
{
    PlayerStatusBar *ret = new (std::nothrow) PlayerStatusBar();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool PlayerStatusBar::init()
{
    if (!Node::init())
    {
        return false;
    }

    createHPBar();
    createMPBar();
    createExpBar();
    createLevelLabel();

    return true;
}

void PlayerStatusBar::createHPBar()
{
    // HP 条背景
    _hpBarBg = DrawNode::create();
    _hpBarBg->drawSolidRect(
        Vec2(0, 0),
        Vec2(_barWidth, _barHeight),
        Color4F(0.2f, 0.1f, 0.1f, 0.8f));
    _hpBarBg->setPosition(Vec2(0, 0));
    this->addChild(_hpBarBg, 0);

    // HP 伤害延迟条（白色，显示受伤前的血量）
    _hpBarDamage = DrawNode::create();
    _hpBarDamage->drawSolidRect(
        Vec2(0, 0),
        Vec2(_barWidth, _barHeight),
        Color4F(1.0f, 1.0f, 1.0f, 0.6f));
    _hpBarDamage->setPosition(Vec2(0, 0));
    this->addChild(_hpBarDamage, 1);

    // HP 条填充
    _hpBarFill = DrawNode::create();
    _hpBarFill->drawSolidRect(
        Vec2(0, 0),
        Vec2(_barWidth, _barHeight),
        Color4F(0.8f, 0.2f, 0.2f, 1.0f));
    _hpBarFill->setPosition(Vec2(0, 0));
    this->addChild(_hpBarFill, 2);

    // HP 边框
    auto hpBorder = DrawNode::create();
    hpBorder->drawRect(
        Vec2(0, 0),
        Vec2(_barWidth, _barHeight),
        Color4F(0.4f, 0.4f, 0.4f, 1.0f));
    hpBorder->setPosition(Vec2(0, 0));
    this->addChild(hpBorder, 3);

    // HP 标签
    _hpLabel = Label::createWithTTF("HP: 0/0", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _hpLabel->setAnchorPoint(Vec2(0, 0.5f));
    _hpLabel->setPosition(Vec2(_barWidth + 10, _barHeight / 2));
    _hpLabel->setColor(Color3B::WHITE);
    this->addChild(_hpLabel, 4);
}

void PlayerStatusBar::createMPBar()
{
    float yOffset = -(_barHeight + _barSpacing);

    // MP 条背景
    _mpBarBg = DrawNode::create();
    _mpBarBg->drawSolidRect(
        Vec2(0, 0),
        Vec2(_barWidth, _barHeight),
        Color4F(0.1f, 0.1f, 0.2f, 0.8f));
    _mpBarBg->setPosition(Vec2(0, yOffset));
    this->addChild(_mpBarBg, 0);

    // MP 条填充
    _mpBarFill = DrawNode::create();
    _mpBarFill->drawSolidRect(
        Vec2(0, 0),
        Vec2(_barWidth, _barHeight),
        Color4F(0.2f, 0.4f, 0.9f, 1.0f));
    _mpBarFill->setPosition(Vec2(0, yOffset));
    this->addChild(_mpBarFill, 1);

    // MP 边框
    auto mpBorder = DrawNode::create();
    mpBorder->drawRect(
        Vec2(0, 0),
        Vec2(_barWidth, _barHeight),
        Color4F(0.4f, 0.4f, 0.4f, 1.0f));
    mpBorder->setPosition(Vec2(0, yOffset));
    this->addChild(mpBorder, 2);

    // MP 标签
    _mpLabel = Label::createWithTTF("MP: 0/0", "fonts/ZCOOLKuaiLe-Regular.ttf", 14);
    _mpLabel->setAnchorPoint(Vec2(0, 0.5f));
    _mpLabel->setPosition(Vec2(_barWidth + 10, yOffset + _barHeight / 2));
    _mpLabel->setColor(Color3B::WHITE);
    this->addChild(_mpLabel, 4);
}

void PlayerStatusBar::createExpBar()
{
    float yOffset = -2 * (_barHeight + _barSpacing);

    // 经验条背景
    _expBarBg = DrawNode::create();
    _expBarBg->drawSolidRect(
        Vec2(0, 0),
        Vec2(_barWidth, _expBarHeight),
        Color4F(0.15f, 0.15f, 0.1f, 0.8f));
    _expBarBg->setPosition(Vec2(0, yOffset));
    this->addChild(_expBarBg, 0);

    // 经验条填充
    _expBarFill = DrawNode::create();
    _expBarFill->drawSolidRect(
        Vec2(0, 0),
        Vec2(_barWidth, _expBarHeight),
        Color4F(0.9f, 0.8f, 0.2f, 1.0f));
    _expBarFill->setPosition(Vec2(0, yOffset));
    this->addChild(_expBarFill, 1);

    // 经验条边框
    auto expBorder = DrawNode::create();
    expBorder->drawRect(
        Vec2(0, 0),
        Vec2(_barWidth, _expBarHeight),
        Color4F(0.4f, 0.4f, 0.4f, 1.0f));
    expBorder->setPosition(Vec2(0, yOffset));
    this->addChild(expBorder, 2);

    // 经验标签
    _expLabel = Label::createWithTTF("EXP: 0/100", "fonts/ZCOOLKuaiLe-Regular.ttf", 12);
    _expLabel->setAnchorPoint(Vec2(0, 0.5f));
    _expLabel->setPosition(Vec2(_barWidth + 10, yOffset + _expBarHeight / 2));
    _expLabel->setColor(Color3B(255, 220, 100));
    this->addChild(_expLabel, 4);
}

void PlayerStatusBar::createLevelLabel()
{
    // 等级显示在状态栏左侧
    _levelLabel = Label::createWithTTF("Lv.1", "fonts/ZCOOLKuaiLe-Regular.ttf", 24);
    _levelLabel->setAnchorPoint(Vec2(1, 0.5f));
    _levelLabel->setPosition(Vec2(-15, 0));
    _levelLabel->setColor(Color3B(255, 215, 0)); // 金色
    _levelLabel->enableOutline(Color4B::BLACK, 2);
    this->addChild(_levelLabel, 5);
}

void PlayerStatusBar::bindPlayer(PlayerCharacter *player)
{
    _player = player;
    if (_player)
    {
        _lastHP = _player->getCurrentHP();
        _damageBarHP = _lastHP;
        updateDisplay();
    }
}

void PlayerStatusBar::updateDisplay()
{
    if (!_player)
        return;

    auto attr = _player->getAttributeComponent();
    if (!attr)
        return;

    // 获取属性值
    float maxHP = attr->getAttributeValue(AttributeType::MAX_HP);
    float maxMP = attr->getAttributeValue(AttributeType::MAX_MP);
    float currentHP = std::max(0.0f, std::min(_player->getCurrentHP(), maxHP));
    float currentMP = std::max(0.0f, std::min(_player->getCurrentMP(), maxMP));

    // 检测HP变化，播放伤害动画
    if (currentHP < _lastHP)
    {
        playHPChangeAnimation(_lastHP, currentHP);
    }
    else if (currentHP > _lastHP)
    {
        // 治疗时直接更新伤害条
        _damageBarHP = currentHP;
    }
    _lastHP = currentHP;

    // 更新各个条
    updateHPBar(currentHP, maxHP);
    updateMPBar(currentMP, maxMP);

    // 更新经验条
    int currentExp = _player->getExperience();
    int level = _player->getLevel();
    int requiredExp = 100 * level; // 升级所需经验
    updateExpBar(currentExp, requiredExp);

    // 更新等级
    updateLevelLabel(level);
}

void PlayerStatusBar::updateHPBar(float current, float max)
{
    float percent = max > 0 ? current / max : 0;
    float damagePercent = max > 0 ? _damageBarHP / max : 0;

    // 更新HP填充条
    _hpBarFill->clear();
    if (percent > 0)
    {
        // 根据血量百分比改变颜色
        Color4F hpColor;
        if (percent > 0.5f)
        {
            hpColor = Color4F(0.2f, 0.8f, 0.2f, 1.0f); // 绿色
        }
        else if (percent > 0.25f)
        {
            hpColor = Color4F(0.9f, 0.7f, 0.1f, 1.0f); // 黄色
        }
        else
        {
            hpColor = Color4F(0.9f, 0.2f, 0.2f, 1.0f); // 红色
        }
        _hpBarFill->drawSolidRect(
            Vec2(0, 0),
            Vec2(_barWidth * percent, _barHeight),
            hpColor);
    }

    // 更新伤害延迟条
    _hpBarDamage->clear();
    if (damagePercent > percent)
    {
        _hpBarDamage->drawSolidRect(
            Vec2(_barWidth * percent, 0),
            Vec2(_barWidth * damagePercent, _barHeight),
            Color4F(1.0f, 1.0f, 1.0f, 0.5f));
    }

    // 更新标签
    if (_hpLabel)
    {
        _hpLabel->setString(StringUtils::format("HP: %.0f/%.0f", current, max));
    }
}

void PlayerStatusBar::updateMPBar(float current, float max)
{
    float percent = max > 0 ? current / max : 0;

    _mpBarFill->clear();
    if (percent > 0)
    {
        _mpBarFill->drawSolidRect(
            Vec2(0, 0),
            Vec2(_barWidth * percent, _barHeight),
            Color4F(0.2f, 0.4f, 0.9f, 1.0f));
    }

    if (_mpLabel)
    {
        _mpLabel->setString(StringUtils::format("MP: %.0f/%.0f", current, max));
    }
}

void PlayerStatusBar::updateExpBar(int current, int required)
{
    if (!_expBarVisible)
        return;

    float percent = required > 0 ? static_cast<float>(current) / required : 0;
    percent = std::min(percent, 1.0f);

    _expBarFill->clear();
    if (percent > 0)
    {
        _expBarFill->drawSolidRect(
            Vec2(0, 0),
            Vec2(_barWidth * percent, _expBarHeight),
            Color4F(0.9f, 0.8f, 0.2f, 1.0f));
    }

    if (_expLabel)
    {
        _expLabel->setString(StringUtils::format("EXP: %d/%d", current, required));
    }
}

void PlayerStatusBar::updateLevelLabel(int level)
{
    if (_levelLabel)
    {
        _levelLabel->setString(StringUtils::format("Lv.%d", level));
    }
}

void PlayerStatusBar::playHPChangeAnimation(float oldHP, float newHP)
{
    // 伤害延迟条动画：先保持旧血量，然后缓慢下降
    _damageBarHP = oldHP;

    // 使用定时器逐渐减少伤害条
    auto delay = DelayTime::create(0.3f);
    auto callback = CallFunc::create([this, newHP]()
                                     {
        // 缓慢减少伤害条
        this->schedule([this, newHP](float dt) {
            float speed = 50.0f; // 每秒减少的HP显示量
            _damageBarHP -= speed * dt;
            if (_damageBarHP <= newHP) {
                _damageBarHP = newHP;
                this->unschedule("damage_bar_update");
            }
        }, "damage_bar_update"); });

    this->runAction(Sequence::create(delay, callback, nullptr));
}

void PlayerStatusBar::setBarPosition(const Vec2 &position)
{
    this->setPosition(position);
}

void PlayerStatusBar::setBarScale(float scale)
{
    this->setScale(scale);
}

void PlayerStatusBar::setExpBarVisible(bool visible)
{
    _expBarVisible = visible;
    if (_expBarBg)
        _expBarBg->setVisible(visible);
    if (_expBarFill)
        _expBarFill->setVisible(visible);
    if (_expLabel)
        _expLabel->setVisible(visible);
}
