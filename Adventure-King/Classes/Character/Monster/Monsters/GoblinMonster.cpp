#include "GoblinMonster.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include "Configs/GameConfig.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

namespace
{
    const char* const GOBLIN_ATTACK_ANIMATION_KEY = "goblin_attack";

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

        cocos2d::Vector<cocos2d::SpriteFrame *> frames;
        frames.pushBack(frame);
        auto anim = cocos2d::Animation::createWithSpriteFrames(frames, delayPerUnit);
        cache->addAnimation(anim, animationKey);
    }

    // 辅助函数实现
    void ensureLoopAnimationCached(const std::string& key, const std::string& formatStr, int frameCount, float delay)
    {
        auto cache = cocos2d::AnimationCache::getInstance();
        if (cache->getAnimation(key)) return; // 如果已经有了就不重复加载

        cocos2d::Vector<cocos2d::SpriteFrame*> frames;
        for (int i = 1; i <= frameCount; i++)
        {
            // 拼接文件名，例如 "Sprites/Enemies/Goblin/Goblin_run_1.png"
            std::string path = cocos2d::StringUtils::format(formatStr.c_str(), i);

            auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(path);
            if (frame)
            {
                frames.pushBack(frame);
            }
            else
            {
                CCLOG("Error: Frame not found: %s", path.c_str());
            }
        }

        if (!frames.empty())
        {
            auto anim = cocos2d::Animation::createWithSpriteFrames(frames, delay);
            cache->addAnimation(anim, key);
        }
    }

    void ensureGoblinAttackAnimationCached()
    {
        auto cache = cocos2d::AnimationCache::getInstance();
        if (cache->getAnimation(GOBLIN_ATTACK_ANIMATION_KEY))
        {
            return;
        }

        cocos2d::Vector<cocos2d::SpriteFrame*> frames;

        // 按序加载攻击帧：从 1 开始，遇到缺失文件就停止（避免刷屏报错）
        auto fileUtils = cocos2d::FileUtils::getInstance();
        bool oldPopupNotify = fileUtils->isPopupNotify();
        fileUtils->setPopupNotify(false);

        for (int i = 1; i <= 20; i++)
        {
            std::string path = cocos2d::StringUtils::format("Sprites/Enemies/Goblin/Goblin_attack_%d.png", i);
            auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(path);
            if (!frame)
            {
                break;
            }
            frames.pushBack(frame);
        }

        fileUtils->setPopupNotify(oldPopupNotify);

        if (frames.empty())
        {
            return;
        }

        auto animation = cocos2d::Animation::createWithSpriteFrames(
            frames, GameConfig::Monster::Goblin::ATTACK_ANIM_FRAME_DELAY);
        cache->addAnimation(animation, GOBLIN_ATTACK_ANIMATION_KEY);
    }
}

GoblinMonster::GoblinMonster()
{
}

GoblinMonster::~GoblinMonster()
{
    CC_SAFE_RELEASE(_attackAnimate);
}

int GoblinMonster::getExpReward(int playerLevel) const
{
    if (playerLevel < 1)
    {
        playerLevel = 1;
    }

    return GameConfig::Monster::Goblin::EXP_REWARD_BASE +
           (playerLevel - 1) * GameConfig::Monster::Goblin::EXP_REWARD_PER_LEVEL;
}

void GoblinMonster::applyHpScalingForPlayerLevel(int playerLevel)
{
    namespace Conf = GameConfig::Monster::Goblin;
    int level = std::max(0, playerLevel);

    auto attr = getAttributeComponent();
    float baseHp = attr->getAttributeValue(AttributeType::MAX_HP); // 这里读取到的就是 initAttributes 设置的值
    float finalHp = baseHp * (Conf::HP_SCALE_BASE + level * Conf::HP_SCALE_PER_LEVEL+(level / 10) * Conf::HP_SCALE_PER_10_LEVEL);
    if (!attr)return;
    // 2. 将计算结果设为基础属性，覆盖 initAttributes 中的初始值
    attr->setBaseAttribute(AttributeType::MAX_HP, static_cast<float>(finalHp));

    refreshCacheAttributes();
    setHpBarScale(GameConfig::Monster::Goblin::HP_BAR_SCALE);
    ensureHpBar();
    setCurrentHP(_maxHP);
    updateHpBar();
}

GoblinMonster *GoblinMonster::create(const std::string &spriteFrameName)
{
    GoblinMonster *ret = new (std::nothrow) GoblinMonster();
    if (ret && ret->init(spriteFrameName))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GoblinMonster::init(const std::string &spriteFrameName)
{
    // === 继承 MonsterBase 的初始化（加载纹理）===
    if (!MonsterBase::init(spriteFrameName))
        return false;

    // 2. 初始化 AI 参数 (关卡策划关注这一块)
    setAIConfig(GameConfig::Monster::Goblin::VISION_RANGE,
                GameConfig::Monster::Goblin::CHASE_RANGE,
                GameConfig::Monster::Goblin::PATROL_ENABLED);

    // === 设置怪物属性 ===
    initAttributes();

    // 哥布林血条放大一倍
    setHpBarScale(GameConfig::Monster::Goblin::HP_BAR_SCALE);

    // === 刷新 HP / MP ===
    setCurrentHP(_maxHP);
    _currentMP = 0;
    updateHpBar();

    // === 注册状态动画（需要动画名已加入 AnimationCache）===
    initStateAnimations();

    initAnimations();
    return true;
}

void GoblinMonster::preloadResources()
{
    // 预缓存状态动画（AnimationCache），并预加载攻击动画帧，避免首次生成卡顿
    ensureSingleFrameAnimationCached("goblin_idle", "Sprites/Enemies/Goblin/Goblin_idle.png");
    ensureSingleFrameAnimationCached("goblin_hurt", "Sprites/Enemies/Goblin/Goblin_beattacked.png");
    ensureLoopAnimationCached(
        "goblin_walk",
        "Sprites/Enemies/Goblin/Goblin_walk_%d.png",
        4,
        GameConfig::Monster::Goblin::WALK_ANIM_FRAME_DELAY);
    ensureGoblinAttackAnimationCached();
}

#pragma region 属性初始化
void GoblinMonster::initAttributes()
{
    // 使用命名空间别名，让下面的代码写起来短一点，不用写 GameConfig::Monster::Goblin::...
    namespace Conf = GameConfig::Monster::Goblin;

    Attributes base;

    // 从配置读取数值
    base.set(AttributeType::STRENGTH, Conf::STRENGTH);
    base.set(AttributeType::DEFENSE, Conf::DEFENSE);
    base.set(AttributeType::MOVE_SPEED, Conf::MOVE_SPEED);
    base.set(AttributeType::CRITICAL_RATE, Conf::CRITICAL_RATE);
    base.set(AttributeType::MAX_HP, Conf::MAX_HP);
    // 如果你有 MP 属性，最好也设置一下，防止为 0 导致某些逻辑除零错误
    base.set(AttributeType::MAX_MP, Conf::MAX_MP);

    // 战斗属性
    base.set(AttributeType::ATTACKINTERVAL, Conf::ATTACK_INTERVAL);
    base.set(AttributeType::ATTACK_RANGE, Conf::ATTACK_RANGE);

    // 2. 交给基类处理
    setupCharacterStats(base);

    // 3. 这里的 Log 也可以直接读配置里的值，或者读成员变量 _moveSpeed
    CCLOG("Goblin Attributes Set. Speed: %.0f, HP: %.0f", Conf::MOVE_SPEED, Conf::MAX_HP);
}
#pragma endregion

#pragma region 状态动画
void GoblinMonster::initStateAnimations()
{
    // 受击/待机素材为单帧：用 AnimationCache 驱动 StateMachineComponent 统一播放
    ensureSingleFrameAnimationCached("goblin_idle", "Sprites/Enemies/Goblin/Goblin_idle.png");
    ensureSingleFrameAnimationCached("goblin_hurt", "Sprites/Enemies/Goblin/Goblin_beattacked.png");
    ensureLoopAnimationCached(
        "goblin_walk",                                      // 缓存Key
        "Sprites/Enemies/Goblin/Goblin_walk_%d.png",         // 路径格式化字符串
        4,                                                  // 帧数
        GameConfig::Monster::Goblin::WALK_ANIM_FRAME_DELAY   // 帧间隔时间(越小跑得越快)
    );
    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "goblin_idle");
        sm->registerStateAnimation(CharacterState::HURT, "goblin_hurt");
        sm->registerStateAnimation(CharacterState::WALKING, "goblin_walk");
    }
}
#pragma endregion
// GoblinMonster.cpp
#pragma region 攻击动画初始化
void GoblinMonster::initAnimations()
{
    CC_SAFE_RELEASE(_attackAnimate);
    _attackAnimate = nullptr;

    ensureGoblinAttackAnimationCached();
    auto animation = cocos2d::AnimationCache::getInstance()->getAnimation(GOBLIN_ATTACK_ANIMATION_KEY);
    if (!animation)
    {
        CCLOG("ERROR: Goblin attack animation cache missing");
        return;
    }

    _attackAnimate = cocos2d::Animate::create(animation);
    _attackAnimate->retain();
}
#pragma endregion

#pragma region Attack

// GoblinMonster.cpp

void GoblinMonster::attack()
{
    CCLOG("Goblin Attack Triggered!");

    // -----------------------------------------------------------
    // 1. 准备动画动作
    // -----------------------------------------------------------
    // ★ 安全保险：如果没有动画，创建一个 1秒的延时动作代替
    // 否则如果 _attackAnimate 为空，函数直接 return，状态机永远卡在 ATTACKING
    cocos2d::FiniteTimeAction *animateAction = nullptr;

    if (_attackAnimate)
    {
        animateAction = _attackAnimate->clone();
    }
    else
    {
        animateAction = cocos2d::DelayTime::create(1.0f); // 假动作
    }

    // -----------------------------------------------------------
    // 2. 计算出刀时间
    // -----------------------------------------------------------
    float hitTime = GameConfig::Monster::Goblin::ATTACK_HIT_FALLBACK_TIME;
    if (_attackAnimate)
    {
        int frameCount = _attackAnimate->getAnimation()->getFrames().size();
        if (frameCount > 0)
        {
            float frameTime = _attackAnimate->getDuration() / frameCount;
            float hitFrame = static_cast<float>(GameConfig::Monster::Goblin::ATTACK_HIT_FRAME_INDEX);
            hitTime = frameTime * hitFrame;
        }
    }

    // -----------------------------------------------------------
    // 3. 判定框逻辑 (Hitbox)
    // -----------------------------------------------------------
    auto logicSequence = cocos2d::Sequence::create(
        cocos2d::DelayTime::create(hitTime),
        cocos2d::CallFunc::create([this]()
                                  {
                                      // 计算朝向
                                      float direction = (this->getScaleX() > 0) ? 1.0f : -1.0f;

                                      // 缩放适配：攻击判定最初按配置缩放手感调过，这里按当前缩放同比例缩放偏移/尺寸
                                      constexpr float kGoblinHitboxTuneScale = GameConfig::Monster::Goblin::HITBOX_TUNE_SCALE;
                                      float scaleRatio = 1.0f;
                                      if (kGoblinHitboxTuneScale > 0.0f)
                                      {
                                          scaleRatio = std::fabs(this->getScaleX()) / kGoblinHitboxTuneScale;
                                      }

                                      // 计算偏移
                                      cocos2d::Vec2 offset(GameConfig::Monster::Goblin::HITBOX_OFFSET_X * direction * scaleRatio,
                                                          GameConfig::Monster::Goblin::HITBOX_OFFSET_Y * scaleRatio);
                                      cocos2d::Size hitboxSize(GameConfig::Monster::Goblin::HITBOX_WIDTH * scaleRatio,
                                                               GameConfig::Monster::Goblin::HITBOX_HEIGHT * scaleRatio);

                                      int damageTag = 1;
                                      if (auto attr = getAttributeComponent())
                                      {
                                          float strength = attr->getAttributeValue(AttributeType::STRENGTH);
                                          if (strength > 0.0f)
                                          {
                                              damageTag = static_cast<int>(std::round(strength));
                                          }
                                      }

                                      spawnMeleeHitbox(offset,
                                                       hitboxSize,
                                                       std::max(1, damageTag),
                                                       GameConfig::Monster::Goblin::HITBOX_LIFE_SECONDS);

                                      // 调试日志
                                      // CCLOG("Hitbox spawned at: %.1f, %.1f", worldPos.x, worldPos.y);
                                  }),
        nullptr);

    // -----------------------------------------------------------
    // 4. 组合并运行 (Spawn = 并行)
    // -----------------------------------------------------------
    // 动画 和 逻辑 同时跑
    auto spawn = cocos2d::Spawn::create(animateAction, logicSequence, nullptr);

    // -----------------------------------------------------------
    // 5. 结束回调 (恢复 IDLE)
    // -----------------------------------------------------------
    auto finalSequence = cocos2d::Sequence::create(
        spawn,
        cocos2d::CallFunc::create([this]()
                                  {
            auto sm = getStateMachineComponent();
            if (sm && sm->getCurrentState() != CharacterState::DEAD)
            {
                sm->changeState(CharacterState::IDLE);
            } }),
        nullptr);

    this->runAction(finalSequence);
}

#pragma endregion
