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

    void ensureLoopAnimationCached(const std::string& key,
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
}

GobluMonster::GobluMonster() = default;

GobluMonster::~GobluMonster()
{
    CC_SAFE_RELEASE(_attackAnimateA);
    CC_SAFE_RELEASE(_attackAnimateB);
}

GobluMonster* GobluMonster::create(const std::string& spriteFrameName)
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

bool GobluMonster::init(const std::string& spriteFrameName)
{
    if (!MonsterBase::init(spriteFrameName))
    {
        return false;
    }

    setScale(GameConfig::Monster::Goblu::SCALE);
    _baseScaleX = GameConfig::Monster::Goblu::SCALE;

    setAIConfig(GameConfig::Monster::Goblu::VISION_RANGE,
                GameConfig::Monster::Goblu::CHASE_RANGE,
                GameConfig::Monster::Goblu::PATROL_ENABLED);

    _baseFrameSize = getContentSize();

    initAttributes();
    setHpBarScale(GameConfig::Monster::Goblu::HP_BAR_SCALE);
    setCurrentHP(_maxHP);
    _currentMP = 0;
    updateHpBar();

    initStateAnimations();
    initAnimations();
    return true;
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
    auto buildAttackAnimation = [this](int startIndex, int endIndex) -> Animate*
    {
        Vector<SpriteFrame*> frames;
        for (int i = startIndex; i <= endIndex; ++i)
        {
            std::string path = StringUtils::format("Sprites/Enemies/Goblu/Goblu_attack_%02d.png", i);
            auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrameWithOriginalSize(
                path,
                _baseFrameSize,
                true);
            if (!frame)
            {
                break;
            }
            frames.pushBack(frame);
        }

        if (frames.empty())
        {
            return nullptr;
        }

        auto animation = Animation::createWithSpriteFrames(
            frames,
            GameConfig::Monster::Goblu::ATTACK_ANIM_FRAME_DELAY);
        auto animate = Animate::create(animation);
        animate->retain();
        return animate;
    };

    _attackAnimateA = buildAttackAnimation(1, 4);
    _attackAnimateB = buildAttackAnimation(11, 15);
}

void GobluMonster::attack()
{
    CCLOG("Goblu Attack Triggered!");

    FiniteTimeAction* animateAction = nullptr;
    Animate* sourceAnimate = nullptr;

    if (_attackAnimateA && _attackAnimateB)
    {
        sourceAnimate = (cocos2d::random(0, 1) == 0) ? _attackAnimateA : _attackAnimateB;
    }
    else
    {
        sourceAnimate = _attackAnimateA ? _attackAnimateA : _attackAnimateB;
    }

    if (sourceAnimate)
    {
        animateAction = sourceAnimate->clone();
    }
    else
    {
        animateAction = DelayTime::create(1.0f);
    }

    float hitTime = GameConfig::Monster::Goblu::ATTACK_HIT_FALLBACK_TIME;
    if (sourceAnimate)
    {
        int frameCount = sourceAnimate->getAnimation()->getFrames().size();
        if (frameCount > 0)
        {
            float frameTime = sourceAnimate->getDuration() / frameCount;
            float hitFrame = static_cast<float>(GameConfig::Monster::Goblu::ATTACK_HIT_FRAME_INDEX);
            hitTime = frameTime * hitFrame;
        }
    }

    auto logicSequence = Sequence::create(
        DelayTime::create(hitTime),
        CallFunc::create([this]()
                         {
                             float direction = (getScaleX() > 0.0f) ? 1.0f : -1.0f;

                             constexpr float kGobluHitboxTuneScale = GameConfig::Monster::Goblu::HITBOX_TUNE_SCALE;
                             float scaleRatio = 1.0f;
                             if (kGobluHitboxTuneScale > 0.0f)
                             {
                                 scaleRatio = std::fabs(getScaleX()) / kGobluHitboxTuneScale;
                             }

                             Vec2 offset(GameConfig::Monster::Goblu::HITBOX_OFFSET_X * direction * scaleRatio,
                                         GameConfig::Monster::Goblu::HITBOX_OFFSET_Y * scaleRatio);
                             Size hitboxSize(GameConfig::Monster::Goblu::HITBOX_WIDTH * scaleRatio,
                                             GameConfig::Monster::Goblu::HITBOX_HEIGHT * scaleRatio);

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
                                 GameConfig::Monster::Goblu::HITBOX_LIFE_SECONDS);
                         }),
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
                             }
                         }),
        nullptr);

    runAction(finalSequence);
}
