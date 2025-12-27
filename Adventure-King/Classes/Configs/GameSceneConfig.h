/**
 * @file GameSceneConfig.h
 * @brief 场景相关配置
 */

#pragma once

#include <string>
#include <vector>

 /// 统一枚举关卡场景 ID 
enum class SceneID
{
    NONE = 0,

    // UI 场景
    HELLO_WORLD = 1,          // 主菜单 HelloWorld
    MAP = 2,                  // 地图选择 MapScene
    LOADING = 3,              // 加载界面 LoadingScene

    // 基础功能场景
    HOME = 100,             // 冒险王之家
    DEBUG = 101,            // 调试场景
    // 战斗关卡 (300系列)
    LEVEL_ORIGIN_MUSHROOM = 301, // 起源之菇
    LEVEL_MYSTERY_FOREST = 302,   // 神秘之森
};
/**
      * @brief 关卡场景配置
      */
struct LevelConfig
{
    SceneID id = SceneID::NONE;

    std::string tmxMapPath;
    std::string backgroundPath;
    std::vector<std::string> backgroundSeriesPaths;
    std::string playerSpritePath;

    std::string collisionLayerName = "collisions";
    std::string bornLayerName = "born";
    std::string gateLayerName = "gate";

    float gravity = -1000.0f;
    bool enablePhysicsDebug = false;
};


 namespace GameSceneConfig{

     namespace UI {
         namespace Loading {
             constexpr float BAR_WIDTH_RATIO = 0.75f;
             constexpr float BAR_HEIGHT = 16.0f;
             constexpr float BAR_BOTTOM_PADDING = 22.0f;
             constexpr float BAR_FILL_PADDING = 2.0f; // 统一管理
         }
         namespace RoleSelectLayer {
            // ==========================================================
            // 布局常量（集中管理，避免散落“魔法数字”）
            // ==========================================================
             constexpr int kOverlayAlpha = 160;
             constexpr float kPanelWidthRatio = 0.80f;
             constexpr float kPanelHeightRatio = 0.65f;
             constexpr float kMaxPanelWidth = 900.0f;
             constexpr float kMaxPanelHeight = 520.0f;
             constexpr float kTitleTopPadding = 18.0f;

             constexpr float kPreviewXRatio = 0.72f;
             constexpr float kPreviewBottomRatio = 0.18f;
             constexpr float kPreviewHeightRatio = 0.62f;

             constexpr float kRoleListXRatio = 0.10f;
             constexpr float kRoleListTopRatio = 0.70f;
             constexpr float kRoleListGap = 58.0f;

             constexpr float kActionYRatio = 0.12f;
             constexpr float kConfirmXRatio = 0.35f;
             constexpr float kCancelXRatio = 0.55f;
         }
     }


     namespace Scene
     {
         const char* const DEFAULT_FONT_PATH = "fonts/ZCOOLKuaiLe-Regular.ttf";
         const char* const DEFAULT_PLAYER_SPRITE = "Sprites/Characters/Player/man/default/spr_man_run.png";
         const char* const MAP_LOAD_FAILED_TEXT = " - Map Load Failed";
         // 场景切换
         inline constexpr float TRANSITION_DURATION = 0.5f;
         inline constexpr float MENU_TRANSITION_DURATION = 0.6f;
         inline constexpr float TRANSITION_MESSAGE_DELAY = 1.0f;
         inline constexpr float TRANSITION_FADE_DURATION = 0.8f;
         // 层级
         inline constexpr int BACKGROUND_Z_ORDER = -1;
         inline constexpr int PLAYER_Z_ORDER = 5;
         inline constexpr int COLLISION_DEBUG_Z_ORDER = 100;
     }

     namespace Map
     {
         namespace OriginMushroom
         {
             inline constexpr int BACKGROUND_COUNT = 6;
             const char* const BACKGROUND_PREFIX = "Map/Origin_Mushroom/Origin_Mushroom_";
         }
     }
     namespace UI
     {
         const char* const GATE_INTERACTION_HINT = "Press W to enter gate";
         inline constexpr int Z_ORDER = 100;
         inline constexpr float UPDATE_INTERVAL_SECONDS = 0.05f;
         inline constexpr int SKILL_BAR_SLOT_COUNT = 4;

         inline constexpr float PADDING = 20.0f;
         inline constexpr float STATUS_BAR_OFFSET_X = 50.0f;
         // 技能栏默认 4 个槽位：偏移适当增大，避免最右侧槽位超出屏幕
         inline constexpr float SKILL_BAR_OFFSET_X = 220.0f;
         inline constexpr float SKILL_BAR_OFFSET_Y = 80.0f;
         inline constexpr float BOSS_BAR_OFFSET_Y = 60.0f;
         inline constexpr float MAP_BUTTON_OFFSET = 40.0f;
         inline constexpr float INTERACTION_HINT_OFFSET_Y = 80.0f;
         inline constexpr float LEVEL_NAME_OFFSET_X = 100.0f;
         inline constexpr float LEVEL_NAME_OFFSET_Y = 50.0f;

         namespace MainMenu
         {
             // 主菜单布局与音乐
             inline constexpr float BUTTON_HORIZONTAL_SPACING = 180.0f;
             inline constexpr float SUB_MENU_Y_MULTIPLIER = 1.2f;
             inline constexpr float MENU_OFFSET_Y_DIVISOR = 20.0f;
             inline constexpr int CONTENT_Z_ORDER = 5;
             inline constexpr int MENU_Z_ORDER = 1;
             inline constexpr float BGM_VOLUME = 0.5f;
             inline constexpr float BGM_DELAY_SECONDS = 0.1f;
         }

         namespace SaveMenu
         {
             // 存档菜单整体缩放
             inline constexpr float TARGET_HEIGHT_RATIO = 0.8f;
         }

         namespace SettingMenu
         {
             // 设置菜单整体缩放
             inline constexpr float TARGET_HEIGHT_RATIO = 0.8f;
         }
     }

     namespace Save
     {
         // 存档槽位与自动存档
         inline constexpr int MAX_SLOTS = 3;
         inline constexpr float AUTO_SAVE_INTERVAL_SECONDS = 300.0f;
     }

     namespace Debug
     {
         // 调试场景常量
         inline constexpr float JUMP_IMPULSE = 350.0f;
         inline constexpr float GROUND_Y = 100.0f;
         inline constexpr size_t MAX_LOG_LINES = 5;
         inline constexpr float DEATH_RESET_DELAY = 2.0f;
     }
}
