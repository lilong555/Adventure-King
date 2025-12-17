#include "Character/Player/PlayerCharacter.h"
#include "Character/Player/SkillSets/KleeSkillSet.h"
// #include "Character/Player/SkillSets/WarriorSkillSet.h" // 以后扩展

#include "Character/components/AttributeComponent.h"
#include "Character/components/SkillComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Objects/Projectiles/Bomb.h"
#include "Physics/GamePhysicsCategory.h"
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
    constexpr int ACTION_TAG_ATTACK = 200;
    constexpr int ACTION_TAG_SKILL = 300;

    // Animation Delays
    constexpr float ANIM_DELAY_RUN = 0.15f;
    constexpr float ANIM_DELAY_WALK = 0.25f;

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

    // 3. 数据层初始化
    initAttributesByRole(role);
    refreshHpMpFromAttributes();

    // 4. 物理层初始化 (使用内部辅助函数)
    helperSetupPhysicsBody(this);

    // 5. 表现层初始化 - 状态机动画注册
    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, _animationKeyPrefix + "_idle");
        sm->registerStateAnimation(CharacterState::WALKING, _animationKeyPrefix + "_walk");
        sm->registerStateAnimation(CharacterState::RUNNING, _animationKeyPrefix + "_run");
        sm->registerStateAnimation(CharacterState::ATTACKING, _animationKeyPrefix + "_attack");
        sm->registerStateAnimation(CharacterState::HURT, _animationKeyPrefix + "_hurt");
        sm->registerStateAnimation(CharacterState::DEAD, _animationKeyPrefix + "_dead");

        sm->changeState(CharacterState::IDLE);
    }

    ensureMoveAnimations();

    // 6. 技能集初始化
    createSkillSet();
    if (_skillSet)
    {
        _skillSet->initSkills(*this);
    }

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
}

// =================================================================
// 角色状态与属性
// =================================================================

void PlayerCharacter::addExperience(int amount)
{
    _experience += amount;
    while (_experience >= _level * 100)
    {
        _experience -= _level * 100;
        levelUp();
    }
}

void PlayerCharacter::levelUp()
{
    _level++;
    // _skillPoints++; 

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
        attrs.set(AttributeType::MAX_MP, 200.0f);
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

    playOneShotAnimation(paths, animSpeed, ACTION_TAG_ATTACK, onFinished);
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

    playOneShotAnimation(paths, 0.13f, ACTION_TAG_SKILL, onFinished);
}

// =================================================================
// 战斗系统
// =================================================================

void PlayerCharacter::attack()
{
    tryNormalAttack();
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
