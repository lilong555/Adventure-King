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
    const char *const PLACEHOLDER_ICON_PATH = "Sprites/Characters/Player/Klee/defalt/TNT.png";
    constexpr float PANEL_WIDTH = 1000.0f;
    constexpr float PANEL_HEIGHT = 650.0f;
    constexpr float PANEL_INNER_PADDING_X = 50.0f;      // 面板左右内边距
    constexpr float PANEL_INNER_PADDING_BOTTOM = 80.0f; // 给"返回"按钮预留空间
    constexpr float COLUMN_GAP = 30.0f;                 // 双栏间距
    constexpr float PANEL_CORNER_RADIUS = 12.0f;        // 面板圆角

    // 图标尺寸（占位）
    constexpr float ICON_TITLE_W = 240.0f;
    constexpr float ICON_TITLE_H = 42.0f;
    constexpr float ICON_TAB = 46.0f;
    constexpr float ICON_ITEM = 34.0f;
    constexpr float ICON_ACTION = 28.0f;

    // 详情图预览面板：放在右侧，不遮挡页面主体
    constexpr float DETAIL_PANEL_W = 260.0f;
    constexpr float DETAIL_PANEL_H = 260.0f;
    constexpr float DETAIL_PANEL_PADDING = 12.0f;

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
    auto closeBtn = createIconButton(Size(ICON_TAB, ICON_TAB), CC_CALLBACK_1(InventoryLayer::onCloseClicked, this),
                                     Color3B(220, 220, 220));
    _closeMenu = Menu::create(closeBtn, nullptr);
    _closeMenu->setPosition(Vec2::ZERO);
    _panelRoot->addChild(_closeMenu, Z_MENU);
    createPages();
    createDetailOverlay();

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
    hideDetailOverlay();
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
    _panelRoot->setContentSize(Size(PANEL_WIDTH, PANEL_HEIGHT));
    _panelRoot->setAnchorPoint(Vec2(0.5f, 0.5f));
    _panelRoot->setPosition(center);
    _container->addChild(_panelRoot, Z_PANEL);

    _panel = DrawNode::create();
    // Draw relative to _panelRoot (0,0 is bottom-left of panel)
    drawRoundedRect(_panel,
                    Vec2(0, 0),
                    Vec2(PANEL_WIDTH, PANEL_HEIGHT),
                    PANEL_BG_COLOR, PANEL_BORDER_COLOR, PANEL_CORNER_RADIUS);
    _panelRoot->addChild(_panel, Z_PANEL);

    // 标题区域背景
    auto titleBg = DrawNode::create();
    titleBg->drawSolidRect(
        Vec2(2, PANEL_HEIGHT - 70),
        Vec2(PANEL_WIDTH - 2, PANEL_HEIGHT - 2),
        Color4F(0.15f, 0.12f, 0.08f, 0.9f));
    _panelRoot->addChild(titleBg, Z_PANEL);

    _titleSprite = createPlaceholderSprite(Size(ICON_TITLE_W, ICON_TITLE_H), TITLE_COLOR);
    if (_titleSprite)
    {
        _titleSprite->setPosition(Vec2(PANEL_WIDTH / 2, PANEL_HEIGHT - 36));
        _panelRoot->addChild(_titleSprite, Z_TEXT);
    }

    // 标题下方分隔线
    auto separator = DrawNode::create();
    drawSeparator(separator,
                  Vec2(20, PANEL_HEIGHT - 70),
                  Vec2(PANEL_WIDTH - 20, PANEL_HEIGHT - 70));
    _panelRoot->addChild(separator, Z_TEXT);
}

void InventoryLayer::createTabs()
{
    // Tab 按钮样式改进
    _tabEquipment = createIconButton(Size(ICON_TAB, ICON_TAB), CC_CALLBACK_1(InventoryLayer::onTabEquipmentClicked, this));
    _tabSkill = createIconButton(Size(ICON_TAB, ICON_TAB), CC_CALLBACK_1(InventoryLayer::onTabSkillClicked, this));

    float tabY = PANEL_HEIGHT - 100;
    _tabEquipment->setPosition(Vec2(PANEL_WIDTH / 2 - 80, tabY));
    _tabSkill->setPosition(Vec2(PANEL_WIDTH / 2 + 80, tabY));

    _tabMenu = Menu::create(_tabEquipment, _tabSkill, nullptr);
    _tabMenu->setPosition(Vec2::ZERO);
    _panelRoot->addChild(_tabMenu, Z_MENU);
}

void InventoryLayer::createPages()
{
    _equipmentPage = Node::create();
    _equipmentPage->setContentSize(Size(PANEL_WIDTH, PANEL_HEIGHT));
    _equipmentPage->setAnchorPoint(Vec2::ZERO);
    _equipmentPage->setPosition(Vec2::ZERO);

    _skillPage = Node::create();
    _skillPage->setContentSize(Size(PANEL_WIDTH, PANEL_HEIGHT));
    _skillPage->setAnchorPoint(Vec2::ZERO);
    _skillPage->setPosition(Vec2::ZERO);

    _panelRoot->addChild(_equipmentPage, Z_TEXT);
    _panelRoot->addChild(_skillPage, Z_TEXT);

    // 关闭按钮放在面板底部居中
    if (_closeMenu && !_closeMenu->getChildren().empty())
    {
        _closeMenu->getChildren().at(0)->setPosition(Vec2(PANEL_WIDTH / 2, 45.0f));
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
        auto normal = dynamic_cast<Sprite *>(_tabEquipment->getNormalImage());
        if (normal)
        {
            normal->setColor(equipVisible ? SELECTED_COLOR : Color3B::WHITE);
        }
    }
    if (_tabSkill)
    {
        auto normal = dynamic_cast<Sprite *>(_tabSkill->getNormalImage());
        if (normal)
        {
            normal->setColor(!equipVisible ? SELECTED_COLOR : Color3B::WHITE);
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

Sprite *InventoryLayer::createPlaceholderSprite(const Size &targetSize, const Color3B &tint)
{
    auto sprite = Sprite::create(PLACEHOLDER_ICON_PATH);
    if (!sprite)
    {
        return nullptr;
    }

    sprite->setColor(tint);
    const auto size = sprite->getContentSize();
    if (size.width > 0.0f && size.height > 0.0f)
    {
        float scale = std::min(targetSize.width / size.width, targetSize.height / size.height);
        sprite->setScale(scale);
    }
    return sprite;
}

MenuItemSprite *InventoryLayer::createIconButton(const Size &targetSize,
                                                 const ccMenuCallback &callback,
                                                 const Color3B &tint)
{
    auto normal = createPlaceholderSprite(targetSize, tint);
    auto selected = createPlaceholderSprite(targetSize, tint);
    if (!normal || !selected)
    {
        return nullptr;
    }

    // 选中态略微放大，提供点击反馈
    selected->setScale(selected->getScale() * 1.08f);
    return MenuItemSprite::create(normal, selected, callback);
}

void InventoryLayer::createDetailOverlay()
{
    if (_detailOverlay || !_panelRoot)
    {
        return;
    }

    _detailOverlay = Node::create();
    _detailOverlay->setContentSize(Size(DETAIL_PANEL_W, DETAIL_PANEL_H));
    _detailOverlay->setAnchorPoint(Vec2::ZERO);
    // 放到面板右侧空白区域：与列表并排，不遮挡主要交互区域
    const float contentTop = PANEL_HEIGHT - 130.0f;
    const float x = PANEL_WIDTH - PANEL_INNER_PADDING_X - DETAIL_PANEL_W;
    const float y = std::max(PANEL_INNER_PADDING_BOTTOM, contentTop - DETAIL_PANEL_H);
    _detailOverlay->setPosition(Vec2(x, y));
    _detailOverlay->setVisible(false);
    _panelRoot->addChild(_detailOverlay, Z_MENU + 50);

    _detailOverlayBg = DrawNode::create();
    _detailOverlayBg->drawSolidRect(Vec2::ZERO, Vec2(DETAIL_PANEL_W, DETAIL_PANEL_H), Color4F(0.12f, 0.12f, 0.18f, 0.95f));
    _detailOverlayBg->drawRect(Vec2::ZERO, Vec2(DETAIL_PANEL_W, DETAIL_PANEL_H), PANEL_BORDER_COLOR);
    _detailOverlay->addChild(_detailOverlayBg, 0);

    _detailOverlaySprite = Sprite::create(PLACEHOLDER_ICON_PATH);
    if (_detailOverlaySprite)
    {
        _detailOverlaySprite->setPosition(Vec2(DETAIL_PANEL_W / 2, DETAIL_PANEL_H / 2));
        _detailOverlay->addChild(_detailOverlaySprite, 1);
    }

    // 仅吞噬预览面板区域内的触摸，避免点击穿透到背包列表
    _detailOverlayListener = EventListenerTouchOneByOne::create();
    _detailOverlayListener->setSwallowTouches(true);
    _detailOverlayListener->onTouchBegan = [this](Touch *touch, Event *) -> bool
    {
        if (!_detailOverlay || !_detailOverlay->isVisible())
        {
            return false;
        }
        const Vec2 local = _detailOverlay->convertToNodeSpace(touch->getLocation());
        const Rect rect(0, 0, _detailOverlay->getContentSize().width, _detailOverlay->getContentSize().height);
        return rect.containsPoint(local);
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_detailOverlayListener, _detailOverlay);
    _detailOverlayListener->setEnabled(false);
}

void InventoryLayer::showDetailOverlay()
{
    if (!_detailOverlay)
    {
        return;
    }

    _detailOverlay->setVisible(true);
    if (_detailOverlayListener)
    {
        _detailOverlayListener->setEnabled(true);
    }

    if (_detailOverlaySprite)
    {
        // 后续可替换为真实的“详情图”；目前仅用占位图展示。
        auto texture = Director::getInstance()->getTextureCache()->addImage(PLACEHOLDER_ICON_PATH);
        if (texture)
        {
            _detailOverlaySprite->setTexture(texture);
        }

        const float targetW = DETAIL_PANEL_W - DETAIL_PANEL_PADDING * 2.0f;
        const float targetH = DETAIL_PANEL_H - DETAIL_PANEL_PADDING * 2.0f;
        const auto size = _detailOverlaySprite->getContentSize();
        float scale = 1.0f;
        if (size.width > 0.0f && size.height > 0.0f)
        {
            scale = std::min(targetW / size.width, targetH / size.height);
        }
        _detailOverlaySprite->setScale(scale);
        _detailOverlaySprite->setPosition(Vec2(DETAIL_PANEL_W / 2, DETAIL_PANEL_H / 2));
    }
}

void InventoryLayer::hideDetailOverlay()
{
    if (_detailOverlay)
    {
        _detailOverlay->setVisible(false);
    }
    if (_detailOverlayListener)
    {
        _detailOverlayListener->setEnabled(false);
    }
}

void InventoryLayer::refreshEquipmentPage()
{
    if (!_equipmentPage)
    {
        return;
    }

    _equipmentPage->removeAllChildren();

    const float contentLeft = PANEL_INNER_PADDING_X;
    const float contentRight = PANEL_WIDTH - PANEL_INNER_PADDING_X;
    const float contentWidth = contentRight - contentLeft;
    const float contentBottom = PANEL_INNER_PADDING_BOTTOM;

    // 内容起始位置（Tab 下方）
    float contentTop = PANEL_HEIGHT - 130.0f;

    // 双栏布局：左侧装备槽位，右侧背包物品
    const float colWidth = (contentWidth - COLUMN_GAP) / 2.0f;
    const float leftColX = contentLeft;
    const float rightColX = contentLeft + colWidth + COLUMN_GAP;

    // 左栏：装备槽位（用图片占位，避免文字排版问题）
    if (auto equipTitle = createPlaceholderSprite(Size(120, 26), SECTION_TITLE_COLOR))
    {
        equipTitle->setAnchorPoint(Vec2(0, 0.5f));
        equipTitle->setPosition(Vec2(leftColX, contentTop));
        _equipmentPage->addChild(equipTitle, Z_TEXT);
    }

    // 右栏：背包物品（用图片占位）
    if (auto bagTitle = createPlaceholderSprite(Size(120, 26), SECTION_TITLE_COLOR))
    {
        bagTitle->setAnchorPoint(Vec2(0, 0.5f));
        bagTitle->setPosition(Vec2(rightColX, contentTop));
        _equipmentPage->addChild(bagTitle, Z_TEXT);
    }

    float yEquip = contentTop - 40.0f;
    float yBag = contentTop - 40.0f;

    // 玩家未绑定
    if (!_player)
    {
        if (auto hint = createPlaceholderSprite(Size(140, 30), Color3B(180, 180, 180)))
        {
            hint->setAnchorPoint(Vec2(0, 0.5f));
            hint->setPosition(Vec2(leftColX, yEquip));
            _equipmentPage->addChild(hint, Z_TEXT);
        }
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

    const float slotHeight = 42.0f;

    for (size_t i = 0; i < slotOrder.size(); ++i)
    {
        EquipmentSlot slot = slotOrder[i];
        auto eq = _player->getEquipment(slot);

        bool isSelected = (_selectedEquipSlotIndex == static_cast<int>(i));

        // 创建可点击的槽位按钮：全部用图片占位
        const Color3B slotTint = isSelected ? SELECTED_COLOR : (eq ? ITEM_TEXT_COLOR : Color3B(120, 120, 120));
        auto slotBtn = createIconButton(Size(ICON_ITEM, ICON_ITEM), [this, i](Ref *) {
            // 切换选中状态
            if (_selectedEquipSlotIndex == static_cast<int>(i))
            {
                _selectedEquipSlotIndex = -1;
            }
            else
            {
                _selectedEquipSlotIndex = static_cast<int>(i);
            }
            showDetailOverlay();
            refresh();
        }, slotTint);

        if (slotBtn)
        {
            slotBtn->setPosition(Vec2(leftColX + ICON_ITEM / 2, yEquip));
            menu->addChild(slotBtn);
        }

        // 卸下按钮
        if (eq)
        {
            auto unequipBtn = createIconButton(Size(ICON_ACTION, ICON_ACTION), [this, slot](Ref *) {
                if (_player)
                {
                    _player->unequip(slot);
                    _selectedEquipSlotIndex = -1;
                    showDetailOverlay();
                    refresh();
                }
            }, Color3B(255, 120, 120));
            if (unequipBtn)
            {
                unequipBtn->setPosition(Vec2(leftColX + colWidth - ICON_ACTION / 2, yEquip));
                menu->addChild(unequipBtn);
            }
        }

        yEquip -= slotHeight;

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

        const Color3B itemTint = isEquipped ? SELECTED_COLOR : ITEM_TEXT_COLOR;
        auto btn = createIconButton(Size(ICON_ITEM, ICON_ITEM), [this, itemId = item->id](Ref *) {
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
                    showDetailOverlay();
                    refresh();
                    break;
                }
            }
        }, itemTint);
        if (!btn)
        {
            continue;
        }

        btn->setPosition(Vec2(rightColX + ICON_ITEM / 2, yBag));
        menu->addChild(btn);

        yBag -= slotHeight;
        shown++;
    }

    if (shown == 0)
    {
        if (auto empty = createPlaceholderSprite(Size(120, 26), Color3B(120, 120, 120)))
        {
            empty->setAnchorPoint(Vec2(0, 0.5f));
            empty->setPosition(Vec2(rightColX, yBag));
            _equipmentPage->addChild(empty, Z_TEXT);
        }
    }
}

void InventoryLayer::refreshSkillPage()
{
    if (!_skillPage)
    {
        return;
    }

    _skillPage->removeAllChildren();

    const float contentLeft = PANEL_INNER_PADDING_X;
    const float contentRight = PANEL_WIDTH - PANEL_INNER_PADDING_X;
    const float contentWidth = contentRight - contentLeft;
    const float contentBottom = PANEL_INNER_PADDING_BOTTOM;

    // 内容起始位置（Tab 下方）
    float contentTop = PANEL_HEIGHT - 130.0f;

    // 双栏布局
    const float colWidth = (contentWidth - COLUMN_GAP) / 2.0f;
    const float leftColX = contentLeft;
    const float rightColX = contentLeft + colWidth + COLUMN_GAP;

    if (!_player)
    {
        if (auto hint = createPlaceholderSprite(Size(140, 30), Color3B(180, 180, 180)))
        {
            hint->setPosition(Vec2(PANEL_WIDTH / 2, PANEL_HEIGHT / 2));
            _skillPage->addChild(hint, Z_TEXT);
        }
        return;
    }

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        if (auto hint = createPlaceholderSprite(Size(180, 30), Color3B(180, 180, 180)))
        {
            hint->setPosition(Vec2(PANEL_WIDTH / 2, PANEL_HEIGHT / 2));
            _skillPage->addChild(hint, Z_TEXT);
        }
        return;
    }

    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    _skillPage->addChild(menu, Z_MENU);

    // 左栏：主动技能槽位（图片占位）
    if (auto activeTitle = createPlaceholderSprite(Size(140, 26), SECTION_TITLE_COLOR))
    {
        activeTitle->setAnchorPoint(Vec2(0, 0.5f));
        activeTitle->setPosition(Vec2(leftColX, contentTop));
        _skillPage->addChild(activeTitle, Z_TEXT);
    }

    // 右栏：被动技能槽位（图片占位）
    if (auto passiveTitle = createPlaceholderSprite(Size(140, 26), SECTION_TITLE_COLOR))
    {
        passiveTitle->setAnchorPoint(Vec2(0, 0.5f));
        passiveTitle->setPosition(Vec2(rightColX, contentTop));
        _skillPage->addChild(passiveTitle, Z_TEXT);
    }

    float yActive = contentTop - 40.0f;
    float yPassive = contentTop - 40.0f;

    const auto &activeSlots = skillComp->getActiveSlots();
    const auto &passiveSlots = skillComp->getPassiveSlots();

    // 主动技能槽位
    const size_t activeSlotCount = static_cast<size_t>(GameConfig::UI::SKILL_BAR_SLOT_COUNT);
    for (size_t i = 0; i < activeSlotCount; ++i)
    {
        const bool hasSkill = (i < activeSlots.size() && activeSlots[i]);
        const bool isSelected = (_selectedActiveSlotIndex == i);
        const Color3B slotTint = isSelected ? SELECTED_COLOR : (hasSkill ? ITEM_TEXT_COLOR : Color3B(120, 120, 120));

        auto btn = createIconButton(Size(ICON_ITEM, ICON_ITEM), [this, i](Ref *) {
            _selectedActiveSlotIndex = i;
            showDetailOverlay();
            refresh();
        }, slotTint);
        if (btn)
        {
            btn->setPosition(Vec2(leftColX + ICON_ITEM / 2, yActive));
            menu->addChild(btn);
        }

        // 卸下按钮
        if (hasSkill)
        {
            auto unequipBtn = createIconButton(Size(ICON_ACTION, ICON_ACTION), [this, i](Ref *) {
                if (_player)
                {
                    if (auto comp = _player->getSkillComponent())
                    {
                        comp->unequipActiveSkill(i);
                    }
                    showDetailOverlay();
                    refresh();
                }
            }, Color3B(255, 120, 120));
            if (unequipBtn)
            {
                unequipBtn->setPosition(Vec2(leftColX + colWidth - ICON_ACTION / 2, yActive));
                menu->addChild(unequipBtn);
            }
        }

        yActive -= 42.0f;
    }

    // 被动技能槽位
    const size_t passiveSlotCount = 3;
    for (size_t i = 0; i < passiveSlotCount; ++i)
    {
        const bool hasSkill = (i < passiveSlots.size() && passiveSlots[i]);
        const bool isSelected = (_selectedPassiveSlotIndex == i);
        const Color3B slotTint = isSelected ? SELECTED_COLOR : (hasSkill ? ITEM_TEXT_COLOR : Color3B(120, 120, 120));

        auto btn = createIconButton(Size(ICON_ITEM, ICON_ITEM), [this, i](Ref *) {
            _selectedPassiveSlotIndex = i;
            showDetailOverlay();
            refresh();
        }, slotTint);
        if (btn)
        {
            btn->setPosition(Vec2(rightColX + ICON_ITEM / 2, yPassive));
            menu->addChild(btn);
        }

        // 卸下按钮
        if (hasSkill)
        {
            auto unequipBtn = createIconButton(Size(ICON_ACTION, ICON_ACTION), [this, i](Ref *) {
                if (_player)
                {
                    if (auto comp = _player->getSkillComponent())
                    {
                        comp->unequipPassiveSkill(i);
                    }
                    showDetailOverlay();
                    refresh();
                }
            }, Color3B(255, 120, 120));
            if (unequipBtn)
            {
                // 右侧预览面板占用了右栏的一部分宽度，这里把“卸下”按钮左移避免被覆盖
                unequipBtn->setPosition(Vec2(rightColX + (colWidth - DETAIL_PANEL_W) - ICON_ACTION / 2, yPassive));
                menu->addChild(unequipBtn);
            }
        }

        yPassive -= 42.0f;
    }

    // 分隔线
    float listTop = std::min(yActive, yPassive) - 20.0f;
    auto separator = DrawNode::create();
    drawSeparator(separator,
                  Vec2(contentLeft, listTop + 10),
                  Vec2(contentRight, listTop + 10));
    _skillPage->addChild(separator, Z_TEXT);

    // 下半部分：可学习技能 / 已学习技能（图片占位）
    if (auto learnTitle = createPlaceholderSprite(Size(120, 24), SECTION_TITLE_COLOR))
    {
        learnTitle->setAnchorPoint(Vec2(0, 0.5f));
        learnTitle->setPosition(Vec2(leftColX, listTop));
        _skillPage->addChild(learnTitle, Z_TEXT);
    }

    if (auto learnedTitle = createPlaceholderSprite(Size(120, 24), SECTION_TITLE_COLOR))
    {
        learnedTitle->setAnchorPoint(Vec2(0, 0.5f));
        learnedTitle->setPosition(Vec2(rightColX, listTop));
        _skillPage->addChild(learnedTitle, Z_TEXT);
    }

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

        const Color3B skillTint = t.isPassive ? ITEM_ATTR_COLOR : ITEM_TEXT_COLOR;
        auto btn = createIconButton(Size(ICON_ITEM, ICON_ITEM), [this, id = t.id](Ref *) {
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

            showDetailOverlay();
            refresh();
        }, skillTint);
        if (btn)
        {
            btn->setPosition(Vec2(leftColX + ICON_ITEM / 2, listYLearn));
            menu->addChild(btn);
        }
        listYLearn -= 44.0f;
        learnShown++;
    }

    if (learnShown == 0)
    {
        if (auto none = createPlaceholderSprite(Size(120, 24), Color3B(120, 120, 120)))
        {
            none->setAnchorPoint(Vec2(0, 0.5f));
            none->setPosition(Vec2(leftColX, listYLearn));
            _skillPage->addChild(none, Z_TEXT);
        }
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

        const Color3B skillTint = s->isPassive ? ITEM_ATTR_COLOR : ITEM_TEXT_COLOR;
        auto btn = createIconButton(Size(ICON_ITEM, ICON_ITEM), [this, id = s->id](Ref *) {
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

            showDetailOverlay();
            refresh();
        }, skillTint);
        if (btn)
        {
            btn->setPosition(Vec2(rightColX + ICON_ITEM / 2, listYLearned));
            menu->addChild(btn);
        }
        listYLearned -= 44.0f;
        learnedShown++;
    }

    if (learnedShown == 0)
    {
        if (auto none = createPlaceholderSprite(Size(120, 24), Color3B(120, 120, 120)))
        {
            none->setAnchorPoint(Vec2(0, 0.5f));
            none->setPosition(Vec2(rightColX, listYLearned));
            _skillPage->addChild(none, Z_TEXT);
        }
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
