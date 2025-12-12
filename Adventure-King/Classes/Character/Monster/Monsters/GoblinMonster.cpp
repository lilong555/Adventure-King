#include "GoblinMonster.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"


USING_NS_CC;

GoblinMonster::GoblinMonster()
{
}

GoblinMonster::~GoblinMonster()
{
}

GoblinMonster* GoblinMonster::create(const std::string& spriteFrameName)
{
    GoblinMonster* ret = new (std::nothrow) GoblinMonster();
    if (ret && ret->init(spriteFrameName))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GoblinMonster::init(const std::string& spriteFrameName)
{
    // === 继承 MonsterBase 的初始化（加载纹理）===
    if (!MonsterBase::init(spriteFrameName))
        return false;
    // 1. 初始化战斗属性 (数值策划关注这一块)
    initAttributes();

    // 2. 初始化 AI 参数 (关卡策划关注这一块)
    setAIConfig(700,0,true);
	// === 设置缩放比例 ===
    setScale(0.6f);
    _baseScaleX = 0.6f;

    // === 设置怪物属性 ===
    initAttributes();

    // === 刷新 HP / MP ===
    _currentHP = _maxHP;
    _currentMP = 0;

    // === 注册状态动画（需要动画名已加入 AnimationCache）===
    initStateAnimations();

    // === 设置物理掩码 ===
    if (_physicsBody)
    {
        _physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::MONSTER));
        _physicsBody->setCollisionBitmask(
            ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::PLAYER)
        );
        _physicsBody->setContactTestBitmask(
            ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::PLAYER_ATTACK)
        );
    }
    initAnimations();
    return true;
}

#pragma region 属性初始化
void GoblinMonster::initAttributes()
{

    Attributes base;
    base.set(AttributeType::STRENGTH, 10.0f);
    base.set(AttributeType::DEFENSE, 2.0f);
    base.set(AttributeType::MOVE_SPEED, 200.0f);   // 基础移速
    base.set(AttributeType::CRITICAL_RATE, 0.05f);
    base.set(AttributeType::MAX_HP, 60.0f);
    base.set(AttributeType::ATTACKINTERVAL, 2.0f); // 基础攻速
    base.set(AttributeType::ATTACK_RANGE, 450.0f);

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
    if (auto sm = getStateMachineComponent())
    {
        sm->registerStateAnimation(CharacterState::IDLE, "goblin_idle");
        sm->registerStateAnimation(CharacterState::WALKING, "goblin_walk");
        sm->registerStateAnimation(CharacterState::ATTACKING, "goblin_attack");
        sm->registerStateAnimation(CharacterState::HURT, "goblin_hurt");
        sm->registerStateAnimation(CharacterState::DEAD, "goblin_dead");
    }
}
#pragma endregion
// GoblinMonster.cpp
#pragma region 攻击动画初始化
void GoblinMonster::initAnimations()
{
    // 1. 获取精灵帧缓存
    auto cache = cocos2d::SpriteFrameCache::getInstance();

    // 如果还没加载过 plist，先加载 (假设叫 goblin.plist)
    // cache->addSpriteFramesWithFile("sprites/goblin.plist");

    // 2. 创建动画帧数组
    cocos2d::Vector<cocos2d::SpriteFrame*> attackFrames;
    char str[100] = { 0 };

    // 假设你有 6 张攻击图：goblin_attack_01.png 到 goblin_attack_06.png
    for (int i = 1; i <= 6; i++)
    {
        // 格式化文件名
        sprintf(str, "goblin_attack_%2d.png", i);

        // 从缓存中获取帧
        auto frame = cache->getSpriteFrameByName(str);
        if (frame)
        {
            attackFrames.pushBack(frame);
        }
        else
        {
            CCLOG("Error: SpriteFrame %s not found!", str);
        }
    }

    // 3. 创建 Animation 对象
    // 0.1f 是每一帧的间隔时间 (每秒10帧)
    auto animation = cocos2d::Animation::createWithSpriteFrames(attackFrames, 0.1f);

    // 4. 创建 Animate 动作 (这是最终能被 runAction 执行的对象)
    _attackAnimate = cocos2d::Animate::create(animation);
    _attackAnimate->retain(); // ★关键：必须 retain，否则函数结束它就被自动释放了
}

//// 别忘了在析构函数里释放
//GoblinMonster::~GoblinMonster()
//{
//    CC_SAFE_RELEASE(_attackAnimate);
//}
#pragma endregion

#pragma region AI
//重写ai逻辑
void GoblinMonster::updateAI(float dt)
{
    // ============================================================
    // 1. 寻找目标逻辑 (保持不变)
    // ============================================================
    if (!_target)
    {
        if (this->getParent())
        {
            auto player = this->getParent()->getChildByTag(100); // 假设 Tag 100
            if (player)
            {
                float dist = this->getPosition().distance(player->getPosition());
                if (dist <= _aggroRadius)
                {
                    _target = player;
                    CCLOG("Monster found target!");
                }
            }
        }
    }

    if (!_target)
    {
        if (_patrolEnabled)
            getStateMachineComponent()->changeState(CharacterState::STATE_PATROL);
        else
            getStateMachineComponent()->changeState(CharacterState::IDLE);
        return;
    }

    // ============================================================
    // 2. 基础数据计算
    // ============================================================
    float distToPlayer = distanceTo(_target);
    Vec2 homePos = _homePosition;
    float distFromHome = this->getPosition().distance(homePos);
    auto stateMachine = getStateMachineComponent();

    // ============================================================
    // 3. 状态检查
    // ============================================================
    if (isDead())
    {
        stateMachine->changeState(CharacterState::DEAD);
        return;
    }

    // ★ 修正提醒：你之前的代码里这里是 changeState(ATTACKING)，这应该是笔误？
    // 硬直状态通常是 HURT 或 IDLE，不能是 ATTACKING
    if (_isStunned)
    {
        stateMachine->changeState(CharacterState::IDLE);
        return;
    }

    // ============================================================
    // 4. 牵引/回家逻辑
    // ============================================================
    if (_leashRadius > 0.0f && distFromHome > _leashRadius)
    {
        _currentTargetPos = homePos;
        stateMachine->changeState(CharacterState::WALKING);
        return;
    }

    // ============================================================
    // 5. 仇恨丢失逻辑 (增加一点缓冲距离，防抖动)
    // ============================================================
    if (_aggroRadius > 0.0f && distToPlayer > _aggroRadius * 1.2f)
    {
        if (_patrolEnabled)
            stateMachine->changeState(CharacterState::STATE_PATROL);
        else
            stateMachine->changeState(CharacterState::IDLE);
        return;
    }

    // ------------------------------------------------------------
        // 6. 攻击与战斗逻辑
        // ------------------------------------------------------------

    _attackTimer += dt; // 累加时间 (配合方案一：只在非攻击时累加也可以，这里先保持简单)

    // 1. 如果正在攻击：什么都别做，彻底退出
    if (getStateMachineComponent()->getCurrentState() == CharacterState::ATTACKING)
    {
        return;
    }

    // 2. 如果距离够近：进入战斗决策
    if (distToPlayer <= _attackRange)
    {
        // 2.1 冷却完毕 -> 打！
        if (_attackTimer >= _attackInterval && !_isStunned)
        {
            getStateMachineComponent()->changeState(CharacterState::ATTACKING);
            this->attack();
            return; // 绝对不要漏掉这个 return
        }
        // 2.2 冷却没好 -> 盯着看
        else
        {
            faceTarget(_target);
            getStateMachineComponent()->changeState(CharacterState::IDLE);
            return; // 绝对不要漏掉这个 return
        }
    }

    // 3. 如果距离远：追击 (只有上面没 return 才会走到这)
    // =========================================================
    _currentTargetPos = _target->getPosition();
    getStateMachineComponent()->changeState(CharacterState::WALKING);
}
#pragma endregion

#pragma region Attack

// GoblinMonster.cpp

void GoblinMonster::attack()
{
    // 1. 重置计时器 & 停止移动
    _attackTimer = 0.0f;
    if (_physicsBody) _physicsBody->setVelocity(cocos2d::Vec2::ZERO);

    // -----------------------------------------------------------
    // 2. 准备动画动作
    // -----------------------------------------------------------
    if (!_attackAnimate) return; // 安全检查

    // 创建一个新的动作实例 (Animate 不能重复使用同一个实例，需要 clone)
    auto animateAction = _attackAnimate->clone();

    // -----------------------------------------------------------
    // 3. 准备逻辑动作 (生成判定框) - 这是你原来的代码
    // -----------------------------------------------------------
    // 假设第 4 帧出刀 (0.1秒/帧 * 4 = 0.4秒)
    float delayTime = 0.4f;

    auto logicSequence = cocos2d::Sequence::create(
        cocos2d::DelayTime::create(delayTime),
        cocos2d::CallFunc::create([this]() {
            // ... 这里是你原来生成 Hitbox 的代码 (粘贴过来) ...
            // this->addChild(attackNode);
            // ...
            CCLOG("Hitbox Spawned!");
            }),
        nullptr
    );

    // -----------------------------------------------------------
    // 4. 组合并运行 (Spawn = 并行)
    // -----------------------------------------------------------
    // 让动画和逻辑同时跑
    auto spawn = cocos2d::Spawn::create(animateAction, logicSequence, nullptr);

    // 5. 动作结束后切回 IDLE
    auto finalSequence = cocos2d::Sequence::create(
        spawn,
        cocos2d::CallFunc::create([this]() {
            auto sm = getStateMachineComponent();
            // 如果还没死，切回 IDLE
            if (sm && sm->getCurrentState() != CharacterState::DEAD) {
                sm->changeState(CharacterState::IDLE);
                // 此时可以恢复默认站立图
                // this->setSpriteFrame("goblin_idle_01.png"); 
            }
            }),
        nullptr
    );

    this->runAction(finalSequence);
}

#pragma endregion
