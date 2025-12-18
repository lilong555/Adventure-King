#include "GoblinMonster.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

namespace
{
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
}

GoblinMonster::GoblinMonster()
{
}

GoblinMonster::~GoblinMonster()
{
    CC_SAFE_RELEASE(_attackAnimate);
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
    setAIConfig(700, 0, true);

    // === 设置怪物属性 ===
    initAttributes();

    // === 刷新 HP / MP ===
    setCurrentHP(_maxHP);
    _currentMP = 0;
    updateHpBar();

    // === 注册状态动画（需要动画名已加入 AnimationCache）===
    initStateAnimations();

    initAnimations();
    return true;
}

#pragma region 属性初始化
void GoblinMonster::initAttributes()
{

    Attributes base;
    base.set(AttributeType::STRENGTH, 10.0f);
    base.set(AttributeType::DEFENSE, 2.0f);
    base.set(AttributeType::MOVE_SPEED, 200.0f); // 基础移速
    base.set(AttributeType::CRITICAL_RATE, 0.05f);
    base.set(AttributeType::MAX_HP, 1000.0f);
    base.set(AttributeType::ATTACKINTERVAL, 2.0f); // 基础攻速
    // 怪物默认缩放见 MonsterBase::init，攻击判定框按缩放同比例缩放，
    // 因此攻击距离也需要匹配当前缩放，避免“看起来够不着却开始攻击”。
    base.set(AttributeType::ATTACK_RANGE, 150.0f); // 发动攻击的距离

    // 2. 交给基类处理 (一行代码搞定逻辑！)
    setupCharacterStats(base);
    // 3. 初始化当前血量 (逻辑上这应该属于 HealthComponent，但写在这也没问题)
    // auto hp = getComponent<HealthComponent>();
    // if(hp) hp->setHP(60.0f);

    CCLOG("Goblin Attributes Set. Speed: %.0f", _moveSpeed);
}
#pragma endregion

#pragma region 状态动画
void GoblinMonster::initStateAnimations()
{
    // 受击/待机素材为单帧：用 AnimationCache 驱动 StateMachineComponent 统一播放
    ensureSingleFrameAnimationCached("goblin_idle", "Sprites/Enemies/Goblin/Goblin_idle.png");
    ensureSingleFrameAnimationCached("goblin_hurt", "Sprites/Enemies/Goblin/Goblin_beattacked.png");

    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "goblin_idle");
        sm->registerStateAnimation(CharacterState::HURT, "goblin_hurt");
    }
}
#pragma endregion
// GoblinMonster.cpp
#pragma region 攻击动画初始化
void GoblinMonster::initAnimations()
{
    cocos2d::Vector<cocos2d::SpriteFrame *> frames;
    char str[200] = {0};

    // 按序加载攻击帧：从 1 开始，遇到缺失文件就停止（避免刷屏报错）
    auto fileUtils = cocos2d::FileUtils::getInstance();
    bool oldPopupNotify = fileUtils->isPopupNotify();
    fileUtils->setPopupNotify(false);

    for (int i = 1; i <= 20; i++)
    {
        // 1. 拼接路径
        // 你的资源根目录是 Resources，所以路径从 Sprites/... 开始
        std::sprintf(str, "Sprites/Enemies/Goblin/Goblin_attack_%d.png", i);

        // 2. 优先从 SpriteFrameCache 获取；缺失时按文件加载并加入缓存
        auto frame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(str);
        if (!frame)
        {
            break;
        }

        frames.pushBack(frame);
    }

    fileUtils->setPopupNotify(oldPopupNotify);

    if (frames.empty())
    {
        CCLOG("ERROR: Goblin attack frames not found under Sprites/Enemies/Goblin/");
        return;
    }

    // 4. 创建动画
    auto animation = cocos2d::Animation::createWithSpriteFrames(frames, 0.1f);
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
    float hitTime = 0.4f; // 默认值
    if (_attackAnimate)
    {
        int frameCount = _attackAnimate->getAnimation()->getFrames().size();
        if (frameCount > 0)
        {
            float frameTime = _attackAnimate->getDuration() / frameCount;
            float hitFrame = 3; // 第4帧 (索引3)
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

                                      // 缩放适配：攻击判定最初按 0.6 的缩放手感调过，这里按当前缩放同比例缩放偏移/尺寸
                                      constexpr float kGoblinHitboxTuneScale = 0.6f;
                                      float scaleRatio = 1.0f;
                                      if (kGoblinHitboxTuneScale > 0.0f)
                                      {
                                          scaleRatio = std::fabs(this->getScaleX()) / kGoblinHitboxTuneScale;
                                      }

                                      // 计算偏移
                                      cocos2d::Vec2 offset(100.f * direction * scaleRatio, 170.f * scaleRatio);
                                      cocos2d::Size hitboxSize(300.0f * scaleRatio, 20.0f * scaleRatio);

                                      int damageTag = 1;
                                      if (auto attr = getAttributeComponent())
                                      {
                                          float strength = attr->getAttributeValue(AttributeType::STRENGTH);
                                          if (strength > 0.0f)
                                          {
                                              damageTag = static_cast<int>(std::round(strength));
                                          }
                                      }

                                      spawnMeleeHitbox(offset, hitboxSize, std::max(1, damageTag), 0.1f);

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
