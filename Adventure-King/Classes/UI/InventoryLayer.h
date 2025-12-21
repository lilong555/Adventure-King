/**
 * @file InventoryLayer.h
 * @brief 仓库界面（技能/装备/属性，占位实现）
 *
 * 目标：
 * - 管理装备：查看背包物品、穿戴/卸下装备
 * - 管理技能：学习技能、装备主动/被动技能到槽位、卸下技能
 * - 查看属性：展示角色属性/等级等信息（当前为占位排版）
 *
 * 说明：
 * - 图标暂用 PNG 占位（TNT.png）；名称/等级/属性等信息使用文字渲染，便于调试与后续替换美术字。
 */

#pragma once

#include "Character/Base/CharacterData.h"
#include "cocos2d.h"
#include <functional>
#include <string>
#include <vector>

class PlayerCharacter;

namespace cocos2d
{
namespace ui
{
class Button;
}
}

class InventoryLayer final : public cocos2d::Layer
{
public:
    static InventoryLayer *create();
    bool init() override;

    void bindPlayer(PlayerCharacter *player);

    void show();
    void hide();
    bool isShowing() const { return _isShowing; }

    void setCloseCallback(const std::function<void()> &callback) { _closeCallback = callback; }

private:
    enum class Tab
    {
        EQUIPMENT,
        SKILL,
        ATTRIBUTE
    };

    struct SkillTemplate
    {
        int id = 0;
        bool isPassive = false;
        std::string name;
        std::string description;

        // 主动技能
        float cooldown = 0.0f;
        float manaCost = 0.0f;

        // 被动技能
        Attributes attributeBonus;
    };

    void buildSkillTemplates();

    void createBackground();
    void createPanel();
    void createTabs();
    void createPages();

    void switchTab(Tab tab);
    void refresh();
    void refreshEquipmentPage();
    void refreshSkillPage();
    void refreshAttributePage();

    // 工具
    cocos2d::Sprite *createPlaceholderSprite(const cocos2d::Size &targetSize,
                                             const cocos2d::Color3B &tint = cocos2d::Color3B::WHITE);

    cocos2d::ui::Button *createIconButton(const cocos2d::Size &targetSize,
                                          const std::function<void(cocos2d::Ref *)> &callback,
                                          const cocos2d::Color3B &tint = cocos2d::Color3B::WHITE);

    void createDetailOverlay();
    void showDetailOverlay();
    void hideDetailOverlay();

    void onCloseClicked(cocos2d::Ref *sender);
    void onTabEquipmentClicked(cocos2d::Ref *sender);
    void onTabSkillClicked(cocos2d::Ref *sender);
    void onTabAttributeClicked(cocos2d::Ref *sender);

private:
    PlayerCharacter *_player = nullptr;
    bool _isShowing = false;
    Tab _currentTab = Tab::EQUIPMENT;

    // 当前选中槽位（用于装备技能）
    size_t _selectedActiveSlotIndex = 0;
    size_t _selectedPassiveSlotIndex = 0;

    // 当前选中的装备槽位（-1 表示未选中）
    int _selectedEquipSlotIndex = -1;

    // 当前选中的背包物品 id（-1 表示未选中）
    int _selectedInventoryItemId = -1;

    // 当前选中的技能 id（-1 表示未选中）
    int _selectedSkillId = -1;

    std::function<void()> _closeCallback;

    cocos2d::Node *_container = nullptr;
    cocos2d::DrawNode *_background = nullptr;
    cocos2d::Node *_panelRoot = nullptr;
    cocos2d::DrawNode *_panel = nullptr;
    cocos2d::Sprite *_titleSprite = nullptr;

    cocos2d::ui::Button *_tabEquipment = nullptr;
    cocos2d::ui::Button *_tabSkill = nullptr;
    cocos2d::ui::Button *_tabAttribute = nullptr;
    cocos2d::ui::Button *_closeButton = nullptr;

    cocos2d::Label *_tabSkillLabel = nullptr;
    cocos2d::Label *_tabEquipmentLabel = nullptr;
    cocos2d::Label *_tabAttributeLabel = nullptr;
    cocos2d::Label *_closeHintLabel = nullptr;

    cocos2d::Node *_equipmentPage = nullptr;
    cocos2d::Node *_skillPage = nullptr;
    cocos2d::Node *_attributePage = nullptr;

    cocos2d::EventListenerTouchOneByOne *_detailOverlayListener = nullptr;

    std::vector<SkillTemplate> _skillTemplates;

    // 详情图弹层：用于在点击后显示“更详细”的图片（目前用占位图）
    cocos2d::Node *_detailOverlay = nullptr;
    cocos2d::DrawNode *_detailOverlayBg = nullptr;
    cocos2d::Sprite *_detailOverlaySprite = nullptr;
};
