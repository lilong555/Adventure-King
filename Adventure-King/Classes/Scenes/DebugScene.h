/**
 * @file DebugScene.h
 * @brief 角色功能调试场景 - 可作为正式关卡开发的参考模板
 *
 * 本场景展示了以下功能的完整实现：
 * - 物理引擎集成（重力、碰撞检测、刚体控制）
 * - 角色移动与动画系统
 * - 技能系统（主动技能、MP消耗、冷却时间）
 * - 装备系统（武器切换、属性加成）
 * - 被动技能系统（常规被动、条件性被动）
 * - 状态效果系统（中毒、亢奋、眩晕等）
 * - UI系统（HP/MP条、属性面板、伤害日志）
 *
 * @author Adventure-King Team
 * @version 1.0
 */

#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Configs/GameConfigs.h"
#include "Scenes/GameInputController.h"
#include "Scenes/GameUIController.h"
#include "Character/Base/CharacterData.h"
#include <memory>

// 前向声明
class PlayerCharacter;
class MonsterBase;


//=============================================================================
// DebugScene 类定义
//=============================================================================

/**
 * @brief 角色功能调试场景类
 *
 * 本类是一个完整的游戏场景实现示例，包含了游戏开发中常见的各种系统。
 * 可以作为开发正式关卡时的参考模板。
 *
 * ## 主要功能模块：
 *
 * ### 1. 物理系统
 * - 使用 cocos2d-x 内置物理引擎
 * - 支持重力、碰撞检测、刚体控制
 * - 平台跳跃机制
 *
 * ### 2. 角色控制
 * - 键盘输入响应（WASD移动、空格跳跃）
 * - 行走/攻击/技能动画
 * - 状态机驱动的角色行为
 *
 * ### 3. 战斗系统
 * - 普通攻击（基于装备属性）
 * - 技能攻击（MP消耗、冷却时间）
 * - 伤害计算（力量、暴击率、穿透）
 *
 * ### 4. 装备系统
 * - 武器切换（剑、法杖、匕首）
 * - 属性加成计算
 * - 装备特殊效果
 *
 * ### 5. 技能系统
 * - 主动技能槽位
 * - 被动技能（常规/条件性）
 * - 状态效果（中毒、亢奋、眩晕）
 *
 * ## 使用快捷键：
 * - A/D：左右移动
 * - Shift：跑步
 * - W/空格：跳跃
 * - 4/J：普攻
 * - E/K/Q/R/F：技能槽 0/1/2/3
 * - 1/2/3/5：调试快捷键（受击/暴击/治疗/升级）
 * - ESC：暂停菜单（背包/存档等）
 */
class DebugScene : public cocos2d::Scene
{
public:
    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * @brief 创建场景实例
     * @return 场景指针
     */
    static cocos2d::Scene *createScene();

    /**
     * @brief 注册到 LoadingScene 的场景注册表（供 LoadingScene 根据 mapId 创建）
     * @note 需要在 AppDelegate 启动时调用一次
     */
    static void setupRegistry();

    /**
     * @brief 初始化场景
     * @return 初始化是否成功
     */
    virtual bool init() override;
    /// @brief 场景进入：禁用输入法（IME），避免抢输入
    virtual void onEnter() override;
    /// @brief 场景退出：恢复输入法（IME）
    virtual void onExit() override;

    /**
     * @brief 析构：在 .cpp 中定义，确保 unique_ptr 释放时类型完整
     */
    ~DebugScene() override;

    /// cocos2d-x 自动生成 create() 方法
    CREATE_FUNC(DebugScene);

private:
    /**
     * @brief 构造函数放在 .cpp 中定义，避免隐式构造函数在头文件中实例化 unique_ptr 的析构逻辑
     * @note 主要用于解决 Win32/MSVC 下 “can't delete an incomplete type” 的编译错误
     */
    DebugScene();

    //=========================================================================
    // 初始化方法
    //=========================================================================

    void initBackground();     ///< 初始化背景和网格
    void initPlayer();         ///< 初始化玩家角色
    void initDebugUI();        ///< 初始化调试UI面板
    void initControlButtons(); ///< 初始化控制按钮
    void initGameUIController(); ///< 初始化与 GameScene 同款的 UI（暂停/背包/技能栏等）
    void initInputController(); ///< 初始化与 GameScene 同款的输入控制器

    /// @brief 返回地图选择界面（与 GameScene 行为一致）
    void returnToMapScene();
    /// @brief 切换暂停菜单（与 GameScene 行为一致）
    void togglePauseMenu();
    /// @brief 设置暂停/恢复（只冻结世界，UI 仍可交互）
    void setGamePaused(bool paused);

    //=========================================================================
    // 主循环更新
    //=========================================================================

    virtual void update(float dt) override; ///< 每帧更新
    void updateDebugInfo();                 ///< 更新调试信息显示

    //=========================================================================
    // UI按钮回调 - 基础功能
    //=========================================================================

    void onTakeDamageClicked(cocos2d::Ref *sender);         ///< 受击（10伤害）
    void onTakeCriticalDamageClicked(cocos2d::Ref *sender); ///< 暴击（25伤害）
    void onHealClicked(cocos2d::Ref *sender);               ///< 治疗（+20HP）
    void onAttackClicked(cocos2d::Ref *sender);             ///< 攻击按钮
    void onLevelUpClicked(cocos2d::Ref *sender);            ///< 升级按钮
    void onResetClicked(cocos2d::Ref *sender);              ///< 重置角色
    void onBackClicked(cocos2d::Ref *sender);               ///< 返回地图

    //=========================================================================
    // UI按钮回调 - 状态效果
    //=========================================================================

    void applyDebugEffect(StatusEffectType type, float power, float duration, const std::string& logMsg);
    void onPoisonClicked(cocos2d::Ref *sender);  ///< 添加中毒效果（5秒）
    void onExcitedClicked(cocos2d::Ref *sender); ///< 添加亢奋效果（8秒）
    void onStunnedClicked(cocos2d::Ref *sender); ///< 添加眩晕效果（3秒）
    void onBurnningClicked(cocos2d::Ref* sender);///< 添加灼烧效果（5秒）
    //=========================================================================
    // 装备系统
    //=========================================================================

    void initEquipments();                             ///< 初始化测试用装备
    void onEquipSwordClicked(cocos2d::Ref *sender);    ///< 装备剑
    void onEquipStaffClicked(cocos2d::Ref *sender);    ///< 装备法杖
    void onEquipDaggerClicked(cocos2d::Ref *sender);   ///< 装备匕首
    void onUnequipWeaponClicked(cocos2d::Ref *sender); ///< 卸下武器

    /**
     * @brief 装备变更回调
     * @param slot 装备槽位
     * @param equipment 新装备（nullptr表示卸下）
     */
    void onEquipmentChanged(EquipmentSlot slot, const std::shared_ptr<Equipment> &equipment);

    //=========================================================================
    // 被动技能系统
    //=========================================================================

    void initPassiveSkills();                          ///< 初始化被动技能
    void onLearnPassive1Clicked(cocos2d::Ref *sender); ///< 学习被动1：力量+5
    void onLearnPassive2Clicked(cocos2d::Ref *sender); ///< 学习被动2：防御+3
    void onLearnPassive3Clicked(cocos2d::Ref *sender); ///< 学习被动3：满血暴击
    void updatePassiveSkillLabel();                    ///< 更新被动技能UI显示

    //=========================================================================
    // 输入处理
    //=========================================================================

    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);

    //=========================================================================
    // 物理系统
    //=========================================================================

    void initPlatforms();              ///< 初始化平台和地面
    void initPhysicsContactListener(); ///< 初始化物理碰撞监听器

    /**
     * @brief 碰撞开始回调
     * @param contact 碰撞信息
     * @return 是否处理该碰撞
     */
    bool onContactBegin(cocos2d::PhysicsContact &contact);

    /**
     * @brief 碰撞分离回调
     * @param contact 碰撞信息
     */
    void onContactSeparate(cocos2d::PhysicsContact &contact);

    //=========================================================================
    // 测试怪物
    //=========================================================================

    void initTestMonsters(); ///< 初始化测试用怪物（用于验证命中/受击/状态效果）

    //=========================================================================
    // 日志系统
    //=========================================================================

    void addDamageLog(const std::string &log); ///< 添加一条日志到伤害日志面板

private:
    //=========================================================================
    // 成员变量 - 核心对象
    //=========================================================================

    PlayerCharacter *_player = nullptr; ///< 玩家角色实例
    MonsterBase *_boss = nullptr; ///< Boss（用于对齐 GameScene 的 Boss UI 解绑逻辑）
    std::unique_ptr<GameInputController> _inputController; ///< 与 GameScene 同款输入控制器
    std::unique_ptr<GameUIController> _uiController; ///< 与 GameScene 同款 UI 编排
    bool _isPaused = false; ///< 是否暂停（由 GameUIController 回调维护）
    cocos2d::Node *_gameLayer = nullptr; ///< 游戏内容层（暂停时冻结，避免影响 UI）
    bool _cachedPhysicsAutoStep = true; ///< 暂停前物理世界 autoStep 状态
    float _cachedPhysicsSpeed = 1.0f;   ///< 暂停前物理世界 speed 值

    /// 地面Y坐标基准线
    static constexpr float GROUND_Y = GameSceneConfig::Debug::GROUND_Y;

    /// 平台列表（存储平台的矩形区域，用于可视化）
    std::vector<cocos2d::Rect> _platforms;

    //=========================================================================
    // 成员变量 - 测试怪物
    //=========================================================================

    std::vector<MonsterBase*> _testMonsters; ///< 测试用怪物列表（由场景持有）

    //=========================================================================
    // 成员变量 - UI标签
    //=========================================================================

    cocos2d::Label *_infoLabel = nullptr;         ///< 角色属性信息面板
    cocos2d::Label *_stateLabel = nullptr;        ///< 角色状态显示
    cocos2d::Label *_statusEffectLabel = nullptr; ///< 状态效果列表
    cocos2d::Label *_damageLogLabel = nullptr;    ///< 伤害日志面板
    cocos2d::Label *_equipmentLabel = nullptr;    ///< 当前装备显示
    cocos2d::Label *_passiveSkillLabel = nullptr; ///< 被动技能列表

    //=========================================================================
    // 成员变量 - 装备系统
    //=========================================================================

    std::shared_ptr<Weapon> _swordWeapon;  ///< 测试用剑武器
    std::shared_ptr<Weapon> _staffWeapon;  ///< 测试用法杖武器
    std::shared_ptr<Weapon> _daggerWeapon; ///< 测试用匕首武器

    //=========================================================================
    // 成员变量 - 被动技能系统
    //=========================================================================

    std::shared_ptr<PassiveSkill> _passiveSkill1; ///< 被动技能1：力量精通
    std::shared_ptr<PassiveSkill> _passiveSkill2; ///< 被动技能2：铁壁
    std::shared_ptr<PassiveSkill> _passiveSkill3; ///< 被动技能3：满血暴击

    //=========================================================================
    // 成员变量 - 日志系统
    //=========================================================================

    std::vector<std::string> _damageLog;   ///< 伤害日志记录
    static constexpr size_t MAX_LOG_LINES = GameSceneConfig::Debug::MAX_LOG_LINES; ///< 日志最大显示行数

    //=========================================================================
    // 成员变量 - 死亡重置系统
    //=========================================================================

    bool _isDeathResetPending = false;               ///< 是否正在等待死亡重置
    float _deathResetTimer = 0.0f;                   ///< 死亡重置倒计时
    static constexpr float DEATH_RESET_DELAY = GameSceneConfig::Debug::DEATH_RESET_DELAY; ///< 死亡后重置延迟（秒）
};
