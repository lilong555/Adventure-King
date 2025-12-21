#include "Character/Player/PlayerCharacter.h"
#include "Character/Player/SkillSets/KleeSkillSet.h"
// #include "Character/Player/SkillSets/WarriorSkillSet.h" // 以后扩展

#include "Character/components/AttributeComponent.h"
#include "Character/components/SkillComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/StatusEffectVfxComponent.h"
#include "Objects/Projectiles/Bomb.h"
#include "Configs/GameConfigs.h"
#include "Configs/GamePhysicsCategory.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include "cocos2d.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

USING_NS_CC;

// =================================================================
// 匿名命名空间：内部常量与静态辅助函数
// =================================================================
namespace
{
    // Action Tags
    constexpr int ACTION_TAG_HURT_FACING = 400;

    // Animation Delays
    constexpr float ANIM_DELAY_RUN = GameConfig::Player::ANIM_DELAY_RUN;
    constexpr float ANIM_DELAY_WALK = GameConfig::Player::ANIM_DELAY_WALK;
    constexpr float HURT_DURATION_SECONDS = GameConfig::Player::HURT_DURATION_SECONDS;

    // Combat
    constexpr float DEFAULT_WEAPON_DAMAGE = GameConfig::Player::DEFAULT_WEAPON_DAMAGE;
    constexpr float STRENGTH_DAMAGE_MULTIPLIER = GameConfig::Player::STRENGTH_DAMAGE_MULTIPLIER;

    // 满血判断的容差：避免浮点误差导致“看似满血却判定不满”
    constexpr float HP_COMPARISON_EPSILON = 0.01f;

    // 辅助：创建动画对象
    Animation* createAnimationFromPaths(const std::vector<std::string>& paths, float delayPerUnit)
    {
        Vector<SpriteFrame*> frames;
        frames.reserve(paths.size());

        for (const auto& path : paths)
        {
            auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(path);
            if (!frame)
            {
#if COCOS2D_DEBUG > 0
                CCLOG("PlayerCharacter: failed to load sprite frame %s", path.c_str());
#endif
                continue;
            }
            frames.pushBack(frame);
        }

        if (frames.empty()) return nullptr;
        return Animation::createWithSpriteFrames(frames, delayPerUnit);
    }

    // 辅助：缓存动画（原成员函数 ensureMoveAnimationCached）
    void helperEnsureAnimationCached(const std::string& key, const std::vector<std::string>& paths, float delay)
    {
        auto cache = AnimationCache::getInstance();
        if (!cache->getAnimation(key))
        {
            if (auto anim = createAnimationFromPaths(paths, delay))
            {
                cache->addAnimation(anim, key);
            }
        }
    }

    // 辅助：初始化物理体（原成员函数 initPhysicsBody）
    void helperSetupPhysicsBody(PlayerCharacter* player)
    {
        Size size = player->getContentSize();
        // 碰撞盒设置为贴图宽度的 40%，高度的 80%
        auto body = PhysicsBody::createBox(Size(size.width * 0.4f, size.height * 0.8f),
            PhysicsMaterial(0.1f, 0.0f, 0.0f));
        body->setDynamic(true);
        body->setRotationEnable(false);

        // 设置掩码
        body->setCategoryBitmask(ToMask(GamePhysicsCategory::PLAYER));
        body->setCollisionBitmask(ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER));
        // 增加 MONSTER_ATTACK 以便检测炸弹/投掷物
        body->setContactTestBitmask(ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER | GamePhysicsCategory::MONSTER_ATTACK));

        player->setPhysicsBody(body);
    }
}

// =================================================================
// 生命周期与创建
// =================================================================

PlayerCharacter* PlayerCharacter::create(CharacterRole role, const std::string& spriteFrameName)
{
    PlayerCharacter* pRet = new(std::nothrow) PlayerCharacter();
    if (pRet && pRet->init(role, spriteFrameName))
    {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

PlayerCharacter::~PlayerCharacter() = default;

bool PlayerCharacter::init(CharacterRole role, const std::string& spriteFrameName)
{
    // 1. 视觉初始化
    if (!initWithSpriteFrameName(spriteFrameName))
    {
        if (!initWithFile(spriteFrameName))
        {
            CCLOG("Error: Failed to init PlayerCharacter sprite: %s", spriteFrameName.c_str());
            return false;
        }
    }

    this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->setScale(GameConfig::Player::SCALE);

    _role = role;
    _isGrounded = true;
    _jumpCount = 0;

    // 解析资源路径
    initAssetPaths(spriteFrameName);

    // 2. 组件挂载
    if (!getAttributeComponent())    this->addComponent(AttributeComponent::create());
    if (!getSkillComponent())        this->addComponent(SkillComponent::create());
    if (!getStateMachineComponent()) this->addComponent(StateMachineComponent::create());
    if (!getComponent("StatusEffectVfxComponent")) this->addComponent(StatusEffectVfxComponent::create());

    // 3. 数据层初始化
    initAttributesByRole(role);
    refreshHpMpFromAttributes();

    // 4. 物理层初始化 (使用内部辅助函数)
    helperSetupPhysicsBody(this);

    // 5. 表现层初始化 - 缓存状态机动画（避免首次切状态时缺帧）
    ensureMoveAnimations();
    ensureStateAnimations();

    // 6. 表现层初始化 - 状态机动画注册
    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, _animationKeyPrefix + "_idle");
        sm->registerStateAnimation(CharacterState::WALKING, _animationKeyPrefix + "_walk");
        sm->registerStateAnimation(CharacterState::RUNNING, _animationKeyPrefix + "_run");
        sm->registerStateAnimation(CharacterState::HURT, _animationKeyPrefix + "_hurt");

        sm->changeState(CharacterState::IDLE);
    }

    // 7. 技能集初始化
    createSkillSet();
    if (_skillSet)
    {
        _skillSet->initSkills(*this);
    }

    // 8. 默认背包物品（占位）：用于背包系统初期调试
    ensureDefaultInventory();

    return true;
}

void PlayerCharacter::onEnter()
{
    CharacterBase::onEnter(); // 必须调用父类，虽然父类可能是 Sprite

    if (!_projectileContactListener)
    {
        _projectileContactListener = EventListenerPhysicsContact::create();
        _projectileContactListener->onContactBegin = CC_CALLBACK_1(PlayerCharacter::onProjectileContactBegin, this);
        _eventDispatcher->addEventListenerWithSceneGraphPriority(_projectileContactListener, this);
    }
}

void PlayerCharacter::onExit()
{
    if (_projectileContactListener)
    {
        _eventDispatcher->removeEventListener(_projectileContactListener);
        _projectileContactListener = nullptr;
    }
    CharacterBase::onExit();
}

void PlayerCharacter::update(float dt)
{
    CharacterBase::update(dt);
    // 如果 SkillSet 需要 update，在此调用
    updateTriggerEffects(dt);

    // 受击方向：持续修正 scaleX 的符号，保证移动时的 setFlippedX（朝向）不会覆盖受击图的镜像
    if (_hurtMirrorActive)
    {
        auto sm = getStateMachineComponent();
        if (!sm || sm->getCurrentState() != CharacterState::HURT)
        {
            _hurtMirrorActive = false;
            setScaleX(std::fabs(getScaleX()));
            return;
        }

        float absScaleX = std::fabs(_hurtMirrorAbsScaleX);
        if (absScaleX <= 0.0f)
        {
            absScaleX = std::fabs(getScaleX());
        }

        bool scaleMirror = _hurtDesiredFinalMirror ^ isFlippedX();
        float desiredScaleX = scaleMirror ? -absScaleX : absScaleX;
        if (std::fabs(getScaleX() - desiredScaleX) > 0.0001f)
        {
            setScaleX(desiredScaleX);
        }
    }
}

// =================================================================
// 装备/被动的触发型机制
// =================================================================

void PlayerCharacter::updateTriggerEffects(float dt)
{
    auto dec = [dt](float &v)
    {
        if (v > 0.0f)
        {
            v = std::max(0.0f, v - dt);
        }
    };

    dec(_burnProcCooldownRemaining);
    dec(_poisonProcCooldownRemaining);
    dec(_critEchoCooldownRemaining);
    dec(_thornsCooldownRemaining);
    dec(_emergencyMaskCooldownRemaining);

    updateFullHpCritEffect();
}

void PlayerCharacter::updateFullHpCritEffect()
{
    auto attr = getAttributeComponent();
    if (!attr)
    {
        _fullHpCritActive = false;
        return;
    }

    const bool equipped = hasPassiveEquipped(GameConfig::Skill::Passive::FULL_HP_CRIT);
    if (!equipped)
    {
        if (_fullHpCritActive)
        {
            attr->removeStatusEffect(StatusEffectType::FULL_HP_CRIT);
            _fullHpCritActive = false;
        }
        return;
    }

    const float maxHp = attr->getAttributeValue(AttributeType::MAX_HP);
    const bool isFullHp = (getCurrentHP() >= maxHp - HP_COMPARISON_EPSILON);

    if (isFullHp && !_fullHpCritActive)
    {
        StatusEffectInstance fullHpCrit;
        fullHpCrit.type = StatusEffectType::FULL_HP_CRIT;
        fullHpCrit.duration = 0.0f;
        fullHpCrit.elapsed = 0.0f;
        fullHpCrit.permanent = true; // 由 updateFullHpCritEffect 显式移除
        fullHpCrit.attributeBonus.set(AttributeType::CRITICAL_RATE, GameConfig::Skill::PassiveEffect::FULL_HP_CRIT_BONUS);
        attr->addStatusEffect(fullHpCrit);
        _fullHpCritActive = true;
    }
    else if (!isFullHp && _fullHpCritActive)
    {
        attr->removeStatusEffect(StatusEffectType::FULL_HP_CRIT);
        _fullHpCritActive = false;
    }
}

bool PlayerCharacter::hasPassiveEquipped(int skillId)
{
    auto comp = getSkillComponent();
    if (!comp)
    {
        return false;
    }
    return comp->isPassiveSkillEquipped(skillId);
}

std::shared_ptr<Equipment> PlayerCharacter::findEquippedItemById(int itemId) const
{
    for (const auto &kv : _equippedItems)
    {
        const auto &item = kv.second;
        if (item && item->id == itemId)
        {
            return item;
        }
    }
    return nullptr;
}

void PlayerCharacter::tryApplyDotStatus(CharacterBase *target,
                                       StatusEffectType type,
                                       int stacks,
                                       float duration,
                                       float tickInterval,
                                       float baseDamageScale,
                                       float perStackDamageScale)
{
    if (!target || target->isDead())
    {
        return;
    }

    auto targetAttr = target->getAttributeComponent();
    if (!targetAttr)
    {
        return;
    }

    StatusEffectInstance inst;
    inst.type = type;
    inst.duration = std::max(0.0f, duration);
    inst.elapsed = 0.0f;
    inst.attributeBonus.clear();

    inst.stacks = std::max(1, stacks);
    inst.maxStacks = 0;
    inst.stackable = true;
    inst.refreshOnAdd = true;

    inst.tickInterval = std::max(0.0f, tickInterval);
    inst.tickAccumulator = 0.0f;
    inst.sourceAttackPower = getAttackPower();
    inst.baseDamageScale = std::max(0.0f, baseDamageScale);
    inst.perStackDamageScale = std::max(0.0f, perStackDamageScale);

    targetAttr->addStatusEffect(inst);
}

void PlayerCharacter::applyExcitedBuff(float durationSeconds, float moveSpeedBonus)
{
    auto attr = getAttributeComponent();
    if (!attr)
    {
        return;
    }

    StatusEffectInstance excited;
    excited.type = StatusEffectType::EXCITED;
    excited.duration = std::max(0.0f, durationSeconds);
    excited.elapsed = 0.0f;
    excited.attributeBonus.set(AttributeType::MOVE_SPEED, moveSpeedBonus);

    attr->addStatusEffect(excited);
}

// =================================================================
// 角色状态与属性
// =================================================================

void PlayerCharacter::addExperience(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    _experience += amount;
    int requiredExp = GameConfig::Player::Leveling::getRequiredExp(_level);
    while (_experience >= requiredExp)
    {
        _experience -= requiredExp;
        levelUp();
        requiredExp = GameConfig::Player::Leveling::getRequiredExp(_level);
    }
}

void PlayerCharacter::levelUp()
{
    _level++;
    _activeSkillPoints += GameConfig::Player::SkillPoint::ACTIVE_POINTS_PER_LEVEL;
    _passiveSkillPoints += GameConfig::Player::SkillPoint::PASSIVE_POINTS_PER_LEVEL;
    _attributePoints += GameConfig::Player::AttributePoint::POINTS_PER_LEVEL;

    if (auto attr = getAttributeComponent())
    {
        auto base = attr->getBaseAttributes();
        // 简单成长数值
        base.add(AttributeType::MAX_HP, 10.0f);
        base.add(AttributeType::MAX_MP, 5.0f);
        base.add(AttributeType::STRENGTH, 2.0f);
        base.add(AttributeType::DEFENSE, 1.0f);
        attr->setBaseAttributes(base);
    }

    // 升级后恢复状态或刷新上限
    refreshHpMpFromAttributes();
}

bool PlayerCharacter::upgradeAttribute(AttributeType type)
{
    if (_attributePoints <= 0)
    {
        return false;
    }

    auto attr = getAttributeComponent();
    if (!attr)
    {
        return false;
    }

    auto base = attr->getBaseAttributes();
    bool applied = true;
    switch (type)
    {
    case AttributeType::MAX_HP:
        base.add(AttributeType::MAX_HP, GameConfig::Player::AttributePoint::MAX_HP_PER_POINT);
        break;
    case AttributeType::STRENGTH:
        base.add(AttributeType::STRENGTH, GameConfig::Player::AttributePoint::STRENGTH_PER_POINT);
        break;
    case AttributeType::MOVE_SPEED:
        base.add(AttributeType::MOVE_SPEED, GameConfig::Player::AttributePoint::MOVE_SPEED_PER_POINT);
        break;
    case AttributeType::DEFENSE:
        base.add(AttributeType::DEFENSE, GameConfig::Player::AttributePoint::DEFENSE_PER_POINT);
        break;
    case AttributeType::CRITICAL_RATE:
        base.add(AttributeType::CRITICAL_RATE, GameConfig::Player::AttributePoint::CRITICAL_RATE_PER_POINT);
        break;
    default:
        applied = false;
        break;
    }

    if (!applied)
    {
        return false;
    }

    _attributePoints = std::max(0, _attributePoints - 1);
    attr->setBaseAttributes(base);
    attr->recalculateFinalAttributes();

    // 维持当前血量/蓝量的相对状态，只做上限夹取
    setCurrentHP(getCurrentHP());
    setCurrentMP(getCurrentMP());
    return true;
}

void PlayerCharacter::initAttributesByRole(CharacterRole role)
{
    Attributes attrs;
    switch (role)
    {
    case CharacterRole::WARRIOR:
        attrs.set(AttributeType::STRENGTH, 10);
        attrs.set(AttributeType::DEFENSE, 5);
        attrs.set(AttributeType::CRITICAL_RATE, 0.10f);
        attrs.set(AttributeType::MOVE_SPEED, 200.0f);
        attrs.set(AttributeType::MAX_HP, 100.0f);
        attrs.set(AttributeType::MAX_MP, GameConfig::Player::DEFAULT_MAX_MP);
        break;
    case CharacterRole::MAGE:
        attrs.set(AttributeType::STRENGTH, 4);
        attrs.set(AttributeType::DEFENSE, 2);
        attrs.set(AttributeType::CRITICAL_RATE, 0.15f);
        attrs.set(AttributeType::MOVE_SPEED, 180.0f);
        attrs.set(AttributeType::MAX_HP, 70.0f);
        attrs.set(AttributeType::MAX_MP, 80.0f);
        break;
    case CharacterRole::ASSASSIN:
        attrs.set(AttributeType::STRENGTH, 7);
        attrs.set(AttributeType::DEFENSE, 3);
        attrs.set(AttributeType::CRITICAL_RATE, 0.25f);
        attrs.set(AttributeType::MOVE_SPEED, 240.0f);
        attrs.set(AttributeType::MAX_HP, 80.0f);
        attrs.set(AttributeType::MAX_MP, 40.0f);
        break;
    case CharacterRole::TANK:
        attrs.set(AttributeType::STRENGTH, 6);
        attrs.set(AttributeType::DEFENSE, 8);
        attrs.set(AttributeType::CRITICAL_RATE, 0.05f);
        attrs.set(AttributeType::MOVE_SPEED, 160.0f);
        attrs.set(AttributeType::MAX_HP, 150.0f);
        attrs.set(AttributeType::MAX_MP, 20.0f);
        break;
    }

    if (auto attr = getAttributeComponent())
    {
        attr->setBaseAttributes(attrs);
    }
}

void PlayerCharacter::refreshHpMpFromAttributes()
{
    auto attr = getAttributeComponent();
    if (!attr) return;
    _currentHP = attr->getAttributeValue(AttributeType::MAX_HP);
    _currentMP = attr->getAttributeValue(AttributeType::MAX_MP);
}

// =================================================================
// 装备系统
// =================================================================

void PlayerCharacter::addToInventory(const std::shared_ptr<Equipment>& item)
{
    if (!item)
    {
        return;
    }

    // 按 id 去重：使用 set 加速（背包变大时避免线性扫描）
    const int itemId = item->id;
    if (_inventoryItemIds.find(itemId) != _inventoryItemIds.end())
    {
        return;
    }

    _inventoryItems.push_back(item);
    _inventoryItemIds.insert(itemId);
}

void PlayerCharacter::clearInventory()
{
    _inventoryItems.clear();
    _inventoryItemIds.clear();
}

void PlayerCharacter::setInventoryItems(const std::vector<std::shared_ptr<Equipment>>& items)
{
    _inventoryItems.clear();
    _inventoryItemIds.clear();
    for (const auto& item : items)
    {
        addToInventory(item);
    }
}

void PlayerCharacter::equip(const std::shared_ptr<Equipment>& item)
{
    if (!item) return;
    auto attr = getAttributeComponent();
    if (!attr) return;

    auto slot = item->slot;
    auto it = _equippedItems.find(slot);
    if (it != _equippedItems.end())
    {
        attr->removeEquipmentBonus(it->second->attributeBonus);
    }

    _equippedItems[slot] = item;
    attr->addEquipmentBonus(item->attributeBonus);

    if (slot == EquipmentSlot::WEAPON)
    {
        auto weapon = std::dynamic_pointer_cast<Weapon>(item);
        onWeaponChanged(weapon);
    }

    refreshHpMpFromAttributes(); // 简单回满，根据需求调整

    if (_equipmentChangeCallback) _equipmentChangeCallback(slot, item);
}

void PlayerCharacter::unequip(EquipmentSlot slot)
{
    auto attr = getAttributeComponent();
    if (!attr) return;

    auto it = _equippedItems.find(slot);
    if (it == _equippedItems.end()) return;

    attr->removeEquipmentBonus(it->second->attributeBonus);
    _equippedItems.erase(it);

    if (slot == EquipmentSlot::WEAPON)
    {
        onWeaponChanged(nullptr);
    }

    refreshHpMpFromAttributes();

    if (_equipmentChangeCallback) _equipmentChangeCallback(slot, nullptr);
}

std::shared_ptr<Equipment> PlayerCharacter::getEquipment(EquipmentSlot slot) const
{
    auto it = _equippedItems.find(slot);
    return (it != _equippedItems.end()) ? it->second : nullptr;
}

std::shared_ptr<Weapon> PlayerCharacter::getEquippedWeapon() const
{
    auto eq = getEquipment(EquipmentSlot::WEAPON);
    return std::dynamic_pointer_cast<Weapon>(eq);
}

WeaponType PlayerCharacter::getCurrentWeaponType() const
{
    if (auto w = getEquippedWeapon()) return w->type;
    return WeaponType::SWORD;
}

void PlayerCharacter::onWeaponChanged(const std::shared_ptr<Weapon>& weapon)
{
    if (weapon)
    {
        _attackAnimationPrefix = weapon->attackAnimationPrefix.empty()
            ? _defaultAttackAnimationPrefix
            : weapon->attackAnimationPrefix;
        _attackFrameCount = weapon->attackFrameCount > 0 ? weapon->attackFrameCount : 3;
    }
    else
    {
        _attackAnimationPrefix = _defaultAttackAnimationPrefix;
        _attackFrameCount = 3;
    }
}

void PlayerCharacter::ensureDefaultInventory()
{
    // 说明：这里放少量“测试/占位物品”，用于背包/装备/被动机制的基本交互验证。
    // 该函数应保持幂等：通过 addToInventory 的去重逻辑，避免重复加入。
    //
    // 注意：目前尚未接入掉落/商店等产出系统，因此通过“默认物品”保证功能可测试。

    // 新手剑（武器）
    {
        auto weapon = std::make_shared<Weapon>();
        weapon->id = GameConfig::Equipment::Weapon::STARTER_SWORD;
        weapon->name = "新手剑";
        weapon->description = "一把趁手的练习用短剑";
        weapon->slot = EquipmentSlot::WEAPON;
        weapon->type = WeaponType::SWORD;
        weapon->attackDamage = GameConfig::Player::DEFAULT_WEAPON_DAMAGE;
        weapon->attackRange = 60.0f;
        weapon->attackSpeed = 1.0f;
        weapon->attackAnimationPrefix = ""; // 为空则沿用角色默认攻击动画
        weapon->attackFrameCount = 3;
        weapon->attributeBonus.add(AttributeType::STRENGTH, 2.0f);
        addToInventory(weapon);
    }

    // 训练法杖（武器）
    {
        auto weapon = std::make_shared<Weapon>();
        weapon->id = GameConfig::Equipment::Weapon::TRAINING_STAFF;
        weapon->name = "训练法杖";
        weapon->description = "木制法杖，适合练习施法";
        weapon->slot = EquipmentSlot::WEAPON;
        weapon->type = WeaponType::STAFF;
        weapon->attackDamage = GameConfig::Player::DEFAULT_WEAPON_DAMAGE;
        weapon->attackRange = 80.0f;
        weapon->attackSpeed = 0.9f;
        weapon->attackAnimationPrefix = "";
        weapon->attackFrameCount = 3;
        weapon->attributeBonus.add(AttributeType::MAX_MP, 20.0f);
        addToInventory(weapon);
    }

    // 焰纹法杖（武器：命中有概率施加燃烧，可叠层）
    {
        auto weapon = std::make_shared<Weapon>();
        weapon->id = GameConfig::Equipment::Weapon::EMBER_STAFF;
        weapon->name = "焰纹法杖";
        weapon->description = "杖身刻着古老火纹。命中时有概率施加燃烧（可叠层），适合持续压制。";
        weapon->slot = EquipmentSlot::WEAPON;
        weapon->type = WeaponType::STAFF;
        weapon->attackDamage = GameConfig::Player::DEFAULT_WEAPON_DAMAGE + 2.0f;
        weapon->attackRange = 90.0f;
        weapon->attackSpeed = 0.95f;
        weapon->attackAnimationPrefix = "";
        weapon->attackFrameCount = 3;
        weapon->attributeBonus.add(AttributeType::MAX_MP, 30.0f);
        addToInventory(weapon);
    }

    // 血契短剑（武器：吸血，随装备等级成长）
    {
        auto weapon = std::make_shared<Weapon>();
        weapon->id = GameConfig::Equipment::Weapon::BLOOD_PACT_SWORD;
        weapon->name = "血契短剑";
        weapon->description = "刀刃渴望鲜血。造成伤害会按比例转化为生命回复（随装备等级成长）。";
        weapon->slot = EquipmentSlot::WEAPON;
        weapon->type = WeaponType::SWORD;
        weapon->attackDamage = GameConfig::Player::DEFAULT_WEAPON_DAMAGE + 3.0f;
        weapon->attackRange = 70.0f;
        weapon->attackSpeed = 1.05f;
        weapon->attackAnimationPrefix = "";
        weapon->attackFrameCount = 3;
        weapon->attributeBonus.add(AttributeType::STRENGTH, 3.0f);
        addToInventory(weapon);
    }

    // 皮帽（头盔）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Helmet::LEATHER_CAP;
        equip->name = "皮帽";
        equip->description = "简单的皮制头盔";
        equip->slot = EquipmentSlot::HELMET;
        equip->attributeBonus.add(AttributeType::MAX_HP, 20.0f);
        addToInventory(equip);
    }

    // 急救面罩（头盔：低血量触发救援，带冷却）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Helmet::EMERGENCY_MASK;
        equip->name = "急救面罩";
        equip->description = "内置应急药剂：生命低于 20% 时将生命抬升到 35%，45 秒冷却。";
        equip->slot = EquipmentSlot::HELMET;
        equip->attributeBonus.add(AttributeType::MAX_HP, 10.0f);
        addToInventory(equip);
    }

    // 皮甲（护甲）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Armor::LEATHER_ARMOR;
        equip->name = "皮甲";
        equip->description = "轻便护甲，提供基础防护";
        equip->slot = EquipmentSlot::ARMOR;
        equip->attributeBonus.add(AttributeType::DEFENSE, 1.0f);
        addToInventory(equip);
    }

    // 荆棘甲（护甲：反弹部分伤害，随装备等级成长）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Armor::THORNS_ARMOR;
        equip->name = "荆棘甲";
        equip->description = "带刺甲片会反弹部分伤害（带冷却，反伤随装备等级成长）。";
        equip->slot = EquipmentSlot::ARMOR;
        equip->attributeBonus.add(AttributeType::DEFENSE, 2.0f);
        addToInventory(equip);
    }

    // 轻便靴（靴子）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Boots::LIGHT_BOOTS;
        equip->name = "轻便靴";
        equip->description = "更轻的鞋子，跑得更快";
        equip->slot = EquipmentSlot::BOOTS;
        equip->attributeBonus.add(AttributeType::MOVE_SPEED, 20.0f);
        addToInventory(equip);
    }

    // 追猎之靴（靴子：击杀后短暂加速）
    {
        auto equip = std::make_shared<Equipment>();
        equip->id = GameConfig::Equipment::Boots::HUNTER_BOOTS;
        equip->name = "追猎之靴";
        equip->description = "击杀目标后进入亢奋：短时间内移动速度提升。";
        equip->slot = EquipmentSlot::BOOTS;
        equip->attributeBonus.add(AttributeType::MOVE_SPEED, 10.0f);
        addToInventory(equip);
    }
}

// =================================================================
// 动作控制
// =================================================================

void PlayerCharacter::setMoving(bool moving, bool running)
{
    if (isDead()) return;

    auto sm = getStateMachineComponent();
    if (!sm) return;

    if (moving)
    {
        CharacterState targetState = running ? CharacterState::RUNNING : CharacterState::WALKING;

        if (sm->getCurrentState() != targetState)
        {
            ensureMoveAnimations();
            sm->changeState(targetState);
        }
    }
    else
    {
        // 停止移动时，如果是 IDLE 则不做操作；如果是移动状态则切回 IDLE
        if (sm->getCurrentState() != CharacterState::IDLE)
        {
            bool wasFlippedX = isFlippedX();
            sm->changeState(CharacterState::IDLE);

            // 兜底：设回默认帧
            auto defaultFrame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(_defaultSpriteDir + "/spr_" + _characterKey + "_run.png");
            if (defaultFrame)
            {
                setSpriteFrame(defaultFrame);
                setFlippedX(wasFlippedX);
            }
        }
    }
}

void PlayerCharacter::playOneShotAnimation(const std::vector<std::string>& paths, float delayPerUnit, int actionTag, const std::function<void()>& onFinished)
{
    auto animation = createAnimationFromPaths(paths, delayPerUnit);
    if (!animation)
    {
        if (onFinished) onFinished();
        return;
    }

    // 停止同类动作
    if (actionTag != 0) stopActionByTag(actionTag);

    auto animate = Animate::create(animation);
    auto callbackAction = CallFunc::create([onFinished]() {
        if (onFinished) onFinished();
        });

    auto sequence = Sequence::create(animate, callbackAction, nullptr);
    if (actionTag != 0) sequence->setTag(actionTag);

    runAction(sequence);
}

bool PlayerCharacter::runActionLocked(const std::function<bool()>& preCheck,
    const std::function<void(const std::function<void()>&)>& playAnimation,
    const std::function<void()>& performEffect,
    const std::function<void()>& onFinished)
{
    if (isDead()) return false;
    if (_actionLocked) return false;
    if (preCheck && !preCheck()) return false;

    // 统一的动作锁：防止攻击/技能并发打断动画状态。
    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::ATTACKING);
    }

    _actionLocked = true;
    playAnimation([this, performEffect, onFinished]() {
        if (performEffect) performEffect();
        _actionLocked = false;
        if (onFinished) onFinished();
        });
    return true;
}

void PlayerCharacter::attackAnimated(const std::function<void()>& onFinished)
{
    if (isDead()) return;

    if (auto sm = getStateMachineComponent())
        sm->changeState(CharacterState::ATTACKING);

    float animSpeed = 0.15f;
    if (auto weapon = getEquippedWeapon())
    {
        if (weapon->attackSpeed > 0.0f) animSpeed = 0.15f / weapon->attackSpeed;
    }

    std::string prefix = _attackAnimationPrefix.empty() ? _defaultAttackAnimationPrefix : _attackAnimationPrefix;
    int frameCount = (_attackFrameCount > 0) ? _attackFrameCount : 3;

    std::vector<std::string> paths;
    paths.reserve(frameCount);

    for (int i = 1; i <= frameCount; ++i)
    {
        if (SpriteFrameCacheHelper::isFilePath(prefix))
        {
            paths.push_back(StringUtils::format("%s_%d.png", prefix.c_str(), i));
        }
        else
        {
            paths.push_back(StringUtils::format("%s/%s_%d.png", _defaultSpriteDir.c_str(), prefix.c_str(), i));
        }
    }

    playOneShotAnimation(paths, animSpeed, PlayerCharacter::ACTION_TAG_ATTACK_ANIM, onFinished);
}

void PlayerCharacter::castSkillAnimated(const std::function<void()>& onFinished)
{
    if (isDead()) return;

    if (auto sm = getStateMachineComponent())
        sm->changeState(CharacterState::ATTACKING);

    std::vector<std::string> paths;
    paths.reserve(3);
    for (int i = 1; i <= 3; ++i)
    {
        paths.push_back(StringUtils::format("%s/spr_%s_attack_%d.png", _defaultSpriteDir.c_str(), _characterKey.c_str(), i));
    }

    playOneShotAnimation(paths, 0.13f, PlayerCharacter::ACTION_TAG_SKILL_ANIM, onFinished);
}

// =================================================================
// 战斗系统
// =================================================================

void PlayerCharacter::attack()
{
    tryNormalAttack();
}

float PlayerCharacter::getAttackPower()
{
    float weaponDamage = DEFAULT_WEAPON_DAMAGE;
    if (auto weapon = getEquippedWeapon())
    {
        weaponDamage = weapon->attackDamage;
    }

    float strength = 0.0f;
    if (auto attr = getAttributeComponent())
    {
        strength = attr->getAttributeValue(AttributeType::STRENGTH);
    }

    return weaponDamage + strength * STRENGTH_DAMAGE_MULTIPLIER;
}

void PlayerCharacter::takeDamage(const DamageInfo& info)
{
    if (isDead()) return;

    CharacterBase::takeDamage(info);
    if (isDead()) return;

    auto sm = getStateMachineComponent();
    if (!sm) return;

    // 保留基类的受击阈值：避免 DOT 等持续伤害频繁触发受击导致无法操控
    if (sm->getCurrentState() != CharacterState::HURT)
        return;

    // 取消上一次的受击镜像（连续受击时重新计算）
    stopActionByTag(ACTION_TAG_HURT_FACING);
    _hurtMirrorActive = false;
    setScaleX(std::fabs(getScaleX()));

    if (info.hasHitWorldPos || (info.attacker && info.attacker != this))
    {
        auto getWorldX = [](const Node* node) -> float {
            if (!node)
                return 0.0f;

            auto parent = node->getParent();
            auto worldPos = parent ? parent->convertToWorldSpace(node->getPosition()) : node->getPosition();
            return worldPos.x;
        };

        float myX = getWorldX(this);
        float attackerX = info.hasHitWorldPos ? info.hitWorldPos.x : getWorldX(info.attacker);
        bool attackerOnLeft = attackerX < myX;

        // beattacked png 有方向：以“攻击来源在左侧”为基准决定最终镜像状态。
        // 玩家面向由 setFlippedX 控制；因此使用 scaleX 的符号作为“额外镜像层”。
        //
        // 目标：最终镜像状态 = attackerOnLeft
        _hurtDesiredFinalMirror = attackerOnLeft;
        _hurtMirrorAbsScaleX = std::fabs(getScaleX());
        _hurtMirrorActive = true;

        // 立即应用一次（update 中也会持续校正，避免移动时朝向覆盖）
        bool scaleMirror = _hurtDesiredFinalMirror ^ isFlippedX();
        setScaleX(scaleMirror ? -_hurtMirrorAbsScaleX : _hurtMirrorAbsScaleX);

        auto restore = Sequence::create(
            DelayTime::create(HURT_DURATION_SECONDS),
            CallFunc::create([this]() {
                _hurtMirrorActive = false;
                setScaleX(std::fabs(_hurtMirrorAbsScaleX));
            }),
            nullptr);
        restore->setTag(ACTION_TAG_HURT_FACING);
        runAction(restore);
    }

    // 受击：打断当前出手（防止“受击仍在投掷/施法”），并解除动作锁
    stopActionByTag(PlayerCharacter::ACTION_TAG_ATTACK_ANIM);
    stopActionByTag(PlayerCharacter::ACTION_TAG_SKILL_ANIM);
    _actionLocked = false;
}

void PlayerCharacter::onReceiveDamage(CharacterBase* attacker, float finalDamage, const DamageInfo& info, bool /*wouldDieBeforeCallback*/)
{
    (void)info;

    if (finalDamage <= 0.0f)
    {
        return;
    }

    // -----------------------------
    // 装备特效：荆棘甲（反伤）
    // -----------------------------
    if (_thornsCooldownRemaining <= 0.0f && attacker && attacker != this && !attacker->isDead())
    {
        auto thorns = findEquippedItemById(GameConfig::Equipment::Armor::THORNS_ARMOR);
        if (thorns)
        {
            const float rate = GameConfig::EquipmentEffect::ThornsArmor::getReflectRate(thorns->level);
            // 反伤按比例计算；最小为 1，避免“触发了但没有伤害”的反馈落差
            float reflect = std::max(1.0f, finalDamage * rate);
            DamageInfo thornDmg;
            thornDmg.amount = reflect;
            // 设置 attacker=nullptr：反伤不触发任何 onDealDamage/吸血/命中特效，避免链式循环
            thornDmg.attacker = nullptr;
            thornDmg.isCritical = false;
            thornDmg.penetration = 0.0f;
            thornDmg.causesHitStun = false;   // 反伤不打断，避免“互相锁死”
            attacker->takeDamage(thornDmg);

            _thornsCooldownRemaining = GameConfig::EquipmentEffect::ThornsArmor::PROC_COOLDOWN;
        }
    }

    // -----------------------------
    // 装备特效：急救面罩（低血量救援）
    // -----------------------------
    if (_emergencyMaskCooldownRemaining <= 0.0f && findEquippedItemById(GameConfig::Equipment::Helmet::EMERGENCY_MASK))
    {
        auto attr = getAttributeComponent();
        if (attr && !isDead())
        {
            const float maxHp = attr->getAttributeValue(AttributeType::MAX_HP);
            const float triggerHp = maxHp * GameConfig::EquipmentEffect::EmergencyMask::TRIGGER_HP_RATIO;
            if (getCurrentHP() <= triggerHp)
            {
                const float targetHp = maxHp * GameConfig::EquipmentEffect::EmergencyMask::HEAL_TARGET_HP_RATIO;
                // 显式夹取：避免配置错误导致目标血量超过最大血量（不依赖 setCurrentHP 的内部夹取）
                const float clampedTargetHp = std::min(targetHp, maxHp);
                setCurrentHP(std::max(getCurrentHP(), clampedTargetHp));
                _emergencyMaskCooldownRemaining = GameConfig::EquipmentEffect::EmergencyMask::PROC_COOLDOWN;
            }
        }
    }
}

void PlayerCharacter::onDealDamage(CharacterBase* target, float finalDamage, const DamageInfo& info, bool targetDied)
{
    if (!target || target == this)
    {
        return;
    }
    if (finalDamage <= 0.0f)
    {
        return;
    }

    // -----------------------------
    // 被动/装备：吸血（按造成伤害回复）
    // -----------------------------
    float lifestealRate = 0.0f;
    if (hasPassiveEquipped(GameConfig::Skill::Passive::BLOODTHIRST))
    {
        lifestealRate += GameConfig::Skill::PassiveEffect::BLOODTHIRST_LIFESTEAL;
    }
    if (auto weapon = getEquippedWeapon())
    {
        if (weapon->id == GameConfig::Equipment::Weapon::BLOOD_PACT_SWORD)
        {
            // 装备吸血：随装备等级成长
            lifestealRate += GameConfig::EquipmentEffect::BloodPactSword::getLifestealRate(weapon->level);
        }
    }
    if (lifestealRate > 0.0f && !isDead())
    {
        // 吸血允许来自“被动 + 装备”叠加，但做总上限夹取，便于后续扩展时控平衡
        lifestealRate = std::min(lifestealRate, GameConfig::Skill::PassiveEffect::LIFESTEAL_TOTAL_MAX);
        setCurrentHP(getCurrentHP() + finalDamage * lifestealRate);
    }

    // -----------------------------
    // 被动/装备：命中附加 DOT（燃烧/中毒）
    // -----------------------------
    if (_burnProcCooldownRemaining <= 0.0f)
    {
        const bool hasPassiveEmber = hasPassiveEquipped(GameConfig::Skill::Passive::EMBER_MARK);
        const auto weapon = getEquippedWeapon();
        const bool hasEmberStaff = (weapon && weapon->id == GameConfig::Equipment::Weapon::EMBER_STAFF);

        float chance = 0.0f;
        if (hasPassiveEmber)
        {
            chance += GameConfig::Skill::PassiveEffect::EMBER_MARK_PROC_CHANCE;
        }
        if (hasEmberStaff)
        {
            chance += GameConfig::EquipmentEffect::EmberStaff::PROC_CHANCE;
        }
        chance = clampf(chance, 0.0f, 1.0f);

        if (chance > 0.0f && RandomHelper::random_real(0.0f, 1.0f) < chance)
        {
            tryApplyDotStatus(target,
                              StatusEffectType::BURNING,
                              1,
                              GameConfig::StatusEffect::Burning::DURATION_SECONDS,
                              GameConfig::StatusEffect::Burning::TICK_INTERVAL_SECONDS,
                              GameConfig::StatusEffect::Burning::BASE_DAMAGE_SCALE,
                              GameConfig::StatusEffect::Burning::PER_STACK_DAMAGE_SCALE);

            // 冷却：取“更长”的冷却（更严格），避免同帧多段伤害刷屏
            float cd = 0.0f;
            if (hasPassiveEmber)
            {
                cd = std::max(cd, GameConfig::Skill::PassiveEffect::EMBER_MARK_PROC_COOLDOWN);
            }
            if (hasEmberStaff)
            {
                cd = std::max(cd, GameConfig::EquipmentEffect::EmberStaff::PROC_COOLDOWN);
            }
            _burnProcCooldownRemaining = std::max(0.0f, cd);
        }
    }

    if (_poisonProcCooldownRemaining <= 0.0f && hasPassiveEquipped(GameConfig::Skill::Passive::POISON_TOUCH))
    {
        const float chance = clampf(GameConfig::Skill::PassiveEffect::POISON_TOUCH_PROC_CHANCE, 0.0f, 1.0f);
        if (chance > 0.0f && RandomHelper::random_real(0.0f, 1.0f) < chance)
        {
            tryApplyDotStatus(target,
                              StatusEffectType::POISONED,
                              1,
                              GameConfig::StatusEffect::Poisoned::DURATION_SECONDS,
                              GameConfig::StatusEffect::Poisoned::TICK_INTERVAL_SECONDS,
                              GameConfig::StatusEffect::Poisoned::BASE_DAMAGE_SCALE,
                              GameConfig::StatusEffect::Poisoned::PER_STACK_DAMAGE_SCALE);

            _poisonProcCooldownRemaining = GameConfig::Skill::PassiveEffect::POISON_TOUCH_PROC_COOLDOWN;
        }
    }

    // -----------------------------
    // 被动：暴击缩短冷却（减少所有主动技能剩余 CD）
    // -----------------------------
    if (info.isCritical && _critEchoCooldownRemaining <= 0.0f && hasPassiveEquipped(GameConfig::Skill::Passive::CRIT_ECHO))
    {
        if (auto comp = getSkillComponent())
        {
            comp->reduceAllActiveCooldown(GameConfig::Skill::PassiveEffect::CRIT_ECHO_REDUCE_SECONDS);
            _critEchoCooldownRemaining = GameConfig::Skill::PassiveEffect::CRIT_ECHO_PROC_COOLDOWN;
        }
    }

    // -----------------------------
    // 装备：击杀加速（追猎之靴）
    // -----------------------------
    if (targetDied && findEquippedItemById(GameConfig::Equipment::Boots::HUNTER_BOOTS))
    {
        applyExcitedBuff(GameConfig::StatusEffect::Excited::DURATION_SECONDS,
                         GameConfig::StatusEffect::Excited::MOVE_SPEED_BONUS);
    }
}

void PlayerCharacter::useSkill(size_t slotIndex)
{
    tryUseSkill(slotIndex);
}

bool PlayerCharacter::tryNormalAttack(const std::function<void()>& onFinished)
{
    if (!_skillSet) createSkillSet();
    return _skillSet ? _skillSet->tryNormalAttack(*this, onFinished) : false;
}

bool PlayerCharacter::tryUseSkill(size_t slotIndex, const std::function<void()>& onFinished)
{
    if (!_skillSet) createSkillSet();
    return _skillSet ? _skillSet->tryUseSkill(*this, slotIndex, onFinished) : false;
}

void PlayerCharacter::onUseActiveSkill(const ActiveSkill& skill)
{
    if (auto sm = getStateMachineComponent())
        sm->changeState(CharacterState::ATTACKING);
}

Node* PlayerCharacter::getCombatLayer()
{
    if (_combatLayer) return _combatLayer;
    return getParent();
}

void PlayerCharacter::addToCombatLayer(Node* node, int zOrder)
{
    if (!node) return;
    if (auto layer = getCombatLayer())
    {
        layer->addChild(node, zOrder);
    }
}

Vec2 PlayerCharacter::getProjectileSpawnPosition(float spawnOffsetXRatio, float spawnOffsetX, float spawnOffsetYRatio, float spawnOffsetY) const
{
    bool facingLeft = isFlippedX();
    float dirX = facingLeft ? -1.0f : 1.0f;

    Rect playerBox = getBoundingBox();
    float spawnX = playerBox.getMidX() + dirX * (playerBox.size.width * spawnOffsetXRatio + spawnOffsetX);
    float spawnY = playerBox.getMidY() + playerBox.size.height * spawnOffsetYRatio + spawnOffsetY;
    return Vec2(spawnX, spawnY);
}

bool PlayerCharacter::onProjectileContactBegin(PhysicsContact& contact)
{
    auto bodyA = contact.getShapeA()->getBody();
    auto bodyB = contact.getShapeB()->getBody();
    if (!bodyA || !bodyB) return true;

    auto nodeA = bodyA->getNode();
    auto nodeB = bodyB->getNode();
    if (!nodeA || !nodeB) return true;

    auto bombA = dynamic_cast<Bomb*>(nodeA);
    auto bombB = dynamic_cast<Bomb*>(nodeB);

    if (bombA || bombB)
    {
        int catA = bodyA->getCategoryBitmask();
        int catB = bodyB->getCategoryBitmask();
        int explodeMask = ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER);

        auto handleBomb = [&](Bomb* bomb, int otherCategory) {
            if (bomb && !bomb->isExploded() && bomb->getExplodeOnContact() && ((otherCategory & explodeMask) != 0))
            {
                bomb->scheduleOnce([bomb](float) {
                    if (bomb && bomb->getParent()) {
                        bomb->explode();
                    }
                    }, 0.0f, "bomb_explode_trigger");
            }
            };

        if (bombA) handleBomb(bombA, catB);
        if (bombB) handleBomb(bombB, catA);
    }

    return true;
}

// =================================================================
// 资源路径与初始化
// =================================================================

void PlayerCharacter::initAssetPaths(const std::string& spriteFrameName)
{
    _defaultSpriteDir = "Sprites/Characters/Player/Klee/defalt";
    _skillSpriteDir = "Sprites/Characters/Player/Klee/rpg";
    _characterKey = "klee";

    if (SpriteFrameCacheHelper::isFilePath(spriteFrameName))
    {
        std::string normalized = spriteFrameName;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        size_t lastSlash = normalized.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            _defaultSpriteDir = normalized.substr(0, lastSlash);

            size_t prevSlash = _defaultSpriteDir.find_last_of('/');
            if (prevSlash != std::string::npos)
            {
                std::string parentDir = _defaultSpriteDir.substr(0, prevSlash);
                _skillSpriteDir = parentDir + "/rpg";

                size_t charSlash = parentDir.find_last_of('/');
                if (charSlash != std::string::npos)
                {
                    _characterKey = parentDir.substr(charSlash + 1);
                }
            }
        }
    }

    std::transform(_characterKey.begin(), _characterKey.end(), _characterKey.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    _animationKeyPrefix = "player_" + _characterKey;
    _defaultAttackAnimationPrefix = "spr_" + _characterKey + "_attack";
    _attackAnimationPrefix = _defaultAttackAnimationPrefix;
    _attackFrameCount = 3;
}

void PlayerCharacter::createSkillSet()
{
    if (_skillSet) return;

    if (_characterKey == "klee")
    {
        _skillSet = std::make_unique<KleeSkillSet>();
    }
    else
    {
        // 默认回退到 Klee，或者可以打印警告
        _skillSet = std::make_unique<KleeSkillSet>();
    }
}

void PlayerCharacter::ensureMoveAnimations()
{
    if (_defaultSpriteDir.empty() || _characterKey.empty()) return;

    auto makePath = [this](const std::string& suffix) {
        return _defaultSpriteDir + "/spr_" + _characterKey + suffix;
        };

    std::vector<std::string> movePaths = {
        makePath("_run_1.png"),
        makePath("_run_2.png"),
        makePath("_run.png"),
    };

    // 调用内部静态辅助函数
    helperEnsureAnimationCached(_animationKeyPrefix + "_run", movePaths, ANIM_DELAY_RUN);
    helperEnsureAnimationCached(_animationKeyPrefix + "_walk", movePaths, ANIM_DELAY_WALK);
}

void PlayerCharacter::ensureStateAnimations()
{
    if (_defaultSpriteDir.empty() || _characterKey.empty() || _animationKeyPrefix.empty())
    {
        return;
    }

    auto cache = AnimationCache::getInstance();
    if (!cache)
    {
        return;
    }

    auto ensureSingleFrame = [cache](const std::string& key, const std::string& framePath) {
        if (cache->getAnimation(key))
        {
            return;
        }

        auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(framePath);
        if (!frame)
        {
#if COCOS2D_DEBUG > 0
            CCLOG("PlayerCharacter: failed to load state frame %s", framePath.c_str());
#endif
            return;
        }

        cocos2d::Vector<cocos2d::SpriteFrame*> frames;
        frames.pushBack(frame);
        auto anim = Animation::createWithSpriteFrames(frames, 0.2f);
        cache->addAnimation(anim, key);
    };

    // IDLE：用默认 run 静帧兜底（多角色兼容）
    ensureSingleFrame(_animationKeyPrefix + "_idle",
        _defaultSpriteDir + "/spr_" + _characterKey + "_run.png");

    // HURT：受击贴图（spr_<角色>_beattacked.png）
    ensureSingleFrame(_animationKeyPrefix + "_hurt",
        _defaultSpriteDir + "/spr_" + _characterKey + "_beattacked.png");
}
