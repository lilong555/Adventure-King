/**
 * @file InventoryLayer.cpp
 * @brief 背包/技能管理界面实现（占位 UI）
 */

#include "UI/InventoryLayer.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/SkillComponent.h"
#include "Configs/GameConfigs.h"

USING_NS_CC;

namespace
{
    const char *const FONT_PATH = "fonts/ZCOOLKuaiLe-Regular.ttf";
    constexpr float PANEL_WIDTH = 980.0f;
    constexpr float PANEL_HEIGHT = 620.0f;

    constexpr int Z_BACKGROUND = 0;
    constexpr int Z_PANEL = 1;
    constexpr int Z_TEXT = 2;
    constexpr int Z_MENU = 3;
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
    _touchListener = EventListenerTouchOneByOne::create();
    _touchListener->setSwallowTouches(true);
    _touchListener->onTouchBegan = [this](Touch *, Event *) -> bool
    {
        return _isShowing;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_touchListener, this);
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
    _panel->drawSolidRect(
        Vec2(center.x - PANEL_WIDTH / 2, center.y - PANEL_HEIGHT / 2),
        Vec2(center.x + PANEL_WIDTH / 2, center.y + PANEL_HEIGHT / 2),
        Color4F(0.10f, 0.10f, 0.14f, 0.97f));
    _panel->drawRect(
        Vec2(center.x - PANEL_WIDTH / 2, center.y - PANEL_HEIGHT / 2),
        Vec2(center.x + PANEL_WIDTH / 2, center.y + PANEL_HEIGHT / 2),
        Color4F(0.45f, 0.45f, 0.55f, 1.0f));
    _panelRoot->addChild(_panel, Z_PANEL);

    _titleLabel = Label::createWithTTF("背包 / 技能", FONT_PATH, 34);
    _titleLabel->setPosition(Vec2(center.x, center.y + PANEL_HEIGHT / 2 - 40));
    _titleLabel->setColor(Color3B(255, 220, 100));
    _titleLabel->enableOutline(Color4B::BLACK, 2);
    _panelRoot->addChild(_titleLabel, Z_TEXT);

    // 关闭按钮位置
    // 关闭按钮由 createPages() 统一定位
}

void InventoryLayer::createTabs()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    _tabEquipment = createMenuButton("装备", CC_CALLBACK_1(InventoryLayer::onTabEquipmentClicked, this), 24);
    _tabSkill = createMenuButton("技能", CC_CALLBACK_1(InventoryLayer::onTabSkillClicked, this), 24);

    _tabEquipment->setPosition(Vec2(center.x - 60, center.y + PANEL_HEIGHT / 2 - 95));
    _tabSkill->setPosition(Vec2(center.x + 60, center.y + PANEL_HEIGHT / 2 - 95));

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
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);
    if (_closeMenu && !_closeMenu->getChildren().empty())
    {
        _closeMenu->getChildren().at(0)->setPosition(Vec2(center.x, center.y - PANEL_HEIGHT / 2 + 45));
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
    return MenuItemLabel::create(label, callback);
}

void InventoryLayer::refreshEquipmentPage()
{
    if (!_equipmentPage)
    {
        return;
    }

    _equipmentPage->removeAllChildren();

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    // 标题
    auto title = Label::createWithTTF("装备管理", FONT_PATH, 26);
    title->setPosition(Vec2(center.x - PANEL_WIDTH / 2 + 90, center.y + PANEL_HEIGHT / 2 - 150));
    title->setAnchorPoint(Vec2(0, 0.5f));
    title->setColor(Color3B(200, 220, 255));
    _equipmentPage->addChild(title, Z_TEXT);

    // 玩家未绑定
    if (!_player)
    {
        auto hint = Label::createWithTTF("未绑定玩家，无法显示背包数据", FONT_PATH, 22);
        hint->setPosition(Vec2(center.x, center.y));
        _equipmentPage->addChild(hint, Z_TEXT);
        return;
    }

    // 穿戴槽位展示
    const float slotStartX = center.x - PANEL_WIDTH / 2 + 90;
    const float slotStartY = center.y + PANEL_HEIGHT / 2 - 200;
    const float slotLineSpacing = 34.0f;

    std::vector<EquipmentSlot> slotOrder = {
        EquipmentSlot::WEAPON,
        EquipmentSlot::HELMET,
        EquipmentSlot::ARMOR,
        EquipmentSlot::BOOTS,
    };

    auto unequipMenu = Menu::create();
    unequipMenu->setPosition(Vec2::ZERO);
    _equipmentPage->addChild(unequipMenu, Z_MENU);

    for (size_t i = 0; i < slotOrder.size(); ++i)
    {
        EquipmentSlot slot = slotOrder[i];
        auto eq = _player->getEquipment(slot);
        std::string eqName = eq ? eq->name : "空";
        std::string attrs = eq ? formatAttributes(eq->attributeBonus) : "";
        std::string line = StringUtils::format("%s：%s%s%s",
                                               getEquipmentSlotName(slot).c_str(),
                                               eqName.c_str(),
                                               attrs.empty() ? "" : "  (",
                                               attrs.empty() ? "" : attrs.c_str());
        if (!attrs.empty())
        {
            line += ")";
        }

        auto label = Label::createWithTTF(line, FONT_PATH, 22);
        label->setAnchorPoint(Vec2(0, 0.5f));
        label->setPosition(Vec2(slotStartX, slotStartY - i * slotLineSpacing));
        _equipmentPage->addChild(label, Z_TEXT);

        auto btn = createMenuButton("卸下", [this, slot](Ref *) {
            if (_player)
            {
                _player->unequip(slot);
                refresh();
            }
        }, 20);
        btn->setPosition(Vec2(center.x + PANEL_WIDTH / 2 - 130, slotStartY - i * slotLineSpacing));
        unequipMenu->addChild(btn);
    }

    // 背包列表
    auto listTitle = Label::createWithTTF("背包物品（点击穿戴）", FONT_PATH, 24);
    listTitle->setPosition(Vec2(center.x - PANEL_WIDTH / 2 + 90, center.y + 60));
    listTitle->setAnchorPoint(Vec2(0, 0.5f));
    listTitle->setColor(Color3B(200, 220, 255));
    _equipmentPage->addChild(listTitle, Z_TEXT);

    auto itemsMenu = Menu::create();
    itemsMenu->setPosition(Vec2::ZERO);
    _equipmentPage->addChild(itemsMenu, Z_MENU);

    const auto &items = _player->getInventoryItems();
    float listX = center.x - PANEL_WIDTH / 2 + 90;
    float listY = center.y + 20;
    const float listSpacing = 30.0f;

    int shown = 0;
    const int maxShow = 12; // 占位：后续可做滚动列表
    for (size_t i = 0; i < items.size() && shown < maxShow; ++i)
    {
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

        std::string attrs = formatAttributes(item->attributeBonus);
        std::string text = StringUtils::format("[%s] %s%s%s",
                                               getEquipmentSlotName(item->slot).c_str(),
                                               item->name.c_str(),
                                               isEquipped ? "（已穿戴）" : "",
                                               attrs.empty() ? "" : ("  " + attrs).c_str());

        auto btn = createMenuButton(text, [this, itemId = item->id](Ref *) {
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
        }, 20);
        btn->setAnchorPoint(Vec2(0, 0.5f));
        btn->setPosition(Vec2(listX, listY - shown * listSpacing));
        itemsMenu->addChild(btn);
        shown++;
    }

    if (shown == 0)
    {
        auto empty = Label::createWithTTF("（背包为空）", FONT_PATH, 20);
        empty->setAnchorPoint(Vec2(0, 0.5f));
        empty->setPosition(Vec2(listX, listY));
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

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2);

    auto title = Label::createWithTTF("技能管理", FONT_PATH, 26);
    title->setPosition(Vec2(center.x - PANEL_WIDTH / 2 + 90, center.y + PANEL_HEIGHT / 2 - 150));
    title->setAnchorPoint(Vec2(0, 0.5f));
    title->setColor(Color3B(200, 220, 255));
    _skillPage->addChild(title, Z_TEXT);

    if (!_player)
    {
        auto hint = Label::createWithTTF("未绑定玩家，无法显示技能数据", FONT_PATH, 22);
        hint->setPosition(Vec2(center.x, center.y));
        _skillPage->addChild(hint, Z_TEXT);
        return;
    }

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        auto hint = Label::createWithTTF("玩家缺少 SkillComponent，无法管理技能", FONT_PATH, 22);
        hint->setPosition(Vec2(center.x, center.y));
        _skillPage->addChild(hint, Z_TEXT);
        return;
    }

    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    _skillPage->addChild(menu, Z_MENU);

    // 主动技能槽位
    auto activeTitle = Label::createWithTTF("主动槽位（点击选择）", FONT_PATH, 22);
    activeTitle->setAnchorPoint(Vec2(0, 0.5f));
    activeTitle->setPosition(Vec2(center.x - PANEL_WIDTH / 2 + 90, center.y + 160));
    _skillPage->addChild(activeTitle, Z_TEXT);

    const auto &activeSlots = skillComp->getActiveSlots();
    const size_t activeSlotCount = static_cast<size_t>(GameConfig::UI::SKILL_BAR_SLOT_COUNT);
    for (size_t i = 0; i < activeSlotCount; ++i)
    {
        std::string name = (i < activeSlots.size() && activeSlots[i]) ? activeSlots[i]->name : "空";
        std::string text = StringUtils::format("%s %zu：%s",
                                               (_selectedActiveSlotIndex == i) ? "▶" : "  ",
                                               i + 1,
                                               name.c_str());
        auto btn = createMenuButton(text, [this, i](Ref *) {
            _selectedActiveSlotIndex = i;
            refresh();
        }, 20);
        btn->setAnchorPoint(Vec2(0, 0.5f));
        btn->setPosition(Vec2(center.x - PANEL_WIDTH / 2 + 90, center.y + 130 - i * 26.0f));
        menu->addChild(btn);
    }

    auto unequipActiveBtn = createMenuButton("卸下选中主动槽", [this](Ref *) {
        if (_player)
        {
            if (auto comp = _player->getSkillComponent())
            {
                comp->unequipActiveSkill(_selectedActiveSlotIndex);
            }
            refresh();
        }
    }, 20);
    unequipActiveBtn->setAnchorPoint(Vec2(0, 0.5f));
    unequipActiveBtn->setPosition(Vec2(center.x - PANEL_WIDTH / 2 + 90, center.y + 130 - activeSlotCount * 26.0f - 10));
    menu->addChild(unequipActiveBtn);

    // 被动技能槽位
    auto passiveTitle = Label::createWithTTF("被动槽位（点击选择）", FONT_PATH, 22);
    passiveTitle->setAnchorPoint(Vec2(0, 0.5f));
    passiveTitle->setPosition(Vec2(center.x + 120, center.y + 160));
    _skillPage->addChild(passiveTitle, Z_TEXT);

    const auto &passiveSlots = skillComp->getPassiveSlots();
    const size_t passiveSlotCount = 3;
    for (size_t i = 0; i < passiveSlotCount; ++i)
    {
        std::string name = (i < passiveSlots.size() && passiveSlots[i]) ? passiveSlots[i]->name : "空";
        std::string text = StringUtils::format("%s %zu：%s",
                                               (_selectedPassiveSlotIndex == i) ? "▶" : "  ",
                                               i + 1,
                                               name.c_str());
        auto btn = createMenuButton(text, [this, i](Ref *) {
            _selectedPassiveSlotIndex = i;
            refresh();
        }, 20);
        btn->setAnchorPoint(Vec2(0, 0.5f));
        btn->setPosition(Vec2(center.x + 120, center.y + 130 - i * 26.0f));
        menu->addChild(btn);
    }

    auto unequipPassiveBtn = createMenuButton("卸下选中被动槽", [this](Ref *) {
        if (_player)
        {
            if (auto comp = _player->getSkillComponent())
            {
                comp->unequipPassiveSkill(_selectedPassiveSlotIndex);
            }
            refresh();
        }
    }, 20);
    unequipPassiveBtn->setAnchorPoint(Vec2(0, 0.5f));
    unequipPassiveBtn->setPosition(Vec2(center.x + 120, center.y + 130 - passiveSlotCount * 26.0f - 10));
    menu->addChild(unequipPassiveBtn);

    // 可学习技能
    auto learnTitle = Label::createWithTTF("可学习技能（点击学习）", FONT_PATH, 22);
    learnTitle->setAnchorPoint(Vec2(0, 0.5f));
    learnTitle->setPosition(Vec2(center.x - PANEL_WIDTH / 2 + 90, center.y + 20));
    _skillPage->addChild(learnTitle, Z_TEXT);

    float learnX = center.x - PANEL_WIDTH / 2 + 90;
    float learnY = center.y - 10;
    float learnSpacing = 28.0f;

    int learnShown = 0;
    for (size_t i = 0; i < _skillTemplates.size(); ++i)
    {
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

        std::string text = StringUtils::format("%s（%s）",
                                               t.name.c_str(),
                                               extra.c_str());
        auto btn = createMenuButton(text, [this, id = t.id](Ref *) {
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
        }, 20);
        btn->setAnchorPoint(Vec2(0, 0.5f));
        btn->setPosition(Vec2(learnX, learnY - learnShown * learnSpacing));
        menu->addChild(btn);
        learnShown++;
    }

    if (learnShown == 0)
    {
        auto none = Label::createWithTTF("（暂无可学习技能）", FONT_PATH, 20);
        none->setAnchorPoint(Vec2(0, 0.5f));
        none->setPosition(Vec2(learnX, learnY));
        _skillPage->addChild(none, Z_TEXT);
    }

    // 已学习技能（点击装备到选中槽）
    auto learnedTitle = Label::createWithTTF("已学习技能（点击装备）", FONT_PATH, 22);
    learnedTitle->setAnchorPoint(Vec2(0, 0.5f));
    learnedTitle->setPosition(Vec2(center.x - PANEL_WIDTH / 2 + 90, center.y - 150));
    _skillPage->addChild(learnedTitle, Z_TEXT);

    float learnedX = center.x - PANEL_WIDTH / 2 + 90;
    float learnedY = center.y - 180;
    float learnedSpacing = 28.0f;

    int learnedShown = 0;
    const auto &learned = skillComp->getLearnedSkills();
    for (size_t i = 0; i < learned.size(); ++i)
    {
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

        std::string text = StringUtils::format("%s%s%s",
                                               s->name.c_str(),
                                               extra.empty() ? "" : "（",
                                               extra.empty() ? "" : extra.c_str());
        if (!extra.empty())
        {
            text += "）";
        }

        auto btn = createMenuButton(text, [this, id = s->id](Ref *) {
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
        }, 20);
        btn->setAnchorPoint(Vec2(0, 0.5f));
        btn->setPosition(Vec2(learnedX, learnedY - learnedShown * learnedSpacing));
        menu->addChild(btn);
        learnedShown++;

        if (learnedShown >= 5)
        {
            // 占位：后续可做滚动
            break;
        }
    }

    if (learnedShown == 0)
    {
        auto none = Label::createWithTTF("（暂无已学习技能）", FONT_PATH, 20);
        none->setAnchorPoint(Vec2(0, 0.5f));
        none->setPosition(Vec2(learnedX, learnedY));
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
