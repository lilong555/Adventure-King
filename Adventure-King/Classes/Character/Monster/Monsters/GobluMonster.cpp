#include "Character/Monster/Monsters/GobluMonster.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Configs/GameConfig.h"
#include "Utils/ParticleVfxHelper.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

namespace
    {
        const char* const GOBLU_ATTACK_NEAR_ANIMATION_KEY = "goblu_attack_near";
        const char* const GOBLU_ATTACK_FAR_ANIMATION_KEY = "goblu_attack_far";
        const char* const GOBLU_DEATH_ANIMATION_KEY = "goblu_death";
        const char* const GOBLU_BREAK_FALL_ANIMATION_KEY = "goblu_break_fall";
        const char* const GOBLU_BREAK_RISE_ANIMATION_KEY = "goblu_break_rise";

        // 远程攻击命中框偏大：宽度缩小为当前的 0.8（即 3.5 * 0.8 = 2.8）
        constexpr float kGobluNearHitboxWidthRatio = 0.8f;
        constexpr float kGobluFarHitboxWidthRatio = 2.8f;
        constexpr float kGobluFarHitboxHeightRatio = 1.0f;
        constexpr float kGobluFarHitboxOffsetRatioX = 2.0f;
        constexpr float kGobluDeathAnimFrameDelay = 0.12f;
        constexpr int kGobluBreakAnimActionTag = 0x4B10;

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

        void ensureOneShotAnimationCached(const std::string& key,
                                          const std::string& formatStr,
                                          int frameCount,
                                          float delay)
        {
            auto cache = AnimationCache::getInstance();
            if (cache->getAnimation(key))
            {
                return;
            }

            Vector<SpriteFrame*> frames;
            for (int i = 1; i <= frameCount; ++i)
            {
                std::string path = StringUtils::format(formatStr.c_str(), i);
                auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(path);
                if (frame)
                {
                    frames.pushBack(frame);
                }
            }

            if (frames.empty())
            {
                return;
            }

            auto anim = Animation::createWithSpriteFrames(frames, delay);
            // 关键：倒地需要停留在最后一帧（fall_3），不要自动还原到原帧
            anim->setRestoreOriginalFrame(false);
            cache->addAnimation(anim, key);
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

int GobluMonster::getExpReward(int playerLevel) const
{
    if (playerLevel < 1)
    {
        playerLevel = 1;
    }

    return GameConfig::Monster::Goblu::EXP_REWARD_BASE +
           (playerLevel - 1) * GameConfig::Monster::Goblu::EXP_REWARD_PER_LEVEL;
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
//此部分仍需重构

//void GobluMonster::preloadResources()
//{
//    // 预缓存状态动画（AnimationCache），并预加载远近攻击动画帧，避免首次生成卡顿
//    ensureSingleFrameAnimationCached("goblu_idle", "Sprites/Enemies/Goblu/Goblu.png");
//    ensureSingleFrameAnimationCached("goblu_hurt", "Sprites/Enemies/Goblu/Goblu.png");
//    ensureLoopAnimationCached(
//        "goblu_walk",
//        "Sprites/Enemies/Goblu/Goblu_walk_%d.png",
//        4,
//        GameConfig::Monster::Goblu::WALK_ANIM_FRAME_DELAY);
//    ensureLoopAnimationCached(
//        GOBLU_DEATH_ANIMATION_KEY,
//        "Sprites/Enemies/Goblu/Goblu_death_%d.png",
//        6,
//        kGobluDeathAnimFrameDelay);
//
//    ensureGobluAttackAnimationCached(GOBLU_ATTACK_NEAR_ANIMATION_KEY, 1, 4);
//    ensureGobluAttackAnimationCached(GOBLU_ATTACK_FAR_ANIMATION_KEY, 11, 15);
//}

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
    ensureLoopAnimationCached(
        GOBLU_DEATH_ANIMATION_KEY,
        "Sprites/Enemies/Goblu/Goblu_death_%d.png",
        6,
        kGobluDeathAnimFrameDelay);

    // 击破：倒地/起身（一次性动画）
    ensureOneShotAnimationCached(
        GOBLU_BREAK_FALL_ANIMATION_KEY,
        "Sprites/Enemies/Goblu/Goblu_fall_%d.png",
        3,
        GameConfig::Monster::Goblu::BREAK_FALL_ANIM_FRAME_DELAY);
    ensureOneShotAnimationCached(
        GOBLU_BREAK_RISE_ANIMATION_KEY,
        "Sprites/Enemies/Goblu/Goblu_rise_%d.png",
        3,
        GameConfig::Monster::Goblu::BREAK_RISE_ANIM_FRAME_DELAY);

    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "goblu_idle");
        sm->registerStateAnimation(CharacterState::HURT, "goblu_hurt");
        sm->registerStateAnimation(CharacterState::WALKING, "goblu_walk");
        sm->registerStateAnimation(CharacterState::DEAD, GOBLU_DEATH_ANIMATION_KEY);
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
    // 击破倒地/起身期间：冻结 AI/移动/攻击（但仍允许受击结算）
    if (_breakState != BreakState::NONE)
    {
        CharacterBase::update(dt);

        if (_physicsBody)
        {
            cocos2d::Vec2 v = _physicsBody->getVelocity();
            v.x = 0.0f;
            _physicsBody->setVelocity(v);
        }
        return;
    }

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

void GobluMonster::takeDamage(const DamageInfo& info)
{
    // 防止死亡后被重复结算（例如 DOT 同帧多 tick、或多次接触回调）
    if (isDead())
    {
        return;
    }

    float dmg = info.amount;
    if (info.isCritical)
    {
        dmg *= info.critMultiplier;
    }

    float hp = getCurrentHP();
    hp -= dmg;

    // UI 辅助：记录“非 DOT 伤害”用于 Boss 血条连击/受击反馈
    recordUiNonDotDamage(dmg, info);

    showDamageNumber(dmg, info.isCritical);
    // Goblu 取消“受击硬直”，但仍保留受击粒子反馈（是否播放由 info.causesHitStun 控制）
    spawnHurtVfx(info);

    setCurrentHP(hp);

    const bool wouldDieBeforeCallback = (hp <= 0.0f);
    if (auto attr = getAttributeComponent())
    {
        attr->executeAfterReceiveDamageHooks(info.attacker, dmg, info, wouldDieBeforeCallback);
    }
    onReceiveDamage(info.attacker, dmg, info, wouldDieBeforeCallback);

    const bool died = isDead();
    if (info.attacker && info.attacker != this)
    {
        if (auto attackerAttr = info.attacker->getAttributeComponent())
        {
            attackerAttr->executeAfterDealDamageHooks(this, dmg, info, died);
        }
        info.attacker->onDealDamage(this, dmg, info, died);
    }

    updateHpBar();

    if (died)
    {
        grantKillExperience(info);
        die();
        return;
    }

    // 取消传统“受击硬直/受击动画”，改为累计击破条
    addBreakDamage(info);
}

void GobluMonster::addBreakDamage(const DamageInfo& info)
{
    if (_breakState != BreakState::NONE)
    {
        return;
    }

    const int gain = std::max(0, info.breakDamage);
    if (gain <= 0)
    {
        return;
    }

    _breakMeter = std::min(GameConfig::Monster::Goblu::BREAK_MAX, _breakMeter + gain);
    if (_breakMeter >= GameConfig::Monster::Goblu::BREAK_MAX)
    {
        startBreakSequence();
    }
}

void GobluMonster::startBreakSequence()
{
    if (_breakState != BreakState::NONE || isDead())
    {
        return;
    }

    _breakState = BreakState::FALLING;
    _breakMeter = GameConfig::Monster::Goblu::BREAK_MAX;

    // 打断当前攻击/移动动作，避免“倒地了还在出手”
    stopAllActions();
    if (auto visual = getVisualSprite())
    {
        visual->stopAllActions();
    }
    _hasMoveGoal = false;
    _returningHome = false;

    if (_physicsBody)
    {
        cocos2d::Vec2 v = _physicsBody->getVelocity();
        v.x = 0.0f;
        _physicsBody->setVelocity(v);
    }

    // 让状态机停止循环动画（ATTACKING 未注册动画，会触发 stopActionByTag(ACTION_TAG_STATE_ANIM)）
    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::ATTACKING);
    }

    FiniteTimeAction* fallAction = nullptr;
    if (auto anim = AnimationCache::getInstance()->getAnimation(GOBLU_BREAK_FALL_ANIMATION_KEY))
    {
        auto a = Animate::create(anim);
        a->setTag(kGobluBreakAnimActionTag);
        fallAction = a;
    }
    else
    {
        fallAction = DelayTime::create(GameConfig::Monster::Goblu::BREAK_FALL_ANIM_FRAME_DELAY * 3.0f);
        fallAction->setTag(kGobluBreakAnimActionTag);
    }

    FiniteTimeAction* riseAction = nullptr;
    if (auto anim = AnimationCache::getInstance()->getAnimation(GOBLU_BREAK_RISE_ANIMATION_KEY))
    {
        auto a = Animate::create(anim);
        a->setTag(kGobluBreakAnimActionTag);
        riseAction = a;
    }
    else
    {
        riseAction = DelayTime::create(GameConfig::Monster::Goblu::BREAK_RISE_ANIM_FRAME_DELAY * 3.0f);
        riseAction->setTag(kGobluBreakAnimActionTag);
    }

    runAction(Sequence::create(
        fallAction,
        CallFunc::create([this]()
                         { _breakState = BreakState::DOWN; }),
        // 倒地后停留在 fall_3（由 Animation::setRestoreOriginalFrame(false) 保证）
        DelayTime::create(std::max(0.0f, GameConfig::Monster::Goblu::BREAK_DOWN_HOLD_SECONDS)),
        CallFunc::create([this]()
                         {
                             _breakMeter = 0;
                             _breakState = BreakState::RISING;
                         }),
        riseAction,
        CallFunc::create([this]()
                         {
                             _breakState = BreakState::NONE;
                             if (auto sm = getStateMachineComponent())
                             {
                                 sm->changeState(CharacterState::IDLE);
                             }
                         }),
        nullptr));
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
    const float hitboxWidth = useNear ? bodyWidth * kGobluNearHitboxWidthRatio : bodyWidth * kGobluFarHitboxWidthRatio;
    const float hitboxHeight = useNear ? bodyHeight * 0.8f : bodyHeight * kGobluFarHitboxHeightRatio;
    const float offsetX = useNear ? 0.5f * bodyWidth : bodyWidth * kGobluFarHitboxOffsetRatioX;

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

    const float hitboxWidth = useNear ? bodyWidth * kGobluNearHitboxWidthRatio : bodyWidth * kGobluFarHitboxWidthRatio;
    const float offsetX = useNear ? 0.5f * bodyWidth : bodyWidth * kGobluFarHitboxOffsetRatioX;
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

                             float hitboxWidth = useNearHitbox ? bodyWidth * kGobluNearHitboxWidthRatio : bodyWidth * kGobluFarHitboxWidthRatio;
                             float hitboxHeight = useNearHitbox ? bodyHeight * 0.8f : bodyHeight * kGobluFarHitboxHeightRatio;
                             float offsetX = useNearHitbox ? 0.5f * bodyWidth : bodyWidth * kGobluFarHitboxOffsetRatioX;

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

                             auto hitboxNode = spawnMeleeHitbox(
                                 offset,
                                 hitboxSize,
                                 std::max(1, damageTag),
                                 GameConfig::Monster::Goblu::HITBOX_LIFE_SECONDS);

                             // 哥布鲁远程攻击：在判定框上挂一个粒子，便于玩家感知命中范围
                             if (!useNearHitbox && hitboxNode)
                             {
                                 // 提高层级：避免被地图/背景遮挡导致“看不到”
                                 hitboxNode->setLocalZOrder(999);

                                 // hitbox 本身存活时间很短(0.12s)，而粒子配置是持续发射型；
                                 // 这里将其改成“短爆发”，并在禁用碰撞后延长节点存活一小段时间用于展示粒子
                                 hitboxNode->stopAllActions();
                                 const float hitboxActiveTime = GameConfig::Monster::Goblu::HITBOX_LIFE_SECONDS;
                                 const float vfxHoldTime = GameConfig::Monster::Goblu::REMOTE_HITBOX_VFX_HOLD_SECONDS;
                                 hitboxNode->runAction(Sequence::create(
                                     DelayTime::create(std::max(0.0f, hitboxActiveTime)),
                                     CallFunc::create([hitboxNode]()
                                                      {
                                                          if (auto body = hitboxNode->getPhysicsBody())
                                                          {
                                                              body->setEnabled(false);
                                                          } }),
                                     DelayTime::create(std::max(0.0f, vfxHoldTime)),
                                     RemoveSelf::create(),
                                     nullptr));

                                 ParticleVfxHelper::PlayOptions vfxOptions;
                                 vfxOptions.zOrder = 1;
                                 vfxOptions.useBodyCenter = false;
                                 vfxOptions.positionType = ParticleSystem::PositionType::GROUPED;
                                 const auto hitboxSize = hitboxNode->getContentSize();
                                 vfxOptions.position = Vec2(hitboxSize.width * 0.5f, hitboxSize.height * 0.5f);
                                 vfxOptions.resetSystem = false; // 先配置参数，再 resetSystem 做短爆发

                                 auto particle = ParticleVfxHelper::playOnce(hitboxNode, "Particle/par_GobluRemoteHit.plist", vfxOptions);
                                 if (particle)
                                 {
                                     // 运行时覆盖：hitbox 生命周期很短，如果完全依赖 plist 的持续发射配置，玩家很难看清命中范围；
                                     // 因此这里将其调整为短爆发（更高发射率/粒子数/尺寸），确保瞬间反馈可见。
                                     particle->setDuration(0.15f);
                                     particle->setTotalParticles(60);
                                     particle->setEmissionRate(260.0f);
                                     particle->setLife(0.25f);
                                     particle->setLifeVar(0.10f);
                                     particle->setStartSize(18.0f);
                                     particle->setStartSizeVar(6.0f);
                                     particle->setEndSize(8.0f);

                                     particle->resetSystem();
                                 }
                             } }),
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

void GobluMonster::die()
{
    if (_deathSequenceStarted)
    {
        return;
    }
    _deathSequenceStarted = true;

    // 禁用物理与移动
    if (_physicsBody)
    {
        _physicsBody->setVelocity(Vec2::ZERO);
        _physicsBody->setDynamic(false);
        _physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::NONE));
        _physicsBody->setCollisionBitmask(0);
        _physicsBody->setContactTestBitmask(0);
    }

    if (_hpBar)
    {
        _hpBar->setVisible(false);
    }

    // 先停止当前攻击/移动等逻辑动作，再播放死亡动画
    stopAllActions();
    if (auto visual = getVisualSprite())
    {
        visual->stopAllActions();
    }

    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::DEAD);
    }

    float deathAnimDuration = 0.0f;
    if (auto anim = AnimationCache::getInstance()->getAnimation(GOBLU_DEATH_ANIMATION_KEY))
    {
        deathAnimDuration = anim->getDuration();
    }
    if (deathAnimDuration <= 0.0f)
    {
        // 兜底：避免动画丢失导致不移除
        deathAnimDuration = 0.6f;
    }

    // 播放完死亡动画后再淡出并移除（避免 0.5s 直接淡出导致动画看不全）
    setCascadeOpacityEnabled(true);
    runAction(Sequence::create(
        DelayTime::create(deathAnimDuration),
        FadeOut::create(0.5f),
        RemoveSelf::create(),
        nullptr));
}
