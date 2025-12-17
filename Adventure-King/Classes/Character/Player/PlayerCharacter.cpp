#include "Character/Player/PlayerCharacter.h"
#include "Character/Player/SkillSets/KleeSkillSet.h"
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

namespace
{
    Animation *createAnimationFromPaths(const std::vector<std::string> &paths, float delayPerUnit)
    {
        // delayPerUnit 为动画每帧的延迟时间，单位为秒；
        Vector<SpriteFrame *> frames;

        for (const auto &path : paths)
        {
            auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(path);
            if (!frame)
            {
                CCLOG("PlayerCharacter: failed to load sprite frame %s", path.c_str());
                continue;
            }

            frames.pushBack(frame);
        }

        if (frames.empty())
            return nullptr;

        return Animation::createWithSpriteFrames(frames, delayPerUnit);
    }

} // namespace

PlayerCharacter::~PlayerCharacter() = default;

PlayerCharacter *PlayerCharacter::create(CharacterRole role,
                                         const std::string &spriteFrameName)
{
    PlayerCharacter *ret = new (std::nothrow) PlayerCharacter();
    if (ret && ret->init(role, spriteFrameName))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool PlayerCharacter::init(CharacterRole role,
                           const std::string &spriteFrameName)
{
    // 优先走 SpriteFrameCache（缺失时会按文件加载并加入缓存），减少重复创建 SpriteFrame 的开销
    bool initSuccess = initWithSpriteFrameName(spriteFrameName);
    if (!initSuccess)
    {
        initSuccess = initWithFile(spriteFrameName);
    }

    if (!initSuccess)
    {
        return false;
    }

    _role = role;
    initAssetPaths(spriteFrameName);

    initAttributesByRole(role);

    if (auto attr = getAttributeComponent())
    {
        attr->recalculateFinalAttributes();
        refreshHpMpFromAttributes();
    }

    // 示例：绑定不同状态的动画名（动画要提前放进 AnimationCache）
    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, _animationKeyPrefix + "_idle");
        sm->registerStateAnimation(CharacterState::WALKING, _animationKeyPrefix + "_walk");
        sm->registerStateAnimation(CharacterState::RUNNING, _animationKeyPrefix + "_run");
        sm->registerStateAnimation(CharacterState::ATTACKING, _animationKeyPrefix + "_attack");
        sm->registerStateAnimation(CharacterState::HURT, _animationKeyPrefix + "_hurt");
        sm->registerStateAnimation(CharacterState::DEAD, _animationKeyPrefix + "_dead");
    }

    // 确保默认跑动动画存在（由 StateMachineComponent 播放）
    ensureMoveAnimations();

    createSkillSet();
    if (_skillSet)
    {
        _skillSet->initSkills(*this);
    }

    return true;
}

void PlayerCharacter::onEnter()
{
    Sprite::onEnter();

    if (_projectileContactListener)
    {
        return;
    }

    _projectileContactListener = EventListenerPhysicsContact::create();
    _projectileContactListener->onContactBegin = CC_CALLBACK_1(PlayerCharacter::onProjectileContactBegin, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_projectileContactListener, this);
}

void PlayerCharacter::onExit()
{
    if (_projectileContactListener)
    {
        _eventDispatcher->removeEventListener(_projectileContactListener);
        _projectileContactListener = nullptr;
    }

    Sprite::onExit();
}

void PlayerCharacter::update(float dt)
{
    CharacterBase::update(dt);
}

Node *PlayerCharacter::getCombatLayer()
{
    if (_combatLayer)
    {
        return _combatLayer;
    }
    return getParent();
}

bool PlayerCharacter::onProjectileContactBegin(PhysicsContact &contact)
{
    auto bodyA = contact.getShapeA()->getBody();
    auto bodyB = contact.getShapeB()->getBody();
    if (!bodyA || !bodyB)
    {
        return true;
    }

    auto nodeA = bodyA->getNode();
    auto nodeB = bodyB->getNode();
    if (!nodeA || !nodeB)
    {
        return true;
    }

    auto bombA = dynamic_cast<Bomb *>(nodeA);
    auto bombB = dynamic_cast<Bomb *>(nodeB);
    if (bombA || bombB)
    {
        int categoryA = bodyA->getCategoryBitmask();
        int categoryB = bodyB->getCategoryBitmask();
        int explodeMask = ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER);

        if (bombA && !bombA->isExploded() && bombA->getExplodeOnContact() && ((categoryB & explodeMask) != 0))
        {
            bombA->scheduleOnce([bombA](float)
                                { bombA->explode(); },
                                0.0f,
                                "bomb_explode_trigger");
        }

        if (bombB && !bombB->isExploded() && bombB->getExplodeOnContact() && ((categoryA & explodeMask) != 0))
        {
            bombB->scheduleOnce([bombB](float)
                                { bombB->explode(); },
                                0.0f,
                                "bomb_explode_trigger");
        }

        return true;
    }

    return true;
}

void PlayerCharacter::initAssetPaths(const std::string &spriteFrameName)
{
    // Default to Klee to keep existing behavior if the input is not a file path.
    _defaultSpriteDir = "Sprites/Characters/Player/Klee/defalt";
    _skillSpriteDir = "Sprites/Characters/Player/Klee/rpg";
    _characterKey = "klee";

    if (SpriteFrameCacheHelper::isFilePath(spriteFrameName))
    {
        std::string normalized = spriteFrameName;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        auto lastSlash = normalized.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash > 0)
        {
            std::string defaultDir = normalized.substr(0, lastSlash);
            if (!defaultDir.empty())
            {
                _defaultSpriteDir = defaultDir;

                auto prevSlash = defaultDir.find_last_of('/');
                if (prevSlash != std::string::npos && prevSlash > 0)
                {
                    std::string characterDir = defaultDir.substr(0, prevSlash);
                    if (!characterDir.empty())
                    {
                        _skillSpriteDir = characterDir + "/rpg";

                        auto charSlash = characterDir.find_last_of('/');
                        std::string characterFolder = (charSlash != std::string::npos) ? characterDir.substr(charSlash + 1) : characterDir;
                        if (!characterFolder.empty())
                        {
                            _characterKey = characterFolder;
                        }
                    }
                }
            }
        }
    }

    std::transform(_characterKey.begin(), _characterKey.end(), _characterKey.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });

    _animationKeyPrefix = "player_" + _characterKey;

    _defaultAttackAnimationPrefix = "spr_" + _characterKey + "_attack";
    _attackAnimationPrefix = _defaultAttackAnimationPrefix;
    _attackFrameCount = 3;
}

void PlayerCharacter::createSkillSet()
{
    if (_skillSet)
    {
        return;
    }

    // TODO: 后续新增角色时，在此根据 _characterKey 选择不同 SkillSet
    _skillSet = std::make_unique<KleeSkillSet>();
}

void PlayerCharacter::ensureMoveAnimations()
{
    if (_defaultSpriteDir.empty() || _characterKey.empty() || _animationKeyPrefix.empty())
    {
        return;
    }

    std::vector<std::string> movePaths = {
        _defaultSpriteDir + "/spr_" + _characterKey + "_run_1.png",
        _defaultSpriteDir + "/spr_" + _characterKey + "_run_2.png",
        _defaultSpriteDir + "/spr_" + _characterKey + "_run.png",
    };

    ensureMoveAnimationCached(_animationKeyPrefix + "_run", movePaths, 0.15f);
    ensureMoveAnimationCached(_animationKeyPrefix + "_walk", movePaths, 0.25f);
}

void PlayerCharacter::ensureMoveAnimationCached(const std::string &animationKey,
                                                const std::vector<std::string> &framePaths,
                                                float delayPerUnit)
{
    auto cache = AnimationCache::getInstance();
    if (cache->getAnimation(animationKey))
    {
        return;
    }

    auto anim = createAnimationFromPaths(framePaths, delayPerUnit);
    if (anim)
    {
        cache->addAnimation(anim, animationKey);
    }
}

void PlayerCharacter::playOneShotAnimation(const std::vector<std::string> &paths,
                                           float delayPerUnit,
                                           int actionTag,
                                           const std::function<void()> &onFinished)
{
    auto animation = createAnimationFromPaths(paths, delayPerUnit);
    if (!animation)
    {
        if (onFinished)
        {
            onFinished();
        }
        return;
    }

    stopActionByTag(actionTag);
    stopAllActions();

    auto animate = Animate::create(animation);
    auto callbackAction = CallFunc::create([onFinished]()
                                           {
                                               if (onFinished)
                                               {
                                                   onFinished();
                                               } });
    auto sequence = Sequence::create(animate, callbackAction, nullptr);
    sequence->setTag(actionTag);
    runAction(sequence);
}

bool PlayerCharacter::runActionLocked(const std::function<bool()> &preCheck,
                                      const std::function<void(const std::function<void()> &)> &playAnimation,
                                      const std::function<void()> &performEffect,
                                      const std::function<void()> &onFinished)
{
    if (isDead())
    {
        return false;
    }
    if (_actionLocked)
    {
        return false;
    }

    if (preCheck && !preCheck())
    {
        return false;
    }

    _actionLocked = true;
    playAnimation([this, performEffect, onFinished]()
                  {
                      if (performEffect)
                      {
                          performEffect();
                      }
                      _actionLocked = false;
                      if (onFinished)
                      {
                          onFinished();
                      } });
    return true;
}

Vec2 PlayerCharacter::getProjectileSpawnPosition(float spawnOffsetXRatio,
                                                 float spawnOffsetX,
                                                 float spawnOffsetYRatio,
                                                 float spawnOffsetY) const
{
    bool facingLeft = isFlippedX();
    float dirX = facingLeft ? -1.0f : 1.0f;

    Rect playerBox = getBoundingBox();
    float spawnX = playerBox.getMidX() + dirX * (playerBox.size.width * spawnOffsetXRatio + spawnOffsetX);
    float spawnY = playerBox.getMidY() + playerBox.size.height * spawnOffsetYRatio + spawnOffsetY;
    return Vec2(spawnX, spawnY);
}

void PlayerCharacter::addToCombatLayer(Node *node, int zOrder)
{
    if (!node)
    {
        return;
    }

    auto layer = getCombatLayer();
    if (!layer)
    {
        return;
    }

    layer->addChild(node, zOrder);
}
// 根据角色职业初始化基础属性
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
// 根据属性组件刷新当前 HP 和 MP
void PlayerCharacter::refreshHpMpFromAttributes()
{
    auto attr = getAttributeComponent();
    if (!attr)
        return;

    _currentHP = attr->getAttributeValue(AttributeType::MAX_HP);
    _currentMP = attr->getAttributeValue(AttributeType::MAX_MP);
}

void PlayerCharacter::addExperience(int amount)
{
    _experience += amount;

    // 简单经验表：每 100 * 当前等级 涨一级
    while (_experience >= _level * 100)
    {
        _experience -= _level * 100;
        levelUp();
    }
}

void PlayerCharacter::levelUp()
{
    _level++;
    _skillPoints++;

    if (auto attr = getAttributeComponent())
    {
        auto base = attr->getBaseAttributes();
        base.add(AttributeType::MAX_HP, 10.0f);
        base.add(AttributeType::MAX_MP, 5.0f);
        base.add(AttributeType::STRENGTH, 2.0f);
        base.add(AttributeType::DEFENSE, 1.0f);
        attr->setBaseAttributes(base);
    }

    refreshHpMpFromAttributes();
}

void PlayerCharacter::equip(const std::shared_ptr<Equipment> &item)
{
    if (!item)
        return;
    auto attr = getAttributeComponent();
    if (!attr)
        return;

    auto slot = item->slot;

    // 如果这个槽位原来有装备，先移除它的属性加成
    auto it = _equippedItems.find(slot);
    if (it != _equippedItems.end())
    {
        attr->removeEquipmentBonus(it->second->attributeBonus);
    }

    _equippedItems[slot] = item;
    attr->addEquipmentBonus(item->attributeBonus);

    // 如果是武器，更新攻击动画配置
    if (slot == EquipmentSlot::WEAPON)
    {
        auto weapon = std::dynamic_pointer_cast<Weapon>(item);
        onWeaponChanged(weapon);
    }

    // 确保 HP/MP 不超过新的上限
    setCurrentHP(_currentHP);
    setCurrentMP(_currentMP);

    // 触发装备变更回调
    if (_equipmentChangeCallback)
    {
        _equipmentChangeCallback(slot, item);
    }

    CCLOG("Equipped: %s (slot: %d)", item->name.c_str(), static_cast<int>(slot));
}

void PlayerCharacter::unequip(EquipmentSlot slot)
{
    auto attr = getAttributeComponent();
    if (!attr)
        return;

    auto it = _equippedItems.find(slot);
    if (it == _equippedItems.end())
        return;

    std::string itemName = it->second->name;
    attr->removeEquipmentBonus(it->second->attributeBonus);
    _equippedItems.erase(it);

    // 如果卸下武器，恢复默认攻击配置
    if (slot == EquipmentSlot::WEAPON)
    {
        onWeaponChanged(nullptr);
    }

    setCurrentHP(_currentHP);
    setCurrentMP(_currentMP);

    // 触发装备变更回调
    if (_equipmentChangeCallback)
    {
        _equipmentChangeCallback(slot, nullptr);
    }

    CCLOG("Unequipped: %s (slot: %d)", itemName.c_str(), static_cast<int>(slot));
}

std::shared_ptr<Equipment> PlayerCharacter::getEquipment(EquipmentSlot slot) const
{
    auto it = _equippedItems.find(slot);
    if (it != _equippedItems.end())
    {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Weapon> PlayerCharacter::getEquippedWeapon() const
{
    auto equipment = getEquipment(EquipmentSlot::WEAPON);
    if (equipment)
    {
        return std::dynamic_pointer_cast<Weapon>(equipment);
    }
    return nullptr;
}

WeaponType PlayerCharacter::getCurrentWeaponType() const
{
    auto weapon = getEquippedWeapon();
    if (weapon)
    {
        return weapon->type;
    }
    return WeaponType::SWORD; // 默认剑
}

void PlayerCharacter::onWeaponChanged(const std::shared_ptr<Weapon> &weapon)
{
    if (weapon)
    {
        _attackAnimationPrefix = weapon->attackAnimationPrefix.empty()
                                     ? _defaultAttackAnimationPrefix
                                     : weapon->attackAnimationPrefix;
        _attackFrameCount = weapon->attackFrameCount > 0 ? weapon->attackFrameCount : 3;

        CCLOG("Weapon changed: %s, animation: %s, frames: %d",
              weapon->name.c_str(),
              _attackAnimationPrefix.c_str(),
              _attackFrameCount);
    }
    else
    {
        // 恢复默认配置（无武器/拳头）
        _attackAnimationPrefix = _defaultAttackAnimationPrefix;
        _attackFrameCount = 3;
        CCLOG("Weapon unequipped, using default attack");
    }
}

void PlayerCharacter::useSkill(size_t slotIndex)
{
    tryUseSkill(slotIndex);
}

bool PlayerCharacter::tryNormalAttack(const std::function<void()> &onFinished)
{
    if (!_skillSet)
    {
        createSkillSet();
    }
    if (!_skillSet)
    {
        return false;
    }

    return _skillSet->tryNormalAttack(*this, onFinished);
}

bool PlayerCharacter::tryUseSkill(size_t slotIndex, const std::function<void()> &onFinished)
{
    if (!_skillSet)
    {
        createSkillSet();
    }
    if (!_skillSet)
    {
        return false;
    }

    return _skillSet->tryUseSkill(*this, slotIndex, onFinished);
}

void PlayerCharacter::setMoving(bool moving)
{
    setMoving(moving, true);
}

void PlayerCharacter::setMoving(bool moving, bool running)
{
    if (isDead())
        return;

    auto sm = getStateMachineComponent();
    if (!sm)
        return;

    if (moving)
    {
        if (running)
        {
            ensureMoveAnimations();
            sm->changeState(CharacterState::RUNNING);
        }
        else
        {
            ensureMoveAnimations();
            sm->changeState(CharacterState::WALKING);
        }
        return;
    }

    // 停止跑动动作并恢复默认静止帧（保持朝向）
    bool wasFlippedX = isFlippedX();
    stopAllActions();

    auto defaultFrame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(_defaultSpriteDir + "/spr_" + _characterKey + "_run.png");
    if (defaultFrame)
    {
        setSpriteFrame(defaultFrame);
        setFlippedX(wasFlippedX);
    }

    sm->changeState(CharacterState::IDLE);
}

void PlayerCharacter::attackAnimated(const std::function<void()> &onFinished)
{
    if (isDead())
        return;

    // 切换至 ATTACKING 状态（不依赖 StateMachine 的动画播放）
    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::ATTACKING);
    }

    // 计算动画速度（攻击速度越高越快）
    float animSpeed = 0.15f;
    auto equippedWeapon = getEquippedWeapon();
    if (equippedWeapon && equippedWeapon->attackSpeed > 0.0f)
    {
        animSpeed = 0.15f / equippedWeapon->attackSpeed;
    }

    // 构建攻击帧路径
    std::string prefix = _attackAnimationPrefix.empty() ? _defaultAttackAnimationPrefix : _attackAnimationPrefix;
    int frameCount = (_attackFrameCount > 0) ? _attackFrameCount : 3;
    std::vector<std::string> paths;
    paths.reserve(static_cast<size_t>(frameCount));

    for (int i = 1; i <= frameCount; ++i)
    {
        if (SpriteFrameCacheHelper::isFilePath(prefix))
        {
            paths.push_back(StringUtils::format("%s_%d.png", prefix.c_str(), i));
            continue;
        }

        paths.push_back(StringUtils::format("%s/%s_%d.png", _defaultSpriteDir.c_str(), prefix.c_str(), i));
    }

    playOneShotAnimation(paths, animSpeed, 1000, onFinished);
}

void PlayerCharacter::castSkillAnimated(const std::function<void()> &onFinished)
{
    if (isDead())
        return;

    // 切换至 ATTACKING 状态（技能施放暂复用攻击状态）
    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::ATTACKING);
    }

    std::vector<std::string> paths;
    paths.reserve(3);
    for (int i = 1; i <= 3; ++i)
    {
        paths.push_back(StringUtils::format("%s/spr_%s_attack_%d.png", _defaultSpriteDir.c_str(), _characterKey.c_str(), i));
    }

    playOneShotAnimation(paths, 0.13f, 1001, onFinished);
}

void PlayerCharacter::attack()
{
    tryNormalAttack();
}

void PlayerCharacter::onUseActiveSkill(const ActiveSkill &skill)
{
    // 默认行为：切换到 ATTACKING 状态
    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::ATTACKING);
    }

    // TODO: 根据 skill.id 进行不同的特效/逻辑
    // 例如 id == 1001 -> 释放火球；id == 1002 -> 冲刺斩 等
}
