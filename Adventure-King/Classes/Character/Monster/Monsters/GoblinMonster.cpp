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
	// === 设置哥布林特有参数 ===
    _attackRange = 300.0f;      
    _aggroRadius = 600.0f;     // 仇恨范围
	_leashRadius = 0.0f;       // 不设牵引范围
	// === 设置缩放比例 ===
    setScale(0.5f);
    _baseScaleX = 0.5f;

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

    return true;
}

#pragma region 属性初始化
void GoblinMonster::initAttributes()
{
    // 1. 获取组件
    auto attr = getAttributeComponent();
    if (!attr) return;

    // 2. 准备基础数据 (假设 Attributes 类有一个 set 方法)
    Attributes base;
    base.set(AttributeType::STRENGTH, 10.0f);
    base.set(AttributeType::ATTACKINTERVAL, 1.5f); // 基础攻速
    base.set(AttributeType::DEFENSE, 2.0f);
    base.set(AttributeType::MOVE_SPEED, 200.0f);   // 基础移速
    base.set(AttributeType::CRITICAL_RATE, 0.05f);
    base.set(AttributeType::MAX_HP, 60.0f);

    // 3. 将数据填入组件的“基础层” (_baseAttributes)
    attr->setBaseAttributes(base);

    // 强制计算一次最终属性
    // 这一步会将 _baseAttributes + _equipmentBonus(0) + ... 计算并存入 _finalAttributes
    attr->recalculateFinalAttributes();

    // 5. ★同步缓存★：将组件计算好的最终值，赋值给 MonsterBase 的成员变量
    // 注意：使用的是 getAttributeValue (获取最终值)，而不是 getBaseAttribute
    //哥布林非特有参数
    _moveSpeed = attr->getAttributeValue(AttributeType::MOVE_SPEED);
    _attackInterval = attr->getAttributeValue(AttributeType::ATTACKINTERVAL);
    _maxHP = attr->getAttributeValue(AttributeType::MAX_HP);

    // 如果有血条组件，记得初始化当前血量
    // auto hpComp = getComponent<HealthComponent>();
    // if (hpComp) hpComp->setHP(_maxHP);

    CCLOG("Goblin Init Complete: Speed=%.1f", _moveSpeed);
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

#pragma region AI
//重写ai逻辑
void GoblinMonster::updateAI(float dt)
{
    if (!_target)
    {
        // 假设场景里有一个 tag 为 1 的节点是玩家
        // 或者你可以通过 GameScene::getInstance()->getPlayer() 获取
        // 这里提供一种通用的通过 Tag 寻找的方法：
        if (this->getParent())
        {
            // 假设玩家的 Tag 是 100 (你需要确保在 GameScene 里给玩家设置了这个 Tag)
            auto player = this->getParent()->getChildByTag(100);

            // 或者通过名字查找: 
            // auto player = this->getParent()->getChildByName("PlayerNode");

            if (player)
            {
                float dist = this->getPosition().distance(player->getPosition());
                // 只有玩家进入仇恨范围才锁定他
                if (dist <= _aggroRadius)
                {
                    _target = player;
                    CCLOG("Monster found target!");
                }
            }
        }
    }

    // 如果尝试寻找后依然没有目标，那就真的没事可做了，保持待机
    if (!_target)
    {
        // 确保状态是 IDLE 或 PATROL
        if (_patrolEnabled)
            getStateMachineComponent()->changeState(CharacterState::STATE_PATROL);
        else
            getStateMachineComponent()->changeState(CharacterState::IDLE);
        return;
    }

    float distToPlayer = distanceTo(_target);
    Vec2 homePos = _homePosition;
    float distFromHome = this->getPosition().distance(homePos);

    auto stateMachine = getStateMachineComponent();

    // 1 ➤ 如果死了或硬直，直接返回
    if (isDead())
    {
        stateMachine->changeState(CharacterState::DEAD);
        return;
    }
    if (_isStunned)
    {
        stateMachine->changeState(CharacterState::ATTACKING);
        return;
    }

    // 2 ➤ 超出牵引半径，强制返回出生点
    if (_leashRadius > 0.0f && distFromHome > _leashRadius)
    {
        _currentTargetPos = homePos;   // 移动目标变成“出生点”
        stateMachine->changeState(CharacterState::WALKING);
        return;
    }

    // 3超出仇恨范围 → Idle 或 Patrol
    if (_aggroRadius > 0.0f && distToPlayer > _aggroRadius)
    {
        if (_patrolEnabled)
            stateMachine->changeState(CharacterState::STATE_PATROL);
        else
            stateMachine->changeState(CharacterState::IDLE);
        return;
    }
    // 4攻击逻辑
    // 4.1 ➤ 必须先更新计时器
    _attackTimer += dt;

    // 4.2 攻击判定
    // 条件：距离够近 && 冷却完毕 && 当前没在攻击 && 没在硬直
    bool isAttacking = (stateMachine->getCurrentState() == CharacterState::ATTACKING);

    if (distToPlayer <= _attackRange)
    {
        if (_attackTimer >= _attackInterval && !isAttacking && !_isStunned)
        {
            // 重置计时器
            _attackTimer = 0.0f;
            // 切换状态，stateMachine 内部应该调用 attack()，或者你在 updateAttack 里调用
            stateMachine->changeState(CharacterState::ATTACKING);
            this->attack(); // 手动触发一次攻击逻辑
        }
        return;
    }

    // 5 ➤ 否则 → 追击玩家
    _currentTargetPos = _target->getPosition();
    stateMachine->changeState(CharacterState::WALKING);
}
#pragma endregion

#pragma region Attack

// GoblinMonster.cpp

void GoblinMonster::attack()
{
    // 1. 停止移动 (防止攻击时滑步)
    if (_physicsBody) _physicsBody->setVelocity(cocos2d::Vec2::ZERO);

    // 2. 播放攻击动画 (假设你已经有了 Animate* attackAnim)
    // auto attackAnim = Animate::create(...); 
    // runAction(attackAnim); 

    // ---------------------------------------------------------
    // 3. 核心：延迟生成伤害判定框 (Hitbox)
    // ---------------------------------------------------------
    // 假设攻击动作总长 1.0秒，刀在 0.4秒 时砍中人
    float delayTime = 0.4f;
    float hitboxDuration = 0.1f; // 判定框存在的时间

    auto delay = cocos2d::DelayTime::create(delayTime);

    auto spawnHitbox = cocos2d::CallFunc::create([this]() {
        // 创建一个临时的 Node 作为攻击判定框
        auto attackNode = cocos2d::Node::create();
        // 设置位置（在怪物前方）
        
        attackNode->setPosition(cocos2d::Vec2(700, 300));

        this->addChild(attackNode);

        // 创建物理身体 (Sensor 模式)
        auto body = cocos2d::PhysicsBody::createBox(cocos2d::Size(400, 40));
        body->setDynamic(false); // 静态，不受重力影响
        body->setGravityEnable(false);

        // ★ 设置掩码 (关键)
        // Category: 我是“怪物攻击”
        body->setCategoryBitmask(ToMask(GamePhysicsCategory::MONSTER_ATTACK));
        // Contact: 我想检测“玩家”
        body->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER));
        // Collision: 0 (我不产生物理碰撞反弹，直接穿过去)
        body->setCollisionBitmask(0);

        // 把数据挂载到 body 上，方便碰撞回调里取伤害值
        // (这里有个小技巧：利用 PhysicsBody 的 tag 或者 UserData)
        body->setTag(10); // 假设 10 代表普通攻击伤害

        attackNode->setPhysicsBody(body);

        // 让这个判定框在短时间后自动销毁
        auto removeSeq = cocos2d::Sequence::create(
            cocos2d::DelayTime::create(0.1f),
            cocos2d::RemoveSelf::create(),
            nullptr
        );
        attackNode->runAction(removeSeq);
        });

    // 4. 攻击结束后的回调 (恢复 IDLE 状态)
    auto finishAttack = cocos2d::CallFunc::create([this]() {
        auto sm = getStateMachineComponent();
        if (sm) sm->changeState(CharacterState::IDLE);
        });

    // 执行序列： 延迟 -> 生成判定框 -> (剩下的动画时间) -> 恢复状态
    auto sequence = cocos2d::Sequence::create(delay, spawnHitbox, cocos2d::DelayTime::create(0.5f), finishAttack, nullptr);
    this->runAction(sequence);
}

#pragma endregion
