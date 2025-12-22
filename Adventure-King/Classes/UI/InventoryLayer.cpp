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
#include <cmath>

USING_NS_CC;

namespace
{
    const char *const PLACEHOLDER_ICON_PATH = "Sprites/Characters/Player/Klee/default/TNT.png";
    const char *const UI_FONT_PATH = "fonts/ZCOOLKuaiLe-Regular.ttf";
    // 设计分辨率（截图为 2560x1440）：所有布局基于该坐标系，再整体缩放适配不同分辨率
    constexpr float DESIGN_WIDTH = 2560.0f;
    constexpr float DESIGN_HEIGHT = 1440.0f;

    // 安全边距（设计坐标）
    constexpr float SAFE_MARGIN_X = 120.0f;
    constexpr float SAFE_MARGIN_Y = 100.0f;

    // 图标尺寸（占位）
    constexpr float ICON_TAB = 110.0f;      // 顶部入口图标
    constexpr float ICON_ITEM = 64.0f;      // 列表/槽位图标
    constexpr float ICON_ACTION = 48.0f;    // 操作按钮图标
    constexpr float ICON_CLOSE = 70.0f;     // 返回按钮图标

    // 详情图预览面板：放在右侧，不遮挡页面主体（设计坐标）
    constexpr float DETAIL_PANEL_W = 720.0f;
    constexpr float DETAIL_PANEL_H = 360.0f;
    constexpr float DETAIL_PANEL_PADDING = 26.0f;
    constexpr int EQUIPMENT_DOUBLE_CLICK_WINDOW_MS = 350; // 装备列表双击判定窗口（毫秒）

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

    cocos2d::Label *createUiLabel(const std::string &text, float fontSize, const cocos2d::Color3B &color,
                                  bool withOutline = true, int outlineSize = 2)
    {
        auto label = cocos2d::Label::createWithTTF(text, UI_FONT_PATH, fontSize);
        if (!label)
        {
            // TTF 加载失败时降级为系统字体，避免 UI 完全不可见
            label = cocos2d::Label::createWithSystemFont(text, "Arial", fontSize);
        }
        if (!label)
        {
            return nullptr;
        }
        label->setColor(color);
        if (withOutline)
        {
            label->enableOutline(cocos2d::Color4B::BLACK, outlineSize);
        }
        return label;
    }

    cocos2d::ui::Button *createTextButton(const std::string &text, const cocos2d::Size &targetSize,
                                          const std::function<void(cocos2d::Ref *)> &callback,
                                          float fontSize = 26.0f,
                                          const cocos2d::Color3B &titleColor = cocos2d::Color3B::WHITE,
                                          const cocos2d::Color3B &backgroundTint = cocos2d::Color3B(80, 80, 90))
    {
        auto btn = cocos2d::ui::Button::create(PLACEHOLDER_ICON_PATH, PLACEHOLDER_ICON_PATH);
        if (!btn)
        {
            return nullptr;
        }

        btn->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
        btn->setPressedActionEnabled(true);

        // 使用 Scale9 让按钮尺寸稳定（占位贴图仅作为背景）
        btn->setScale9Enabled(true);
        btn->setCapInsets(cocos2d::Rect(10, 10, 10, 10));
        btn->setContentSize(targetSize);
        btn->setColor(backgroundTint);

        btn->setTitleText(text);
        btn->setTitleFontName(UI_FONT_PATH);
        btn->setTitleFontSize(fontSize);
        btn->setTitleColor(titleColor);

        if (callback)
        {
            btn->addClickEventListener(callback);
        }
        return btn;
    }

    void showToast(cocos2d::Node *parent, const cocos2d::Vec2 &pos, const std::string &text,
                   const cocos2d::Color3B &color = cocos2d::Color3B(230, 230, 230))
    {
        if (!parent)
        {
            return;
        }
        auto label = createUiLabel(text, 24.0f, color);
        if (!label)
        {
            return;
        }
        label->setOpacity(0);
        label->setPosition(pos);
        parent->addChild(label, 999);
        label->runAction(cocos2d::Sequence::create(
            cocos2d::FadeIn::create(0.08f),
            cocos2d::DelayTime::create(1.2f),
            cocos2d::FadeOut::create(0.18f),
            cocos2d::RemoveSelf::create(),
            nullptr));
    }

    std::string getPlayerDisplayName()
    {
        // 当前项目默认玩家为 Klee；后续可接入真实角色名/存档数据
        return "可莉";
    }

    int getEquipmentDisplayLevel(const std::shared_ptr<Equipment> &item, const PlayerCharacter *player)
    {
        if (!item)
        {
            return 1;
        }
        (void)player;
        // 装备等级独立于角色等级
        return std::max(1, item->level);
    }

    std::string getEquipmentSlotName(EquipmentSlot slot)
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
            return "未知槽位";
        }
    }

    std::string getAttributeDisplayName(AttributeType type)
    {
        switch (type)
        {
        case AttributeType::MAX_HP:
            return "生命力";
        case AttributeType::STRENGTH:
            return "力量";
        case AttributeType::MOVE_SPEED:
            return "敏捷";
        case AttributeType::DEFENSE:
            return "防御";
        case AttributeType::CRITICAL_RATE:
            return "暴击率";
        case AttributeType::MAX_MP:
            return "能量";
        case AttributeType::ATTACKINTERVAL:
        case AttributeType::ATTACK_INTERVAL:
            return "攻速";
        case AttributeType::ATTACK_RANGE:
            return "攻击范围";
        default:
            return "未知属性";
        }
    }

    bool isPercentAttribute(AttributeType type)
    {
        switch (type)
        {
        case AttributeType::CRITICAL_RATE:
            return true;
        default:
            return false;
        }
    }

    std::string formatAttributeDelta(AttributeType type, float value)
    {
        if (isPercentAttribute(type))
        {
            const int percent = static_cast<int>(std::round(value * 100.0f));
            return StringUtils::format("%+d%%", percent);
        }

        const int rounded = static_cast<int>(std::round(value));
        return StringUtils::format("%+d", rounded);
    }

    std::string formatAttributesBlock(const Attributes &attrs)
    {
        if (attrs.values.empty())
        {
            return "无";
        }

        std::string out;
        for (const auto &kv : attrs.values)
        {
            out += " - " + getAttributeDisplayName(kv.first) + " " + formatAttributeDelta(kv.first, kv.second) + "\n";
        }
        if (!out.empty() && out.back() == '\n')
        {
            out.pop_back();
        }
        return out;
    }

    std::string getEquipmentSpecialEffectBlock(const std::shared_ptr<Equipment> &item)
    {
        if (!item)
        {
            return " - 无";
        }

        const int level = std::max(1, item->level);

        // 武器：焰纹法杖（命中燃烧）
        if (item->id == GameConfig::Equipment::Weapon::EMBER_STAFF)
        {
            std::string out;
            out += StringUtils::format(" - 命中燃烧：%.0f%% 概率触发（%.2fs 冷却）\n",
                                       GameConfig::EquipmentEffect::EmberStaff::PROC_CHANCE * 100.0f,
                                       GameConfig::EquipmentEffect::EmberStaff::PROC_COOLDOWN);
            out += StringUtils::format(" - 燃烧：持续 %.1fs，间隔 %.1fs\n",
                                       GameConfig::StatusEffect::Burning::DURATION_SECONDS,
                                       GameConfig::StatusEffect::Burning::TICK_INTERVAL_SECONDS);
            out += StringUtils::format(" - 每跳伤害：(%.2f + %.2f×层数) × 攻击力",
                                       GameConfig::StatusEffect::Burning::BASE_DAMAGE_SCALE,
                                       GameConfig::StatusEffect::Burning::PER_STACK_DAMAGE_SCALE);
            return out;
        }

        // 武器：血契短剑（吸血，随等级成长）
        if (item->id == GameConfig::Equipment::Weapon::BLOOD_PACT_SWORD)
        {
            const float rate = GameConfig::EquipmentEffect::BloodPactSword::getLifestealRate(level);
            return StringUtils::format(" - 吸血：造成伤害的 %.1f%% 转为生命（随装备等级成长）",
                                       rate * 100.0f);
        }

        // 头盔：急救面罩（低血量救援）
        if (item->id == GameConfig::Equipment::Helmet::EMERGENCY_MASK)
        {
            return StringUtils::format(" - 急救：生命低于 %.0f%% 时将生命抬升到 %.0f%%（%.0fs 冷却）",
                                       GameConfig::EquipmentEffect::EmergencyMask::TRIGGER_HP_RATIO * 100.0f,
                                       GameConfig::EquipmentEffect::EmergencyMask::HEAL_TARGET_HP_RATIO * 100.0f,
                                       GameConfig::EquipmentEffect::EmergencyMask::PROC_COOLDOWN);
        }

        // 护甲：荆棘甲（反伤，随等级成长）
        if (item->id == GameConfig::Equipment::Armor::THORNS_ARMOR)
        {
            const float rate = GameConfig::EquipmentEffect::ThornsArmor::getReflectRate(level);
            return StringUtils::format(" - 反伤：反弹 %.0f%% 受到的伤害（%.2fs 冷却，随装备等级成长）",
                                       rate * 100.0f,
                                       GameConfig::EquipmentEffect::ThornsArmor::PROC_COOLDOWN);
        }

        // 靴子：追猎之靴（击杀加速）
        if (item->id == GameConfig::Equipment::Boots::HUNTER_BOOTS)
        {
            return StringUtils::format(" - 亢奋：击杀后移动速度 %+d，持续 %.1fs",
                                       static_cast<int>(std::round(GameConfig::StatusEffect::Excited::MOVE_SPEED_BONUS)),
                                       GameConfig::StatusEffect::Excited::DURATION_SECONDS);
        }

        return " - 无";
    }

    std::string getPassiveSkillSpecialEffectBlock(int skillId)
    {
        switch (skillId)
        {
        case GameConfig::Skill::Passive::BLOODTHIRST:
            return StringUtils::format(" - 吸血：造成伤害的 %.0f%% 转为生命",
                                       GameConfig::Skill::PassiveEffect::BLOODTHIRST_LIFESTEAL * 100.0f);
        case GameConfig::Skill::Passive::EMBER_MARK:
            return StringUtils::format(" - 余烬：%.0f%% 概率施加燃烧（%.2fs 冷却，可叠层）",
                                       GameConfig::Skill::PassiveEffect::EMBER_MARK_PROC_CHANCE * 100.0f,
                                       GameConfig::Skill::PassiveEffect::EMBER_MARK_PROC_COOLDOWN);
        case GameConfig::Skill::Passive::FULL_HP_CRIT:
            return StringUtils::format(" - 条件：满血时暴击率 %+d%%",
                                       static_cast<int>(std::round(GameConfig::Skill::PassiveEffect::FULL_HP_CRIT_BONUS * 100.0f)));
        case GameConfig::Skill::Passive::CRIT_ECHO:
            return StringUtils::format(" - 暴击：减少所有主动技能冷却 %.2fs（%.2fs 冷却）",
                                       GameConfig::Skill::PassiveEffect::CRIT_ECHO_REDUCE_SECONDS,
                                       GameConfig::Skill::PassiveEffect::CRIT_ECHO_PROC_COOLDOWN);
        case GameConfig::Skill::Passive::POISON_TOUCH:
            return StringUtils::format(" - 淬毒：%.0f%% 概率施加中毒（%.2fs 冷却，可叠层）",
                                       GameConfig::Skill::PassiveEffect::POISON_TOUCH_PROC_CHANCE * 100.0f,
                                       GameConfig::Skill::PassiveEffect::POISON_TOUCH_PROC_COOLDOWN);
        default:
            return " - 无";
        }
    }

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

    // 返回提示（参考截图右下角“Esc 返回”）
    if (_panelRoot)
    {
        _closeHintLabel = createUiLabel("Esc 返回", 26.0f, Color3B(220, 220, 220));
        if (_closeHintLabel)
        {
            _closeHintLabel->setAnchorPoint(Vec2(1.0f, 0.5f));
            _closeHintLabel->setPosition(Vec2(DESIGN_WIDTH - SAFE_MARGIN_X - 90.0f, SAFE_MARGIN_Y * 0.55f));
            _panelRoot->addChild(_closeHintLabel, Z_UI);
        }
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

    // 顶部“仓库入口”四分支：主动技能 / 被动技能 / 装备 / 属性
    const float tabY = DESIGN_HEIGHT - SAFE_MARGIN_Y * 0.55f;
    const float centerX = DESIGN_WIDTH * 0.5f;
    const float spacing = 190.0f;

    _tabSkill = createIconButton(Size(ICON_TAB, ICON_TAB),
                                 CC_CALLBACK_1(InventoryLayer::onTabSkillClicked, this),
                                 Color3B(200, 220, 255));
    _tabPassiveSkill = createIconButton(Size(ICON_TAB, ICON_TAB),
                                        CC_CALLBACK_1(InventoryLayer::onTabPassiveSkillClicked, this),
                                        Color3B(220, 200, 255));
    _tabEquipment = createIconButton(Size(ICON_TAB, ICON_TAB),
                                     CC_CALLBACK_1(InventoryLayer::onTabEquipmentClicked, this),
                                     Color3B(255, 220, 160));
    _tabAttribute = createIconButton(Size(ICON_TAB, ICON_TAB),
                                     CC_CALLBACK_1(InventoryLayer::onTabAttributeClicked, this),
                                     Color3B(200, 255, 200));

    if (_tabSkill)
    {
        _tabSkill->setPosition(Vec2(centerX - spacing * 1.5f, tabY));
        _panelRoot->addChild(_tabSkill, Z_UI);
    }
    if (_tabPassiveSkill)
    {
        _tabPassiveSkill->setPosition(Vec2(centerX - spacing * 0.5f, tabY));
        _panelRoot->addChild(_tabPassiveSkill, Z_UI);
    }
    if (_tabEquipment)
    {
        _tabEquipment->setPosition(Vec2(centerX + spacing * 0.5f, tabY));
        _panelRoot->addChild(_tabEquipment, Z_UI);
    }
    if (_tabAttribute)
    {
        _tabAttribute->setPosition(Vec2(centerX + spacing * 1.5f, tabY));
        _panelRoot->addChild(_tabAttribute, Z_UI);
    }

    // 入口文字：允许用 Label 渲染（武器名/人物名/等级/属性等也同理）
    const float labelY = tabY - ICON_TAB * 0.72f;
    auto addTabLabel = [this, labelY](cocos2d::Label *&outLabel, float x, const std::string &text, const Color3B &tint)
    {
        if (!_panelRoot)
        {
            return;
        }
        if (outLabel)
        {
            outLabel->removeFromParent();
            outLabel = nullptr;
        }

        auto label = createUiLabel(text, 28.0f, tint);
        if (!label)
        {
            return;
        }
        label->setAnchorPoint(Vec2(0.5f, 0.5f));
        label->setPosition(Vec2(x, labelY));
        _panelRoot->addChild(label, Z_UI);
        outLabel = label;
    };
    addTabLabel(_tabSkillLabel, centerX - spacing * 1.5f, "主动技能", Color3B(200, 220, 255));
    addTabLabel(_tabPassiveSkillLabel, centerX - spacing * 0.5f, "被动技能", Color3B(220, 200, 255));
    addTabLabel(_tabEquipmentLabel, centerX + spacing * 0.5f, "装备", Color3B(255, 220, 160));
    addTabLabel(_tabAttributeLabel, centerX + spacing * 1.5f, "属性", Color3B(200, 255, 200));
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

    _passiveSkillPage = Node::create();
    _passiveSkillPage->setContentSize(Size(DESIGN_WIDTH, DESIGN_HEIGHT));
    _passiveSkillPage->setAnchorPoint(Vec2::ZERO);
    _passiveSkillPage->setPosition(Vec2::ZERO);

    _attributePage = Node::create();
    _attributePage->setContentSize(Size(DESIGN_WIDTH, DESIGN_HEIGHT));
    _attributePage->setAnchorPoint(Vec2::ZERO);
    _attributePage->setPosition(Vec2::ZERO);

    if (_panelRoot)
    {
        _panelRoot->addChild(_equipmentPage, Z_PAGE);
        _panelRoot->addChild(_skillPage, Z_PAGE);
        _panelRoot->addChild(_passiveSkillPage, Z_PAGE);
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
        t.id = GameConfig::Skill::Passive::TOUGHNESS;
        t.isPassive = true;
        t.name = "体魄强化";
        t.description = "提升最大生命值";
        t.attributeBonus.add(AttributeType::MAX_HP, 30.0f);
        _skillTemplates.push_back(t);
    }

    // 被动：迅捷
    {
        SkillTemplate t;
        t.id = GameConfig::Skill::Passive::SWIFTNESS;
        t.isPassive = true;
        t.name = "迅捷步伐";
        t.description = "提升移动速度";
        t.attributeBonus.add(AttributeType::MOVE_SPEED, 30.0f);
        _skillTemplates.push_back(t);
    }

    // 被动：专注
    {
        SkillTemplate t;
        t.id = GameConfig::Skill::Passive::FOCUS;
        t.isPassive = true;
        t.name = "战斗专注";
        t.description = "提升暴击率";
        t.attributeBonus.add(AttributeType::CRITICAL_RATE, 0.05f);
        _skillTemplates.push_back(t);
    }

    // 被动：嗜血（机制：吸血）
    {
        SkillTemplate t;
        t.id = GameConfig::Skill::Passive::BLOODTHIRST;
        t.isPassive = true;
        t.name = "嗜血";
        t.description = "造成伤害会按比例回复生命";
        _skillTemplates.push_back(t);
    }

    // 被动：余烬印记（机制：命中燃烧）
    {
        SkillTemplate t;
        t.id = GameConfig::Skill::Passive::EMBER_MARK;
        t.isPassive = true;
        t.name = "余烬印记";
        t.description = "命中有概率施加燃烧（可叠层）";
        _skillTemplates.push_back(t);
    }

    // 被动：满血暴击（机制：条件触发暴击）
    {
        SkillTemplate t;
        t.id = GameConfig::Skill::Passive::FULL_HP_CRIT;
        t.isPassive = true;
        t.name = "满血暴击";
        t.description = "生命值满时，暴击率提升";
        _skillTemplates.push_back(t);
    }

    // 被动：冷却回响（机制：暴击缩短冷却）
    {
        SkillTemplate t;
        t.id = GameConfig::Skill::Passive::CRIT_ECHO;
        t.isPassive = true;
        t.name = "冷却回响";
        t.description = "暴击时减少所有主动技能的剩余冷却";
        _skillTemplates.push_back(t);
    }

    // 被动：淬毒（机制：命中中毒）
    {
        SkillTemplate t;
        t.id = GameConfig::Skill::Passive::POISON_TOUCH;
        t.isPassive = true;
        t.name = "淬毒";
        t.description = "命中有概率施加中毒（可叠层）";
        _skillTemplates.push_back(t);
    }
}

void InventoryLayer::switchTab(Tab tab)
{
    _currentTab = tab;
    bool equipVisible = (tab == Tab::EQUIPMENT);
    bool activeSkillVisible = (tab == Tab::ACTIVE_SKILL);
    bool passiveSkillVisible = (tab == Tab::PASSIVE_SKILL);
    bool attrVisible = (tab == Tab::ATTRIBUTE);

    if (_equipmentPage)
    {
        _equipmentPage->setVisible(equipVisible);
    }
    if (_skillPage)
    {
        _skillPage->setVisible(activeSkillVisible);
    }
    if (_passiveSkillPage)
    {
        _passiveSkillPage->setVisible(passiveSkillVisible);
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

    updateTabStyle(_tabSkill, activeSkillVisible, Color3B(200, 220, 255));
    updateTabStyle(_tabPassiveSkill, passiveSkillVisible, Color3B(220, 200, 255));
    updateTabStyle(_tabEquipment, equipVisible, Color3B(255, 220, 160));
    updateTabStyle(_tabAttribute, attrVisible, Color3B(200, 255, 200));

    auto updateTabLabelStyle = [](cocos2d::Label *label, bool selected, const Color3B &baseColor)
    {
        if (!label)
        {
            return;
        }
        label->setColor(selected ? baseColor : Color3B(180, 180, 180));
        label->setOpacity(selected ? 255 : 200);
    };

    updateTabLabelStyle(_tabSkillLabel, activeSkillVisible, Color3B(200, 220, 255));
    updateTabLabelStyle(_tabPassiveSkillLabel, passiveSkillVisible, Color3B(220, 200, 255));
    updateTabLabelStyle(_tabEquipmentLabel, equipVisible, Color3B(255, 220, 160));
    updateTabLabelStyle(_tabAttributeLabel, attrVisible, Color3B(200, 255, 200));

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
    else if (_currentTab == Tab::ACTIVE_SKILL)
    {
        refreshSkillPage();
    }
    else if (_currentTab == Tab::PASSIVE_SKILL)
    {
        refreshPassiveSkillPage();
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

    // 标题
    _detailOverlayTitle = createUiLabel("详情", 30.0f, TITLE_COLOR, true, 3);
    if (_detailOverlayTitle)
    {
        _detailOverlayTitle->setAnchorPoint(Vec2(0.0f, 1.0f));
        _detailOverlayTitle->setPosition(Vec2(DETAIL_PANEL_PADDING, DETAIL_PANEL_H - DETAIL_PANEL_PADDING));
        _detailOverlay->addChild(_detailOverlayTitle, 1);
    }

    // 内容区（可滚动，避免长描述溢出）
    const float titleAreaH = 64.0f;
    const float scrollW = DETAIL_PANEL_W - DETAIL_PANEL_PADDING * 2.0f;
    const float scrollH = DETAIL_PANEL_H - DETAIL_PANEL_PADDING * 2.0f - titleAreaH;

    _detailOverlayScroll = cocos2d::ui::ScrollView::create();
    if (_detailOverlayScroll)
    {
        _detailOverlayScroll->setDirection(cocos2d::ui::ScrollView::Direction::VERTICAL);
        _detailOverlayScroll->setBounceEnabled(true);
        _detailOverlayScroll->setScrollBarEnabled(true);
        _detailOverlayScroll->setContentSize(Size(scrollW, scrollH));
        _detailOverlayScroll->setAnchorPoint(Vec2(0.0f, 0.0f));
        _detailOverlayScroll->setPosition(Vec2(DETAIL_PANEL_PADDING, DETAIL_PANEL_PADDING));
        _detailOverlay->addChild(_detailOverlayScroll, 1);

        _detailOverlayBody = createUiLabel("", 24.0f, ITEM_TEXT_COLOR, false);
        if (_detailOverlayBody)
        {
            _detailOverlayBody->setAnchorPoint(Vec2(0.0f, 1.0f));
            _detailOverlayBody->setHorizontalAlignment(TextHAlignment::LEFT);
            _detailOverlayBody->setVerticalAlignment(TextVAlignment::TOP);
            _detailOverlayBody->setDimensions(scrollW, 0.0f);

            _detailOverlayScroll->addChild(_detailOverlayBody, 1);
            _detailOverlayScroll->setInnerContainerSize(Size(scrollW, scrollH));
            _detailOverlayBody->setPosition(Vec2(0.0f, scrollH));
        }
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

    std::string title = "详情";
    std::string body = "请先选择一个条目";

    // 根据当前页面与选中项填充详情文字
    if (_currentTab == Tab::EQUIPMENT)
    {
        title = "装备详情";

        if (!_player || _selectedInventoryItemId < 0)
        {
            body = "请先选择一件装备";
        }
        else
        {
            std::shared_ptr<Equipment> item;
            const auto &items = _player->getInventoryItems();
            for (const auto &it : items)
            {
                if (it && it->id == _selectedInventoryItemId)
                {
                    item = it;
                    break;
                }
            }

            if (!item)
            {
                body = "未找到该装备";
            }
            else
            {
                title = StringUtils::format("%s  Lv%d", item->name.c_str(), std::max(1, item->level));

                body = "故事：\n";
                body += (item->description.empty() ? "暂无故事" : item->description);
                body += "\n\n槽位：";
                body += getEquipmentSlotName(item->slot);

                body += "\n\n属性：\n";
                body += formatAttributesBlock(item->attributeBonus);

                if (auto weapon = std::dynamic_pointer_cast<Weapon>(item))
                {
                    body += "\n\n武器信息：\n";
                    body += StringUtils::format(" - 基础攻击力 %d\n", static_cast<int>(std::round(weapon->attackDamage)));
                    body += StringUtils::format(" - 攻击范围 %d\n", static_cast<int>(std::round(weapon->attackRange)));
                    body += StringUtils::format(" - 攻速倍率 ×%.2f\n", weapon->attackSpeed);
                    if (!weapon->attackAnimationPrefix.empty())
                    {
                        body += StringUtils::format(" - 攻击动画 %s\n", weapon->attackAnimationPrefix.c_str());
                    }
                    body += StringUtils::format(" - 动画帧数 %d", weapon->attackFrameCount);
                }

                body += "\n\n特效：\n";
                body += getEquipmentSpecialEffectBlock(item);
            }
        }
    }
    else if (_currentTab == Tab::ACTIVE_SKILL)
    {
        title = "主动技能详情";

        const int skillId = _selectedSkillId;
        const SkillTemplate *tpl = nullptr;
        for (const auto &t : _skillTemplates)
        {
            if (!t.isPassive && t.id == skillId)
            {
                tpl = &t;
                break;
            }
        }

        if (skillId < 0 || !tpl)
        {
            body = "请先选择一个技能";
        }
        else
        {
            std::shared_ptr<Skill> learned;
            auto comp = _player ? _player->getSkillComponent() : nullptr;
            if (comp)
            {
                learned = comp->findLearnedSkillById(skillId);
            }

            title = tpl->name;
            body = "描述：\n";
            body += (learned ? learned->description : tpl->description);

            body += "\n\n基础信息：\n";
            float cooldown = tpl->cooldown;
            float manaCost = tpl->manaCost;
            if (auto active = std::dynamic_pointer_cast<ActiveSkill>(learned))
            {
                cooldown = active->cooldown;
                manaCost = active->manaCost;
            }
            body += StringUtils::format(" - 冷却：%.2fs\n", cooldown);
            body += StringUtils::format(" - 消耗：%.0f MP\n", manaCost);

            body += "\n伤害构成：\n";
            if (skillId == GameConfig::Bomb::BOMB_ID)
            {
                body += StringUtils::format(" - 爆炸伤害：攻击力 × %.2f\n", GameConfig::Bomb::DAMAGE_SCALE);
                body += StringUtils::format(" - 爆炸半径：%.0f\n", GameConfig::Bomb::EXPLOSION_RADIUS);
                body += " - 暴击：按暴击率判定，暴击伤害 ×1.5\n";
                body += " - 命中方式：范围爆炸";
            }
            else if (skillId == GameConfig::Fireball::FIREBALL_ID)
            {
                body += StringUtils::format(" - 爆炸伤害：攻击力 × %.2f\n", GameConfig::Fireball::DAMAGE_SCALE);
                body += StringUtils::format(" - 爆炸半径：%.0f\n", GameConfig::Fireball::EXPLOSION_RADIUS);
                body += " - 暴击：按暴击率判定，暴击伤害 ×1.5\n";
                body += " - 命中方式：飞行命中后爆炸\n";

                body += "\n持续效果（燃烧）：\n";
                body += StringUtils::format(" - 持续：%.1fs\n", GameConfig::StatusEffect::Burning::DURATION_SECONDS);
                body += StringUtils::format(" - 间隔：%.1fs\n", GameConfig::StatusEffect::Burning::TICK_INTERVAL_SECONDS);
                body += StringUtils::format(" - 每跳伤害：(%.2f + %.2f×层数) × 攻击力\n",
                                            GameConfig::StatusEffect::Burning::BASE_DAMAGE_SCALE,
                                            GameConfig::StatusEffect::Burning::PER_STACK_DAMAGE_SCALE);
                body += " - 规则：可叠层并刷新持续时间";
            }
            else
            {
                body += " - 暂无伤害描述";
            }
        }
    }
    else if (_currentTab == Tab::PASSIVE_SKILL)
    {
        title = "被动技能详情";

        const int skillId = _selectedSkillId;
        const SkillTemplate *tpl = nullptr;
        for (const auto &t : _skillTemplates)
        {
            if (t.isPassive && t.id == skillId)
            {
                tpl = &t;
                break;
            }
        }

        if (skillId < 0 || !tpl)
        {
            body = "请先选择一个被动技能";
        }
        else
        {
            std::shared_ptr<Skill> learned;
            auto comp = _player ? _player->getSkillComponent() : nullptr;
            if (comp)
            {
                learned = comp->findLearnedSkillById(skillId);
            }

            title = tpl->name;
            body = "描述：\n";
            body += (learned ? learned->description : tpl->description);

            body += "\n\n属性：\n";
            Attributes bonus;
            if (auto passive = std::dynamic_pointer_cast<PassiveSkill>(learned))
            {
                bonus = passive->attributeBonus;
            }
            else
            {
                bonus = tpl->attributeBonus;
            }
            body += formatAttributesBlock(bonus);

            body += "\n\n特殊效果：\n";
            body += getPassiveSkillSpecialEffectBlock(skillId);
        }
    }

    if (_detailOverlayTitle)
    {
        _detailOverlayTitle->setString(title);
    }
    if (_detailOverlayBody && _detailOverlayScroll)
    {
        const float viewW = _detailOverlayScroll->getContentSize().width;
        const float viewH = _detailOverlayScroll->getContentSize().height;

        // 先设定宽度，再设置文本以便正确计算高度
        _detailOverlayBody->setDimensions(viewW, 0.0f);
        _detailOverlayBody->setString(body);

        const float textH = _detailOverlayBody->getContentSize().height;
        const float innerH = std::max(viewH, textH);
        _detailOverlayScroll->setInnerContainerSize(Size(viewW, innerH));
        _detailOverlayBody->setPosition(Vec2(0.0f, innerH));
        _detailOverlayScroll->scrollToTop(0.0f, false);
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
    const float safeLeft = SAFE_MARGIN_X;
    const float safeRight = DESIGN_WIDTH - SAFE_MARGIN_X;
    const float safeBottom = SAFE_MARGIN_Y;
    const float safeTop = DESIGN_HEIGHT - SAFE_MARGIN_Y;

    // 玩家未绑定时仅展示占位
    if (!_player || !_player->getAttributeComponent())
    {
        if (auto hint = createUiLabel("请先绑定玩家", 32.0f, Color3B(200, 200, 200)))
        {
            hint->setPosition(Vec2(DESIGN_WIDTH * 0.5f, DESIGN_HEIGHT * 0.5f));
            _attributePage->addChild(hint, 1);
        }
        return;
    }

    auto attrComp = _player->getAttributeComponent();

    // 中间属性列表面板（参考截图 1 的横条列表）
    // 注意：需要避免与右侧“等级/战斗数据”面板重叠
    const float statsW = 860.0f;
    const float rightPanelLeft = safeRight - statsW;
    const float gapX = 60.0f;

    const float attrListW = std::min(900.0f, rightPanelLeft - gapX - safeLeft);
    const float attrListH = 520.0f;
    const float attrListX = rightPanelLeft - gapX - attrListW;
    const float attrListTop = safeTop - 220.0f;
    const Rect attrListRect(attrListX, attrListTop - attrListH, attrListW, attrListH);

    auto attrBg = DrawNode::create();
    drawPanelRect(attrBg, attrListRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _attributePage->addChild(attrBg, 1);

    // “现有点数”
    const int availablePoints = _player ? _player->getAttributePoints() : 0;
    if (auto points = createUiLabel(StringUtils::format("现有点数 %d", availablePoints), 32.0f, TITLE_COLOR))
    {
        points->setPosition(Vec2(attrListRect.getMidX(), attrListRect.getMaxY() + 60.0f));
        _attributePage->addChild(points, 2);
    }

    // 5 行属性条（使用文字渲染，避免“纯 PNG”导致排版错位）
    constexpr int kRowCount = 5;
    struct AttrRowDef
    {
        const char *name = nullptr;
        AttributeType type = AttributeType::STRENGTH;
        bool isPercent = false;
    };
    const AttrRowDef rowDefs[kRowCount] = {
        {"生命力", AttributeType::MAX_HP, false},
        {"力量", AttributeType::STRENGTH, false},
        {"敏捷", AttributeType::MOVE_SPEED, false},
        {"防御", AttributeType::DEFENSE, false},
        {"暴击率", AttributeType::CRITICAL_RATE, true},
    };

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

        const auto &def = rowDefs[i];
        if (auto name = createUiLabel(def.name ? def.name : "", 30.0f, ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(rowRect.getMinX() + 22.0f, rowRect.getMidY()));
            _attributePage->addChild(name, 3);
        }

        float rawValue = 0.0f;
        if (attrComp)
        {
            rawValue = attrComp->getAttributeValue(def.type);
        }
        const std::string valueText = def.isPercent
                                          ? StringUtils::format("%d%%", static_cast<int>(std::round(rawValue * 100.0f)))
                                          : StringUtils::format("%d", static_cast<int>(std::round(rawValue)));

        // 数值靠右，右侧预留“+”按钮位置
        if (auto value = createUiLabel(valueText, 30.0f, Color3B::WHITE))
        {
            value->setAnchorPoint(Vec2(1.0f, 0.5f));
            value->setPosition(Vec2(rowRect.getMaxX() - 86.0f, rowRect.getMidY()));
            _attributePage->addChild(value, 3);
        }

        // 单项加点按钮：消耗 1 点属性点
        const Vec2 toastPos(rowRect.getMaxX() - 38.0f, rowRect.getMidY() + 60.0f);
        const bool hasPoints = (_player && _player->getAttributePoints() > 0);
        auto plusBtn = createTextButton("+", Size(54.0f, 54.0f), [this, type = def.type, toastPos](Ref *) {
            if (!_player)
            {
                return;
            }
            if (_player->getAttributePoints() <= 0)
            {
                showToast(_panelRoot, toastPos, "属性点不足", Color3B(255, 220, 160));
                return;
            }
            if (_player->upgradeAttribute(type))
            {
                showToast(_panelRoot, toastPos, "已提升属性", Color3B(200, 255, 200));
                refresh();
            }
        }, 36.0f, Color3B::WHITE, hasPoints ? Color3B(90, 90, 110) : Color3B(70, 70, 80));
        if (plusBtn)
        {
            plusBtn->setOpacity(hasPoints ? 255 : 190);
            plusBtn->setPosition(Vec2(rowRect.getMaxX() - 38.0f, rowRect.getMidY()));
            _attributePage->addChild(plusBtn, 3);
        }
    }

    // 右上角：等级区块（参考截图 1 的“等级 + 大数字”）
    const float levelW = 520.0f;
    const float levelH = 300.0f;
    const Rect levelRect(safeRight - levelW, safeTop - levelH, levelW, levelH);
    auto levelBg = DrawNode::create();
    drawPanelRect(levelBg, levelRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _attributePage->addChild(levelBg, 1);

    if (auto levelTitle = createUiLabel("等级", 44.0f, TITLE_COLOR, true, 3))
    {
        levelTitle->setAnchorPoint(Vec2(0.5f, 1.0f));
        levelTitle->setPosition(Vec2(levelRect.getMidX(), levelRect.getMaxY() - 24.0f));
        _attributePage->addChild(levelTitle, 2);
    }
    if (auto levelNum = createUiLabel(StringUtils::format("%d", _player->getLevel()), 150.0f, Color3B(250, 240, 210), true, 4))
    {
        levelNum->setPosition(Vec2(levelRect.getMidX(), levelRect.getMidY() - 30.0f));
        _attributePage->addChild(levelNum, 2);
    }

    // 右侧：战斗数据列表（参考截图 1 的右侧数据列）
    const float statsH = 560.0f;
    float statsY = levelRect.getMinY() - 40.0f - statsH;
    statsY = std::max(statsY, safeBottom + 220.0f);
    const Rect statsRect(safeRight - statsW, statsY, statsW, statsH);

    auto statsBg = DrawNode::create();
    drawPanelRect(statsBg, statsRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _attributePage->addChild(statsBg, 1);

    auto addStatRow = [this, statsRect](int index, const std::string &nameText, const std::string &valueText)
    {
        const float statRowH = 86.0f;
        const float y = statsRect.getMaxY() - 80.0f - statRowH * index;

        if (auto name = createUiLabel(nameText, 28.0f, ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(statsRect.getMinX() + 30.0f, y));
            _attributePage->addChild(name, 2);
        }
        if (auto value = createUiLabel(valueText, 28.0f, Color3B::WHITE))
        {
            value->setAnchorPoint(Vec2(1.0f, 0.5f));
            value->setPosition(Vec2(statsRect.getMaxX() - 30.0f, y));
            _attributePage->addChild(value, 2);
        }
    };

    const float maxHp = attrComp ? attrComp->getAttributeValue(AttributeType::MAX_HP) : 0.0f;
    const float attack = _player ? _player->getAttackPower() : 0.0f;
    const float speed = attrComp ? attrComp->getAttributeValue(AttributeType::MOVE_SPEED) : 0.0f;
    const float defense = attrComp ? attrComp->getAttributeValue(AttributeType::DEFENSE) : 0.0f;
    const float crit = attrComp ? attrComp->getAttributeValue(AttributeType::CRITICAL_RATE) : 0.0f;

    addStatRow(0, "生命", StringUtils::format("%d", static_cast<int>(std::round(maxHp))));
    addStatRow(1, "攻击力", StringUtils::format("%d", static_cast<int>(std::round(attack))));
    addStatRow(2, "速度", StringUtils::format("%d", static_cast<int>(std::round(speed))));
    addStatRow(3, "防御", StringUtils::format("%d", static_cast<int>(std::round(defense))));
    addStatRow(4, "暴击率", StringUtils::format("%d%%", static_cast<int>(std::round(crit * 100.0f))));
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
        if (auto hint = createUiLabel("请先绑定玩家", 32.0f, Color3B(200, 200, 200)))
        {
            hint->setPosition(Vec2(listRect.getMidX(), listRect.getMidY()));
            _equipmentPage->addChild(hint, 2);
        }
        return;
    }

    // 左上：分类/排序占位
    if (auto category = createUiLabel("武器", 30.0f, SECTION_TITLE_COLOR))
    {
        category->setAnchorPoint(Vec2(0.0f, 0.5f));
        category->setPosition(Vec2(listRect.getMinX(), contentTop + 40.0f));
        _equipmentPage->addChild(category, 2);
    }
    if (auto sort = createUiLabel("字母顺序", 30.0f, SECTION_TITLE_COLOR))
    {
        sort->setAnchorPoint(Vec2(0.0f, 0.5f));
        sort->setPosition(Vec2(listRect.getMinX() + 260.0f, contentTop + 40.0f));
        _equipmentPage->addChild(sort, 2);
    }

    // 左侧列表背景
    auto listBg = DrawNode::create();
    drawPanelRect(listBg, listRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _equipmentPage->addChild(listBg, 1);

    // 滚动列表：物品列表
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
                                       const auto now = std::chrono::steady_clock::now();
                                       bool isDoubleClick = false;
                                       if (_lastEquipmentListClickItemId == itemId)
                                       {
                                           const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastEquipmentListClickTime).count();
                                           isDoubleClick = (delta >= 0 && delta <= EQUIPMENT_DOUBLE_CLICK_WINDOW_MS);
                                       }

                                       _lastEquipmentListClickItemId = itemId;
                                       _lastEquipmentListClickTime = now;

                                       _selectedInventoryItemId = itemId;

                                       if (isDoubleClick)
                                       {
                                           const auto &inv = _player->getInventoryItems();
                                           for (const auto &it : inv)
                                           {
                                               if (it && it->id == itemId)
                                               {
                                                   _player->equip(it);
                                                   showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "已装备", Color3B(200, 255, 200));
                                                   break;
                                               }
                                           }
                                           // 避免三击触发多次
                                           _lastEquipmentListClickItemId = -1;
                                       }

                                       showDetailOverlay();
                                       refresh(); });

        // 行背景（选中高亮）
        auto rowBg = DrawNode::create();
        Color4F rowFill = Color4F(0.10f, 0.10f, 0.14f, 0.72f);
        if (isSelected)
        {
            rowFill = Color4F(0.18f, 0.14f, 0.10f, 0.92f); // 选中：黄色系
        }
        else if (isEquipped)
        {
            rowFill = Color4F(0.10f, 0.20f, 0.10f, 0.82f); // 已装备：绿色
        }
        drawPanelRect(rowBg,
                      Rect(0, 0, listRect.size.width, rowH),
                      rowFill,
                      Color4F(0.25f, 0.25f, 0.30f, 0.55f));
        row->addChild(rowBg, 0);

        const Color3B iconTint = isEquipped ? SELECTED_COLOR : ITEM_TEXT_COLOR;
        if (auto icon = createPlaceholderSprite(Size(ICON_ITEM, ICON_ITEM), iconTint))
        {
            icon->setPosition(Vec2(60.0f, rowH * 0.5f));
            row->addChild(icon, 1);
        }
        if (auto name = createUiLabel(item->name.empty() ? "未命名" : item->name, 26.0f, ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(120.0f, rowH * 0.5f + 12.0f));
            row->addChild(name, 1);
        }
        const int displayLevel = getEquipmentDisplayLevel(item, _player);
        if (auto level = createUiLabel(StringUtils::format("Lv%d", displayLevel), 24.0f, Color3B(190, 190, 190)))
        {
            level->setAnchorPoint(Vec2(1.0f, 0.5f));
            level->setPosition(Vec2(listRect.size.width - 24.0f, rowH * 0.5f + 12.0f));
            row->addChild(level, 1);
        }

        scroll->getInnerContainer()->addChild(row, 1);
    }

    // 右侧信息栏（人物信息/等级/属性，参考截图 3 右侧面板）
    auto rightBg = DrawNode::create();
    drawPanelRect(rightBg, rightStatsRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _equipmentPage->addChild(rightBg, 1);

    const std::string playerName = getPlayerDisplayName();
    if (auto title = createUiLabel(playerName, 34.0f, TITLE_COLOR, true, 3))
    {
        title->setAnchorPoint(Vec2(0.0f, 1.0f));
        title->setPosition(Vec2(rightStatsRect.getMinX() + 30.0f, rightStatsRect.getMaxY() - 26.0f));
        _equipmentPage->addChild(title, 2);
    }

    // 等级与经验条
    if (auto levelLabel = createUiLabel(StringUtils::format("等级 %d", _player->getLevel()), 26.0f, Color3B(220, 220, 220)))
    {
        levelLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        levelLabel->setPosition(Vec2(rightStatsRect.getMaxX() - 30.0f, rightStatsRect.getMaxY() - 26.0f));
        _equipmentPage->addChild(levelLabel, 2);
    }

    const int requiredExp = GameConfig::Player::Leveling::getRequiredExp(_player->getLevel());
    const float expRatio = (requiredExp > 0) ? clampf(static_cast<float>(_player->getExperience()) / requiredExp, 0.0f, 1.0f) : 0.0f;
    const float barW = rightStatsRect.size.width - 60.0f;
    const float barH = 18.0f;
    const float barLeft = rightStatsRect.getMinX() + 30.0f;
    const float barTop = rightStatsRect.getMaxY() - 80.0f;
    const Rect expRect(barLeft, barTop - barH, barW, barH);
    auto expBar = DrawNode::create();
    expBar->drawSolidRect(expRect.origin, expRect.origin + Vec2(expRect.size.width, expRect.size.height), Color4F(0.18f, 0.18f, 0.22f, 0.95f));
    expBar->drawSolidRect(expRect.origin,
                          expRect.origin + Vec2(expRect.size.width * expRatio, expRect.size.height),
                          Color4F(0.92f, 0.78f, 0.25f, 0.95f));
    expBar->drawRect(expRect.origin, expRect.origin + Vec2(expRect.size.width, expRect.size.height), Color4F(0.35f, 0.35f, 0.40f, 0.85f));
    _equipmentPage->addChild(expBar, 2);

    if (auto expText = createUiLabel(StringUtils::format("%d / %d", _player->getExperience(), requiredExp), 22.0f, Color3B(210, 210, 210)))
    {
        expText->setAnchorPoint(Vec2(1.0f, 1.0f));
        expText->setPosition(Vec2(expRect.getMaxX(), expRect.getMinY() - 10.0f));
        _equipmentPage->addChild(expText, 2);
    }

    auto attrComp = _player->getAttributeComponent();
    const float maxHp = attrComp ? attrComp->getAttributeValue(AttributeType::MAX_HP) : 0.0f;
    const float attack = _player->getAttackPower();
    const float defense = attrComp ? attrComp->getAttributeValue(AttributeType::DEFENSE) : 0.0f;
    const float crit = attrComp ? attrComp->getAttributeValue(AttributeType::CRITICAL_RATE) : 0.0f;

    auto addStatRow = [this, rightStatsRect](int index, const std::string &nameText, const std::string &valueText)
    {
        const float y = rightStatsRect.getMaxY() - 150.0f - 82.0f * index;
        if (auto name = createUiLabel(nameText, 26.0f, ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(rightStatsRect.getMinX() + 30.0f, y));
            _equipmentPage->addChild(name, 2);
        }
        if (auto value = createUiLabel(valueText, 26.0f, Color3B::WHITE))
        {
            value->setAnchorPoint(Vec2(1.0f, 0.5f));
            value->setPosition(Vec2(rightStatsRect.getMaxX() - 30.0f, y));
            _equipmentPage->addChild(value, 2);
        }
    };

    addStatRow(0, "生命", StringUtils::format("%d", static_cast<int>(std::round(maxHp))));
    addStatRow(1, "攻击力", StringUtils::format("%d", static_cast<int>(std::round(attack))));
    addStatRow(2, "防御", StringUtils::format("%d", static_cast<int>(std::round(defense))));
    addStatRow(3, "暴击率", StringUtils::format("%d%%", static_cast<int>(std::round(crit * 100.0f))));

    // 操作按钮（装备/升级）
    std::shared_ptr<Equipment> selectedItem;
    for (const auto &it : items)
    {
        if (it && it->id == _selectedInventoryItemId)
        {
            selectedItem = it;
            break;
        }
    }

    const float actionY = rightStatsRect.getMinY() + 70.0f;
    const float actionXMid = rightStatsRect.getMidX();
    const Size actionBtnSize(160.0f, 56.0f);

    auto equipBtn = createTextButton(
        "装备",
        actionBtnSize,
        [this](Ref *)
        {
            if (!_player)
            {
                return;
            }
            const int itemId = _selectedInventoryItemId;
            if (itemId < 0)
            {
                showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "请先选择一件装备", Color3B(255, 220, 160));
                return;
            }

            const auto &inv = _player->getInventoryItems();
            for (const auto &it : inv)
            {
                if (it && it->id == itemId)
                {
                    _player->equip(it);
                    showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "已装备", Color3B(200, 255, 200));
                    break;
                }
            }
            showDetailOverlay();
            refresh();
        },
        28.0f,
        Color3B::WHITE,
        Color3B(70, 110, 70));

    auto upgradeBtn = createTextButton(
        "升级",
        actionBtnSize,
        [this](Ref *)
        {
            if (!_player)
            {
                return;
            }
            const int itemId = _selectedInventoryItemId;
            if (itemId < 0)
            {
                showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "请先选择一件装备", Color3B(255, 220, 160));
                return;
            }

            const auto &inv = _player->getInventoryItems();
            bool upgraded = false;
            for (const auto &it : inv)
            {
                if (!it || it->id != itemId)
                {
                    continue;
                }

                // 如果当前已穿戴该装备，先卸下再升级，避免“属性加成变更但未重新结算”
                bool wasEquipped = false;
                if (auto equipped = _player->getEquipment(it->slot))
                {
                    wasEquipped = (equipped->id == it->id);
                }
                if (wasEquipped)
                {
                    _player->unequip(it->slot);
                }

                // 装备等级：独立于角色等级
                it->level = std::max(1, it->level) + 1;

                // 升级逻辑（占位）：提高现有属性加成；武器额外提升攻击力
                if (!it->attributeBonus.values.empty())
                {
                    for (auto &kv : it->attributeBonus.values)
                    {
                        kv.second *= 1.05f;
                    }
                }
                else
                {
                    // 没有属性加成时，给一个很小的默认提升，避免“升级无变化”
                    it->attributeBonus.add(AttributeType::DEFENSE, 1.0f);
                }

                if (auto weapon = std::dynamic_pointer_cast<Weapon>(it))
                {
                    weapon->attackDamage = weapon->attackDamage * 1.05f + 1.0f;
                }

                if (wasEquipped)
                {
                    _player->equip(it);
                }

                showToast(_panelRoot,
                          Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f),
                          StringUtils::format("已升级至 Lv%d", std::max(1, it->level)),
                          Color3B(255, 220, 160));
                upgraded = true;
                break;
            }

            if (upgraded)
            {
                showDetailOverlay();
            }
            refresh();
        },
        28.0f,
        Color3B::WHITE,
        Color3B(120, 105, 60));

    if (equipBtn && upgradeBtn)
    {
        equipBtn->setPosition(Vec2(actionXMid - 120.0f, actionY));
        upgradeBtn->setPosition(Vec2(actionXMid + 120.0f, actionY));

        const bool hasSelection = (_selectedInventoryItemId >= 0);
        equipBtn->setEnabled(hasSelection);
        upgradeBtn->setEnabled(hasSelection);
        equipBtn->setOpacity(hasSelection ? 255 : 160);
        upgradeBtn->setOpacity(hasSelection ? 255 : 160);

        _equipmentPage->addChild(equipBtn, 3);
        _equipmentPage->addChild(upgradeBtn, 3);
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
    if (auto matText = createUiLabel("所需材料：夺目催化源色 25/3", 28.0f, ITEM_TEXT_COLOR))
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
        if (auto hint = createUiLabel("请先绑定玩家", 32.0f, Color3B(200, 200, 200)))
        {
            hint->setPosition(Vec2(DESIGN_WIDTH * 0.5f, DESIGN_HEIGHT * 0.5f));
            _skillPage->addChild(hint, 1);
        }
        return;
    }

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        if (auto hint = createUiLabel("技能组件未初始化", 32.0f, Color3B(200, 200, 200)))
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
    if (auto pointsTitle = createUiLabel("现有主动技能点数", 30.0f, TITLE_COLOR, true, 3))
    {
        pointsTitle->setAnchorPoint(Vec2(0.0f, 1.0f));
        pointsTitle->setPosition(Vec2(pointsRect.getMinX() + 26.0f, pointsRect.getMaxY() - 20.0f));
        _skillPage->addChild(pointsTitle, 2);
    }
    if (auto pointsValue = createUiLabel(StringUtils::format("%d", _player->getActiveSkillPoints()), 64.0f, Color3B::WHITE, true, 3))
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
    const std::string playerName = getPlayerDisplayName();
    if (auto nameBar = createUiLabel(playerName, 34.0f, TITLE_COLOR, true, 3))
    {
        nameBar->setAnchorPoint(Vec2(0.0f, 1.0f));
        nameBar->setPosition(Vec2(statsRect.getMinX() + 30.0f, statsRect.getMaxY() - 18.0f));
        _skillPage->addChild(nameBar, 2);
    }
    if (auto levelLabel = createUiLabel(StringUtils::format("等级 %d", _player->getLevel()), 24.0f, Color3B(220, 220, 220)))
    {
        levelLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        levelLabel->setPosition(Vec2(statsRect.getMaxX() - 30.0f, statsRect.getMaxY() - 18.0f));
        _skillPage->addChild(levelLabel, 2);
    }

    // 简要战斗数据（参考截图 2 右侧）
    auto attrComp = _player->getAttributeComponent();
    const float maxHp = attrComp ? attrComp->getAttributeValue(AttributeType::MAX_HP) : 0.0f;
    const float attack = _player->getAttackPower();
    const float speed = attrComp ? attrComp->getAttributeValue(AttributeType::MOVE_SPEED) : 0.0f;
    const float crit = attrComp ? attrComp->getAttributeValue(AttributeType::CRITICAL_RATE) : 0.0f;

    auto addSmallStat = [this, statsRect](int index, const std::string &nameText, const std::string &valueText)
    {
        const float firstY = statsRect.getMaxY() - 74.0f;
        const float rowGap = 42.0f;
        const float y = firstY - rowGap * index;

        if (auto name = createUiLabel(nameText, 22.0f, ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(statsRect.getMinX() + 30.0f, y));
            _skillPage->addChild(name, 2);
        }
        if (auto value = createUiLabel(valueText, 22.0f, Color3B::WHITE))
        {
            value->setAnchorPoint(Vec2(1.0f, 0.5f));
            value->setPosition(Vec2(statsRect.getMaxX() - 30.0f, y));
            _skillPage->addChild(value, 2);
        }
    };

    addSmallStat(0, "生命", StringUtils::format("%d", static_cast<int>(std::round(maxHp))));
    addSmallStat(1, "攻击力", StringUtils::format("%d", static_cast<int>(std::round(attack))));
    addSmallStat(2, "速度", StringUtils::format("%d", static_cast<int>(std::round(speed))));
    addSmallStat(3, "暴击率", StringUtils::format("%d%%", static_cast<int>(std::round(crit * 100.0f))));

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

    // 主动技能节点：仅展示主动技能模板
    std::vector<const SkillTemplate *> activeTemplates;
    activeTemplates.reserve(_skillTemplates.size());
    for (const auto &t : _skillTemplates)
    {
        if (!t.isPassive)
        {
            activeTemplates.push_back(&t);
        }
    }

    const size_t showCount = std::min(nodePositions.size(), activeTemplates.size());
    for (size_t i = 0; i < showCount; ++i)
    {
        const auto &tpl = *activeTemplates[i];
        const bool learned = (skillComp->findLearnedSkillById(tpl.id) != nullptr);
        const bool selected = (_selectedSkillId == tpl.id);

        Color3B tint = learned ? ITEM_TEXT_COLOR : Color3B(120, 120, 120);
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
                // 未学习：点击学习（只学习，不直接装备，符合“已学习才能装备”的规则）
                if (_player->getActiveSkillPoints() <= 0)
                {
                    _selectedSkillId = id;
                    showDetailOverlay();
                    showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "主动技能点不足", Color3B(255, 220, 160));
                    refresh();
                    return;
                }

                for (const auto &t : _skillTemplates)
                {
                    if (t.id != id)
                    {
                        continue;
                    }
                    auto s = std::make_shared<ActiveSkill>();
                    s->id = t.id;
                    s->name = t.name;
                    s->description = t.description;
                    s->isPassive = false;
                    s->cooldown = t.cooldown;
                    s->manaCost = t.manaCost;
                    s->currentCooldown = 0.0f;
                    comp->learnSkill(s);
                    _player->setActiveSkillPoints(_player->getActiveSkillPoints() - 1);
                    showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "已学习", Color3B(200, 255, 200));
                    break;
                }
            }
            else
            {
                // 已学习：点击装备到当前选中槽位
                auto active = std::dynamic_pointer_cast<ActiveSkill>(skill);
                if (active)
                {
                    comp->equipActiveSkill(active, _selectedActiveSlotIndex);
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

        // 技能名字与状态（未学习/已学习）
        const Vec2 pos = nodePositions[i];
        if (auto nameLabel = createUiLabel(tpl.name, 20.0f, tint, true, 2))
        {
            nameLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
            nameLabel->setPosition(Vec2(pos.x, pos.y - 52.0f));
            treeScroll->getInnerContainer()->addChild(nameLabel, 2);
        }

        const std::string stateText = learned ? "已学习" : "未学习";
        const Color3B stateColor = learned ? Color3B(200, 255, 200) : Color3B(160, 160, 160);
        if (auto stateLabel = createUiLabel(stateText, 18.0f, stateColor, true, 2))
        {
            stateLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
            stateLabel->setPosition(Vec2(pos.x, pos.y - 76.0f));
            treeScroll->getInnerContainer()->addChild(stateLabel, 2);
        }
    }

    // 槽位选择/卸下（参考截图 2 右侧中间的“锁/槽位”位置）
    const float slotX = treeRect.getMaxX() - 140.0f;
    float slotY = treeRect.getMaxY() - 140.0f;

    const auto &activeSlots = skillComp->getActiveSlots();
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
            auto unequipBtn = createTextButton("卸下", Size(86.0f, 40.0f), [this, i](Ref *) {
                if (_player)
                {
                    if (auto comp = _player->getSkillComponent())
                    {
                        comp->unequipActiveSkill(i);
                    }
                }
                showDetailOverlay();
                refresh();
            }, 22.0f, Color3B::WHITE, Color3B(120, 60, 60));
            if (unequipBtn)
            {
                unequipBtn->setPosition(Vec2(slotX + 118.0f, slotY));
                _skillPage->addChild(unequipBtn, 3);
            }
        }

        slotY -= 90.0f;
    }
}

void InventoryLayer::refreshPassiveSkillPage()
{
    if (!_passiveSkillPage)
    {
        return;
    }
    _passiveSkillPage->removeAllChildren();

    // 布局锚点（设计坐标）
    const float safeLeft = SAFE_MARGIN_X;
    const float safeRight = DESIGN_WIDTH - SAFE_MARGIN_X;
    const float safeBottom = SAFE_MARGIN_Y;
    const float contentTop = DESIGN_HEIGHT - 240.0f;

    if (!_player)
    {
        if (auto hint = createUiLabel("请先绑定玩家", 32.0f, Color3B(200, 200, 200)))
        {
            hint->setPosition(Vec2(DESIGN_WIDTH * 0.5f, DESIGN_HEIGHT * 0.5f));
            _passiveSkillPage->addChild(hint, 1);
        }
        return;
    }

    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        if (auto hint = createUiLabel("技能组件未初始化", 32.0f, Color3B(200, 200, 200)))
        {
            hint->setPosition(Vec2(DESIGN_WIDTH * 0.5f, DESIGN_HEIGHT * 0.5f));
            _passiveSkillPage->addChild(hint, 1);
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

    // 右上：技能点面板
    const float pointsH = 240.0f;
    const Rect pointsRect(rightX, contentTop - pointsH, rightW, pointsH);
    auto pointsBg = DrawNode::create();
    drawPanelRect(pointsBg, pointsRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _passiveSkillPage->addChild(pointsBg, 1);
    if (auto pointsTitle = createUiLabel("现有被动技能点数", 30.0f, TITLE_COLOR, true, 3))
    {
        pointsTitle->setAnchorPoint(Vec2(0.0f, 1.0f));
        pointsTitle->setPosition(Vec2(pointsRect.getMinX() + 26.0f, pointsRect.getMaxY() - 20.0f));
        _passiveSkillPage->addChild(pointsTitle, 2);
    }
    if (auto pointsValue = createUiLabel(StringUtils::format("%d", _player->getPassiveSkillPoints()), 64.0f, Color3B::WHITE, true, 3))
    {
        pointsValue->setPosition(Vec2(pointsRect.getMidX(), pointsRect.getMidY()));
        _passiveSkillPage->addChild(pointsValue, 2);
    }

    // 右中：角色数据面板
    const float statsH = 240.0f;
    const Rect statsRect(rightX, pointsRect.getMinY() - 20.0f - statsH, rightW, statsH);
    auto statsBg = DrawNode::create();
    drawPanelRect(statsBg, statsRect, Color4F(0.10f, 0.10f, 0.14f, 0.85f), PANEL_BORDER_COLOR);
    _passiveSkillPage->addChild(statsBg, 1);
    const std::string playerName = getPlayerDisplayName();
    if (auto nameBar = createUiLabel(playerName, 34.0f, TITLE_COLOR, true, 3))
    {
        nameBar->setAnchorPoint(Vec2(0.0f, 1.0f));
        nameBar->setPosition(Vec2(statsRect.getMinX() + 30.0f, statsRect.getMaxY() - 18.0f));
        _passiveSkillPage->addChild(nameBar, 2);
    }
    if (auto levelLabel = createUiLabel(StringUtils::format("等级 %d", _player->getLevel()), 24.0f, Color3B(220, 220, 220)))
    {
        levelLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        levelLabel->setPosition(Vec2(statsRect.getMaxX() - 30.0f, statsRect.getMaxY() - 18.0f));
        _passiveSkillPage->addChild(levelLabel, 2);
    }

    // 简要战斗数据（与主动技能页面一致）
    auto attrComp = _player->getAttributeComponent();
    const float maxHp = attrComp ? attrComp->getAttributeValue(AttributeType::MAX_HP) : 0.0f;
    const float attack = _player->getAttackPower();
    const float speed = attrComp ? attrComp->getAttributeValue(AttributeType::MOVE_SPEED) : 0.0f;
    const float crit = attrComp ? attrComp->getAttributeValue(AttributeType::CRITICAL_RATE) : 0.0f;

    auto addSmallStat = [this, statsRect](int index, const std::string &nameText, const std::string &valueText)
    {
        const float firstY = statsRect.getMaxY() - 74.0f;
        const float rowGap = 42.0f;
        const float y = firstY - rowGap * index;

        if (auto name = createUiLabel(nameText, 22.0f, ITEM_TEXT_COLOR))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(statsRect.getMinX() + 30.0f, y));
            _passiveSkillPage->addChild(name, 2);
        }
        if (auto value = createUiLabel(valueText, 22.0f, Color3B::WHITE))
        {
            value->setAnchorPoint(Vec2(1.0f, 0.5f));
            value->setPosition(Vec2(statsRect.getMaxX() - 30.0f, y));
            _passiveSkillPage->addChild(value, 2);
        }
    };

    addSmallStat(0, "生命", StringUtils::format("%d", static_cast<int>(std::round(maxHp))));
    addSmallStat(1, "攻击力", StringUtils::format("%d", static_cast<int>(std::round(attack))));
    addSmallStat(2, "速度", StringUtils::format("%d", static_cast<int>(std::round(speed))));
    addSmallStat(3, "暴击率", StringUtils::format("%d%%", static_cast<int>(std::round(crit * 100.0f))));

    // 左侧：被动技能列表（可滚动）
    const float listBottom = 260.0f;
    const float listRight = rightX - 80.0f;
    const Rect listRect(safeLeft, listBottom, listRight - safeLeft, contentTop - listBottom);

    auto listBg = DrawNode::create();
    drawPanelRect(listBg, listRect, Color4F(0.06f, 0.06f, 0.10f, 0.25f), Color4F(0.25f, 0.25f, 0.30f, 0.45f));
    _passiveSkillPage->addChild(listBg, 0);

    auto scroll = cocos2d::ui::ScrollView::create();
    scroll->setDirection(cocos2d::ui::ScrollView::Direction::VERTICAL);
    scroll->setBounceEnabled(true);
    scroll->setScrollBarEnabled(true);
    scroll->setContentSize(listRect.size);
    scroll->setAnchorPoint(Vec2::ZERO);
    scroll->setPosition(listRect.origin);
    _passiveSkillPage->addChild(scroll, 1);

    std::vector<const SkillTemplate *> passiveTemplates;
    passiveTemplates.reserve(_skillTemplates.size());
    for (const auto &t : _skillTemplates)
    {
        if (t.isPassive)
        {
            passiveTemplates.push_back(&t);
        }
    }

    constexpr float rowH = 96.0f;
    const int itemCount = static_cast<int>(passiveTemplates.size());
    const float innerH = std::max(listRect.size.height, rowH * itemCount);
    scroll->setInnerContainerSize(Size(listRect.size.width, innerH));

    for (int i = 0; i < itemCount; ++i)
    {
        const auto &tpl = *passiveTemplates[i];
        const bool learned = (skillComp->findLearnedSkillById(tpl.id) != nullptr);
        const bool selected = (_selectedSkillId == tpl.id);

        const bool isEquipped = skillComp->isPassiveSkillEquipped(tpl.id);

        auto row = cocos2d::ui::Layout::create();
        row->setContentSize(Size(listRect.size.width, rowH));
        row->setAnchorPoint(Vec2::ZERO);
        row->setPosition(Vec2(0.0f, innerH - rowH * (i + 1)));
        row->setTouchEnabled(true);

        row->addTouchEventListener([this, id = tpl.id](Ref *, cocos2d::ui::Widget::TouchEventType type)
                                   {
                                       if (type != cocos2d::ui::Widget::TouchEventType::ENDED)
                                       {
                                           return;
                                       }
                                       _selectedSkillId = id;
                                       showDetailOverlay();
                                       refresh();
                                   });

        auto rowBg = DrawNode::create();
        Color4F rowFill = Color4F(0.10f, 0.10f, 0.14f, 0.72f);
        if (selected)
        {
            rowFill = Color4F(0.18f, 0.14f, 0.10f, 0.92f); // 选中：黄色
        }
        else if (isEquipped)
        {
            rowFill = Color4F(0.10f, 0.20f, 0.10f, 0.82f); // 已装备：绿色
        }
        drawPanelRect(rowBg,
                      Rect(0, 0, listRect.size.width, rowH),
                      rowFill,
                      Color4F(0.25f, 0.25f, 0.30f, 0.55f));
        row->addChild(rowBg, 0);

        const Color3B iconTint = isEquipped ? SELECTED_COLOR : (learned ? ITEM_ATTR_COLOR : Color3B(120, 120, 120));
        if (auto icon = createPlaceholderSprite(Size(ICON_ITEM, ICON_ITEM), iconTint))
        {
            icon->setPosition(Vec2(60.0f, rowH * 0.5f));
            row->addChild(icon, 1);
        }

        if (auto name = createUiLabel(tpl.name, 26.0f, learned ? ITEM_TEXT_COLOR : Color3B(180, 180, 180)))
        {
            name->setAnchorPoint(Vec2(0.0f, 0.5f));
            name->setPosition(Vec2(120.0f, rowH * 0.5f + 12.0f));
            row->addChild(name, 1);
        }

        const std::string stateText = learned ? (isEquipped ? "已装备" : "已学习") : "未学习";
        const Color3B stateColor = learned ? (isEquipped ? SELECTED_COLOR : Color3B(200, 255, 200)) : Color3B(160, 160, 160);
        if (auto state = createUiLabel(stateText, 22.0f, stateColor))
        {
            state->setAnchorPoint(Vec2(1.0f, 0.5f));
            state->setPosition(Vec2(listRect.size.width - 190.0f, rowH * 0.5f + 12.0f));
            row->addChild(state, 1);
        }

        // 行内操作：学习 / 装备 / 卸下（无槽位限制）
        std::string actionText;
        Color3B actionTint(90, 90, 110);
        if (!learned)
        {
            actionText = "学习";
            actionTint = Color3B(80, 90, 130);
        }
        else if (!isEquipped)
        {
            actionText = "装备";
            actionTint = Color3B(70, 110, 70);
        }
        else
        {
            actionText = "卸下";
            actionTint = Color3B(120, 60, 60);
        }

        auto actionBtn = createTextButton(actionText, Size(120.0f, 46.0f), [this, id = tpl.id](Ref *) {
            _selectedSkillId = id;
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
                // 学习
                if (_player->getPassiveSkillPoints() <= 0)
                {
                    showDetailOverlay();
                    showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "被动技能点不足", Color3B(255, 220, 160));
                    refresh();
                    return;
                }

                for (const auto &t : _skillTemplates)
                {
                    if (t.id != id)
                    {
                        continue;
                    }
                    auto s = std::make_shared<PassiveSkill>();
                    s->id = t.id;
                    s->name = t.name;
                    s->description = t.description;
                    s->isPassive = true;
                    s->attributeBonus = t.attributeBonus;
                    comp->learnSkill(s);
                    _player->setPassiveSkillPoints(_player->getPassiveSkillPoints() - 1);
                    showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "已学习", Color3B(200, 255, 200));
                    break;
                }
                showDetailOverlay();
                refresh();
                return;
            }

            if (comp->isPassiveSkillEquipped(id))
            {
                // 卸下
                if (comp->unequipPassiveSkillById(id))
                {
                    showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "已卸下", Color3B(255, 220, 160));
                }
            }
            else
            {
                // 装备（无槽位限制）
                auto passive = std::dynamic_pointer_cast<PassiveSkill>(skill);
                if (passive && comp->equipPassiveSkill(passive))
                {
                    showToast(_panelRoot, Vec2(DESIGN_WIDTH * 0.5f, SAFE_MARGIN_Y + 160.0f), "已装备", Color3B(200, 255, 200));
                }
            }

            showDetailOverlay();
            refresh();
        }, 24.0f, Color3B::WHITE, actionTint);
        if (actionBtn)
        {
            actionBtn->setAnchorPoint(Vec2(1.0f, 0.5f));
            actionBtn->setPosition(Vec2(listRect.size.width - 24.0f, rowH * 0.5f + 12.0f));
            row->addChild(actionBtn, 2);
        }

        scroll->getInnerContainer()->addChild(row, 1);
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
    switchTab(Tab::ACTIVE_SKILL);
}

void InventoryLayer::onTabPassiveSkillClicked(Ref *)
{
    switchTab(Tab::PASSIVE_SKILL);
}
