/**
 * @file SkillBar.cpp
 * @brief 技能栏组件实现
 */

#include "SkillBar.h"
#include "Character/PlayerCharacter.h"
#include "Character/components/SkillComponent.h"
#include "Character/CharacterData.h"

USING_NS_CC;

SkillBar *SkillBar::create(int slotCount)
{
    SkillBar *ret = new (std::nothrow) SkillBar();
    if (ret)
    {
        ret->_slotCount = slotCount;
        if (ret->init())
        {
            ret->autorelease();
            return ret;
        }
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool SkillBar::init()
{
    if (!Node::init())
    {
        return false;
    }

    initSlots(_slotCount);
    return true;
}

void SkillBar::initSlots(int slotCount)
{
    // 清除旧槽位
    for (auto &slot : _slots)
    {
        if (slot.container)
        {
            slot.container->removeFromParent();
        }
    }
    _slots.clear();

    // 创建新槽位
    for (int i = 0; i < slotCount; ++i)
    {
        createSlot(i);
    }
}

void SkillBar::createSlot(int index)
{
    SkillSlotUI slot;

    // 创建槽位容器
    slot.container = Node::create();

    // 计算位置
    float posX = 0, posY = 0;
    if (_horizontalLayout)
    {
        posX = index * (_slotSize + _slotSpacing);
    }
    else
    {
        posY = -index * (_slotSize + _slotSpacing);
    }
    slot.container->setPosition(Vec2(posX, posY));

    // 创建背景
    slot.iconBg = Sprite::create();
    if (!slot.iconBg)
    {
        // 如果没有图片，使用DrawNode绘制背景
        auto bgDraw = DrawNode::create();
        bgDraw->drawSolidRect(
            Vec2(-_slotSize / 2, -_slotSize / 2),
            Vec2(_slotSize / 2, _slotSize / 2),
            Color4F(0.15f, 0.15f, 0.2f, 0.9f));
        slot.container->addChild(bgDraw, 0);
    }
    else
    {
        slot.iconBg->setContentSize(Size(_slotSize, _slotSize));
        slot.container->addChild(slot.iconBg, 0);
    }

    // 创建技能图标（初始为空）
    slot.icon = Sprite::create();
    if (slot.icon)
    {
        slot.icon->setVisible(false);
        slot.container->addChild(slot.icon, 1);
    }

    // 创建冷却遮罩
    slot.cooldownMask = DrawNode::create();
    slot.cooldownMask->setVisible(false);
    slot.container->addChild(slot.cooldownMask, 2);

    // 创建冷却倒计时文字
    slot.cooldownLabel = Label::createWithTTF("", "fonts/ZCOOLKuaiLe-Regular.ttf", 18);
    slot.cooldownLabel->setColor(Color3B::WHITE);
    slot.cooldownLabel->enableOutline(Color4B::BLACK, 2);
    slot.cooldownLabel->setVisible(false);
    slot.container->addChild(slot.cooldownLabel, 3);

    // 创建边框
    slot.border = DrawNode::create();
    slot.border->drawRect(
        Vec2(-_slotSize / 2, -_slotSize / 2),
        Vec2(_slotSize / 2, _slotSize / 2),
        Color4F(0.5f, 0.5f, 0.5f, 1.0f));
    slot.container->addChild(slot.border, 4);

    // 创建快捷键提示
    slot.hotkeyLabel = Label::createWithTTF("", "fonts/ZCOOLKuaiLe-Regular.ttf", 12);
    slot.hotkeyLabel->setAnchorPoint(Vec2(1, 0));
    slot.hotkeyLabel->setPosition(Vec2(_slotSize / 2 - 2, -_slotSize / 2 + 2));
    slot.hotkeyLabel->setColor(Color3B(200, 200, 200));
    slot.container->addChild(slot.hotkeyLabel, 5);

    slot.isEmpty = true;
    this->addChild(slot.container);
    _slots.push_back(slot);
}

void SkillBar::bindPlayer(PlayerCharacter *player)
{
    _player = player;
    updateDisplay();
}

void SkillBar::updateDisplay()
{
    if (!_player)
        return;

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
        return;

    const auto &activeSlots = skillComp->getActiveSlots();

    for (size_t i = 0; i < _slots.size(); ++i)
    {
        if (i < activeSlots.size() && activeSlots[i])
        {
            auto skill = activeSlots[i];
            updateSlotEmpty(i, false);
            updateSlotCooldown(i, skill->currentCooldown, skill->cooldown);
        }
        else
        {
            updateSlotEmpty(i, true);
            updateSlotCooldown(i, 0, 0);
        }
    }
}

void SkillBar::updateSlotCooldown(size_t index, float currentCD, float maxCD)
{
    if (index >= _slots.size())
        return;

    auto &slot = _slots[index];

    if (currentCD > 0 && maxCD > 0)
    {
        // 显示冷却遮罩
        slot.cooldownMask->setVisible(true);
        slot.cooldownMask->clear();

        // 计算冷却百分比（从上到下填充）
        float percent = currentCD / maxCD;
        float maskHeight = _slotSize * percent;

        slot.cooldownMask->drawSolidRect(
            Vec2(-_slotSize / 2, _slotSize / 2 - maskHeight),
            Vec2(_slotSize / 2, _slotSize / 2),
            Color4F(0.0f, 0.0f, 0.0f, 0.6f));

        // 显示冷却倒计时
        slot.cooldownLabel->setVisible(true);
        if (currentCD >= 1.0f)
        {
            slot.cooldownLabel->setString(StringUtils::format("%.0f", currentCD));
        }
        else
        {
            slot.cooldownLabel->setString(StringUtils::format("%.1f", currentCD));
        }

        // 边框变暗
        slot.border->clear();
        slot.border->drawRect(
            Vec2(-_slotSize / 2, -_slotSize / 2),
            Vec2(_slotSize / 2, _slotSize / 2),
            Color4F(0.3f, 0.3f, 0.3f, 1.0f));
    }
    else
    {
        // 隐藏冷却遮罩
        slot.cooldownMask->setVisible(false);
        slot.cooldownLabel->setVisible(false);

        // 边框恢复正常
        slot.border->clear();
        slot.border->drawRect(
            Vec2(-_slotSize / 2, -_slotSize / 2),
            Vec2(_slotSize / 2, _slotSize / 2),
            Color4F(0.6f, 0.6f, 0.6f, 1.0f));
    }
}

void SkillBar::updateSlotEmpty(size_t index, bool isEmpty)
{
    if (index >= _slots.size())
        return;

    auto &slot = _slots[index];
    slot.isEmpty = isEmpty;

    if (isEmpty)
    {
        if (slot.icon)
            slot.icon->setVisible(false);
    }
    else
    {
        if (slot.icon)
            slot.icon->setVisible(true);
    }
}

void SkillBar::setSlotIcon(size_t slotIndex, const std::string &iconPath)
{
    if (slotIndex >= _slots.size())
        return;

    auto &slot = _slots[slotIndex];

    if (slot.icon)
    {
        auto texture = Director::getInstance()->getTextureCache()->addImage(iconPath);
        if (texture)
        {
            slot.icon->setTexture(texture);
            slot.icon->setTextureRect(Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height));

            // 缩放图标以适应槽位
            float scale = (_slotSize - 8) / std::max(texture->getContentSize().width, texture->getContentSize().height);
            slot.icon->setScale(scale);
            slot.icon->setVisible(true);
            slot.isEmpty = false;
        }
    }
}

void SkillBar::setSlotHotkey(size_t slotIndex, const std::string &hotkey)
{
    if (slotIndex >= _slots.size())
        return;

    auto &slot = _slots[slotIndex];
    if (slot.hotkeyLabel)
    {
        slot.hotkeyLabel->setString(hotkey);
    }
}

void SkillBar::playUseAnimation(size_t slotIndex)
{
    if (slotIndex >= _slots.size())
        return;

    auto &slot = _slots[slotIndex];
    if (!slot.container)
        return;

    // 播放缩放动画
    auto scaleUp = ScaleTo::create(0.1f, 1.2f);
    auto scaleDown = ScaleTo::create(0.1f, 1.0f);
    slot.container->runAction(Sequence::create(scaleUp, scaleDown, nullptr));

    // 边框闪烁
    if (slot.border)
    {
        slot.border->clear();
        slot.border->drawRect(
            Vec2(-_slotSize / 2, -_slotSize / 2),
            Vec2(_slotSize / 2, _slotSize / 2),
            Color4F(1.0f, 1.0f, 0.0f, 1.0f)); // 黄色高亮

        // 延迟恢复
        auto delay = DelayTime::create(0.2f);
        auto restore = CallFunc::create([this, slotIndex]()
                                        {
            if (slotIndex < _slots.size()) {
                auto& s = _slots[slotIndex];
                if (s.border) {
                    s.border->clear();
                    s.border->drawRect(
                        Vec2(-_slotSize / 2, -_slotSize / 2),
                        Vec2(_slotSize / 2, _slotSize / 2),
                        Color4F(0.6f, 0.6f, 0.6f, 1.0f));
                }
            } });
        slot.container->runAction(Sequence::create(delay, restore, nullptr));
    }
}

void SkillBar::setHorizontalLayout(bool horizontal)
{
    if (_horizontalLayout == horizontal)
        return;

    _horizontalLayout = horizontal;

    // 重新排列槽位
    for (size_t i = 0; i < _slots.size(); ++i)
    {
        float posX = 0, posY = 0;
        if (_horizontalLayout)
        {
            posX = i * (_slotSize + _slotSpacing);
        }
        else
        {
            posY = -static_cast<float>(i) * (_slotSize + _slotSpacing);
        }
        _slots[i].container->setPosition(Vec2(posX, posY));
    }
}
