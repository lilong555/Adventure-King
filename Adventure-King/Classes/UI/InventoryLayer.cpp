/**
 * @file InventoryLayer.cpp
 * @brief 背包/技能管理界面实现（占位 UI）
 */

#include "UI/InventoryLayer.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/SkillComponent.h"
#include "Configs/GameConfigs.h"
#include <algorithm>

USING_NS_CC;

namespace
{
    const char *const FONT_PATH = "fonts/ZCOOLKuaiLe-Regular.ttf";
    constexpr float PANEL_WIDTH = 1000.0f;
    constexpr float PANEL_HEIGHT = 650.0f;
    constexpr float PANEL_INNER_PADDING_X = 50.0f;       // 面板左右内边距
    constexpr float PANEL_INNER_PADDING_BOTTOM = 80.0f;  // 给"返回"按钮预留空间
    constexpr float COLUMN_GAP = 30.0f;                  // 双栏间距
    constexpr float PANEL_CORNER_RADIUS = 12.0f;         // 面板圆角

    constexpr int Z_BACKGROUND = 0;
    constexpr int Z_PANEL = 1;
    constexpr int Z_TEXT = 2;
    constexpr int Z_MENU = 3;

    // 颜色主题
    const Color4F PANEL_BG_COLOR = Color4F(0.08f, 0.08f, 0.12f, 0.95f);
    const Color4F PANEL_BORDER_COLOR = Color4F(0.5f, 0.45f, 0.35f, 1.0f);
    const Color3B TITLE_COLOR = Color3B(255, 215, 80);
    const Color3B SECTION_TITLE_COLOR = Color3B(180, 200, 230);
    const Color3B ITEM_TEXT_COLOR = Color3B(230, 230, 230);
    const Color3B ITEM_ATTR_COLOR = Color3B(160, 180, 160);
    const Color3B SELECTED_COLOR = Color3B(255, 200, 100);

    // 绘制圆角矩形（简化实现）
    void drawRoundedRect(DrawNode *node, const Vec2 &origin, const Vec2 &destination,
                         const Color4F &fillColor, const Color4F &borderColor,
                         float /*cornerRadius*/, float /*borderWidth*/ = 2.0f)
    {
        node->drawSolidRect(origin, destination, fillColor);
        node->drawRect(origin, destination, borderColor);
    }

    // 绘制分隔线
    void drawSeparator(DrawNode *node, const Vec2 &start, const Vec2 &end,
                       const Color4F &color = Color4F(0.3f, 0.3f, 0.35f, 0.8f))
    {
        node->drawLine(start, end, color);
    }

    Rect getPanelRect()
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        auto origin = Director::getInstance()->getVisibleOrigin();
        Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);
        return Rect(center.x - PANEL_WIDTH / 2, center.y - PANEL_HEIGHT / 2, PANEL_WIDTH, PANEL_HEIGHT);
    }

    MenuItemLabel *createFixedWidthMenuItem(const std::string &text,
                                            float width,
                                            int fontSize,
                                            const ccMenuCallback &callback)
    {
        auto label = Label::createWithTTF(text, FONT_PATH, fontSize);
        if (!label)
        {
            return nullptr;
        }

        label->setColor(Color3B::WHITE);
        label->enableOutline(Color4B::BLACK, 2);
        label->setAlignment(TextHAlignment::LEFT, TextVAlignment::CENTER);
        label->setOverflow(Label::Overflow::RESIZE_HEIGHT);
        label->setDimensions(width, 0);

        auto item = MenuItemLabel::create(label, callback);
        if (!item)
        {
            return nullptr;
        }

        // 让点击区域与文本区域一致，并把文本放在中间（文本本身左对齐）
        item->setContentSize(label->getContentSize());
        label->setPosition(Vec2(item->getContentSize().width / 2, item->getContentSize().height / 2));
        return item;
    }
}

InventoryLayer *InventoryLayer::create()
{
    auto ret = new (std::nothrow) InventoryLayer();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool InventoryLayer::init()
{
    if (!Layer::init())
    {
        return false;
    }

    _container = Node::create();
    addChild(_container);

    createBackground();
    createPanel();
    createTabs();
    buildSkillTemplates();

    // 关闭按钮
    auto closeBtn = createMenuButton("返回", CC_CALLBACK_1(InventoryLayer::onCloseClicked, this), 24);
    _closeMenu = Menu::create(closeBtn, nullptr);
    _closeMenu->setPosition(Vec2::ZERO);
    _panelRoot->addChild(_closeMenu, Z_MENU);
    createPages();

    // 触摸吞噬：防止事件穿透到底层
    // 使用固定优先级，确保 Menu 的事件优先级更高（Menu 默认优先级为 -128）
    _touchListener = EventListenerTouchOneByOne::create();
    _touchListener->setSwallowTouches(true);
    _touchListener->onTouchBegan = [this](Touch *touch, Event *) -> bool
    {
        if (!_isShowing)
        {
            return false;
        }
        // 只吞噬面板外的点击，面板内的点击交给 Menu 处理
        Rect panelRect = getPanelRect();
        Vec2 touchLocation = touch->getLocation();
        if (!panelRect.containsPoint(touchLocation))
        {
            // 点击面板外部，关闭背包
            hide();
            if (_closeCallback)
            {
                _closeCallback();
            }
            return true;
        }
        // 面板内的点击，不吞噬，让 Menu 处理
        return false;
    };
    // 使用较低的固定优先级（数值越大优先级越低），确保 Menu 优先处理
    _eventDispatcher->addEventListenerWithFixedPriority(_touchListener, 100);
    _touchListener->setEnabled(false);

    // 初始隐藏
    setVisible(false);
    _isShowing = false;

    switchTab(Tab::EQUIPMENT);
    refresh();
    return true;
}

void InventoryLayer::bindPlayer(PlayerCharacter *player)
{
    _player = player;
    refresh();
}

void InventoryLayer::show()
{
    if (_isShowing)
    {
        return;
    }
    _isShowing = true;
    setVisible(true);
    if (_touchListener)
    {
        _touchListener->setEnabled(true);
    }

    refresh();
    _container->setOpacity(0);
    _container->runAction(FadeIn::create(0.15f));
}

void InventoryLayer::hide()
{
    if (!_isShowing)
    {
        return;
    }
    _isShowing = false;
    if (_touchListener)
    {
        _touchListener->setEnabled(false);
    }

    auto fadeOut = FadeOut::create(0.15f);
    auto callback = CallFunc::create([this]()
                                     { setVisible(false); });
    _container->runAction(Sequence::create(fadeOut, callback, nullptr));
}

void InventoryLayer::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    _background = DrawNode::create();
    _background->drawSolidRect(
        origin,
        Vec2(origin.x + visibleSize.width, origin.y + visibleSize.height),
        Color4F(0.0f, 0.0f, 0.0f, 0.75f));
    _container->addChild(_background, Z_BACKGROUND);
}

void InventoryLayer::createPanel()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    _panelRoot = Node::create();
    _container->addChild(_panelRoot, Z_PANEL);

    _panel = DrawNode::create();
    // 使用主题颜色绘制面板
    drawRoundedRect(_panel,
                    Vec2(center.x - PANEL_WIDTH / 2, center.y - PANEL_HEIGHT / 2),
                    Vec2(center.x + PANEL_WIDTH / 2, center.y + PANEL_HEIGHT / 2),
                    PANEL_BG_COLOR, PANEL_BORDER_COLOR, PANEL_CORNER_RADIUS);
    _panelRoot->addChild(_panel, Z_PANEL);

    // 标题区域背景
    auto titleBg = DrawNode::create();
    titleBg->drawSolidRect(
        Vec2(center.x - PANEL_WIDTH / 2 + 2, center.y + PANEL_HEIGHT / 2 - 70),
        Vec2(center.x + PANEL_WIDTH / 2 - 2, center.y + PANEL_HEIGHT / 2 - 2),
        Color4F(0.15f, 0.12f, 0.08f, 0.9f));
    _panelRoot->addChild(titleBg, Z_PANEL);

    _titleLabel = Label::createWithTTF("背包 / 技能", FONT_PATH, 36);
    _titleLabel->setPosition(Vec2(center.x, center.y + PANEL_HEIGHT / 2 - 36));
    _titleLabel->setColor(TITLE_COLOR);
    _titleLabel->enableOutline(Color4B::BLACK, 2);
    _panelRoot->addChild(_titleLabel, Z_TEXT);

    // 标题下方分隔线
    auto separator = DrawNode::create();
    drawSeparator(separator,
                  Vec2(center.x - PANEL_WIDTH / 2 + 20, center.y + PANEL_HEIGHT / 2 - 70),
                  Vec2(center.x + PANEL_WIDTH / 2 - 20, center.y + PANEL_HEIGHT / 2 - 70));
    _panelRoot->addChild(separator, Z_TEXT);
}

void InventoryLayer::createTabs()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // Tab 按钮样式改进
    _tabEquipment = createMenuButton("装备", CC_CALLBACK_1(InventoryLayer::onTabEquipmentClicked, this), 26);
    _tabSkill = createMenuButton("技能", CC_CALLBACK_1(InventoryLayer::onTabSkillClicked, this), 26);

    float tabY = center.y + PANEL_HEIGHT / 2 - 100;
    _tabEquipment->setPosition(Vec2(center.x - 80, tabY));
    _tabSkill->setPosition(Vec2(center.x + 80, tabY));

    _tabMenu = Menu::create(_tabEquipment, _tabSkill, nullptr);
    _tabMenu->setPosition(Vec2::ZERO);
    _panelRoot->addChild(_tabMenu, Z_MENU);
}

void InventoryLayer::createPages()
{
    _equipmentPage = Node::create();
    _skillPage = Node::create();
    _panelRoot->addChild(_equipmentPage, Z_TEXT);
    _panelRoot->addChild(_skillPage, Z_TEXT);

    // 关闭按钮放在面板底部居中
    Rect panel = getPanelRect();
    if (_closeMenu && !_closeMenu->getChildren().empty())
    {
        _closeMenu->getChildren().at(0)->setPosition(Vec2(panel.getMidX(), panel.getMinY() + 45.0f));
    }
}

void InventoryLayer::buildSkillTemplates()
{
    _skillTemplates.clear();

    // 主动：炸弹（示例，可通过背包界面学习并装备）
    {
        SkillTemplate t;
        t.id = GameConfig::Bomb::BOMB_ID;
        t.isPassive = false;
        t.name = "炸弹投掷";
        t.description = "投掷一枚炸弹，碰撞后爆炸造成范围伤害";
        t.cooldown = GameConfig::Bomb::BOMB_CD;
        t.manaCost = GameConfig::Bomb::BOMB_MP;
        _skillTemplates.push_back(t);
    }

    // 主动：火球（默认 Klee 已学习，但仍保留在模板列表里用于展示）
    {
        SkillTemplate t;
        t.id = GameConfig::Fireball::FIREBALL_ID;
        t.isPassive = false;
        t.name = "火球";
        t.description = "发射火球，命中后爆炸造成范围伤害";
        t.cooldown = GameConfig::Fireball::FIREBALL_CD;
        t.manaCost = GameConfig::Fireball::FIREBALL_MP;
        _skillTemplates.push_back(t);
    }

    // 被动：体魄
    {
        SkillTemplate t;
        t.id = 2001;
        t.isPassive = true;
        t.name = "体魄强化";
        t.description = "提升最大生命值";
        t.attributeBonus.add(AttributeType::MAX_HP, 30.0f);
        _skillTemplates.push_back(t);
    }

    // 被动：迅捷
    {
        SkillTemplate t;
        t.id = 2002;
        t.isPassive = true;
        t.name = "迅捷步伐";
        t.description = "提升移动速度";
        t.attributeBonus.add(AttributeType::MOVE_SPEED, 30.0f);
        _skillTemplates.push_back(t);
    }

    // 被动：专注
    {
        SkillTemplate t;
        t.id = 2003;
        t.isPassive = true;
        t.name = "战斗专注";
        t.description = "提升暴击率";
        t.attributeBonus.add(AttributeType::CRITICAL_RATE, 0.05f);
        _skillTemplates.push_back(t);
    }
}

void InventoryLayer::switchTab(Tab tab)
{
    _currentTab = tab;
    bool equipVisible = (tab == Tab::EQUIPMENT);
    if (_equipmentPage)
    {
        _equipmentPage->setVisible(equipVisible);
    }
    if (_skillPage)
    {
        _skillPage->setVisible(!equipVisible);
    }

    if (_tabEquipment)
    {
        auto label = dynamic_cast<Label *>(_tabEquipment->getLabel());
        if (label)
        {
            label->setTextColor(equipVisible ? Color4B(255, 220, 100, 255) : Color4B::WHITE);
        }
    }
    if (_tabSkill)
    {
        auto label = dynamic_cast<Label *>(_tabSkill->getLabel());
        if (label)
        {
            label->setTextColor(!equipVisible ? Color4B(255, 220, 100, 255) : Color4B::WHITE);
        }
    }

    refresh();
}

void InventoryLayer::refresh()
{
    if (!_panelRoot)
    {
        return;
    }

    if (_currentTab == Tab::EQUIPMENT)
    {
        refreshEquipmentPage();
    }
    else
    {
        refreshSkillPage();
    }
}

std::string InventoryLayer::getEquipmentSlotName(EquipmentSlot slot) const
{
    switch (slot)
    {
    case EquipmentSlot::WEAPON:
        return "武器";
    case EquipmentSlot::HELMET:
        return "头盔";
    case EquipmentSlot::ARMOR:
        return "护甲";
    case EquipmentSlot::BOOTS:
        return "靴子";
    default:
        return "未知";
    }
}

std::string InventoryLayer::formatAttributes(const Attributes &attrs) const
{
    if (attrs.values.empty())
    {
        return "";
    }

    std::string out;
    for (std::map<AttributeType, float>::const_iterator it = attrs.values.begin();
         it != attrs.values.end(); ++it)
    {
        if (!out.empty())
        {
            out += " ";
        }

        const AttributeType type = it->first;
        const float val = it->second;
        switch (type)
        {
        case AttributeType::STRENGTH:
            out += StringUtils::format("力+%.0f", val);
            break;
        case AttributeType::DEFENSE:
            out += StringUtils::format("防+%.0f", val);
            break;
        case AttributeType::MOVE_SPEED:
            out += StringUtils::format("速+%.0f", val);
            break;
        case AttributeType::MAX_HP:
            out += StringUtils::format("HP+%.0f", val);
            break;
        case AttributeType::MAX_MP:
            out += StringUtils::format("MP+%.0f", val);
            break;
        case AttributeType::CRITICAL_RATE:
            out += StringUtils::format("暴+%.0f%%", val * 100.0f);
            break;
        default:
            out += StringUtils::format("属性%d+%.2f", static_cast<int>(type), val);
            break;
        }
    }
    return out;
}

MenuItemLabel *InventoryLayer::createMenuButton(const std::string &text,
                                                const ccMenuCallback &callback,
                                                int fontSize)
{
    auto label = Label::createWithTTF(text, FONT_PATH, fontSize);
    label->setColor(Color3B::WHITE);
    label->enableOutline(Color4B::BLACK, 2);
    auto item = MenuItemLabel::create(label, callback);
    if (item && label)
    {
        item->setContentSize(label->getContentSize());
        label->setPosition(Vec2(item->getContentSize().width / 2, item->getContentSize().height / 2));
    }
    return item;
}

void InventoryLayer::refreshEquipmentPage()
{
    if (!_equipmentPage)
    {
        return;
    }

    _equipmentPage->removeAllChildren();

    Rect panel = getPanelRect();
    const float contentLeft = panel.getMinX() + PANEL_INNER_PADDING_X;
    const float contentRight = panel.getMaxX() - PANEL_INNER_PADDING_X;
    const float contentWidth = contentRight - contentLeft;
    const float contentBottom = panel.getMinY() + PANEL_INNER_PADDING_BOTTOM;

    // 内容起始位置（Tab 下方）
    float contentTop = panel.getMaxY() - 130.0f;

    // 双栏布局：左侧装备槽位，右侧背包物品
    const float colWidth = (contentWidth - COLUMN_GAP) / 2.0f;
    const float leftColX = contentLeft;
    const float rightColX = contentLeft + colWidth + COLUMN_GAP;

    // 左栏：装备槽位
    auto equipTitle = Label::createWithTTF("当前装备", FONT_PATH, 24);
    equipTitle->setAnchorPoint(Vec2(0, 0.5f));
    equipTitle->setColor(SECTION_TITLE_COLOR);
    equipTitle->enableOutline(Color4B::BLACK, 1);
    equipTitle->setPosition(Vec2(leftColX, contentTop));
    _equipmentPage->addChild(equipTitle, Z_TEXT);

    // 右栏：背包物品
    auto bagTitle = Label::createWithTTF("背包物品", FONT_PATH, 24);
    bagTitle->setAnchorPoint(Vec2(0, 0.5f));
    bagTitle->setColor(SECTION_TITLE_COLOR);
    bagTitle->enableOutline(Color4B::BLACK, 1);
    bagTitle->setPosition(Vec2(rightColX, contentTop));
    _equipmentPage->addChild(bagTitle, Z_TEXT);

    float yEquip = contentTop - 40.0f;
    float yBag = contentTop - 40.0f;

    // 玩家未绑定
    if (!_player)
    {
        auto hint = Label::createWithTTF("未绑定玩家", FONT_PATH, 20);
        hint->setAnchorPoint(Vec2(0, 0.5f));
        hint->setPosition(Vec2(leftColX, yEquip));
        _equipmentPage->addChild(hint, Z_TEXT);
        return;
    }

    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    _equipmentPage->addChild(menu, Z_MENU);

    // 装备槽位列表
    std::vector<EquipmentSlot> slotOrder = {
        EquipmentSlot::WEAPON,
        EquipmentSlot::HELMET,
        EquipmentSlot::ARMOR,
        EquipmentSlot::BOOTS,
    };

    const float slotHeight = 36.0f;
    const float detailHeight = 60.0f;

    for (size_t i = 0; i < slotOrder.size(); ++i)
    {
        EquipmentSlot slot = slotOrder[i];
        auto eq = _player->getEquipment(slot);

        std::string slotName = getEquipmentSlotName(slot);
        std::string eqName = eq ? eq->name : "空";
        bool isSelected = (_selectedEquipSlotIndex == static_cast<int>(i));

        // 选中指示符
        std::string prefix = isSelected ? "▶ " : "   ";
        std::string displayText = prefix + slotName + "：" + eqName;

        // 创建可点击的槽位按钮
        auto slotBtn = createFixedWidthMenuItem(displayText, colWidth - 60, 20, [this, i](Ref *) {
            // 切换选中状态
            if (_selectedEquipSlotIndex == static_cast<int>(i))
            {
                _selectedEquipSlotIndex = -1;
            }
            else
            {
                _selectedEquipSlotIndex = static_cast<int>(i);
            }
            refresh();
        });

        if (slotBtn)
        {
            // 设置颜色
            if (auto label = dynamic_cast<Label *>(slotBtn->getLabel()))
            {
                if (isSelected)
                {
                    label->setColor(SELECTED_COLOR);
                }
                else if (eq)
                {
                    label->setColor(ITEM_TEXT_COLOR);
                }
                else
                {
                    label->setColor(Color3B(120, 120, 120));
                }
            }
            slotBtn->setAnchorPoint(Vec2(0, 0.5f));
            slotBtn->setPosition(Vec2(leftColX + (colWidth - 60) / 2, yEquip));
            menu->addChild(slotBtn);
        }

        // 卸下按钮
        if (eq)
        {
            auto unequipBtn = createMenuButton("卸下", [this, slot](Ref *) {
                if (_player)
                {
                    _player->unequip(slot);
                    _selectedEquipSlotIndex = -1;
                    refresh();
                }
            }, 16);
            if (unequipBtn)
            {
                unequipBtn->setPosition(Vec2(leftColX + colWidth - 25, yEquip));
                menu->addChild(unequipBtn);
            }
        }

        yEquip -= slotHeight;

        // 如果选中，显示详细信息
        if (isSelected && eq)
        {
            // 绘制详情背景
            auto detailBg = DrawNode::create();
            detailBg->drawSolidRect(
                Vec2(leftColX, yEquip - detailHeight + 10),
                Vec2(leftColX + colWidth - 10, yEquip + 5),
                Color4F(0.15f, 0.15f, 0.2f, 0.8f));
            _equipmentPage->addChild(detailBg, Z_TEXT);

            // 属性加成
            std::string attrs = formatAttributes(eq->attributeBonus);
            if (!attrs.empty())
            {
                auto attrLabel = Label::createWithTTF("属性：" + attrs, FONT_PATH, 16);
                attrLabel->setAnchorPoint(Vec2(0, 0.5f));
                attrLabel->setColor(ITEM_ATTR_COLOR);
                attrLabel->setPosition(Vec2(leftColX + 10, yEquip - 15));
                _equipmentPage->addChild(attrLabel, Z_TEXT + 1);
            }

            // 描述（如果有）
            if (!eq->description.empty())
            {
                auto descLabel = Label::createWithTTF(eq->description, FONT_PATH, 14);
                descLabel->setAnchorPoint(Vec2(0, 0.5f));
                descLabel->setColor(Color3B(180, 180, 180));
                descLabel->setDimensions(colWidth - 30, 0);
                descLabel->setPosition(Vec2(leftColX + 10, yEquip - 40));
                _equipmentPage->addChild(descLabel, Z_TEXT + 1);
            }

            yEquip -= detailHeight;
        }

        yEquip -= 6.0f;
    }

    // 背包物品列表
    const auto &items = _player->getInventoryItems();
    int shown = 0;
    const int maxShow = 20;

    for (size_t i = 0; i < items.size() && shown < maxShow; ++i)
    {
        if (yBag < contentBottom)
        {
            break;
        }

        const auto &item = items[i];
        if (!item)
        {
            continue;
        }

        bool isEquipped = false;
        auto equipped = _player->getEquipment(item->slot);
        if (equipped && equipped->id == item->id)
        {
            isEquipped = true;
        }

        std::string slotTag = "[" + getEquipmentSlotName(item->slot) + "]";
        std::string text = slotTag + " " + item->name;
        if (isEquipped)
        {
            text += " (已穿戴)";
        }

        auto btn = createFixedWidthMenuItem(text, colWidth - 10, 18, [this, itemId = item->id](Ref *) {
            if (!_player)
            {
                return;
            }
            const auto &inv = _player->getInventoryItems();
            for (const auto &it : inv)
            {
                if (it && it->id == itemId)
                {
                    _player->equip(it);
                    refresh();
                    break;
                }
            }
        });
        if (!btn)
        {
            continue;
        }

        // 已穿戴的物品显示不同颜色
        if (isEquipped)
        {
            if (auto label = dynamic_cast<Label *>(btn->getLabel()))
            {
                label->setColor(SELECTED_COLOR);
            }
        }

        btn->setPosition(Vec2(rightColX + (colWidth - 10) / 2, yBag));
        menu->addChild(btn);

        yBag -= btn->getContentSize().height + 8.0f;
        shown++;
    }

    if (shown == 0)
    {
        auto empty = Label::createWithTTF("（背包为空）", FONT_PATH, 18);
        empty->setAnchorPoint(Vec2(0, 0.5f));
        empty->setColor(Color3B(120, 120, 120));
        empty->setPosition(Vec2(rightColX, yBag));
        _equipmentPage->addChild(empty, Z_TEXT);
    }
}

void InventoryLayer::refreshSkillPage()
{
    if (!_skillPage)
    {
        return;
    }

    _skillPage->removeAllChildren();

    Rect panel = getPanelRect();
    const float contentLeft = panel.getMinX() + PANEL_INNER_PADDING_X;
    const float contentRight = panel.getMaxX() - PANEL_INNER_PADDING_X;
    const float contentWidth = contentRight - contentLeft;
    const float contentBottom = panel.getMinY() + PANEL_INNER_PADDING_BOTTOM;

    // 内容起始位置（Tab 下方）
    float contentTop = panel.getMaxY() - 130.0f;

    // 双栏布局
    const float colWidth = (contentWidth - COLUMN_GAP) / 2.0f;
    const float leftColX = contentLeft;
    const float rightColX = contentLeft + colWidth + COLUMN_GAP;

    if (!_player)
    {
        auto hint = Label::createWithTTF("未绑定玩家", FONT_PATH, 20);
        hint->setPosition(panel.getMidX(), panel.getMidY());
        _skillPage->addChild(hint, Z_TEXT);
        return;
    }

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        auto hint = Label::createWithTTF("玩家缺少 SkillComponent", FONT_PATH, 20);
        hint->setPosition(panel.getMidX(), panel.getMidY());
        _skillPage->addChild(hint, Z_TEXT);
        return;
    }

    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    _skillPage->addChild(menu, Z_MENU);

    // 左栏：主动技能槽位
    auto activeTitle = Label::createWithTTF("主动技能槽位", FONT_PATH, 24);
    activeTitle->setAnchorPoint(Vec2(0, 0.5f));
    activeTitle->setColor(SECTION_TITLE_COLOR);
    activeTitle->enableOutline(Color4B::BLACK, 1);
    activeTitle->setPosition(Vec2(leftColX, contentTop));
    _skillPage->addChild(activeTitle, Z_TEXT);

    // 右栏：被动技能槽位
    auto passiveTitle = Label::createWithTTF("被动技能槽位", FONT_PATH, 24);
    passiveTitle->setAnchorPoint(Vec2(0, 0.5f));
    passiveTitle->setColor(SECTION_TITLE_COLOR);
    passiveTitle->enableOutline(Color4B::BLACK, 1);
    passiveTitle->setPosition(Vec2(rightColX, contentTop));
    _skillPage->addChild(passiveTitle, Z_TEXT);

    float yActive = contentTop - 40.0f;
    float yPassive = contentTop - 40.0f;

    const auto &activeSlots = skillComp->getActiveSlots();
    const auto &passiveSlots = skillComp->getPassiveSlots();

    // 主动技能槽位
    const size_t activeSlotCount = static_cast<size_t>(GameConfig::UI::SKILL_BAR_SLOT_COUNT);
    for (size_t i = 0; i < activeSlotCount; ++i)
    {
        std::string name = (i < activeSlots.size() && activeSlots[i]) ? activeSlots[i]->name : "空";
        bool isSelected = (_selectedActiveSlotIndex == i);
        std::string prefix = isSelected ? "▶ " : "   ";
        std::string text = StringUtils::format("%s槽位%zu：%s", prefix.c_str(), i + 1, name.c_str());

        auto btn = createFixedWidthMenuItem(text, colWidth - 60, 18, [this, i](Ref *) {
            _selectedActiveSlotIndex = i;
            refresh();
        });
        if (btn)
        {
            if (auto label = dynamic_cast<Label *>(btn->getLabel()))
            {
                label->setColor(isSelected ? SELECTED_COLOR : ITEM_TEXT_COLOR);
            }
            btn->setPosition(Vec2(leftColX + (colWidth - 60) / 2, yActive));
            menu->addChild(btn);
        }

        // 卸下按钮
        if (i < activeSlots.size() && activeSlots[i])
        {
            auto unequipBtn = createMenuButton("卸下", [this, i](Ref *) {
                if (_player)
                {
                    if (auto comp = _player->getSkillComponent())
                    {
                        comp->unequipActiveSkill(i);
                    }
                    refresh();
                }
            }, 14);
            if (unequipBtn)
            {
                unequipBtn->setPosition(Vec2(leftColX + colWidth - 25, yActive));
                menu->addChild(unequipBtn);
            }
        }

        yActive -= 32.0f;
    }

    // 被动技能槽位
    const size_t passiveSlotCount = 3;
    for (size_t i = 0; i < passiveSlotCount; ++i)
    {
        std::string name = (i < passiveSlots.size() && passiveSlots[i]) ? passiveSlots[i]->name : "空";
        bool isSelected = (_selectedPassiveSlotIndex == i);
        std::string prefix = isSelected ? "▶ " : "   ";
        std::string text = StringUtils::format("%s槽位%zu：%s", prefix.c_str(), i + 1, name.c_str());

        auto btn = createFixedWidthMenuItem(text, colWidth - 60, 18, [this, i](Ref *) {
            _selectedPassiveSlotIndex = i;
            refresh();
        });
        if (btn)
        {
            if (auto label = dynamic_cast<Label *>(btn->getLabel()))
            {
                label->setColor(isSelected ? SELECTED_COLOR : ITEM_TEXT_COLOR);
            }
            btn->setPosition(Vec2(rightColX + (colWidth - 60) / 2, yPassive));
            menu->addChild(btn);
        }

        // 卸下按钮
        if (i < passiveSlots.size() && passiveSlots[i])
        {
            auto unequipBtn = createMenuButton("卸下", [this, i](Ref *) {
                if (_player)
                {
                    if (auto comp = _player->getSkillComponent())
                    {
                        comp->unequipPassiveSkill(i);
                    }
                    refresh();
                }
            }, 14);
            if (unequipBtn)
            {
                unequipBtn->setPosition(Vec2(rightColX + colWidth - 25, yPassive));
                menu->addChild(unequipBtn);
            }
        }

        yPassive -= 32.0f;
    }

    // 分隔线
    float listTop = std::min(yActive, yPassive) - 20.0f;
    auto separator = DrawNode::create();
    drawSeparator(separator,
                  Vec2(contentLeft, listTop + 10),
                  Vec2(contentRight, listTop + 10));
    _skillPage->addChild(separator, Z_TEXT);

    // 下半部分：可学习技能 / 已学习技能
    auto learnTitle = Label::createWithTTF("可学习技能", FONT_PATH, 22);
    learnTitle->setAnchorPoint(Vec2(0, 0.5f));
    learnTitle->setColor(SECTION_TITLE_COLOR);
    learnTitle->enableOutline(Color4B::BLACK, 1);
    learnTitle->setPosition(Vec2(leftColX, listTop));
    _skillPage->addChild(learnTitle, Z_TEXT);

    auto learnedTitle = Label::createWithTTF("已学习技能", FONT_PATH, 22);
    learnedTitle->setAnchorPoint(Vec2(0, 0.5f));
    learnedTitle->setColor(SECTION_TITLE_COLOR);
    learnedTitle->enableOutline(Color4B::BLACK, 1);
    learnedTitle->setPosition(Vec2(rightColX, listTop));
    _skillPage->addChild(learnedTitle, Z_TEXT);

    float listYLearn = listTop - 35.0f;
    float listYLearned = listTop - 35.0f;

    // 可学习技能列表
    int learnShown = 0;
    for (size_t i = 0; i < _skillTemplates.size(); ++i)
    {
        if (listYLearn < contentBottom)
        {
            break;
        }

        const auto &t = _skillTemplates[i];
        if (skillComp->findLearnedSkillById(t.id))
        {
            continue;
        }

        std::string extra;
        if (t.isPassive)
        {
            extra = formatAttributes(t.attributeBonus);
        }
        else
        {
            extra = StringUtils::format("CD%.1f MP%.0f", t.cooldown, t.manaCost);
        }

        std::string text = t.name;
        auto btn = createFixedWidthMenuItem(text, colWidth - 10, 18, [this, id = t.id](Ref *) {
            if (!_player)
            {
                return;
            }
            auto comp = _player->getSkillComponent();
            if (!comp || comp->findLearnedSkillById(id))
            {
                return;
            }

            for (const auto &tpl : _skillTemplates)
            {
                if (tpl.id != id)
                {
                    continue;
                }

                if (tpl.isPassive)
                {
                    auto s = std::make_shared<PassiveSkill>();
                    s->id = tpl.id;
                    s->name = tpl.name;
                    s->description = tpl.description;
                    s->isPassive = true;
                    s->attributeBonus = tpl.attributeBonus;
                    comp->learnSkill(s);
                }
                else
                {
                    auto s = std::make_shared<ActiveSkill>();
                    s->id = tpl.id;
                    s->name = tpl.name;
                    s->description = tpl.description;
                    s->isPassive = false;
                    s->cooldown = tpl.cooldown;
                    s->manaCost = tpl.manaCost;
                    s->currentCooldown = 0.0f;
                    comp->learnSkill(s);
                }
                break;
            }

            refresh();
        });
        if (btn)
        {
            btn->setPosition(Vec2(leftColX + (colWidth - 10) / 2, listYLearn));
            menu->addChild(btn);
        }

        // 显示技能信息
        if (!extra.empty())
        {
            auto infoLabel = Label::createWithTTF(extra, FONT_PATH, 14);
            infoLabel->setAnchorPoint(Vec2(0, 0.5f));
            infoLabel->setColor(ITEM_ATTR_COLOR);
            infoLabel->setPosition(Vec2(leftColX + 10, listYLearn - 22));
            _skillPage->addChild(infoLabel, Z_TEXT);
            listYLearn -= 22.0f;
        }

        listYLearn -= 32.0f;
        learnShown++;
    }

    if (learnShown == 0)
    {
        auto none = Label::createWithTTF("（暂无）", FONT_PATH, 16);
        none->setAnchorPoint(Vec2(0, 0.5f));
        none->setColor(Color3B(120, 120, 120));
        none->setPosition(Vec2(leftColX, listYLearn));
        _skillPage->addChild(none, Z_TEXT);
    }

    // 已学习技能列表
    int learnedShown = 0;
    const auto &learned = skillComp->getLearnedSkills();
    for (size_t i = 0; i < learned.size(); ++i)
    {
        if (listYLearned < contentBottom)
        {
            break;
        }

        const auto &s = learned[i];
        if (!s)
        {
            continue;
        }

        std::string extra;
        if (s->isPassive)
        {
            auto p = std::dynamic_pointer_cast<PassiveSkill>(s);
            extra = p ? formatAttributes(p->attributeBonus) : "";
        }
        else
        {
            auto a = std::dynamic_pointer_cast<ActiveSkill>(s);
            if (a)
            {
                extra = StringUtils::format("CD%.1f MP%.0f", a->cooldown, a->manaCost);
            }
        }

        std::string text = s->name;
        auto btn = createFixedWidthMenuItem(text, colWidth - 10, 18, [this, id = s->id](Ref *) {
            if (!_player)
            {
                return;
            }
            auto comp = _player->getSkillComponent();
            if (!comp)
            {
                return;
            }

            auto skill = comp->findLearnedSkillById(id);
            if (!skill)
            {
                return;
            }

            if (skill->isPassive)
            {
                auto passive = std::dynamic_pointer_cast<PassiveSkill>(skill);
                if (passive)
                {
                    comp->equipPassiveSkill(passive, _selectedPassiveSlotIndex);
                }
            }
            else
            {
                auto active = std::dynamic_pointer_cast<ActiveSkill>(skill);
                if (active)
                {
                    comp->equipActiveSkill(active, _selectedActiveSlotIndex);
                }
            }

            refresh();
        });
        if (btn)
        {
            btn->setPosition(Vec2(rightColX + (colWidth - 10) / 2, listYLearned));
            menu->addChild(btn);
        }

        // 显示技能信息
        if (!extra.empty())
        {
            auto infoLabel = Label::createWithTTF(extra, FONT_PATH, 14);
            infoLabel->setAnchorPoint(Vec2(0, 0.5f));
            infoLabel->setColor(ITEM_ATTR_COLOR);
            infoLabel->setPosition(Vec2(rightColX + 10, listYLearned - 22));
            _skillPage->addChild(infoLabel, Z_TEXT);
            listYLearned -= 22.0f;
        }

        listYLearned -= 32.0f;
        learnedShown++;
    }

    if (learnedShown == 0)
    {
        auto none = Label::createWithTTF("（暂无）", FONT_PATH, 16);
        none->setAnchorPoint(Vec2(0, 0.5f));
        none->setColor(Color3B(120, 120, 120));
        none->setPosition(Vec2(rightColX, listYLearned));
        _skillPage->addChild(none, Z_TEXT);
    }
}

void InventoryLayer::onCloseClicked(Ref *)
{
    hide();
    if (_closeCallback)
    {
        _closeCallback();
    }
}

void InventoryLayer::onTabEquipmentClicked(Ref *)
{
    switchTab(Tab::EQUIPMENT);
}

void InventoryLayer::onTabSkillClicked(Ref *)
{
    switchTab(Tab::SKILL);
}
