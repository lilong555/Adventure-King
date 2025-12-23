#include "Character/Monster/Monsters/ObscurMonster.h"

#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Configs/GamePhysicsCategory.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

namespace
{
    const char* const OBSCUR_MELEE_ATTACK_ANIM_KEY = "obscur_attack";
    const char* const OBSCUR_USEICE_LOOP_ANIM_KEY = "obscur_useice_loop";
    const char* const OBSCUR_ICE_ANIM_KEY = "obscur_ice";

    // Obscur_useice_x 循环动作标签（避免和状态机动画互相影响）
    constexpr int ACTION_TAG_OBSCUR_USEICE_LOOP = 2331;

    void ensureSingleFrameAnimationCached(const std::string& animationKey,
                                          const std::string& framePath,
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

        Vector<SpriteFrame*> frames;
        frames.pushBack(frame);
        auto anim = Animation::createWithSpriteFrames(frames, delayPerUnit);
        cache->addAnimation(anim, animationKey);
    }

    void ensureAnimationCached(const std::string& key,
                               const std::string& formatStr,
                               int frameCount,
                               float delayPerFrame)
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

        auto anim = Animation::createWithSpriteFrames(frames, delayPerFrame);
        cache->addAnimation(anim, key);
    }

    float getFrameTimeForAnimation(const Animation* animation, float fallbackDelayPerFrame)
    {
        if (!animation)
        {
            return std::max(0.0f, fallbackDelayPerFrame);
        }

        const auto& frames = animation->getFrames();
        const int frameCount = static_cast<int>(frames.size());
        if (frameCount <= 0)
        {
            return std::max(0.0f, fallbackDelayPerFrame);
        }

        const float duration = animation->getDuration();
        if (duration <= 0.0f)
        {
            return std::max(0.0f, fallbackDelayPerFrame);
        }

        return duration / static_cast<float>(frameCount);
    }
}

ObscurMonster::ObscurMonster() = default;

ObscurMonster::~ObscurMonster()
{
    CC_SAFE_RELEASE(_meleeAttackAnimate);
}

int ObscurMonster::getExpReward(int playerLevel) const
{
    if (playerLevel < 1)
    {
        playerLevel = 1;
    }

    return GameConfig::Monster::Obscur::EXP_REWARD_BASE +
           (playerLevel - 1) * GameConfig::Monster::Obscur::EXP_REWARD_PER_LEVEL;
}

ObscurMonster* ObscurMonster::create(const std::string& spriteFrameName)
{
    auto ret = new (std::nothrow) ObscurMonster();
    if (ret && ret->init(spriteFrameName))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void ObscurMonster::preloadResources()
{
    // 说明：这里预热的是 AnimationCache（不是 TextureCache）。
    // 贴图可以通过 LoadingScene/MapScene 预加载，但如果不创建 AnimationCache：
    // - Obscur_attack/useice/ice 的动画会缺失（只能走兜底 DelayTime）
    // - 或首次播放时临时拼帧造成卡顿
    ensureSingleFrameAnimationCached("obscur_idle", "Sprites/Enemies/Obscur/Obscur_idle.png");
    ensureSingleFrameAnimationCached("obscur_hurt", "Sprites/Enemies/Obscur/Obscur_beattacked.png");

    // 无 walk 素材：走路阶段复用 idle
    ensureAnimationCached(OBSCUR_MELEE_ATTACK_ANIM_KEY,
                          "Sprites/Enemies/Obscur/Obscur_attack_%d.png",
                          4,
                          GameConfig::Monster::Obscur::ATTACK_ANIM_FRAME_DELAY);
    ensureAnimationCached(OBSCUR_USEICE_LOOP_ANIM_KEY,
                          "Sprites/Enemies/Obscur/Obscur_useice_%d.png",
                          2,
                          GameConfig::Monster::Obscur::USEICE_ANIM_FRAME_DELAY);
    ensureAnimationCached(OBSCUR_ICE_ANIM_KEY,
                          "Sprites/Enemies/Obscur/Obscur_ice_%d.png",
                          5,
                          GameConfig::Monster::Obscur::ICE_ANIM_FRAME_DELAY);
}

std::vector<std::string> ObscurMonster::getPreloadResourcePaths()
{
    std::vector<std::string> paths;

    // 1. 基础状态素材
    paths.push_back("Sprites/Enemies/Obscur/Obscur_idle.png");
    paths.push_back("Sprites/Enemies/Obscur/Obscur_beattacked.png");

    // 2. 近战攻击序列帧 (按逻辑对应 4 帧)
    for (int i = 1; i <= 4; ++i) {
        paths.push_back(cocos2d::StringUtils::format("Sprites/Enemies/Obscur/Obscur_attack_%d.png", i));
    }

    // 3. 施法动作序列帧 (按逻辑对应 2 帧)
    for (int i = 1; i <= 2; ++i) {
        paths.push_back(cocos2d::StringUtils::format("Sprites/Enemies/Obscur/Obscur_useice_%d.png", i));
    }

    // 4. 冰刺特效序列帧 (按逻辑对应 5 帧)
    for (int i = 1; i <= 5; ++i) {
        paths.push_back(cocos2d::StringUtils::format("Sprites/Enemies/Obscur/Obscur_ice_%d.png", i));
    }

    return paths;
}


bool ObscurMonster::init(const std::string& spriteFrameName)
{
    if (!MonsterBase::init(spriteFrameName))
    {
        return false;
    }

    // Obscur 以脚底为锚点（底部对齐），避免“以中心为锚点”导致的站立表现悬空
    // ⚠️ X 轴保持默认 0.5，避免左右错位；只调整 Y（脚底对齐）
    const auto contentSize = getContentSize();
    setAnchorPoint(Vec2(GameConfig::Monster::Base::ANCHOR_X, GameConfig::Monster::Base::ANCHOR_Y));

    // Obscur 体型缩放与碰撞箱：按策划要求使用固定大小碰撞箱（235x449）
    setScale(GameConfig::Monster::Obscur::SCALE);
    _baseScaleX = GameConfig::Monster::Obscur::SCALE;

    setAIConfig(GameConfig::Monster::Obscur::VISION_RANGE,
                GameConfig::Monster::Obscur::CHASE_RANGE,
                GameConfig::Monster::Obscur::PATROL_ENABLED);

    {
        // 仅保留 Y 轴对齐（解决“悬空”）；X 轴不做偏移（避免左右错位）
        float bodyOffsetY = 0.0f;
        if (contentSize.height > 0.0f)
        {
            bodyOffsetY = (GameConfig::Monster::Obscur::PHYSICS_BOX_HEIGHT - contentSize.height) * 0.5f;
        }

        auto body = PhysicsBody::createBox(
            Size(GameConfig::Monster::Obscur::PHYSICS_BOX_WIDTH, GameConfig::Monster::Obscur::PHYSICS_BOX_HEIGHT),
            GameConfig::Material::MONSTER,
            Vec2(0.0f, bodyOffsetY));
        body->setDynamic(true);
        body->setRotationEnable(false);
        body->setGravityEnable(true);

        body->setCategoryBitmask(ToMask(GamePhysicsCategory::MONSTER));
        body->setCollisionBitmask(ToMask(GamePhysicsCategory::PLATFORM |
                                         GamePhysicsCategory::PLAYER |
                                         GamePhysicsCategory::PLAYER_ATTACK |
                                         GamePhysicsCategory::BOMB |
                                         GamePhysicsCategory::COLLISION));
        body->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER |
                                           GamePhysicsCategory::PLAYER_ATTACK |
                                           GamePhysicsCategory::BOMB));

        _physicsBody = body;
        setPhysicsBody(_physicsBody);
    }

    initAttributes();
    setHpBarScale(GameConfig::Monster::Obscur::HP_BAR_SCALE);
    setCurrentHP(_maxHP);
    setCurrentMP(0.0f);
    updateHpBar();

    initStateAnimations();
    initAnimations();
    return true;
}

void ObscurMonster::initAttributes()
{
    namespace Conf = GameConfig::Monster::Obscur;

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
}

void ObscurMonster::initStateAnimations()
{
    ensureSingleFrameAnimationCached("obscur_idle", "Sprites/Enemies/Obscur/Obscur_idle.png");
    ensureSingleFrameAnimationCached("obscur_hurt", "Sprites/Enemies/Obscur/Obscur_beattacked.png");

    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "obscur_idle");
        sm->registerStateAnimation(CharacterState::HURT, "obscur_hurt");
        sm->registerStateAnimation(CharacterState::WALKING, "obscur_idle"); // 无 walk 素材：复用 idle
        sm->registerStateAnimation(CharacterState::STATE_PATROL, "obscur_idle");
    }
}

void ObscurMonster::initAnimations()
{
    CC_SAFE_RELEASE(_meleeAttackAnimate);
    _meleeAttackAnimate = nullptr;

    // 确保近战/远程所需动画已进入 AnimationCache（即便未走 LoadingScene 也能正常播放）
    preloadResources();

    if (auto anim = AnimationCache::getInstance()->getAnimation(OBSCUR_MELEE_ATTACK_ANIM_KEY))
    {
        _meleeAttackAnimate = Animate::create(anim);
        _meleeAttackAnimate->retain();
    }
}

void ObscurMonster::attack()
{
    if (!_target)
    {
        return;
    }

    faceTarget(_target);

    const float distX = horizontalDistanceTo(_target);
    if (distX <= GameConfig::Monster::Obscur::MELEE_TRIGGER_RANGE)
    {
        performMeleeAttack();
        return;
    }

    performRemoteAttack();
}

void ObscurMonster::performMeleeAttack()
{
    // 防御：避免远程残留循环动画叠加
    stopActionByTag(ACTION_TAG_OBSCUR_USEICE_LOOP);

    FiniteTimeAction* animateAction = nullptr;
    if (_meleeAttackAnimate)
    {
        animateAction = static_cast<FiniteTimeAction*>(_meleeAttackAnimate->clone());
    }
    else
    {
        animateAction = DelayTime::create(GameConfig::Monster::Obscur::ATTACK_ANIM_FRAME_DELAY * 4);
    }

    const auto attackAnim = AnimationCache::getInstance()->getAnimation(OBSCUR_MELEE_ATTACK_ANIM_KEY);
    const float frameTime = getFrameTimeForAnimation(attackAnim, GameConfig::Monster::Obscur::ATTACK_ANIM_FRAME_DELAY);
    const float hitStartTime = frameTime * static_cast<float>(std::max(0, GameConfig::Monster::Obscur::MELEE_HIT_START_FRAME - 1));
    const float hitEndTime = frameTime * static_cast<float>(std::max(1, GameConfig::Monster::Obscur::MELEE_HIT_END_FRAME));
    const float hitDuration = std::max(0.0f, hitEndTime - hitStartTime);

    auto logicSequence = Sequence::create(
        DelayTime::create(hitStartTime),
        CallFunc::create([this, hitDuration]()
                         {
                             float direction = (getScaleX() > 0.0f) ? 1.0f : -1.0f;

                             // 按当前缩放同比例调整偏移与尺寸，避免后续调 scale 导致判定错位
                             float scaleRatio = 1.0f;
                             const float tuneScale = GameConfig::Monster::Obscur::SCALE;
                             if (tuneScale > 0.0f)
                             {
                                 scaleRatio = std::fabs(getScaleX()) / tuneScale;
                             }

                             Vec2 offset(GameConfig::Monster::Obscur::MELEE_HITBOX_OFFSET_X * direction * scaleRatio,
                                         GameConfig::Monster::Obscur::MELEE_HITBOX_OFFSET_Y * scaleRatio);
                             Size hitboxSize(GameConfig::Monster::Obscur::MELEE_HITBOX_WIDTH * scaleRatio,
                                             GameConfig::Monster::Obscur::MELEE_HITBOX_HEIGHT * scaleRatio);

                             int damageTag = 1;
                             if (auto attr = getAttributeComponent())
                             {
                                 float strength = attr->getAttributeValue(AttributeType::STRENGTH);
                                 if (strength > 0.0f)
                                 {
                                     damageTag = static_cast<int>(std::round(strength));
                                 }
                             }

                             spawnMeleeHitbox(offset, hitboxSize, std::max(1, damageTag), hitDuration); }),
        nullptr);

    auto spawn = Spawn::create(animateAction, logicSequence, nullptr);
    auto finalSequence = Sequence::create(
        spawn,
        CallFunc::create([this]()
                         {
                             if (auto sm = getStateMachineComponent())
                             {
                                 if (sm->getCurrentState() != CharacterState::DEAD)
                                 {
                                     sm->changeState(CharacterState::IDLE);
                                 }
                             } }),
        nullptr);
    runAction(finalSequence);
}

void ObscurMonster::performRemoteAttack()
{
    stopActionByTag(ACTION_TAG_OBSCUR_USEICE_LOOP);

    if (!_target)
    {
        return;
    }

    auto parent = getParent();
    if (!parent)
    {
        return;
    }

    const Vec2 targetBottomCenterPos = getTargetBottomCenterPosInParentSpace();

    // 1) 自身循环播放 useice
    if (auto useIceAnim = AnimationCache::getInstance()->getAnimation(OBSCUR_USEICE_LOOP_ANIM_KEY))
    {
        auto loop = RepeatForever::create(Animate::create(useIceAnim));
        loop->setTag(ACTION_TAG_OBSCUR_USEICE_LOOP);
        runAction(loop);
    }

    // 2) 在锁定位置生成冰动画（一次性播放）
    auto iceFrame1 = SpriteFrameCacheHelper::getOrCreateSpriteFrame("Sprites/Enemies/Obscur/Obscur_ice_1.png");
    if (iceFrame1)
    {
        auto iceSprite = Sprite::createWithSpriteFrame(iceFrame1);
        iceSprite->setAnchorPoint(Vec2(0.5f, 0.0f));
        iceSprite->setPosition(targetBottomCenterPos);
        iceSprite->setScale(std::fabs(getScaleX()));
        parent->addChild(iceSprite, 6);

        if (auto iceAnim = AnimationCache::getInstance()->getAnimation(OBSCUR_ICE_ANIM_KEY))
        {
            auto animate = Animate::create(iceAnim);
            iceSprite->runAction(Sequence::create(animate, RemoveSelf::create(), nullptr));
        }
        else
        {
            // 兜底：如果动画缺失，仍然展示一小段时间
            iceSprite->runAction(Sequence::create(DelayTime::create(0.35f), RemoveSelf::create(), nullptr));
        }
    }

    // 3) 命中判定：冰动画第 2 帧开始到第 3 帧结束
    const auto iceAnim = AnimationCache::getInstance()->getAnimation(OBSCUR_ICE_ANIM_KEY);
    const float iceFrameTime = getFrameTimeForAnimation(iceAnim, GameConfig::Monster::Obscur::ICE_ANIM_FRAME_DELAY);
    const float hitStartTime = iceFrameTime * static_cast<float>(std::max(0, GameConfig::Monster::Obscur::REMOTE_HIT_START_FRAME - 1));
    const float hitEndTime = iceFrameTime * static_cast<float>(std::max(1, GameConfig::Monster::Obscur::REMOTE_HIT_END_FRAME));
    const float hitDuration = std::max(0.0f, hitEndTime - hitStartTime);

    int damageTag = 1;
    if (auto attr = getAttributeComponent())
    {
        float strength = attr->getAttributeValue(AttributeType::STRENGTH);
        if (strength > 0.0f)
        {
            damageTag = static_cast<int>(std::round(strength));
        }
    }

    const Size hitboxSize(GameConfig::Monster::Obscur::REMOTE_HITBOX_WIDTH,
                          GameConfig::Monster::Obscur::REMOTE_HITBOX_HEIGHT);
    const Vec2 hitboxCenter = targetBottomCenterPos + Vec2(0.0f, hitboxSize.height * 0.5f);

    const float iceDuration = iceAnim ? iceAnim->getDuration() : 0.5f;
    const float finishDelay = std::max(0.0f, iceDuration - hitStartTime);

    auto remoteSequence = Sequence::create(
        DelayTime::create(hitStartTime),
        CallFunc::create([this, hitboxCenter, hitboxSize, damageTag, hitDuration]()
                         { this->spawnAttackHitboxAt(hitboxCenter, hitboxSize, std::max(1, damageTag), hitDuration, 999); }),
        // 远程攻击总时长：对齐冰动画时长，结束后恢复状态并停止 useice 循环
        DelayTime::create(finishDelay),
        CallFunc::create([this]()
                         {
                             stopActionByTag(ACTION_TAG_OBSCUR_USEICE_LOOP);

                             if (auto sm = getStateMachineComponent())
                             {
                                 if (sm->getCurrentState() != CharacterState::DEAD)
                                 {
                                     sm->changeState(CharacterState::IDLE);
                                 }
                             } }),
        nullptr);
    runAction(remoteSequence);
}

Vec2 ObscurMonster::getTargetBottomCenterPosInParentSpace() const
{
    if (!_target)
    {
        return getPosition();
    }

    Vec2 pos = getPositionInParentSpace(_target);
    const auto anchor = _target->getAnchorPoint();
    const float scaledWidth = _target->getContentSize().width * std::fabs(_target->getScaleX());
    const float scaledHeight = _target->getContentSize().height * std::fabs(_target->getScaleY());

    // 转换为“脚下居中”
    pos.x += (0.5f - anchor.x) * scaledWidth;
    pos.y += (0.0f - anchor.y) * scaledHeight;
    return pos;
}
