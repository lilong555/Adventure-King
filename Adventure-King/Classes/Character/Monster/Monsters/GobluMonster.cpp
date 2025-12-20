#include "Character/Monster/Monsters/GobluMonster.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Configs/GameConfigs.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

namespace
{
    const char* const GOBLU_ATTACK_NEAR_ANIMATION_KEY = "goblu_attack_near";
    const char* const GOBLU_ATTACK_FAR_ANIMATION_KEY = "goblu_attack_far";

    void ensureSingleFrameAnimationCached(const std::string &animationKey,
                                          const std::string &framePath,
                                          float delayPerUnit = 0.2f)
    {
        auto cache = AnimationCache::getInstance();
        if (cache->getAnimation(animationKey))
        {
            return;
        }

        auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(framePath);
        if (!frame)
        {
            return;
        }

        Vector<SpriteFrame *> frames;
        frames.pushBack(frame);
        auto anim = Animation::createWithSpriteFrames(frames, delayPerUnit);
        cache->addAnimation(anim, animationKey);
    }

    void ensureLoopAnimationCached(const std::string &key,
                                   const std::string &formatStr,
                                   int frameCount,
                                   float delay)
    {
        auto cache = AnimationCache::getInstance();
        if (cache->getAnimation(key))
        {
            return;
        }

        Vector<SpriteFrame *> frames;
        for (int i = 1; i <= frameCount; i++)
        {
            std::string path = StringUtils::format(formatStr.c_str(), i);
            auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(path);
            if (frame)
            {
                frames.pushBack(frame);
            }
        }

        if (!frames.empty())
        {
            auto anim = Animation::createWithSpriteFrames(frames, delay);
            cache->addAnimation(anim, key);
        }
    }

    void ensureGobluAttackAnimationCached(const char* animationKey, int startIndex, int endIndex)
    {
        auto cache = AnimationCache::getInstance();
        if (cache->getAnimation(animationKey))
        {
            return;
        }

        Vector<SpriteFrame*> frames;
        for (int i = startIndex; i <= endIndex; ++i)
        {
            std::string path = StringUtils::format("Sprites/Enemies/Goblu/Goblu_attack_%02d.png", i);
            auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(path);
            if (!frame)
            {
                break;
            }
            frames.pushBack(frame);
        }

        if (frames.empty())
        {
            return;
        }

        auto animation = Animation::createWithSpriteFrames(frames, GameConfig::Monster::Goblu::ATTACK_ANIM_FRAME_DELAY);
        cache->addAnimation(animation, animationKey);
    }
}

GobluMonster::GobluMonster() = default;

GobluMonster::~GobluMonster()
{
    CC_SAFE_RELEASE(_attackAnimateNear);
    CC_SAFE_RELEASE(_attackAnimateFar);
}

GobluMonster *GobluMonster::create(const std::string &spriteFrameName)
{
    auto ret = new (std::nothrow) GobluMonster();
    if (ret && ret->init(spriteFrameName))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GobluMonster::init(const std::string &spriteFrameName)
{
    if (!MonsterBase::init(spriteFrameName))
    {
        return false;
    }

    const float visualScale = GameConfig::Monster::Goblu::SCALE *
                              GameConfig::Monster::Goblu::SCALE_MULTIPLIER;
    setScale(visualScale);
    _baseScaleX = visualScale;

    setAIConfig(GameConfig::Monster::Goblu::VISION_RANGE,
                GameConfig::Monster::Goblu::CHASE_RANGE,
                GameConfig::Monster::Goblu::PATROL_ENABLED);

    {
        const Size size = getContentSize();
        const Size bodySize(size.width * GameConfig::Monster::Goblu::PHYSICS_BOX_RATIO_W,
                            size.height * GameConfig::Monster::Base::PHYSICS_BOX_RATIO_H);
        auto body = PhysicsBody::createBox(bodySize, GameConfig::Material::MONSTER);
        body->setDynamic(true);
        body->setRotationEnable(false);
        body->setGravityEnable(true);

        body->setCategoryBitmask(ToMask(GamePhysicsCategory::MONSTER));
        body->setCollisionBitmask(
            ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::PLAYER | GamePhysicsCategory::PLAYER_ATTACK |
                   GamePhysicsCategory::BOMB | GamePhysicsCategory::COLLISION));
        body->setContactTestBitmask(
            ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB));

        _physicsBody = body;
        setPhysicsBody(_physicsBody);
    }

    initAttributes();
    _baseAttackRange = _attackRange;
    setHpBarScale(GameConfig::Monster::Goblu::HP_BAR_SCALE);
    setCurrentHP(_maxHP);
    _currentMP = 0;
    updateHpBar();

    initStateAnimations();
    initAnimations();
    return true;
}

void GobluMonster::preloadResources()
{
    // 预缓存状态动画（AnimationCache），并预加载远近攻击动画帧，避免首次生成卡顿
    ensureSingleFrameAnimationCached("goblu_idle", "Sprites/Enemies/Goblu/Goblu.png");
    ensureSingleFrameAnimationCached("goblu_hurt", "Sprites/Enemies/Goblu/Goblu.png");
    ensureLoopAnimationCached(
        "goblu_walk",
        "Sprites/Enemies/Goblu/Goblu_walk_%d.png",
        4,
        GameConfig::Monster::Goblu::WALK_ANIM_FRAME_DELAY);

    ensureGobluAttackAnimationCached(GOBLU_ATTACK_NEAR_ANIMATION_KEY, 1, 4);
    ensureGobluAttackAnimationCached(GOBLU_ATTACK_FAR_ANIMATION_KEY, 11, 15);
}

void GobluMonster::initAttributes()
{
    namespace Conf = GameConfig::Monster::Goblu;

    Attributes base;
    base.set(AttributeType::STRENGTH, Conf::STRENGTH);
    base.set(AttributeType::DEFENSE, Conf::DEFENSE);
    base.set(AttributeType::MOVE_SPEED, Conf::MOVE_SPEED);
    base.set(AttributeType::CRITICAL_RATE, Conf::CRITICAL_RATE);
    base.set(AttributeType::MAX_HP, Conf::MAX_HP);
    base.set(AttributeType::MAX_MP, Conf::MAX_MP);
    base.set(AttributeType::ATTACKINTERVAL, Conf::ATTACK_INTERVAL);
    base.set(AttributeType::ATTACK_RANGE, Conf::ATTACK_RANGE);

    setupCharacterStats(base);

    CCLOG("Goblu Attributes Set. Speed: %.0f, HP: %.0f", Conf::MOVE_SPEED, Conf::MAX_HP);
}

void GobluMonster::initStateAnimations()
{
    ensureSingleFrameAnimationCached("goblu_idle", "Sprites/Enemies/Goblu/Goblu.png");
    ensureSingleFrameAnimationCached("goblu_hurt", "Sprites/Enemies/Goblu/Goblu.png");
    ensureLoopAnimationCached(
        "goblu_walk",
        "Sprites/Enemies/Goblu/Goblu_walk_%d.png",
        4,
        GameConfig::Monster::Goblu::WALK_ANIM_FRAME_DELAY);

    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "goblu_idle");
        sm->registerStateAnimation(CharacterState::HURT, "goblu_hurt");
        sm->registerStateAnimation(CharacterState::WALKING, "goblu_walk");
    }
}

void GobluMonster::initAnimations()
{
    CC_SAFE_RELEASE(_attackAnimateNear);
    CC_SAFE_RELEASE(_attackAnimateFar);
    _attackAnimateNear = nullptr;
    _attackAnimateFar = nullptr;

    ensureGobluAttackAnimationCached(GOBLU_ATTACK_NEAR_ANIMATION_KEY, 1, 4);
    ensureGobluAttackAnimationCached(GOBLU_ATTACK_FAR_ANIMATION_KEY, 11, 15);

    if (auto nearAnim = AnimationCache::getInstance()->getAnimation(GOBLU_ATTACK_NEAR_ANIMATION_KEY))
    {
        _attackAnimateNear = Animate::create(nearAnim);
        _attackAnimateNear->retain();
    }
    if (auto farAnim = AnimationCache::getInstance()->getAnimation(GOBLU_ATTACK_FAR_ANIMATION_KEY))
    {
        _attackAnimateFar = Animate::create(farAnim);
        _attackAnimateFar->retain();
    }
}

void GobluMonster::update(float dt)
{
    if (_target)
    {
        const float myHalfWidth = getNodeHalfWidth(this);
        const float targetHalfWidth = getNodeHalfWidth(_target);
        const float farReach = getAttackReachX(false);
        _attackRange = farReach + myHalfWidth + targetHalfWidth;
    }
    else
    {
        _attackRange = _baseAttackRange;
    }

    MonsterBase::update(dt);
}

float GobluMonster::getNodeHalfWidth(cocos2d::Node *node)
{
    if (!node)
    {
        return 0.0f;
    }

    if (auto body = node->getPhysicsBody())
    {
        if (auto shape = body->getFirstShape())
        {
            if (auto box = dynamic_cast<PhysicsShapeBox *>(shape))
            {
                return 0.5f * box->getSize().width * std::fabs(node->getScaleX());
            }
        }
    }

    return 0.5f * node->getContentSize().width * std::fabs(node->getScaleX());
}

cocos2d::Size GobluMonster::getBodyWorldSize() const
{
    Size bodySize = getContentSize();
    if (auto body = getPhysicsBody())
    {
        if (auto shape = body->getFirstShape())
        {
            if (auto box = dynamic_cast<PhysicsShapeBox *>(shape))
            {
                bodySize = box->getSize();
            }
        }
    }

    const float scaleX = std::fabs(getScaleX()) * GameConfig::Monster::Goblu::SCALE;
    const float scaleY = std::fabs(getScaleY()) * GameConfig::Monster::Goblu::SCALE;
    return Size(bodySize.width * scaleX,
                bodySize.height * scaleY);
}

cocos2d::Size GobluMonster::getNodeBodyWorldSize(const cocos2d::Node* node) const
{
    if (!node)
    {
        return Size::ZERO;
    }

    Size size = node->getContentSize();
    if (auto body = node->getPhysicsBody())
    {
        if (auto shape = body->getFirstShape())
        {
            if (auto box = dynamic_cast<PhysicsShapeBox*>(shape))
            {
                size = box->getSize();
            }
        }
    }

    return Size(size.width * std::fabs(node->getScaleX()),
                size.height * std::fabs(node->getScaleY()));
}

bool GobluMonster::canHitTarget(bool useNear) const
{
    if (!_target)
    {
        return false;
    }

    const Size bodySize = getBodyWorldSize();
    const Size targetSize = getNodeBodyWorldSize(_target);
    if (bodySize.width <= 0.0f || bodySize.height <= 0.0f ||
        targetSize.width <= 0.0f || targetSize.height <= 0.0f)
    {
        return false;
    }

    const float direction = (getScaleX() > 0.0f) ? 1.0f : -1.0f;
    const float bodyWidth = bodySize.width;
    const float bodyHeight = bodySize.height;
    const float hitboxWidth = useNear ? bodyWidth * 0.8f : bodyWidth * 3.5f;
    const float hitboxHeight = useNear ? bodyHeight * 0.8f : bodyHeight;
    const float offsetX = useNear ? 0.5f * bodyWidth : bodyWidth * 2.0f;

    Vec2 hitboxCenter = getWorldPosition(this) + Vec2(offsetX * direction, 0.5f * hitboxHeight);
    Rect hitboxRect(hitboxCenter.x - hitboxWidth * 0.5f,
                    hitboxCenter.y - hitboxHeight * 0.5f,
                    hitboxWidth,
                    hitboxHeight);

    Vec2 targetCenter = getWorldPosition(_target);
    Rect targetRect(targetCenter.x - targetSize.width * 0.5f,
                    targetCenter.y - targetSize.height * 0.5f,
                    targetSize.width,
                    targetSize.height);

    return hitboxRect.intersectsRect(targetRect);
}

float GobluMonster::getGapXToTarget(cocos2d::Node *target)
{
    if (!target)
    {
        return 999999.0f;
    }
    const float myHalfWidth = getNodeHalfWidth(this);
    const float targetHalfWidth = getNodeHalfWidth(target);
    const float centerGap = std::fabs(getWorldPosition(this).x - getWorldPosition(target).x);
    return std::max(0.0f, centerGap - (myHalfWidth + targetHalfWidth));
}

float GobluMonster::getAttackReachX(bool useNear)
{
    const float bodyWidth = getBodyWorldSize().width;
    if (bodyWidth <= 0.0f)
    {
        return _baseAttackRange;
    }

    const float hitboxWidth = useNear ? bodyWidth * 0.8f : bodyWidth * 3.5f;
    const float offsetX = useNear ? 0.5f * bodyWidth : bodyWidth * 2.0f;
    const float reachFromEdge = (offsetX + 0.5f * hitboxWidth) - 0.5f * bodyWidth;
    return std::max(0.0f, reachFromEdge);
}

void GobluMonster::attack()
{
    CCLOG("Goblu Attack Triggered!");

    FiniteTimeAction *animateAction = nullptr;
    cocos2d::Animate *selectedAnimate = nullptr;

    float gapX = -1.0f;
    bool canNear = false;
    bool canFar = false;
    bool useNearHitbox = false;
    if (_target)
    {
        gapX = getGapXToTarget(_target);
        canNear = canHitTarget(true);
        canFar = canHitTarget(false);

        if (canNear)
        {
            useNearHitbox = true;
            selectedAnimate = _attackAnimateNear;
        }
        else if (canFar)
        {
            useNearHitbox = false;
            selectedAnimate = _attackAnimateFar;
        }
    }

    if (gapX >= 0.0f)
    {
        CCLOG("Goblu Attack Select: gap=%.1f, useNear=%s",
              gapX,
              useNearHitbox ? "true" : "false");
    }

    if (!canNear && !canFar)
    {
        auto sm = getStateMachineComponent();
        if (sm && sm->getCurrentState() == CharacterState::ATTACKING)
        {
            sm->changeState(CharacterState::IDLE);
        }
        return;
    }

    if (!selectedAnimate)
    {
        selectedAnimate = _attackAnimateNear ? _attackAnimateNear : _attackAnimateFar;
    }

    animateAction = selectedAnimate ? static_cast<FiniteTimeAction *>(selectedAnimate->clone())
                                    : static_cast<FiniteTimeAction *>(DelayTime::create(1.0f));

    float hitTime = GameConfig::Monster::Goblu::ATTACK_HIT_FALLBACK_TIME;
    if (selectedAnimate)
    {
        int frameCount = selectedAnimate->getAnimation()->getFrames().size();
        if (frameCount > 0)
        {
            const int preferredHitFrame = useNearHitbox ? 3 : 4;
            const int hitFrameIndex = std::min(preferredHitFrame, frameCount);
            float frameTime = selectedAnimate->getDuration() / frameCount;
            const int startIndex = std::max(0, hitFrameIndex - 1);
            hitTime = frameTime * static_cast<float>(startIndex);
        }
    }

    auto logicSequence = Sequence::create(
        DelayTime::create(hitTime),
        CallFunc::create([this, useNearHitbox]()
                         {
                             float direction = (getScaleX() > 0.0f) ? 1.0f : -1.0f;
                             const Size worldBodySize = getBodyWorldSize();
                             const float bodyWidth = worldBodySize.width;
                             const float bodyHeight = worldBodySize.height;

                             float hitboxWidth = useNearHitbox ? bodyWidth * 0.8f : bodyWidth * 3.5f;
                             float hitboxHeight = useNearHitbox ? bodyHeight * 0.8f : bodyHeight;
                             float offsetX = useNearHitbox ? 0.5f * bodyWidth : bodyWidth * 2.0f;

                             Size hitboxSize(hitboxWidth, hitboxHeight);
                             Vec2 offset(offsetX * direction, 0.5f * hitboxSize.height);

                             int damageTag = 1;
                             if (auto attr = getAttributeComponent())
                             {
                                 float strength = attr->getAttributeValue(AttributeType::STRENGTH);
                                 if (strength > 0.0f)
                                 {
                                     damageTag = static_cast<int>(std::round(strength));
                                 }
                             }

                             spawnMeleeHitbox(
                                 offset,
                                 hitboxSize,
                                 std::max(1, damageTag),
                                 GameConfig::Monster::Goblu::HITBOX_LIFE_SECONDS); }),
        nullptr);

    auto spawn = Spawn::create(animateAction, logicSequence, nullptr);
    auto finalSequence = Sequence::create(
        spawn,
        CallFunc::create([this]()
                         {
                             auto sm = getStateMachineComponent();
                             if (sm && sm->getCurrentState() != CharacterState::DEAD)
                             {
                                 sm->changeState(CharacterState::IDLE);
                             } }),
        nullptr);

    runAction(finalSequence);
}
