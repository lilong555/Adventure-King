/**
 * @file InventoryLayer.cpp
 * @brief 仓库界面实现（技能/装备/属性，占位 UI）
 */

#include "UI/InventoryLayer.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/SkillComponent.h"
#include "Character/components/AttributeComponent.h"
#include "Configs/GameConfigs.h"
#include "ui/CocosGUI.h"
#include <algorithm>

USING_NS_CC;

namespace
{
    const char *const PLACEHOLDER_ICON_PATH = "Sprites/Characters/Player/Klee/defalt/TNT.png";
    // 设计分辨率（截图为 2560x1440）：所有布局基于该坐标系，再整体缩放适配不同分辨率
    constexpr float DESIGN_WIDTH = 2560.0f;
    constexpr float DESIGN_HEIGHT = 1440.0f;

    // 安全边距（设计坐标）
    constexpr float SAFE_MARGIN_X = 120.0f;
    constexpr float SAFE_MARGIN_Y = 100.0f;

    // 图标尺寸（占位）
    constexpr float ICON_TAB = 110.0f;      // 顶部入口图标
    constexpr float ICON_TAB_LABEL_W = 96;  // 顶部入口“文字”占位（PNG）
    constexpr float ICON_TAB_LABEL_H = 28;
    constexpr float ICON_ITEM = 64.0f;      // 列表/槽位图标
    constexpr float ICON_ACTION = 48.0f;    // 操作按钮图标
    constexpr float ICON_CLOSE = 70.0f;     // 返回按钮图标

    // 详情图预览面板：放在右侧，不遮挡页面主体（设计坐标）
    constexpr float DETAIL_PANEL_W = 720.0f;
    constexpr float DETAIL_PANEL_H = 360.0f;
    constexpr float DETAIL_PANEL_PADDING = 26.0f;

    constexpr int Z_BACKGROUND = 0;
    constexpr int Z_ROOT = 1;
    constexpr int Z_PAGE = 2;
    constexpr int Z_UI = 3;

    // 颜色主题
    const Color4F PANEL_BG_COLOR = Color4F(0.08f, 0.08f, 0.12f, 0.92f);
    const Color4F PANEL_BORDER_COLOR = Color4F(0.5f, 0.45f, 0.35f, 0.95f);
    const Color3B TITLE_COLOR = Color3B(255, 215, 80);
    const Color3B SECTION_TITLE_COLOR = Color3B(180, 200, 230);
    const Color3B ITEM_TEXT_COLOR = Color3B(230, 230, 230);
    const Color3B ITEM_ATTR_COLOR = Color3B(160, 180, 160);
    const Color3B SELECTED_COLOR = Color3B(255, 200, 100);

    // 绘制面板（简化：实心矩形 + 描边）
    void drawPanelRect(DrawNode *node, const Rect &rect, const Color4F &fillColor, const Color4F &borderColor)
    {
        if (!node)
        {
            return;
        }
        const Vec2 origin = rect.origin;
        const Vec2 destination = rect.origin + Vec2(rect.size.width, rect.size.height);
        node->drawSolidRect(origin, destination, fillColor);
        node->drawRect(origin, destination, borderColor);
    }

    // 绘制分隔线
    void drawSeparator(DrawNode *node, const Vec2 &start, const Vec2 &end,
                       const Color4F &color = Color4F(0.3f, 0.3f, 0.35f, 0.8f))
    {
        node->drawLine(start, end, color);
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
    createPages();
    createDetailOverlay();

    // 返回按钮（右下角，布局参考截图）
    _closeButton = createIconButton(Size(ICON_CLOSE, ICON_CLOSE),
                                    CC_CALLBACK_1(InventoryLayer::onCloseClicked, this),
                                    Color3B(220, 220, 220));
    if (_closeButton && _panelRoot)
    {
        _closeButton->setAnchorPoint(Vec2(1.0f, 0.5f));
        _closeButton->setPosition(Vec2(DESIGN_WIDTH - SAFE_MARGIN_X, SAFE_MARGIN_Y * 0.55f));
        _panelRoot->addChild(_closeButton, Z_UI);
    }

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

    // 以截图的 2560x1440 为设计坐标系：在不同分辨率下等比缩放并居中
    const float scaleX = visibleSize.width / DESIGN_WIDTH;
    const float scaleY = visibleSize.height / DESIGN_HEIGHT;
    const float scale = std::min(scaleX, scaleY);

    const Vec2 designOrigin(
        origin.x + (visibleSize.width - DESIGN_WIDTH * scale) * 0.5f,
        origin.y + (visibleSize.height - DESIGN_HEIGHT * scale) * 0.5f);

    _panelRoot = Node::create();
    _panelRoot->setContentSize(Size(DESIGN_WIDTH, DESIGN_HEIGHT));
    _panelRoot->setAnchorPoint(Vec2::ZERO);
    _panelRoot->setPosition(designOrigin);
    _panelRoot->setScale(scale);
    _container->addChild(_panelRoot, Z_ROOT);

    // 预留：后续可替换为更精致的背景纹理；目前由各子页面自行绘制面板区域
    _panel = nullptr;
    _titleSprite = nullptr;
}

void InventoryLayer::createTabs()
{
    if (!_panelRoot)
    {
        return;
    }

    // 顶部“仓库入口”三分支：技能 / 装备 / 属性（布局参考截图，全部使用 PNG 占位）
    const float tabY = DESIGN_HEIGHT - SAFE_MARGIN_Y * 0.55f;
    const float centerX = DESIGN_WIDTH * 0.5f;
    const float spacing = 200.0f;

    _tabSkill = createIconButton(Size(ICON_TAB, ICON_TAB),
                                 CC_CALLBACK_1(InventoryLayer::onTabSkillClicked, this),
                                 Color3B(200, 220, 255));
    _tabEquipment = createIconButton(Size(ICON_TAB, ICON_TAB),
                                     CC_CALLBACK_1(InventoryLayer::onTabEquipmentClicked, this),
                                     Color3B(255, 220, 160));
    _tabAttribute = createIconButton(Size(ICON_TAB, ICON_TAB),
                                     CC_CALLBACK_1(InventoryLayer::onTabAttributeClicked, this),
                                     Color3B(200, 255, 200));

    if (_tabSkill)
    {
        _tabSkill->setPosition(Vec2(centerX - spacing, tabY));
        _panelRoot->addChild(_tabSkill, Z_UI);
    }
    if (_tabEquipment)
    {
        _tabEquipment->setPosition(Vec2(centerX, tabY));
        _panelRoot->addChild(_tabEquipment, Z_UI);
    }
    if (_tabAttribute)
    {
        _tabAttribute->setPosition(Vec2(centerX + spacing, tabY));
        _panelRoot->addChild(_tabAttribute, Z_UI);
    }

    // “文字”占位（PNG）：只用于对齐布局，后续替换为真实资源
    const float labelY = tabY - ICON_TAB * 0.72f;
    auto addTabLabel = [this, labelY](float x, const Color3B &tint)
    {
        auto labelSprite = createPlaceholderSprite(Size(ICON_TAB_LABEL_W, ICON_TAB_LABEL_H), tint);
        if (labelSprite && _panelRoot)
        {
            labelSprite->setPosition(Vec2(x, labelY));
            _panelRoot->addChild(labelSprite, Z_UI);
        }
    };
    addTabLabel(centerX - spacing, Color3B(200, 220, 255));
    addTabLabel(centerX, Color3B(255, 220, 160));
    addTabLabel(centerX + spacing, Color3B(200, 255, 200));
}

void InventoryLayer::createPages()
{
    _equipmentPage = Node::create();
    _equipmentPage->setContentSize(Size(DESIGN_WIDTH, DESIGN_HEIGHT));
    _equipmentPage->setAnchorPoint(Vec2::ZERO);
    _equipmentPage->setPosition(Vec2::ZERO);

    _skillPage = Node::create();
    _skillPage->setContentSize(Size(DESIGN_WIDTH, DESIGN_HEIGHT));
    _skillPage->setAnchorPoint(Vec2::ZERO);
    _skillPage->setPosition(Vec2::ZERO);

    _attributePage = Node::create();
    _attributePage->setContentSize(Size(DESIGN_WIDTH, DESIGN_HEIGHT));
    _attributePage->setAnchorPoint(Vec2::ZERO);
    _attributePage->setPosition(Vec2::ZERO);

    if (_panelRoot)
    {
        _panelRoot->addChild(_equipmentPage, Z_PAGE);
        _panelRoot->addChild(_skillPage, Z_PAGE);
        _panelRoot->addChild(_attributePage, Z_PAGE);
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
    bool skillVisible = (tab == Tab::SKILL);
    bool attrVisible = (tab == Tab::ATTRIBUTE);

    if (_equipmentPage)
    {
        _equipmentPage->setVisible(equipVisible);
    }
    if (_skillPage)
    {
        _skillPage->setVisible(skillVisible);
    }
    if (_attributePage)
    {
        _attributePage->setVisible(attrVisible);
    }

    // 切换页面时隐藏详情预览，避免遮挡
    hideDetailOverlay();

    auto updateTabStyle = [](cocos2d::ui::Button *tabBtn, bool selected, const Color3B &baseColor)
    {
        if (tabBtn)
        {
            tabBtn->setColor(selected ? baseColor : Color3B(140, 140, 140));
            tabBtn->setOpacity(selected ? 255 : 180);
        }
    };

    updateTabStyle(_tabSkill, skillVisible, Color3B(200, 220, 255));
    updateTabStyle(_tabEquipment, equipVisible, Color3B(255, 220, 160));
    updateTabStyle(_tabAttribute, attrVisible, Color3B(200, 255, 200));

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
    else if (_currentTab == Tab::SKILL)
    {
        refreshSkillPage();
    }
    else if (_currentTab == Tab::ATTRIBUTE)
    {
        refreshAttributePage();
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

cocos2d::ui::Button *InventoryLayer::createIconButton(const Size &targetSize,
                                                      const std::function<void(cocos2d::Ref *)> &callback,
                                                      const Color3B &tint)
{
    auto btn = cocos2d::ui::Button::create(PLACEHOLDER_ICON_PATH, PLACEHOLDER_ICON_PATH);
    if (!btn)
    {
        return nullptr;
    }

    btn->setAnchorPoint(Vec2(0.5f, 0.5f));
    btn->setPressedActionEnabled(true); // 点击时轻微缩放，提供反馈

    // 等比缩放到目标尺寸（点击区域跟随缩放）
    const auto texSize = btn->getNormalTextureSize();
    if (texSize.width > 0.0f && texSize.height > 0.0f)
    {
        const float scale = std::min(targetSize.width / texSize.width, targetSize.height / texSize.height);
        btn->setScale(scale);
    }
    btn->setColor(tint);
    if (callback)
    {
        btn->addClickEventListener(callback);
    }
    return btn;
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
    // 放到右侧下方：与列表并排，不遮挡主要交互区域（布局参考截图）
    const float x = DESIGN_WIDTH - SAFE_MARGIN_X - DETAIL_PANEL_W;
    const float y = SAFE_MARGIN_Y + 220.0f;
    _detailOverlay->setPosition(Vec2(x, y));
    _detailOverlay->setVisible(false);
    _panelRoot->addChild(_detailOverlay, Z_UI + 50);

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

void InventoryLayer::onTabAttributeClicked(cocos2d::Ref *)
{
    switchTab(Tab::ATTRIBUTE);
}

void InventoryLayer::refreshAttributePage()
{
    if (!_attributePage)
    {
        return;
    }
    _attributePage->removeAllChildren();

    // 布局锚点（设计坐标）
    const float safeRight = DESIGN_WIDTH - SAFE_MARGIN_X;
    const float safeBottom = SAFE_MARGIN_Y;
    const float safeTop = DESIGN_HEIGHT - SAFE_MARGIN_Y;

    // 左侧立绘占位（后续可替换为真实角色资源）
    if (auto character = createPlaceholderSprite(Size(920.0f, 1320.0f), Color3B(175, 175, 175)))
    {
        character->setAnchorPoint(Vec2::ZERO);
        character->setPosition(Vec2(0, 0));
        _attributePage->addChild(character, 0);
    }

    // 玩家未绑定时仅展示占位
    if (!_player || !_player->getAttributeComponent())
    {
        if (auto hint = createPlaceholderSprite(Size(360, 48), Color3B(180, 180, 180)))
        {
            hint->setPosition(Vec2(DESIGN_WIDTH * 0.5f, DESIGN_HEIGHT * 0.5f));
            _attributePage->addChild(hint, 1);
        }
        return;
    }

    // 中间属性列表面板（参考截图 1 的横条列表）
    const float characterW = 920.0f;
    const float attrListW = 960.0f;
    const float attrListH = 520.0f;
    const float attrListX = characterW + 80.0f;
    const float attrListTop = safeTop - 220.0f;
    const Rect attrListRect(attrListX, attrListTop - attrListH, attrListW, attrListH);

    auto attrBg = DrawNode::create();
    drawPanelRect(attrBg, attrListRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _attributePage->addChild(attrBg, 1);

    // “现有点数”占位
    if (auto points = createPlaceholderSprite(Size(260, 44), TITLE_COLOR))
    {
        points->setPosition(Vec2(attrListRect.getMidX(), attrListRect.getMaxY() + 60.0f));
        _attributePage->addChild(points, 2);
    }

    // 5 行属性条（全部用 PNG 占位）
    constexpr int kRowCount = 5;
    const float rowH = 84.0f;
    const float rowGap = 16.0f;
    const float rowW = attrListRect.size.width - 80.0f;
    const float rowX = attrListRect.getMinX() + 40.0f;
    const float firstRowTop = attrListRect.getMaxY() - 60.0f;

    for (int i = 0; i < kRowCount; ++i)
    {
        const float y = firstRowTop - (rowH + rowGap) * i - rowH;
        const Rect rowRect(rowX, y, rowW, rowH);

        auto rowBg = DrawNode::create();
        const bool isEven = (i % 2 == 0);
        drawPanelRect(rowBg,
                      rowRect,
                      isEven ? Color4F(0.12f, 0.12f, 0.18f, 0.90f) : Color4F(0.10f, 0.10f, 0.16f, 0.90f),
                      Color4F(0.25f, 0.25f, 0.30f, 0.70f));
        _attributePage->addChild(rowBg, 2);

        if (auto name = createPlaceholderSprite(Size(150, 30), ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(rowRect.getMinX() + 18.0f, rowRect.getMidY()));
            _attributePage->addChild(name, 3);
        }

        if (auto value = createPlaceholderSprite(Size(120, 30), Color3B::WHITE))
        {
            value->setAnchorPoint(Vec2(1.0f, 0.5f));
            value->setPosition(Vec2(rowRect.getMaxX() - 18.0f, rowRect.getMidY()));
            _attributePage->addChild(value, 3);
        }
    }

    // 右上角：等级区块（参考截图 1 的“等级 + 大数字”）
    const float levelW = 520.0f;
    const float levelH = 300.0f;
    const Rect levelRect(safeRight - levelW, safeTop - levelH, levelW, levelH);
    auto levelBg = DrawNode::create();
    drawPanelRect(levelBg, levelRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _attributePage->addChild(levelBg, 1);

    if (auto levelTitle = createPlaceholderSprite(Size(160, 40), TITLE_COLOR))
    {
        levelTitle->setAnchorPoint(Vec2(0.5f, 1.0f));
        levelTitle->setPosition(Vec2(levelRect.getMidX(), levelRect.getMaxY() - 24.0f));
        _attributePage->addChild(levelTitle, 2);
    }
    if (auto levelNum = createPlaceholderSprite(Size(220, 180), Color3B(250, 240, 210)))
    {
        levelNum->setPosition(Vec2(levelRect.getMidX(), levelRect.getMidY() - 30.0f));
        _attributePage->addChild(levelNum, 2);
    }

    // 右侧：战斗数据列表（参考截图 1 的右侧数据列）
    const float statsW = 860.0f;
    const float statsH = 560.0f;
    float statsY = levelRect.getMinY() - 40.0f - statsH;
    statsY = std::max(statsY, safeBottom + 220.0f);
    const Rect statsRect(safeRight - statsW, statsY, statsW, statsH);

    auto statsBg = DrawNode::create();
    drawPanelRect(statsBg, statsRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _attributePage->addChild(statsBg, 1);

    const float statRowH = 86.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float y = statsRect.getMaxY() - 80.0f - statRowH * i;
        if (auto name = createPlaceholderSprite(Size(180, 30), ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(statsRect.getMinX() + 30.0f, y));
            _attributePage->addChild(name, 2);
        }
        if (auto value = createPlaceholderSprite(Size(140, 30), Color3B::WHITE))
        {
            value->setAnchorPoint(Vec2(1.0f, 0.5f));
            value->setPosition(Vec2(statsRect.getMaxX() - 30.0f, y));
            _attributePage->addChild(value, 2);
        }
    }
}

void InventoryLayer::refreshEquipmentPage()
{
    if (!_equipmentPage)
    {
        return;
    }
    _equipmentPage->removeAllChildren();

    // 布局锚点（设计坐标）
    const float safeLeft = SAFE_MARGIN_X;
    const float safeRight = DESIGN_WIDTH - SAFE_MARGIN_X;
    const float safeBottom = SAFE_MARGIN_Y;

    // 内容顶部：放在顶部分支按钮下方（布局参考截图）
    const float contentTop = DESIGN_HEIGHT - 240.0f;

    // 右侧信息栏（与详情预览对齐）
    const float rightW = DETAIL_PANEL_W;
    const float rightX = safeRight - rightW;
    const float rightStatsH = 560.0f;
    const Rect rightStatsRect(rightX, contentTop - rightStatsH, rightW, rightStatsH);

    // 详情预览面板位置对齐右栏
    if (_detailOverlay)
    {
        _detailOverlay->setPosition(Vec2(rightX, safeBottom + 220.0f));
    }

    // 左侧物品列表（可滚动）
    const float listW = 760.0f;
    const float listBottom = 260.0f;
    const Rect listRect(safeLeft, listBottom, listW, contentTop - listBottom);

    // 玩家未绑定：仅展示布局占位
    if (!_player)
    {
        if (auto hint = createPlaceholderSprite(Size(360, 48), Color3B(180, 180, 180)))
        {
            hint->setPosition(Vec2(listRect.getMidX(), listRect.getMidY()));
            _equipmentPage->addChild(hint, 2);
        }
        return;
    }

    // 左上：分类/排序占位
    if (auto category = createPlaceholderSprite(Size(220, 36), SECTION_TITLE_COLOR))
    {
        category->setAnchorPoint(Vec2(0.0f, 0.5f));
        category->setPosition(Vec2(listRect.getMinX(), contentTop + 40.0f));
        _equipmentPage->addChild(category, 2);
    }
    if (auto sort = createPlaceholderSprite(Size(220, 36), SECTION_TITLE_COLOR))
    {
        sort->setAnchorPoint(Vec2(0.0f, 0.5f));
        sort->setPosition(Vec2(listRect.getMinX() + 260.0f, contentTop + 40.0f));
        _equipmentPage->addChild(sort, 2);
    }

    // 左侧列表背景
    auto listBg = DrawNode::create();
    drawPanelRect(listBg, listRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _equipmentPage->addChild(listBg, 1);

    // ScrollView：物品列表
    auto scroll = cocos2d::ui::ScrollView::create();
    scroll->setDirection(cocos2d::ui::ScrollView::Direction::VERTICAL);
    scroll->setBounceEnabled(true);
    scroll->setScrollBarEnabled(true);
    scroll->setContentSize(listRect.size);
    scroll->setAnchorPoint(Vec2::ZERO);
    scroll->setPosition(listRect.origin);
    _equipmentPage->addChild(scroll, 3);

    const auto &items = _player->getInventoryItems();
    constexpr float rowH = 96.0f;
    const int itemCount = static_cast<int>(items.size());
    const float innerH = std::max(listRect.size.height, rowH * itemCount);
    scroll->setInnerContainerSize(Size(listRect.size.width, innerH));

    for (int i = 0; i < itemCount; ++i)
    {
        const auto &item = items[i];
        if (!item)
        {
            continue;
        }

        const bool isSelected = (_selectedInventoryItemId == item->id);
        bool isEquipped = false;
        if (auto equipped = _player->getEquipment(item->slot))
        {
            isEquipped = (equipped->id == item->id);
        }

        auto row = cocos2d::ui::Layout::create();
        row->setContentSize(Size(listRect.size.width, rowH));
        row->setAnchorPoint(Vec2::ZERO);
        row->setPosition(Vec2(0.0f, innerH - rowH * (i + 1)));
        row->setTouchEnabled(true);

        row->addTouchEventListener([this, itemId = item->id](Ref *, cocos2d::ui::Widget::TouchEventType type)
                                   {
                                       if (type != cocos2d::ui::Widget::TouchEventType::ENDED)
                                       {
                                           return;
                                       }
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
                                               _selectedInventoryItemId = itemId;
                                               showDetailOverlay();
                                               refresh();
                                               break;
                                           }
                                       } });

        // 行背景（选中高亮）
        auto rowBg = DrawNode::create();
        drawPanelRect(rowBg,
                      Rect(0, 0, listRect.size.width, rowH),
                      isSelected ? Color4F(0.18f, 0.14f, 0.10f, 0.92f) : Color4F(0.10f, 0.10f, 0.14f, 0.72f),
                      Color4F(0.25f, 0.25f, 0.30f, 0.55f));
        row->addChild(rowBg, 0);

        const Color3B iconTint = isEquipped ? SELECTED_COLOR : ITEM_TEXT_COLOR;
        if (auto icon = createPlaceholderSprite(Size(ICON_ITEM, ICON_ITEM), iconTint))
        {
            icon->setPosition(Vec2(60.0f, rowH * 0.5f));
            row->addChild(icon, 1);
        }
        if (auto name = createPlaceholderSprite(Size(360, 30), ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(120.0f, rowH * 0.5f + 12.0f));
            row->addChild(name, 1);
        }
        if (auto level = createPlaceholderSprite(Size(90, 30), Color3B(190, 190, 190)))
        {
            level->setAnchorPoint(Vec2(1.0f, 0.5f));
            level->setPosition(Vec2(listRect.size.width - 24.0f, rowH * 0.5f + 12.0f));
            row->addChild(level, 1);
        }

        scroll->getInnerContainer()->addChild(row, 1);
    }

    // 右侧信息栏（占位）
    auto rightBg = DrawNode::create();
    drawPanelRect(rightBg, rightStatsRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _equipmentPage->addChild(rightBg, 1);

    // 右栏内容占位：顶部标题/进度条/若干数据行
    if (auto title = createPlaceholderSprite(Size(320, 40), TITLE_COLOR))
    {
        title->setAnchorPoint(Vec2(0.0f, 1.0f));
        title->setPosition(Vec2(rightStatsRect.getMinX() + 30.0f, rightStatsRect.getMaxY() - 26.0f));
        _equipmentPage->addChild(title, 2);
    }
    if (auto bar = createPlaceholderSprite(Size(rightStatsRect.size.width - 60.0f, 22.0f), Color3B(210, 210, 210)))
    {
        bar->setAnchorPoint(Vec2(0.0f, 1.0f));
        bar->setPosition(Vec2(rightStatsRect.getMinX() + 30.0f, rightStatsRect.getMaxY() - 80.0f));
        _equipmentPage->addChild(bar, 2);
    }
    for (int i = 0; i < 4; ++i)
    {
        const float y = rightStatsRect.getMaxY() - 150.0f - 82.0f * i;
        if (auto name = createPlaceholderSprite(Size(160, 28), ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(rightStatsRect.getMinX() + 30.0f, y));
            _equipmentPage->addChild(name, 2);
        }
        if (auto value = createPlaceholderSprite(Size(140, 28), Color3B::WHITE))
        {
            value->setAnchorPoint(Vec2(1.0f, 0.5f));
            value->setPosition(Vec2(rightStatsRect.getMaxX() - 30.0f, y));
            _equipmentPage->addChild(value, 2);
        }
    }

    // 底部材料栏（占位）
    const float matW = 980.0f;
    const float matH = 150.0f;
    const Rect matRect(DESIGN_WIDTH * 0.5f - matW * 0.5f, safeBottom + 30.0f, matW, matH);
    auto matBg = DrawNode::create();
    drawPanelRect(matBg, matRect, Color4F(0.10f, 0.10f, 0.14f, 0.80f), PANEL_BORDER_COLOR);
    _equipmentPage->addChild(matBg, 1);
    if (auto matIcon = createPlaceholderSprite(Size(70, 70), ITEM_ATTR_COLOR))
    {
        matIcon->setPosition(Vec2(matRect.getMinX() + 70.0f, matRect.getMidY()));
        _equipmentPage->addChild(matIcon, 2);
    }
    if (auto matText = createPlaceholderSprite(Size(matRect.size.width - 180.0f, 30.0f), ITEM_TEXT_COLOR))
    {
        matText->setAnchorPoint(Vec2(0.0f, 0.5f));
        matText->setPosition(Vec2(matRect.getMinX() + 130.0f, matRect.getMidY()));
        _equipmentPage->addChild(matText, 2);
    }
}

void InventoryLayer::refreshSkillPage()
{
    if (!_skillPage)
    {
        return;
    }
    _skillPage->removeAllChildren();

    // 布局锚点（设计坐标）
    const float safeLeft = SAFE_MARGIN_X;
    const float safeRight = DESIGN_WIDTH - SAFE_MARGIN_X;
    const float safeBottom = SAFE_MARGIN_Y;
    const float contentTop = DESIGN_HEIGHT - 240.0f;

    if (!_player)
    {
        if (auto hint = createPlaceholderSprite(Size(360, 48), Color3B(180, 180, 180)))
        {
            hint->setPosition(Vec2(DESIGN_WIDTH * 0.5f, DESIGN_HEIGHT * 0.5f));
            _skillPage->addChild(hint, 1);
        }
        return;
    }

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        if (auto hint = createPlaceholderSprite(Size(360, 48), Color3B(180, 180, 180)))
        {
            hint->setPosition(Vec2(DESIGN_WIDTH * 0.5f, DESIGN_HEIGHT * 0.5f));
            _skillPage->addChild(hint, 1);
        }
        return;
    }

    // 右侧信息栏（与详情预览对齐）
    const float rightW = DETAIL_PANEL_W;
    const float rightX = safeRight - rightW;

    // 详情预览面板位置对齐右栏
    if (_detailOverlay)
    {
        _detailOverlay->setPosition(Vec2(rightX, safeBottom + 220.0f));
    }

    // 右上：技能点面板（占位，参考截图 2）
    const float pointsH = 240.0f;
    const Rect pointsRect(rightX, contentTop - pointsH, rightW, pointsH);
    auto pointsBg = DrawNode::create();
    drawPanelRect(pointsBg, pointsRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _skillPage->addChild(pointsBg, 1);
    if (auto pointsTitle = createPlaceholderSprite(Size(260, 40), TITLE_COLOR))
    {
        pointsTitle->setAnchorPoint(Vec2(0.0f, 1.0f));
        pointsTitle->setPosition(Vec2(pointsRect.getMinX() + 26.0f, pointsRect.getMaxY() - 20.0f));
        _skillPage->addChild(pointsTitle, 2);
    }
    if (auto pointsValue = createPlaceholderSprite(Size(140, 60), Color3B::WHITE))
    {
        pointsValue->setPosition(Vec2(pointsRect.getMidX(), pointsRect.getMidY()));
        _skillPage->addChild(pointsValue, 2);
    }

    // 右中：角色数据面板（占位，参考截图 2 右侧信息）
    const float statsH = 240.0f;
    const Rect statsRect(rightX, pointsRect.getMinY() - 20.0f - statsH, rightW, statsH);
    auto statsBg = DrawNode::create();
    drawPanelRect(statsBg, statsRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _skillPage->addChild(statsBg, 1);
    if (auto nameBar = createPlaceholderSprite(Size(statsRect.size.width - 60.0f, 34.0f), TITLE_COLOR))
    {
        nameBar->setAnchorPoint(Vec2(0.0f, 1.0f));
        nameBar->setPosition(Vec2(statsRect.getMinX() + 30.0f, statsRect.getMaxY() - 18.0f));
        _skillPage->addChild(nameBar, 2);
    }

    // 左侧：技能树区域（可滚动，占位实现）
    const float treeBottom = 260.0f;
    const float treeRight = rightX - 80.0f;
    const Rect treeRect(safeLeft, treeBottom, treeRight - safeLeft, contentTop - treeBottom);

    auto treeBg = DrawNode::create();
    drawPanelRect(treeBg, treeRect, Color4F(0.06f, 0.06f, 0.10f, 0.25f), Color4F(0.25f, 0.25f, 0.30f, 0.45f));
    _skillPage->addChild(treeBg, 0);

    auto treeScroll = cocos2d::ui::ScrollView::create();
    treeScroll->setDirection(cocos2d::ui::ScrollView::Direction::VERTICAL);
    treeScroll->setBounceEnabled(true);
    treeScroll->setScrollBarEnabled(true);
    treeScroll->setContentSize(treeRect.size);
    treeScroll->setAnchorPoint(Vec2::ZERO);
    treeScroll->setPosition(treeRect.origin);
    _skillPage->addChild(treeScroll, 1);
    treeScroll->setInnerContainerSize(treeRect.size);

    const Vec2 center(treeRect.size.width * 0.45f, treeRect.size.height * 0.55f);
    std::vector<Vec2> nodePositions = {
        center,
        center + Vec2(0, 220),
        center + Vec2(-220, 0),
        center + Vec2(220, 0),
        center + Vec2(0, -220),
    };

    // 连线占位（仅用于布局示意）
    auto lines = DrawNode::create();
    const Color4F lineColor(0.65f, 0.65f, 0.75f, 0.55f);
    for (size_t i = 1; i < nodePositions.size(); ++i)
    {
        lines->drawLine(nodePositions[0], nodePositions[i], lineColor);
    }
    treeScroll->getInnerContainer()->addChild(lines, 0);

    // 技能节点：用 _skillTemplates 前几项填充到固定位置（后续可扩展为真实技能树）
    const size_t showCount = std::min(nodePositions.size(), _skillTemplates.size());
    for (size_t i = 0; i < showCount; ++i)
    {
        const auto &tpl = _skillTemplates[i];
        const bool learned = (skillComp->findLearnedSkillById(tpl.id) != nullptr);
        const bool selected = (_selectedSkillId == tpl.id);

        Color3B tint = learned ? (tpl.isPassive ? ITEM_ATTR_COLOR : ITEM_TEXT_COLOR) : Color3B(120, 120, 120);
        if (selected)
        {
            tint = SELECTED_COLOR;
        }

        auto btn = createIconButton(Size(88, 88), [this, id = tpl.id](Ref *) {
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
                // 未学习：点击学习（占位逻辑，后续接入技能点消耗）
                for (const auto &t : _skillTemplates)
                {
                    if (t.id != id)
                    {
                        continue;
                    }
                    if (t.isPassive)
                    {
                        auto s = std::make_shared<PassiveSkill>();
                        s->id = t.id;
                        s->name = t.name;
                        s->description = t.description;
                        s->isPassive = true;
                        s->attributeBonus = t.attributeBonus;
                        comp->learnSkill(s);
                    }
                    else
                    {
                        auto s = std::make_shared<ActiveSkill>();
                        s->id = t.id;
                        s->name = t.name;
                        s->description = t.description;
                        s->isPassive = false;
                        s->cooldown = t.cooldown;
                        s->manaCost = t.manaCost;
                        s->currentCooldown = 0.0f;
                        comp->learnSkill(s);
                    }
                    break;
                }
            }
            else
            {
                // 已学习：点击装备到当前选中槽位
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
            }

            _selectedSkillId = id;
            showDetailOverlay();
            refresh();
        }, tint);
        if (!btn)
        {
            continue;
        }
        btn->setPosition(nodePositions[i]);
        treeScroll->getInnerContainer()->addChild(btn, 1);
    }

    // 槽位选择/卸下（参考截图 2 右侧中间的“锁/槽位”位置）
    const float slotX = treeRect.getMaxX() - 140.0f;
    float slotY = treeRect.getMaxY() - 140.0f;

    const auto &activeSlots = skillComp->getActiveSlots();
    const auto &passiveSlots = skillComp->getPassiveSlots();
    const size_t activeSlotCount = static_cast<size_t>(GameConfig::UI::SKILL_BAR_SLOT_COUNT);

    for (size_t i = 0; i < activeSlotCount; ++i)
    {
        const bool hasSkill = (i < activeSlots.size() && activeSlots[i]);
        const bool isSelected = (_selectedActiveSlotIndex == i);
        const Color3B tint = isSelected ? SELECTED_COLOR : (hasSkill ? ITEM_TEXT_COLOR : Color3B(120, 120, 120));

        auto btn = createIconButton(Size(64, 64), [this, i](Ref *) {
            _selectedActiveSlotIndex = i;
            showDetailOverlay();
            refresh();
        }, tint);
        if (btn)
        {
            btn->setPosition(Vec2(slotX, slotY));
            _skillPage->addChild(btn, 3);
        }

        if (hasSkill)
        {
            auto unequipBtn = createIconButton(Size(40, 40), [this, i](Ref *) {
                if (_player)
                {
                    if (auto comp = _player->getSkillComponent())
                    {
                        comp->unequipActiveSkill(i);
                    }
                }
                showDetailOverlay();
                refresh();
            }, Color3B(255, 120, 120));
            if (unequipBtn)
            {
                unequipBtn->setPosition(Vec2(slotX + 66.0f, slotY));
                _skillPage->addChild(unequipBtn, 3);
            }
        }

        slotY -= 90.0f;
    }

    for (size_t i = 0; i < 3; ++i)
    {
        const bool hasSkill = (i < passiveSlots.size() && passiveSlots[i]);
        const bool isSelected = (_selectedPassiveSlotIndex == i);
        const Color3B tint = isSelected ? SELECTED_COLOR : (hasSkill ? ITEM_ATTR_COLOR : Color3B(120, 120, 120));

        auto btn = createIconButton(Size(64, 64), [this, i](Ref *) {
            _selectedPassiveSlotIndex = i;
            showDetailOverlay();
            refresh();
        }, tint);
        if (btn)
        {
            btn->setPosition(Vec2(slotX, slotY));
            _skillPage->addChild(btn, 3);
        }

        if (hasSkill)
        {
            auto unequipBtn = createIconButton(Size(40, 40), [this, i](Ref *) {
                if (_player)
                {
                    if (auto comp = _player->getSkillComponent())
                    {
                        comp->unequipPassiveSkill(i);
                    }
                }
                showDetailOverlay();
                refresh();
            }, Color3B(255, 120, 120));
            if (unequipBtn)
            {
                unequipBtn->setPosition(Vec2(slotX + 66.0f, slotY));
                _skillPage->addChild(unequipBtn, 3);
            }
        }

        slotY -= 90.0f;
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
