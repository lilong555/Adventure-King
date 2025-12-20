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
#include "Objects/Projectiles/Bomb.h"
#include "Configs/GameConfigs.h"
#include "Character/Base/CharacterData.h"
#include <memory>

// 前向声明
class PlayerCharacter;

//=============================================================================
// 物理碰撞分类掩码
//=============================================================================
/**
 * @brief 物理碰撞分类掩码枚举
 *
 * 用于设置物理刚体的碰撞分类和掩码，控制哪些物体之间可以发生碰撞。
 * 使用位掩码（bitmask）方式，支持组合多个分类。
 *
 * @example 设置玩家只与平台碰撞：
 *   physicsBody->setCategoryBitmask(CATEGORY_PLAYER);
 *   physicsBody->setCollisionBitmask(CATEGORY_PLATFORM);
 */
//enum PhysicsCategory
//{
//    CATEGORY_NONE = 0,          ///< 无碰撞
//    CATEGORY_PLAYER = 1 << 0,   ///< 玩家 (0x01)
//    CATEGORY_PLATFORM = 1 << 1, ///< 平台/地面 (0x02)
//    CATEGORY_BOMB = 1 << 2,     ///< 炸弹/投掷物 (0x04)
//    CATEGORY_ENEMY = 1 << 3,    ///< 敌人/木桩 (0x08)
//    CATEGORY_ALL = 0xFFFFFFFF   ///< 所有类别
//};

//=============================================================================
// 游戏对象结构体定义
//=============================================================================

/**
 * @brief 木桩（靶子）数据结构
 *
 * 木桩是用于测试攻击系统的静态目标，拥有血量和血条显示。
 * 可以被玩家的普通攻击和技能攻击命中。
 */
struct TargetDummy
{
    cocos2d::Sprite *sprite = nullptr;  ///< 木桩精灵节点
    float maxHP = 1000.0f;              ///< 最大生命值
    float currentHP = 1000.0f;          ///< 当前生命值
    cocos2d::DrawNode *hpBar = nullptr; ///< 血条绘制节点
    cocos2d::Label *hpLabel = nullptr;  ///< 血量数值显示标签
};

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
 * - W/空格：跳跃
 * - E：丢炸弹
 * - 4：攻击
 * - 1-5：测试按钮快捷键
 * - R：重置角色
 * - ESC：返回地图
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
     * @brief 初始化场景
     * @return 初始化是否成功
     */
    virtual bool init() override;

    /// cocos2d-x 自动生成 create() 方法
    CREATE_FUNC(DebugScene);

private:
    //=========================================================================
    // 初始化方法
    //=========================================================================

    void initBackground();     ///< 初始化背景和网格
    void initPlayer();         ///< 初始化玩家角色
    void initDebugUI();        ///< 初始化调试UI面板
    void initControlButtons(); ///< 初始化控制按钮

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

    void onPoisonClicked(cocos2d::Ref *sender);  ///< 添加中毒效果（5秒）
    void onExcitedClicked(cocos2d::Ref *sender); ///< 添加亢奋效果（8秒）
    void onStunnedClicked(cocos2d::Ref *sender); ///< 添加眩晕效果（3秒）
    void applyPoisonDamage(float dt);            ///< 应用中毒持续伤害

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
    void updateConditionalPassives();                  ///< 更新条件性被动效果

    //=========================================================================
    // 输入处理
    //=========================================================================

    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);

    //=========================================================================
    // 角色移动与动画
    //=========================================================================

    void updatePlayerMovement(float dt); ///< 更新玩家移动（物理驱动）

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

    void jump(); ///< 执行跳跃

    //=========================================================================
    // 攻击系统
    //=========================================================================

    void onAttackAnimationFinished(); ///< 攻击动画结束回调

    //=========================================================================
    // 技能系统
    //=========================================================================

    void initPlayerSkills();         ///< 初始化玩家技能
    void onSkillAnimationFinished(); ///< 技能动画结束回调
    void throwBomb();                ///< 释放炸弹技能（入口）
    void doThrowBomb();              ///< 实际创建并投掷炸弹

    //=========================================================================
    // 木桩/敌人系统
    //=========================================================================

    void initTargetDummy(); ///< 初始化测试用木桩

    /**
     * @brief 对木桩造成伤害
     * @param damage 伤害值
     * @param isCrit 是否暴击
     */
    void dealDamageToTarget(float damage, bool isCrit = false);

    /**
     * @brief 显示伤害数字飘字
     * @param pos 显示位置
     * @param damage 伤害值
     * @param isCrit 是否暴击（影响显示颜色和大小）
     */
    void showDamageNumber(const cocos2d::Vec2 &pos, float damage, bool isCrit = false);

    void updateTargetHPBar(); ///< 更新木桩血条显示

    //=========================================================================
    // 日志系统
    //=========================================================================

    void addDamageLog(const std::string &log); ///< 添加一条日志到伤害日志面板

private:
    //=========================================================================
    // 成员变量 - 核心对象
    //=========================================================================

    PlayerCharacter *_player = nullptr; ///< 玩家角色实例

    //=========================================================================
    // 成员变量 - 移动状态
    //=========================================================================

    bool _isMovingLeft = false;           ///< 是否正在向左移动
    bool _isMovingRight = false;          ///< 是否正在向右移动
    bool _isRunPressed = false;           ///< 是否按下跑步键（Shift）
    float _moveSpeed = 200.0f;            ///< 基础移动速度（像素/秒）

    //=========================================================================
    // 成员变量 - 战斗状态
    //=========================================================================

    bool _isAttacking = false;    ///< 是否正在执行攻击动画
    bool _isCastingSkill = false; ///< 是否正在施放技能

    //=========================================================================
    // 成员变量 - 物理系统
    //=========================================================================

    bool _isGrounded = false;    ///< 是否在地面上（通过碰撞检测更新）
    int _groundContactCount = 0; ///< 与地面接触的计数（处理多平台边缘情况）
    int _jumpCount = 0;          ///< 当前空中已跳次数（落地重置）

    /// 跳跃冲量（数值越大跳得越高）
    static constexpr float JUMP_IMPULSE = GameConfig::Debug::JUMP_IMPULSE;

    /// 地面Y坐标基准线
    static constexpr float GROUND_Y = GameConfig::Debug::GROUND_Y;

    /// 平台列表（存储平台的矩形区域，用于可视化）
    std::vector<cocos2d::Rect> _platforms;

    //=========================================================================
    // 成员变量 - 炸弹系统
    //=========================================================================

    std::vector<Bomb> _bombs; ///< 当前场景中的炸弹列表

    static constexpr float BOMB_THROW_SPEED_X = GameConfig::Bomb::THROW_SPEED_X;   ///< 炸弹水平初速度
    static constexpr float BOMB_THROW_SPEED_Y = GameConfig::Bomb::THROW_SPEED_Y;   ///< 炸弹垂直初速度
    static constexpr float BOMB_DAMAGE = GameConfig::Bomb::BASE_DAMAGE;            ///< 炸弹基础伤害
    static constexpr float BOMB_EXPLOSION_RADIUS = GameConfig::Bomb::EXPLOSION_RADIUS; ///< 爆炸范围半径

    //=========================================================================
    // 成员变量 - 技能配置
    //=========================================================================

    static constexpr size_t BOMB_SKILL_SLOT = GameConfig::Skill::SLOT_BOMB;       ///< 炸弹技能所在槽位索引
    static constexpr int BOMB_SKILL_ID = GameConfig::Bomb::BOMB_ID;               ///< 炸弹技能唯一ID
    static constexpr float BOMB_SKILL_MP_COST = GameConfig::Bomb::BOMB_MP;        ///< 炸弹技能MP消耗
    static constexpr float BOMB_SKILL_COOLDOWN = GameConfig::Bomb::BOMB_CD;       ///< 炸弹技能冷却时间（秒）

    //=========================================================================
    // 成员变量 - 木桩（测试靶子）
    //=========================================================================

    TargetDummy _targetDummy; ///< 测试用木桩实例

    //=========================================================================
    // 成员变量 - UI标签
    //=========================================================================

    cocos2d::Label *_infoLabel = nullptr;         ///< 角色属性信息面板
    cocos2d::Label *_stateLabel = nullptr;        ///< 角色状态显示
    cocos2d::Label *_statusEffectLabel = nullptr; ///< 状态效果列表
    cocos2d::Label *_damageLogLabel = nullptr;    ///< 伤害日志面板
    cocos2d::Label *_hpLabel = nullptr;           ///< HP数值标签
    cocos2d::Label *_mpLabel = nullptr;           ///< MP数值标签
    cocos2d::Label *_equipmentLabel = nullptr;    ///< 当前装备显示
    cocos2d::Label *_passiveSkillLabel = nullptr; ///< 被动技能列表

    //=========================================================================
    // 成员变量 - HP/MP进度条
    //=========================================================================

    cocos2d::DrawNode *_hpBarBg = nullptr;   ///< HP进度条背景
    cocos2d::DrawNode *_hpBarFill = nullptr; ///< HP进度条填充
    cocos2d::DrawNode *_mpBarBg = nullptr;   ///< MP进度条背景
    cocos2d::DrawNode *_mpBarFill = nullptr; ///< MP进度条填充

    //=========================================================================
    // 成员变量 - 状态效果
    //=========================================================================

    bool _isPoisoned = false; ///< 中毒状态标记（用于持续伤害计时）

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

    bool _hasFullHpCritPassive = false; ///< 是否已学习满血暴击被动
    bool _isFullHpCritActive = false;   ///< 满血暴击效果是否激活中

    //=========================================================================
    // 成员变量 - 日志系统
    //=========================================================================

    std::vector<std::string> _damageLog;   ///< 伤害日志记录
    static constexpr size_t MAX_LOG_LINES = GameConfig::Debug::MAX_LOG_LINES; ///< 日志最大显示行数

    //=========================================================================
    // 成员变量 - 死亡重置系统
    //=========================================================================

    bool _isDeathResetPending = false;               ///< 是否正在等待死亡重置
    float _deathResetTimer = 0.0f;                   ///< 死亡重置倒计时
    static constexpr float DEATH_RESET_DELAY = GameConfig::Debug::DEATH_RESET_DELAY; ///< 死亡后重置延迟（秒）
};
