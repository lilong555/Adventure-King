/**
 * @file InventoryLayer.h
 * @brief 背包/技能管理界面（占位实现）
 *
 * 目标：
 * - 管理装备：查看背包物品、穿戴/卸下装备
 * - 管理技能：学习技能、装备主动/被动技能到槽位、卸下技能
 *
 * 说明：
 * - 图标与美术资源先使用占位 UI（DrawNode + 文本）替代，后续可接入真实图标与排版。
 */

#pragma once

#include "Character/Base/CharacterData.h"
#include "cocos2d.h"
#include <functional>
#include <string>
#include <vector>

class PlayerCharacter;

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
        SKILL
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

    // 工具
    cocos2d::Sprite *createPlaceholderSprite(const cocos2d::Size &targetSize,
                                             const cocos2d::Color3B &tint = cocos2d::Color3B::WHITE);

    cocos2d::MenuItemSprite *createIconButton(const cocos2d::Size &targetSize,
                                              const cocos2d::ccMenuCallback &callback,
                                              const cocos2d::Color3B &tint = cocos2d::Color3B::WHITE);

    void createDetailOverlay();
    void showDetailOverlay();
    void hideDetailOverlay();

    void onCloseClicked(cocos2d::Ref *sender);
    void onTabEquipmentClicked(cocos2d::Ref *sender);
    void onTabSkillClicked(cocos2d::Ref *sender);

private:
    PlayerCharacter *_player = nullptr;
    bool _isShowing = false;
    Tab _currentTab = Tab::EQUIPMENT;

    // 当前选中槽位（用于装备技能）
    size_t _selectedActiveSlotIndex = 0;
    size_t _selectedPassiveSlotIndex = 0;

    // 当前选中的装备槽位（-1 表示未选中）
    int _selectedEquipSlotIndex = -1;

    std::function<void()> _closeCallback;

    cocos2d::Node *_container = nullptr;
    cocos2d::DrawNode *_background = nullptr;
    cocos2d::Node *_panelRoot = nullptr;
    cocos2d::DrawNode *_panel = nullptr;
    cocos2d::Sprite *_titleSprite = nullptr;

    cocos2d::Menu *_tabMenu = nullptr;
    cocos2d::MenuItemSprite *_tabEquipment = nullptr;
    cocos2d::MenuItemSprite *_tabSkill = nullptr;

    cocos2d::Menu *_closeMenu = nullptr;

    cocos2d::Node *_equipmentPage = nullptr;
    cocos2d::Node *_skillPage = nullptr;

    cocos2d::EventListenerTouchOneByOne *_touchListener = nullptr;
    cocos2d::EventListenerTouchOneByOne *_detailOverlayListener = nullptr;

    std::vector<SkillTemplate> _skillTemplates;

    // 详情图弹层：用于在点击后显示“更详细”的图片（目前用占位图）
    cocos2d::Node *_detailOverlay = nullptr;
    cocos2d::DrawNode *_detailOverlayBg = nullptr;
    cocos2d::Sprite *_detailOverlaySprite = nullptr;
};
