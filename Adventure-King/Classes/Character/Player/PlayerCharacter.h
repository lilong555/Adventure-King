#pragma once

#include "Character/Base/CharacterBase.h"
#include "Character/Base/CharacterData.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// 前向声明，减少头文件依赖
class PlayerSkillSet;
class Equipment;
class Weapon;

class PlayerCharacter : public CharacterBase
{
public:
    // =============================================================
    // 生命周期与创建
    // =============================================================
    static PlayerCharacter* create(CharacterRole role, const std::string& spriteFrameName);
    virtual ~PlayerCharacter();

    bool init(CharacterRole role, const std::string& spriteFrameName);

    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void update(float dt) override;

    // =============================================================
    // 角色状态与属性 (Level, Exp, HP/MP)
    // =============================================================
    void addExperience(int amount);

    // 存档与数据访问
    CharacterRole getRole() const { return _role; }
    void setRole(CharacterRole role) { _role = role; }

    int getSkillPoints() const { return _skillPoints; }
    void setSkillPoints(int points) { _skillPoints = points; }

    // =============================================================
    // 装备系统 (Equipment)
    // =============================================================
    void equip(const std::shared_ptr<Equipment>& item);
    void unequip(EquipmentSlot slot);

    std::shared_ptr<Equipment> getEquipment(EquipmentSlot slot) const;
    std::shared_ptr<Weapon> getEquippedWeapon() const;
    WeaponType getCurrentWeaponType() const;

    // 获取当前装备列表（用于存档）
    const std::map<EquipmentSlot, std::shared_ptr<Equipment>>& getEquippedItems() const { return _equippedItems; }
    void setEquippedItems(const std::map<EquipmentSlot, std::shared_ptr<Equipment>>& items) { _equippedItems = items; }

    using EquipmentChangeCallback = std::function<void(EquipmentSlot, const std::shared_ptr<Equipment>&)>;
    void setEquipmentChangeCallback(const EquipmentChangeCallback& callback) { _equipmentChangeCallback = callback; }

    // =============================================================
    // 动作控制 (Movement & Animation)
    // =============================================================
    // 场景输入层调用：切换跑动/待机状态
    void setMoving(bool moving, bool running = false);

    // 播放攻击/技能动画（场景侧调用，用于同步状态）
    void attackAnimated(const std::function<void()>& onFinished = nullptr);
    void castSkillAnimated(const std::function<void()>& onFinished = nullptr);

    // 辅助：统一播放一次性动画
    void playOneShotAnimation(const std::vector<std::string>& paths, float delayPerUnit, int actionTag, const std::function<void()>& onFinished);

    // 动作锁查询
    bool isActionLocked() const { return _actionLocked; }

    // =============================================================
    // 战斗系统 (Combat & Skills)
    // =============================================================
    // 核心攻击接口
    virtual void attack() override; // 普攻
    void useSkill(size_t slotIndex); // 技能

    // 尝试执行攻击/技能（包含资源检查、CD检查、动作锁检查）
    bool tryNormalAttack(const std::function<void()>& onFinished = nullptr);
    bool tryUseSkill(size_t slotIndex, const std::function<void()>& onFinished = nullptr);

    // SkillComponent 回调
    virtual void onUseActiveSkill(const ActiveSkill& skill) override;

    // 战斗辅助：设置投掷物挂载的父节点
    void setCombatLayer(cocos2d::Node* gameLayer) { _combatLayer = gameLayer; }
    void addToCombatLayer(cocos2d::Node* node, int zOrder = 4);

    // 计算投掷物生成位置
    cocos2d::Vec2 getProjectileSpawnPosition(float spawnOffsetXRatio, float spawnOffsetX, float spawnOffsetYRatio, float spawnOffsetY) const;

    // 统一的“动作锁”流程控制
    bool runActionLocked(const std::function<bool()>& preCheck,
        const std::function<void(const std::function<void()>&)>& playAnimation,
        const std::function<void()>& performEffect,
        const std::function<void()>& onFinished);

    // =============================================================
    // 资源路径信息 (供 SkillSet 使用)
    // =============================================================
    const std::string& getDefaultSpriteDir() const { return _defaultSpriteDir; }
    const std::string& getSkillSpriteDir() const { return _skillSpriteDir; }
    const std::string& getCharacterKey() const { return _characterKey; }

private:
    // 构造函数私有化，强制使用 create
    PlayerCharacter() = default;

    // 内部初始化流程
    void initAttributesByRole(CharacterRole role);
    void refreshHpMpFromAttributes();
    void initAssetPaths(const std::string& spriteFrameName);
    void createSkillSet();

    // 动画管理
    void ensureMoveAnimations();
    // ensureMoveAnimationCached 已移至 .cpp 内部实现，不再暴露

    // 战斗逻辑
    cocos2d::Node* getCombatLayer();
    void onWeaponChanged(const std::shared_ptr<Weapon>& weapon);
    void levelUp();

    // 物理回调
    bool onProjectileContactBegin(cocos2d::PhysicsContact& contact);

    // ------------------- 成员变量 -------------------

    // 基础数据
    CharacterRole _role = CharacterRole::WARRIOR;
    int _skillPoints = 0;
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
    std::unique_ptr<PlayerSkillSet> _skillSet;

    EquipmentChangeCallback _equipmentChangeCallback = nullptr;

    // 弱引用 (Weak References)
    cocos2d::Node* _combatLayer = nullptr;
    cocos2d::EventListenerPhysicsContact* _projectileContactListener = nullptr;
};
