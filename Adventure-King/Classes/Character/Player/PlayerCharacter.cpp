#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/SkillComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Physics/GamePhysicsCategory.h"
#include "cocos2d.h"
#include <algorithm>
#include <cmath>
#include <vector>

USING_NS_CC;

namespace
{
constexpr float BOMB_THROW_SPEED_X = 300.0f;
constexpr float BOMB_THROW_SPEED_Y = 350.0f;
constexpr float BOMB_DAMAGE = 150.0f;
constexpr float BOMB_EXPLOSION_RADIUS = 80.0f;

constexpr float FIREBALL_SPEED_X = 650.0f;
constexpr float FIREBALL_DAMAGE = 220.0f;
constexpr float FIREBALL_EXPLOSION_RADIUS = 90.0f;

const char *const PROJECTILE_SPRITE_PATH = "Sprites/Characters/Player/Klee/defalt/TNT.png";
const char *const EXPLOSION_SPRITE_PATH = "Sprites/Characters/Player/Klee/defalt/BOOM_1.png";

Animation *createAnimationFromPaths(const std::vector<std::string> &paths, float delayPerUnit)
{
    auto textureCache = Director::getInstance()->getTextureCache();
    Vector<SpriteFrame *> frames;

    for (const auto &path : paths)
    {
        auto texture = textureCache->addImage(path);
        if (!texture)
        {
            CCLOG("PlayerCharacter: failed to load texture %s", path.c_str());
            continue;
        }

        frames.pushBack(SpriteFrame::createWithTexture(
            texture, Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height)));
    }

    if (frames.empty())
        return nullptr;

    return Animation::createWithSpriteFrames(frames, delayPerUnit);
}

void ensureDefaultRunAnimation()
{
    auto cache = AnimationCache::getInstance();
    if (cache->getAnimation("hero_run"))
        return;

    std::vector<std::string> runPaths = {
        "Sprites/Characters/Player/Klee/defalt/spr_klee_run_1.png",
        "Sprites/Characters/Player/Klee/defalt/spr_klee_run_2.png",
        "Sprites/Characters/Player/Klee/defalt/spr_klee_run.png",
    };

    auto runAnim = createAnimationFromPaths(runPaths, 0.15f);
    if (runAnim)
    {
        cache->addAnimation(runAnim, "hero_run");
    }
}

void ensureDefaultWalkAnimation()
{
    auto cache = AnimationCache::getInstance();
    if (cache->getAnimation("hero_walk"))
        return;

    std::vector<std::string> walkPaths = {
        "Sprites/Characters/Player/Klee/defalt/spr_klee_run_1.png",
        "Sprites/Characters/Player/Klee/defalt/spr_klee_run_2.png",
        "Sprites/Characters/Player/Klee/defalt/spr_klee_run.png",
    };

    auto walkAnim = createAnimationFromPaths(walkPaths, 0.25f);
    if (walkAnim)
    {
        cache->addAnimation(walkAnim, "hero_walk");
    }
}
} // namespace

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
    // 先尝试用精灵帧名初始化，如果失败则尝试用文件路径
    bool initSuccess = initWithSpriteFrameName(spriteFrameName);
    if (!initSuccess)
    {
        // 尝试作为普通文件路径初始化
        initSuccess = initWithFile(spriteFrameName);
    }

    if (!initSuccess)
    {
        return false;
    }

    _role = role;

    initAttributesByRole(role);

    if (auto attr = getAttributeComponent())
    {
        attr->recalculateFinalAttributes();
        refreshHpMpFromAttributes();
    }

    // 示例：绑定不同状态的动画名（动画要提前放进 AnimationCache）
    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "hero_idle");
        sm->registerStateAnimation(CharacterState::WALKING, "hero_walk");
        sm->registerStateAnimation(CharacterState::RUNNING, "hero_run");
        sm->registerStateAnimation(CharacterState::ATTACKING, "hero_attack");
        sm->registerStateAnimation(CharacterState::HURT, "hero_hurt");
        sm->registerStateAnimation(CharacterState::DEAD, "hero_dead");
    }

    // 确保默认跑动动画存在（由 StateMachineComponent 播放）
    ensureDefaultRunAnimation();
    ensureDefaultWalkAnimation();

    return true;
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
                                     ? "default"
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
        _attackAnimationPrefix = "default";
        _attackFrameCount = 3;
        CCLOG("Weapon unequipped, using default attack");
    }
}

void PlayerCharacter::useSkill(size_t slotIndex)
{
    if (auto skillComp = getSkillComponent())
    {
        skillComp->useActiveSkill(slotIndex);
    }
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
            ensureDefaultRunAnimation();
            sm->changeState(CharacterState::RUNNING);
        }
        else
        {
            ensureDefaultWalkAnimation();
            sm->changeState(CharacterState::WALKING);
        }
        return;
    }

    // 停止跑动动作并恢复默认静止帧（保持朝向）
    bool wasFlippedX = isFlippedX();
    stopAllActions();

    auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/defalt/spr_klee_run.png");
    if (defaultTexture)
    {
        setTexture(defaultTexture);
        setTextureRect(Rect(0, 0,
                             defaultTexture->getContentSize().width,
                             defaultTexture->getContentSize().height));
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
    std::string prefix = _attackAnimationPrefix.empty() ? "spr_klee_attack" : _attackAnimationPrefix;
    int frameCount = (_attackFrameCount > 0) ? _attackFrameCount : 3;
    std::vector<std::string> paths;
    paths.reserve(static_cast<size_t>(frameCount));

    for (int i = 1; i <= frameCount; ++i)
    {
        std::string path;
        if (prefix.find('/') != std::string::npos)
        {
            path = StringUtils::format("%s_%d.png", prefix.c_str(), i);
        }
        else
        {
            path = StringUtils::format("Sprites/Characters/Player/Klee/defalt/%s_%d.png", prefix.c_str(), i);
        }
        paths.push_back(path);
    }

    auto animation = createAnimationFromPaths(paths, animSpeed);
    if (!animation)
    {
        CCLOG("PlayerCharacter: failed to create attack animation (prefix=%s)", prefix.c_str());
        if (onFinished)
            onFinished();
        return;
    }

    // 停止当前动作，避免与跑动等动画冲突
    stopActionByTag(1000);
    stopAllActions();

    auto animate = Animate::create(animation);
    auto callbackAction = CallFunc::create([onFinished]()
                                           {
                                               if (onFinished)
                                                   onFinished();
                                           });
    auto sequence = Sequence::create(animate, callbackAction, nullptr);
    sequence->setTag(1000);
    runAction(sequence);
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

    std::vector<std::string> paths = {
        "Sprites/Characters/Player/Klee/defalt/spr_klee_attack_1.png",
        "Sprites/Characters/Player/Klee/defalt/spr_klee_attack_2.png",
        "Sprites/Characters/Player/Klee/defalt/spr_klee_attack_3.png",
    };

    auto animation = createAnimationFromPaths(paths, 0.13f);
    if (!animation)
    {
        CCLOG("PlayerCharacter: failed to create skill cast animation");
        if (onFinished)
            onFinished();
        return;
    }

    // 停止当前动作，避免与跑动/攻击动画冲突
    stopActionByTag(1001);
    stopAllActions();

    auto animate = Animate::create(animation);
    auto callbackAction = CallFunc::create([onFinished]()
                                           {
                                               if (onFinished)
                                                   onFinished();
                                           });
    auto sequence = Sequence::create(animate, callbackAction, nullptr);
    sequence->setTag(1001);
    runAction(sequence);
}

void PlayerCharacter::attack()
{
    attackAnimated(nullptr);
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

void PlayerCharacter::cleanupProjectiles()
{
    if (_projectiles.empty())
        return;

    _projectiles.erase(
        std::remove_if(_projectiles.begin(), _projectiles.end(),
                       [](const Projectile &p)
                       { return p.sprite == nullptr; }),
        _projectiles.end());
}

PlayerCharacter::Projectile *PlayerCharacter::findProjectile(Node *node)
{
    if (!node)
        return nullptr;

    for (auto &projectile : _projectiles)
    {
        if (projectile.sprite == node)
        {
            return &projectile;
        }
    }

    return nullptr;
}

void PlayerCharacter::spawnBombProjectile(Node *gameLayer)
{
    if (isDead())
        return;
    if (!gameLayer)
        return;

    auto bombSprite = Sprite::create(PROJECTILE_SPRITE_PATH);
    if (!bombSprite)
    {
        CCLOG("PlayerCharacter::spawnBombProjectile - Failed to create bomb sprite");
        return;
    }

    Projectile projectile;
    projectile.type = ProjectileType::BOMB;
    projectile.isExploded = false;
    projectile.sprite = bombSprite;
    projectile.damage = BOMB_DAMAGE;
    projectile.explosionRadius = BOMB_EXPLOSION_RADIUS;

    bool facingLeft = isFlippedX();
    float throwDirX = facingLeft ? -1.0f : 1.0f;

    Vec2 playerPos = getPosition();
    float offsetX = throwDirX * bombSprite->getContentSize().width;
    float offsetY = bombSprite->getContentSize().height;
    bombSprite->setPosition(playerPos + Vec2(offsetX, offsetY));
    bombSprite->setScale(0.5f);

    PhysicsMaterial bombMaterial(0.5f, 0.3f, 0.2f);
    auto physicsBody = PhysicsBody::createCircle(15.0f, bombMaterial);
    physicsBody->setDynamic(true);
    physicsBody->setMass(0.5f);
    physicsBody->setRotationEnable(true);

    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLAYER_ATTACK));
    physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER));
    physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER));
    physicsBody->setTag(0);

    bombSprite->addComponent(physicsBody);
    gameLayer->addChild(bombSprite, 4);

    Vec2 impulse(throwDirX * BOMB_THROW_SPEED_X * physicsBody->getMass(),
                 BOMB_THROW_SPEED_Y * physicsBody->getMass());
    physicsBody->applyImpulse(impulse);

    _projectiles.push_back(projectile);
}

void PlayerCharacter::spawnFireballProjectile(Node *gameLayer)
{
    if (isDead())
        return;
    if (!gameLayer)
        return;

    auto fireballSprite = Sprite::create(PROJECTILE_SPRITE_PATH);
    if (!fireballSprite)
    {
        CCLOG("PlayerCharacter::spawnFireballProjectile - Failed to create fireball sprite");
        return;
    }

    Projectile projectile;
    projectile.type = ProjectileType::FIREBALL;
    projectile.isExploded = false;
    projectile.sprite = fireballSprite;
    projectile.damage = FIREBALL_DAMAGE;
    projectile.explosionRadius = FIREBALL_EXPLOSION_RADIUS;

    bool facingLeft = isFlippedX();
    float dirX = facingLeft ? -1.0f : 1.0f;

    Vec2 playerPos = getPosition();
    fireballSprite->setPosition(playerPos + Vec2(dirX * 60.0f, 60.0f));
    fireballSprite->setScale(0.35f);
    fireballSprite->setColor(Color3B(255, 120, 60));

    PhysicsMaterial fireballMaterial(0.5f, 0.0f, 0.0f);
    auto physicsBody = PhysicsBody::createCircle(12.0f, fireballMaterial);
    physicsBody->setDynamic(true);
    physicsBody->setMass(0.4f);
    physicsBody->setRotationEnable(false);
    physicsBody->setGravityEnable(false);

    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::PLAYER_ATTACK));
    physicsBody->setCollisionBitmask(ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER));
    physicsBody->setContactTestBitmask(ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION | GamePhysicsCategory::MONSTER));
    physicsBody->setTag(0);

    fireballSprite->addComponent(physicsBody);
    gameLayer->addChild(fireballSprite, 4);

    Vec2 impulse(dirX * FIREBALL_SPEED_X * physicsBody->getMass(), 0.0f);
    physicsBody->applyImpulse(impulse);

    _projectiles.push_back(projectile);
}

bool PlayerCharacter::handleProjectileContact(Node *nodeA, int categoryA,
                                              Node *nodeB, int categoryB,
                                              Node *gameLayer)
{
    if (!_projectiles.empty())
    {
        Projectile *projectile = findProjectile(nodeA);
        Node *projectileNode = nodeA;
        Node *otherNode = nodeB;
        int otherMask = categoryB;

        if (!projectile)
        {
            projectile = findProjectile(nodeB);
            projectileNode = nodeB;
            otherNode = nodeA;
            otherMask = categoryA;
        }

        if (projectile && !projectile->isExploded)
        {
            bool hitPlayer = (otherMask & ToMask(GamePhysicsCategory::PLAYER)) != 0;
            bool hitAnotherProjectile = (findProjectile(otherNode) != nullptr);

            if (!hitPlayer && !hitAnotherProjectile)
            {
                projectile->isExploded = true;

                // Avoid modifying physics bodies inside the contact callback.
                // Defer the actual explosion to the next tick.
                this->runAction(Sequence::create(
                    DelayTime::create(0.0f),
                    CallFunc::create([this, gameLayer, projectileNode]()
                                     {
                                         if (!gameLayer)
                                             return;

                                         auto pending = findProjectile(projectileNode);
                                         if (pending && pending->isExploded)
                                         {
                                             explodeProjectile(*pending, gameLayer);
                                         }
                                     }),
                    nullptr));
                return true;
            }
        }
    }

    return false;
}

void PlayerCharacter::explodeProjectile(Projectile &projectile, Node *gameLayer)
{
    if (!projectile.sprite)
        return;

    Vec2 explodePos = projectile.sprite->getPosition();
    Vec2 explosionWorld = explodePos;
    if (projectile.sprite->getParent())
    {
        explosionWorld = projectile.sprite->getParent()->convertToWorldSpace(explodePos);
    }

    projectile.sprite->removeFromParent();

    float explosionDamage = projectile.damage;
    float explosionRadius = projectile.explosionRadius;
    if (explosionDamage <= 0.0f)
    {
        explosionDamage = (projectile.type == ProjectileType::FIREBALL) ? FIREBALL_DAMAGE : BOMB_DAMAGE;
    }
    if (explosionRadius <= 0.0f)
    {
        explosionRadius = (projectile.type == ProjectileType::FIREBALL) ? FIREBALL_EXPLOSION_RADIUS : BOMB_EXPLOSION_RADIUS;
    }

    if (gameLayer)
    {
        DamageInfo dmg;
        dmg.amount = explosionDamage;
        dmg.attacker = this;

        std::vector<CharacterBase *> hitTargets;

        std::function<void(Node *)> collectTargets = [&](Node *node)
        {
            if (!node)
                return;

            if (auto character = dynamic_cast<CharacterBase *>(node))
            {
                if (character != this && !character->isDead())
                {
                    Rect hitRectWorld;
                    bool hasHitRectWorld = false;

                    if (auto body = character->getPhysicsBody())
                    {
                        auto shape = body->getFirstShape();
                        if (shape)
                        {
                            Size shapeSizeWorld;
                            switch (shape->getType())
                            {
                            case PhysicsShape::Type::BOX:
                                shapeSizeWorld = static_cast<PhysicsShapeBox *>(shape)->getSize();
                                break;
                            case PhysicsShape::Type::CIRCLE:
                            {
                                float r = static_cast<PhysicsShapeCircle *>(shape)->getRadius();
                                shapeSizeWorld = Size(r * 2.0f, r * 2.0f);
                                break;
                            }
                            default:
                                break;
                            }

                            if (shapeSizeWorld.width > 0.0f && shapeSizeWorld.height > 0.0f)
                            {
                                Vec2 centerLocal(character->getContentSize().width * 0.5f,
                                                 character->getContentSize().height * 0.5f);
                                Vec2 bodyCenterWorld = character->convertToWorldSpace(centerLocal);
                                Vec2 rectCenterWorld = bodyCenterWorld + shape->getCenter();

                                hitRectWorld = Rect(rectCenterWorld.x - shapeSizeWorld.width / 2.0f,
                                                    rectCenterWorld.y - shapeSizeWorld.height / 2.0f,
                                                    shapeSizeWorld.width,
                                                    shapeSizeWorld.height);
                                hasHitRectWorld = true;
                            }
                        }
                    }

                    if (!hasHitRectWorld)
                    {
                        Rect bboxParent = character->getBoundingBox();
                        Vec2 originWorld = bboxParent.origin;
                        Vec2 topRightWorld = bboxParent.origin + bboxParent.size;
                        if (character->getParent())
                        {
                            originWorld = character->getParent()->convertToWorldSpace(bboxParent.origin);
                            topRightWorld = character->getParent()->convertToWorldSpace(bboxParent.origin + bboxParent.size);
                        }

                        hitRectWorld = Rect(
                            std::min(originWorld.x, topRightWorld.x),
                            std::min(originWorld.y, topRightWorld.y),
                            std::fabs(topRightWorld.x - originWorld.x),
                            std::fabs(topRightWorld.y - originWorld.y));
                    }

                    float dx = 0.0f;
                    if (explosionWorld.x < hitRectWorld.getMinX())
                        dx = hitRectWorld.getMinX() - explosionWorld.x;
                    else if (explosionWorld.x > hitRectWorld.getMaxX())
                        dx = explosionWorld.x - hitRectWorld.getMaxX();

                    float dy = 0.0f;
                    if (explosionWorld.y < hitRectWorld.getMinY())
                        dy = hitRectWorld.getMinY() - explosionWorld.y;
                    else if (explosionWorld.y > hitRectWorld.getMaxY())
                        dy = explosionWorld.y - hitRectWorld.getMaxY();

                    if ((dx * dx + dy * dy) <= (explosionRadius * explosionRadius))
                    {
                        hitTargets.push_back(character);
                    }
                }
            }

            // Copy child list to avoid iterator invalidation if takeDamage spawns nodes.
            auto children = node->getChildren();
            for (auto child : children)
            {
                collectTargets(child);
            }
        };

        collectTargets(gameLayer);

        for (auto target : hitTargets)
        {
            if (target && !target->isDead())
            {
                target->takeDamage(dmg);
            }
        }
    }

    auto boomSprite = Sprite::create(EXPLOSION_SPRITE_PATH);
    if (boomSprite && gameLayer)
    {
        boomSprite->setPosition(explodePos);
        boomSprite->setScale(0.8f);
        gameLayer->addChild(boomSprite, 6);

        auto scaleUp = ScaleTo::create(0.2f, 1.2f);
        auto fadeOut = FadeOut::create(0.3f);
        auto spawn = Spawn::create(scaleUp, fadeOut, nullptr);
        auto remove = RemoveSelf::create();
        boomSprite->runAction(Sequence::create(spawn, remove, nullptr));
    }

    projectile.sprite = nullptr;
}
