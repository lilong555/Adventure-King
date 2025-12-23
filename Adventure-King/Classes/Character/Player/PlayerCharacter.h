#pragma once

#include "Character/Base/CharacterBase.h"
#include "Character/Base/CharacterData.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

// 前向声明，减少头文件依赖
class PlayerSkillSet;
struct Equipment;
struct Weapon;

class PlayerCharacter : public CharacterBase
{
public:
    // =============================================================
    // 生命周期与创建
    // =============================================================
    /// @brief 创建玩家实例
    static PlayerCharacter* create(CharacterRole role, const std::string& spriteFrameName);
    /// @brief 析构玩家实例
    virtual ~PlayerCharacter();

    /// @brief 初始化玩家（角色、资源、组件）
    bool init(CharacterRole role, const std::string& spriteFrameName);

    /// @brief 进入场景回调
    virtual void onEnter() override;
    /// @brief 离开场景回调
    virtual void onExit() override;
    /// @brief 每帧更新
    virtual void update(float dt) override;

    // =============================================================
    // 角色状态与属性 (Level, Exp, HP/MP)
    // =============================================================
    /// @brief 增加经验并处理升级
    void addExperience(int amount);

    // 存档与数据访问
    /// @brief 获取职业
    CharacterRole getRole() const { return _role; }
    /// @brief 设置职业
    void setRole(CharacterRole role) { _role = role; }

    /// @brief 获取主动技能点
    int getActiveSkillPoints() const { return _activeSkillPoints; }
    /// @brief 设置主动技能点
    void setActiveSkillPoints(int points) { _activeSkillPoints = points; }

    /// @brief 获取被动技能点
    int getPassiveSkillPoints() const { return _passiveSkillPoints; }
    /// @brief 设置被动技能点
    void setPassiveSkillPoints(int points) { _passiveSkillPoints = points; }

    /// @brief 获取属性点
    int getAttributePoints() const { return _attributePoints; }
    /// @brief 设置属性点
    void setAttributePoints(int points) { _attributePoints = points; }

    /**
     * @brief 消耗 1 点属性点提升指定基础属性
     * @details 仅对玩家基础属性生效；最终属性会在 AttributeComponent 中重新计算。
     */
    bool upgradeAttribute(AttributeType type);

    // =============================================================
    // 装备系统 (Equipment)
    // =============================================================
    /// @brief 装备物品
    void equip(const std::shared_ptr<Equipment>& item);
    /// @brief 卸下指定槽位装备
    void unequip(EquipmentSlot slot);

    // 背包（当前仅管理装备/武器；图标资源可后续补齐）
    /// @brief 获取背包物品列表
    const std::vector<std::shared_ptr<Equipment>>& getInventoryItems() const { return _inventoryItems; }
    /// @brief 添加物品到背包（按 id 去重）
    void addToInventory(const std::shared_ptr<Equipment>& item);
    /// @brief 清空背包（读档/重置用）
    void clearInventory();
    /// @brief 设置背包物品列表（读档用，会按 id 去重）
    void setInventoryItems(const std::vector<std::shared_ptr<Equipment>>& items);
    /// @brief 确保默认测试物品存在（不会重复添加）
    void ensureDefaultInventory();

    /// @brief 获取指定槽位装备
    std::shared_ptr<Equipment> getEquipment(EquipmentSlot slot) const;
    /// @brief 获取当前武器
    std::shared_ptr<Weapon> getEquippedWeapon() const;
    /// @brief 获取当前武器类型
    WeaponType getCurrentWeaponType() const;

    // 获取当前装备列表（用于存档）
    /// @brief 获取当前装备列表
    const std::map<EquipmentSlot, std::shared_ptr<Equipment>>& getEquippedItems() const { return _equippedItems; }
    /// @brief 设置装备列表（读档用）
    void setEquippedItems(const std::map<EquipmentSlot, std::shared_ptr<Equipment>>& items) { _equippedItems = items; }

    using EquipmentChangeCallback = std::function<void(EquipmentSlot, const std::shared_ptr<Equipment>&)>;
    /// @brief 设置装备变更回调
    void setEquipmentChangeCallback(const EquipmentChangeCallback& callback) { _equipmentChangeCallback = callback; }

    // =============================================================
    // 动作控制 (Movement & Animation)
    // =============================================================
    // 场景输入层调用：切换跑动/待机状态
    /// @brief 切换移动/跑步状态
    void setMoving(bool moving, bool running = false);

    // 播放攻击/技能动画（场景侧调用，用于同步状态）
    /// @brief 播放攻击动画
    void attackAnimated(const std::function<void()>& onFinished = nullptr);
    /// @brief 播放技能动画
    void castSkillAnimated(const std::function<void()>& onFinished = nullptr);

    // 辅助：统一播放一次性动画
    /// @brief 播放一次性动画序列
    void playOneShotAnimation(const std::vector<std::string>& paths, float delayPerUnit, int actionTag, const std::function<void()>& onFinished);

    // 动作锁查询
    /// @brief 是否处于动作锁定
    bool isActionLocked() const { return _actionLocked; }

    // =============================================================
    // Action Tags（供 SkillSet/动画复用）
    // =============================================================
    static constexpr int ACTION_TAG_ATTACK_ANIM = 200;
    static constexpr int ACTION_TAG_SKILL_ANIM = 300;

    // =============================================================
    // 战斗系统 (Combat & Skills)
    // =============================================================
    // 核心攻击接口
    /// @brief 普通攻击入口
    virtual void attack() override; // 普攻
    /// @brief 受击处理（打断动作/播放受击）
    virtual void takeDamage(const DamageInfo& info) override; // 受击（打断动作/播放受击）
    /// @brief 造成伤害回调（用于吸血/命中特效/击杀触发等）
    virtual void onDealDamage(CharacterBase* target, float finalDamage, const DamageInfo& info, bool targetDied) override;
    /// @brief 受到伤害回调（用于反伤/濒死救援等）
    virtual void onReceiveDamage(CharacterBase* attacker, float finalDamage, const DamageInfo& info, bool wouldDieBeforeCallback) override;
    /// @brief 计算攻击力（用于 DOT 等）
    virtual float getAttackPower() override; // 攻击力（用于 DOT 等计算）
    /// @brief 使用技能槽位
    void useSkill(size_t slotIndex); // 技能

    // 尝试执行攻击/技能（包含资源检查、CD检查、动作锁检查）
    /// @brief 尝试普攻
    bool tryNormalAttack(const std::function<void()>& onFinished = nullptr);
    /// @brief 尝试释放技能
    bool tryUseSkill(size_t slotIndex, const std::function<void()>& onFinished = nullptr);

    // SkillComponent 回调
    /// @brief 技能组件使用技能回调
    virtual void onUseActiveSkill(const ActiveSkill& skill) override;

    // 战斗辅助：设置投掷物挂载的父节点
    /// @brief 设置战斗层（投掷物挂载层）
    void setCombatLayer(cocos2d::Node* gameLayer) { _combatLayer = gameLayer; }
    /// @brief 添加节点到战斗层
    void addToCombatLayer(cocos2d::Node* node, int zOrder = 4);

    // 计算投掷物生成位置
    /// @brief 计算投掷物生成坐标
    cocos2d::Vec2 getProjectileSpawnPosition(float spawnOffsetXRatio, float spawnOffsetX, float spawnOffsetYRatio, float spawnOffsetY) const;

    // 统一的“动作锁”流程控制
    /// @brief 动作锁封装（前置检查/动画/效果/回调）
    bool runActionLocked(const std::function<bool()>& preCheck,
        const std::function<void(const std::function<void()>&)>& playAnimation,
        const std::function<void()>& performEffect,
        const std::function<void()>& onFinished);

    // =============================================================
    // 资源路径信息 (供 SkillSet 使用)
    // =============================================================
    /// @brief 获取默认资源目录
    const std::string& getDefaultSpriteDir() const { return _defaultSpriteDir; }
    /// @brief 获取技能资源目录
    const std::string& getSkillSpriteDir() const { return _skillSpriteDir; }
    /// @brief 获取角色关键字
    const std::string& getCharacterKey() const { return _characterKey; }

private:
    // 构造函数私有化，强制使用 create
    PlayerCharacter() = default;

    // 内部初始化流程
    /// @brief 根据职业初始化属性
    void initAttributesByRole(CharacterRole role);
    /// @brief 根据属性刷新 HP/MP
    void refreshHpMpFromAttributes();
    /// @brief 初始化资源路径
    void initAssetPaths(const std::string& spriteFrameName);
    /// @brief 创建技能集
    void createSkillSet();

    // 触发型被动/装备特效的统一更新（条件类被动、冷却计时等）
    void updateTriggerEffects(float dt);
    void updateFullHpCritEffect();

    // 查询辅助
    bool hasPassiveEquipped(int skillId);
    std::shared_ptr<Equipment> findEquippedItemById(int itemId) const;

    // 特效实现：命中附加 DOT / 击杀加速
    void tryApplyDotStatus(CharacterBase* target,
                           StatusEffectType type,
                           int stacks,
                           float duration,
                           float tickInterval,
                           float baseDamageScale,
                           float perStackDamageScale);
    void applyExcitedBuff(float durationSeconds, float moveSpeedBonus);

    // 动画管理
    /// @brief 缓存移动动画
    void ensureMoveAnimations();
    /// @brief 缓存状态机动画
    void ensureStateAnimations();
    // ensureMoveAnimationCached 已移至 .cpp 内部实现，不再暴露

    // 战斗逻辑
    /// @brief 获取战斗层节点
    cocos2d::Node* getCombatLayer();
    /// @brief 武器变更回调
    void onWeaponChanged(const std::shared_ptr<Weapon>& weapon);
    /// @brief 升级处理
    void levelUp();
    void applyAttributeGrowth();
    void playLevelUpVFX();

    // 物理回调
    /// @brief 投掷物碰撞回调
    bool onProjectileContactBegin(cocos2d::PhysicsContact& contact);

    // ------------------- 成员变量 -------------------

    // 基础数据
    CharacterRole _role = CharacterRole::WARRIOR;
    int _activeSkillPoints = 0;
    int _passiveSkillPoints = 0;
    int _attributePoints = 0;
    bool _isGrounded = false;
    int _jumpCount = 0;
    bool _actionLocked = false;

    // 资源路径缓存
    std::string _defaultSpriteDir;
    std::string _skillSpriteDir;
    std::string _characterKey;
    std::string _animationKeyPrefix;
    std::string _attackAnimationPrefix = "default";
    std::string _defaultAttackAnimationPrefix = "default";
    int _attackFrameCount = 3;

    // 组件与对象
    std::map<EquipmentSlot, std::shared_ptr<Equipment>> _equippedItems;
    std::vector<std::shared_ptr<Equipment>> _inventoryItems;
    std::unordered_set<int> _inventoryItemIds; // 用于背包按 id 去重加速
    std::unique_ptr<PlayerSkillSet> _skillSet;

    EquipmentChangeCallback _equipmentChangeCallback = nullptr;

    // 弱引用 (Weak References)
    cocos2d::Node* _combatLayer = nullptr;
    cocos2d::EventListenerPhysicsContact* _projectileContactListener = nullptr;

    // 受击方向：受击 png 带方向，角色朝向由 setFlippedX 管理；
    // 这里使用 scaleX 的符号作为“额外镜像层”，避免与移动逻辑冲突。
    bool _hurtMirrorActive = false;
    bool _hurtDesiredFinalMirror = false; // 期望的最终镜像状态（= scaleX<0 XOR flippedX）
    float _hurtMirrorAbsScaleX = 1.0f;    // 受击期间保持的 |scaleX|

    // ------------------------------------------------------------
    // 装备/被动“触发型机制”缓存（不入存档）
    // ------------------------------------------------------------
    float _burnProcCooldownRemaining = 0.0f;      // 命中燃烧触发冷却
    float _poisonProcCooldownRemaining = 0.0f;    // 命中中毒触发冷却
    float _critEchoCooldownRemaining = 0.0f;      // 暴击缩冷却触发冷却
    float _thornsCooldownRemaining = 0.0f;        // 反伤触发冷却
    float _emergencyMaskCooldownRemaining = 0.0f; // 急救面罩触发冷却
    bool _fullHpCritActive = false;               // 满血暴击状态是否已激活
};
