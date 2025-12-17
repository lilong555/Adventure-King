#pragma once

#include "Character/Base/CharacterBase.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class PlayerSkillSet;

class PlayerCharacter : public CharacterBase
{
public:
    virtual ~PlayerCharacter();

    // 统一用 create(role, spriteFrameName)
    static PlayerCharacter *create(CharacterRole role,
                                   const std::string &spriteFrameName);

    bool init(CharacterRole role,
              const std::string &spriteFrameName);

    void addExperience(int amount);
    void levelUp();

    // 装备管理
    void equip(const std::shared_ptr<Equipment> &item);
    void unequip(EquipmentSlot slot);
    std::shared_ptr<Equipment> getEquipment(EquipmentSlot slot) const;
    std::shared_ptr<Weapon> getEquippedWeapon() const;

    // 技能管理（外层接口，内部转发给 SkillComponent）
    void useSkill(size_t slotIndex);

    // ================== 技能/普攻入口 ==================
    // 内部处理：冷却/MP 检查、动画播放、投掷物生成（依赖 setCombatLayer 设置的父节点）
    bool tryNormalAttack(const std::function<void()> &onFinished = nullptr);
    bool tryUseSkill(size_t slotIndex, const std::function<void()> &onFinished = nullptr);

    // 设置投掷物/爆炸等战斗节点挂载的父节点（通常为 GameScene 的 _gameLayer）
    void setCombatLayer(cocos2d::Node *gameLayer) { _combatLayer = gameLayer; }

    // 角色素材路径信息（供 SkillSet 构建动画/投掷物配置）
    const std::string &getDefaultSpriteDir() const { return _defaultSpriteDir; }
    const std::string &getSkillSpriteDir() const { return _skillSpriteDir; }
    const std::string &getCharacterKey() const { return _characterKey; }

    // ================== SkillSet 通用工具 ==================
    // 统一播放一次序列帧动画（失败则直接回调 onFinished）
    void playOneShotAnimation(const std::vector<std::string> &paths,
                              float delayPerUnit,
                              int actionTag,
                              const std::function<void()> &onFinished);

    // 统一的“锁动作 → 播放动画 → 执行效果 → 解锁”流程
    bool runActionLocked(const std::function<bool()> &preCheck,
                         const std::function<void(const std::function<void()> &)> &playAnimation,
                         const std::function<void()> &performEffect,
                         const std::function<void()> &onFinished);

    // 计算投掷物生成位置（基于玩家包围盒，避免贴图尺寸导致偏移）
    cocos2d::Vec2 getProjectileSpawnPosition(float spawnOffsetXRatio,
                                             float spawnOffsetX,
                                             float spawnOffsetYRatio,
                                             float spawnOffsetY) const;

    // 将技能/投掷物节点添加到战斗层（通常是 GameScene 的 _gameLayer）
    void addToCombatLayer(cocos2d::Node *node, int zOrder = 4);

    // ================== 动作/状态驱动 ==================
    // 由场景输入层调用，用于切换跑动/待机动画状态
    void setMoving(bool moving);
    void setMoving(bool moving, bool running);

    // 播放一次攻击动画，结束后回调（用于场景侧伤害/状态恢复）
    void attackAnimated(const std::function<void()> &onFinished = nullptr);

    // 播放一次技能施放动画，结束后回调（用于场景侧触发技能效果）
    void castSkillAnimated(const std::function<void()> &onFinished = nullptr);

    // 实现角色基础攻击
    virtual void attack() override;

    // 技能回调：SkillComponent 在技能成功释放时调用
    virtual void onUseActiveSkill(const ActiveSkill &skill) override;

    // 获取当前武器类型
    WeaponType getCurrentWeaponType() const;

    // 获取当前攻击动画前缀
    const std::string &getAttackAnimationPrefix() const { return _attackAnimationPrefix; }
    int getAttackFrameCount() const { return _attackFrameCount; }

    bool isActionLocked() const { return _actionLocked; }

    // 装备变更回调（供外部监听装备变化）
    using EquipmentChangeCallback = std::function<void(EquipmentSlot, const std::shared_ptr<Equipment> &)>;
    void setEquipmentChangeCallback(const EquipmentChangeCallback &callback) { _equipmentChangeCallback = callback; }

    //================== 存档系统支持 ==================

    // 获取角色职业
    CharacterRole getRole() const { return _role; }
    void setRole(CharacterRole role) { _role = role; }

    // 技能点管理
    int getSkillPoints() const { return _skillPoints; }
    void setSkillPoints(int points) { _skillPoints = points; }

    // 装备管理（用于存档）
    const std::map<EquipmentSlot, std::shared_ptr<Equipment>> &getEquippedItems() const { return _equippedItems; }
    void setEquippedItems(const std::map<EquipmentSlot, std::shared_ptr<Equipment>> &items) { _equippedItems = items; }

private:
    PlayerCharacter() = default;

    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void update(float dt) override;

    cocos2d::Node *getCombatLayer();

    bool onProjectileContactBegin(cocos2d::PhysicsContact &contact);
    void initAttributesByRole(CharacterRole role);
    void refreshHpMpFromAttributes();
    void onWeaponChanged(const std::shared_ptr<Weapon> &weapon); // 武器变更时调用
    void createSkillSet();

    void initAssetPaths(const std::string &spriteFrameName);
    void ensureMoveAnimations();
    void ensureMoveAnimationCached(const std::string &animationKey,
                                   const std::vector<std::string> &framePaths,
                                   float delayPerUnit);

    CharacterRole _role = CharacterRole::WARRIOR;
    int _skillPoints = 0;

    std::map<EquipmentSlot, std::shared_ptr<Equipment>> _equippedItems;

    // 当前攻击动画配置
    std::string _attackAnimationPrefix = "default";
    int _attackFrameCount = 3;
    std::string _defaultAttackAnimationPrefix = "default";

    // 装备变更回调
    EquipmentChangeCallback _equipmentChangeCallback = nullptr;

    cocos2d::Node *_combatLayer = nullptr;
    cocos2d::EventListenerPhysicsContact *_projectileContactListener = nullptr;

    std::unique_ptr<PlayerSkillSet> _skillSet;

    std::string _defaultSpriteDir;
    std::string _skillSpriteDir;
    std::string _characterKey;
    std::string _animationKeyPrefix;

    bool _actionLocked = false;
};
