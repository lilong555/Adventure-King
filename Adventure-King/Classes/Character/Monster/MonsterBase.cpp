#include "MonsterBase.h"
#include "cocos2d.h"
#include <algorithm>
#include <cmath>
USING_NS_CC;

MonsterBase::MonsterBase()
{
}

MonsterBase::~MonsterBase()
{
}

bool MonsterBase::init(const std::string& spriteFrameName)
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

    // 默认缩放：配合地图比例（统一怪物体型）
    setScale(0.36f);
    _baseScaleX = 0.36f;

    // 默认锚点：底部对齐（更符合横版地面站立表现）
    setAnchorPoint(Vec2(0.5f, 0.0f));

    // 怪物默认开启受击飘字
    setDamageNumbersEnabled(true);

    // === 创建怪物碰撞体 ===
    Size size = getContentSize();
    Size boxSize(size.width * 0.35f, size.height * 0.9f);

    PhysicsMaterial material(1.0f, 0.0f, 0.0f);

    _physicsBody = PhysicsBody::createBox(boxSize, material);
    _physicsBody->setDynamic(true);
    _physicsBody->setRotationEnable(false);
    _physicsBody->setGravityEnable(true);

    // 默认物理掩码：怪物本体与平台/玩家/玩家攻击产生碰撞；同时需要接收 Contact 回调用于伤害结算
    _physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::MONSTER));
    _physicsBody->setCollisionBitmask(
        ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::PLAYER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB | GamePhysicsCategory::COLLISION));
    _physicsBody->setContactTestBitmask(
        ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB));

    addComponent(_physicsBody);
    return true;
}
// MonsterBase.cpp
#pragma region 属性初始化
void MonsterBase::setupCharacterStats(const Attributes& stats)
{
    auto attr = getAttributeComponent();
    if (!attr)
    {
        CCLOG("Error: AttributeComponent not found on Monster!");
        return;
    }

    // 1. 设置基础属性 (这一步是通用的)
    attr->setBaseAttributes(stats);

    // 2. 强制计算 (这一步是通用的)
    attr->recalculateFinalAttributes();

    // 3. 同步到成员变量 (这一步绝对是通用的！)
    // 这样你就不用在每个怪物里都写一遍 _moveSpeed = ... 了
    refreshCacheAttributes();

    ensureHpBar();
    updateHpBar();
}

void MonsterBase::refreshCacheAttributes()
{
    auto attr = getAttributeComponent();
    if (!attr) return;

    attr->recalculateFinalAttributes(); // 确保数据最新

    // 从组件拉取数据到成员变量
    _moveSpeed = attr->getAttributeValue(AttributeType::MOVE_SPEED);
    _attackInterval = attr->getAttributeValue(AttributeType::ATTACKINTERVAL);
    _attackRange = attr->getAttributeValue(AttributeType::ATTACK_RANGE);
    _maxHP = attr->getAttributeValue(AttributeType::MAX_HP);

    // 如果有防御力等其他属性，以后只改这里一个地方就行了
}
#pragma endregion
// MonsterBase.cpp -> update

void MonsterBase::update(float dt)
{
    CharacterBase::update(dt);
    updateAI(dt);

    auto sm = getStateMachineComponent();
    if (!sm)
        return;

    auto state = sm->getCurrentState();

    if (state == CharacterState::WALKING || state == CharacterState::STATE_PATROL)
    {
        updateMovement(dt); // 移动函数里通常包含了 faceTarget
    }
    else if (state == CharacterState::IDLE)
    {
        if (_physicsBody)
        {
            // 1. 获取当前速度 (包含重力产生的向下速度)
            cocos2d::Vec2 v = _physicsBody->getVelocity();

            // 2. 只把 X 轴归零 (停止左右走)
            v.x = 0;

            // 3. 重新设置回去 (Y 轴保持不变，怪就能掉下去了)
            _physicsBody->setVelocity(v);
        }

        if (_target)
        {
            faceTarget(_target);
        }
    }

    // 攻击状态下 (ATTACKING) 不调用 faceTarget，也就是“锁死方向”
    updateAttack(dt);
}

#pragma region AI
//设置索敌、追击、巡逻
void MonsterBase::setAIConfig(float AR, float LR, bool PTL) {
    _aggroRadius = AR;  // 仇恨范围 (必须 > AttackRange)
    _leashRadius = LR;    // 牵引范围 (0 代表死追到底，不回家)

    // --- 巡逻设置 ---
    _patrolEnabled = PTL;  // 是否巡逻
}
void MonsterBase::updateAI(float dt)
{
    CC_UNUSED_PARAM(dt);

    auto sm = getStateMachineComponent();
    if (!sm)
        return;

    if (isDead())
    {
        sm->changeState(CharacterState::DEAD);
        return;
    }

    if (_isStunned)
    {
        sm->changeState(CharacterState::IDLE);
        _hasMoveGoal = false;
        return;
    }

    // 没有目标：如果有移动目标（回家/巡逻），继续走；否则待机
    if (!_target)
    {
        if (_hasMoveGoal)
        {
            sm->changeState(CharacterState::WALKING);
        }
        else if (_patrolEnabled && std::fabs(_patrolRight.x - _patrolLeft.x) > 1.0f)
        {
            // 简单巡逻：在左右边界之间往返
            const float kPatrolReachEpsilon = 8.0f;
            float dx = (_patrolDir > 0 ? _patrolRight.x : _patrolLeft.x) - getPositionX();
            if (std::fabs(dx) <= kPatrolReachEpsilon)
            {
                _patrolDir *= -1;
            }

            _moveGoalPos = (_patrolDir > 0) ? _patrolRight : _patrolLeft;
            _hasMoveGoal = true;
            sm->changeState(CharacterState::STATE_PATROL);
        }
        else
        {
            sm->changeState(CharacterState::IDLE);
        }
        return;
    }

    const float distToTarget = distanceTo(_target);

    // 超出仇恨范围（带一点缓冲，防止边界抖动）-> 丢失目标
    if (_aggroRadius > 0.0f && distToTarget > _aggroRadius * 1.2f)
    {
        _target = nullptr;
        _hasMoveGoal = false;
        const bool canPatrol = _patrolEnabled && std::fabs(_patrolRight.x - _patrolLeft.x) > 1.0f;
        sm->changeState(canPatrol ? CharacterState::STATE_PATROL : CharacterState::IDLE);
        return;
    }

    // 牵引/回家逻辑
    if (_leashRadius > 0.0f && _hasHome)
    {
        const float distFromHome = getPosition().distance(_homePos);
        if (distFromHome > _leashRadius)
        {
            _target = nullptr;
            _moveGoalPos = _homePos;
            _hasMoveGoal = true;
            sm->changeState(CharacterState::WALKING);
            return;
        }
    }

    // 进入攻击距离：停下并面向目标，攻击由 updateAttack 触发
    const float horizontalDist = horizontalDistanceTo(_target);
    if (horizontalDist <= _attackRange)
    {
        _hasMoveGoal = false;
        sm->changeState(CharacterState::IDLE);
        return;
    }

    // 追击
    _moveGoalPos = getPositionInParentSpace(_target);
    _hasMoveGoal = true;
    sm->changeState(CharacterState::WALKING);
}

#pragma endregion


#pragma region 移动
void MonsterBase::updateMovement(float dt)
{
    CC_UNUSED_PARAM(dt);
    if (!_physicsBody) return;

    float moveSpeed = 0.0f;
    if (auto attr = getAttributeComponent())
    {
        moveSpeed = attr->getAttributeValue(AttributeType::MOVE_SPEED);
    }

    // 没有目标且没有移动目标 → 不移动
    if (!_target && !_hasMoveGoal)
    {
        const float currentVy = _physicsBody->getVelocity().y;
        _physicsBody->setVelocity(Vec2(0, currentVy));
        return;
    }

    // 进入攻击距离 -> 停止水平移动（垂直速度保留）
    if (_target && horizontalDistanceTo(_target) <= _attackRange)
    {
        // 停止移动时，也要保留 Y 轴速度（防止怪物在空中攻击时突然定住不掉下来）
        float currentVy = _physicsBody->getVelocity().y;
        _physicsBody->setVelocity(Vec2(0, currentVy));
        return;
    }

    // 1. 获取当前物理引擎计算出的 Y 轴速度 (包含重力影响)
    float currentVy = _physicsBody->getVelocity().y;

    // 2. 只计算水平方向 (X轴) 的向量
    Vec2 targetPos = _target ? getPositionInParentSpace(_target) : _moveGoalPos;
    Vec2 myPos = getPosition();

    // 当水平距离非常接近时，由于物理抖动可能导致方向频繁反转
    const float kChaseDeadzoneX = 8.0f;
    float dx = targetPos.x - myPos.x;
    if (fabs(dx) <= kChaseDeadzoneX)
    {
        _physicsBody->setVelocity(Vec2(0, currentVy));

        // 到达移动目标（回家/巡逻）后清空目标
        if (!_target && _hasMoveGoal)
        {
            _hasMoveGoal = false;
        }
        return;
    }

    // 判断在左边还是右边
    float dirX = (dx > 0.0f) ? 1.0f : -1.0f;

    // 3. 组合新速度：
    // X轴 = 我们想要的移动速度
    // Y轴 = 物理引擎原本的速度 (让重力继续拉着它)
    _physicsBody->setVelocity(Vec2(dirX * moveSpeed, currentVy));

    if (_target)
    {
        faceTarget(_target);
    }
    else
    {
        float targetWorldX = targetPos.x;
        if (auto parent = getParent())
        {
            targetWorldX = parent->convertToWorldSpace(targetPos).x;
        }
        faceToX(targetWorldX);
    }
}
#pragma endregion


#pragma region 攻击

void MonsterBase::updateAttack(float dt)
{
    if (isDead() || _isStunned)
        return;

    auto sm = getStateMachineComponent();
    if (!sm)
        return;

    // 正在攻击：保持朝向不变
    if (sm->getCurrentState() == CharacterState::ATTACKING)
        return;

    _attackTimer += dt;

    if (!_target)
        return;

    if (horizontalDistanceTo(_target) > _attackRange)
        return;

    if (_attackTimer < _attackInterval)
        return;

    // 准备攻击：停止水平速度（保留重力速度），锁定朝向
    if (_physicsBody)
    {
        cocos2d::Vec2 v = _physicsBody->getVelocity();
        v.x = 0;
        _physicsBody->setVelocity(v);
    }

    _attackTimer = 0.0f;
    sm->changeState(CharacterState::ATTACKING);
    attack();
}

#pragma endregion


#pragma region 基础战斗事件

void MonsterBase::attack()
{
    // 默认普通攻击：使用 SkillComponent 简化为使用技能槽 0
    if (auto skill = getSkillComponent())
        skill->useActiveSkill(0);
}

void MonsterBase::takeDamage(const DamageInfo& info)
{
    float dmg = info.amount;

    float hp = getCurrentHP();
    hp -= dmg;

    showDamageNumber(dmg, info.isCritical);

    setCurrentHP(hp);
    updateHpBar();

    if (hp <= 0)
    {
        die();
        return;
    }

    getStateMachineComponent()->changeState(CharacterState::HURT);
}

void MonsterBase::die()
{
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

    stopAllActions();
    CharacterBase::die();
}

#pragma endregion

// ===================================================================
// HP Bar
// ===================================================================

void MonsterBase::ensureHpBar()
{
    if (_hpBar)
        return;

    _hpBar = DrawNode::create();
    if (_hpBar)
    {
        addChild(_hpBar, 10);
    }
}

void MonsterBase::updateHpBar()
{
    if (!_hpBar)
        return;

    _hpBar->clear();

    float maxHp = _maxHP;
    if (auto attr = getAttributeComponent())
    {
        maxHp = attr->getAttributeValue(AttributeType::MAX_HP);
    }
    if (maxHp <= 0.0f)
        return;

    float barWidth = 60.0f;
    float barHeight = 8.0f;
    float yOffset = getContentSize().height + 10.0f;

    Vec2 barPos(getContentSize().width / 2 - barWidth / 2, yOffset);

    // 背景
    _hpBar->drawSolidRect(
        barPos,
        barPos + Vec2(barWidth, barHeight),
        Color4F(0.2f, 0.2f, 0.2f, 1.0f));

    float hpRatio = clampf(getCurrentHP() / maxHp, 0.0f, 1.0f);
    float currentWidth = barWidth * hpRatio;

    Color4F hpColor;
    if (hpRatio > 0.5f)
    {
        hpColor = Color4F(0.2f, 0.8f, 0.2f, 1.0f);
    }
    else if (hpRatio > 0.25f)
    {
        hpColor = Color4F(1.0f, 0.8f, 0.0f, 1.0f);
    }
    else
    {
        hpColor = Color4F(1.0f, 0.2f, 0.2f, 1.0f);
    }

    _hpBar->drawSolidRect(
        barPos,
        barPos + Vec2(currentWidth, barHeight),
        hpColor);

    _hpBar->drawRect(
        barPos,
        barPos + Vec2(barWidth, barHeight),
        Color4F::WHITE);
}


#pragma region 工具函数

Vec2 MonsterBase::getWorldPosition(const Node *node) const
{
    if (!node)
        return Vec2::ZERO;

    auto parent = node->getParent();
    return parent ? parent->convertToWorldSpace(node->getPosition()) : node->getPosition();
}

Vec2 MonsterBase::getPositionInParentSpace(const Node *node) const
{
    Vec2 worldPos = getWorldPosition(node);
    auto parent = getParent();
    return parent ? parent->convertToNodeSpace(worldPos) : worldPos;
}

float MonsterBase::horizontalDistanceTo(const Node *target) const
{
    if (!target)
        return 999999.0f;

    Vec2 myWorldPos = getWorldPosition(this);
    Vec2 targetWorldPos = getWorldPosition(target);
    return std::fabs(targetWorldPos.x - myWorldPos.x);
}

void MonsterBase::faceToX(float targetWorldX)
{
    const float kFaceDeadzoneX = 8.0f;
    float myWorldX = getWorldPosition(this).x;
    float dx = targetWorldX - myWorldX;
    if (std::fabs(dx) <= kFaceDeadzoneX)
        return;

    float sign = (dx < 0.0f) ? -1.0f : 1.0f;
    setScaleX(sign * std::fabs(_baseScaleX));
}

void MonsterBase::spawnMeleeHitbox(const Vec2 &offsetInParentSpace,
                                  const Size &hitboxSize,
                                  int damageTag,
                                  float lifeSeconds)
{
    auto parent = getParent();
    if (!parent)
        return;

    auto attackNode = Node::create();
    attackNode->setPosition(getPosition() + offsetInParentSpace);
    attackNode->setContentSize(hitboxSize);
    attackNode->setAnchorPoint(Vec2(0.5f, 0.5f));
    parent->addChild(attackNode);

    auto body = PhysicsBody::createBox(hitboxSize);
    body->setDynamic(false);
    body->setGravityEnable(false);
    body->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER));
    body->setCategoryBitmask(ToMask(GamePhysicsCategory::MONSTER_ATTACK));
    body->setCollisionBitmask(0);
    body->setTag(damageTag);
    attackNode->setPhysicsBody(body);

    attackNode->runAction(Sequence::create(
        DelayTime::create(std::max(0.0f, lifeSeconds)),
        RemoveSelf::create(),
        nullptr));
}

void MonsterBase::faceTarget(Node* target)
{
    if (!target) return;
    faceToX(getWorldPosition(target).x);
}

// MonsterBase.cpp

float MonsterBase::distanceTo(cocos2d::Node* target) const
{
    if (!target)
        return 999999.0f;

    Vec2 myWorldPos = getWorldPosition(this);
    Vec2 targetWorldPos = getWorldPosition(target);
    return myWorldPos.distance(targetWorldPos);
}

bool MonsterBase::inAttackRange(Node* target)
{
    return horizontalDistanceTo(target) <= _attackRange;
}
void MonsterBase::setTarget(Node* target)
{
    _target = target;
}

void MonsterBase::setHome(const cocos2d::Vec2& pos)
{
    _homePos = pos;
    _hasHome = true;
}

void MonsterBase::setAggroRadius(float r)
{
    _aggroRadius = r;
}

void MonsterBase::setLeashRadius(float r)
{
    _leashRadius = r;
}

void MonsterBase::enablePatrol(const cocos2d::Vec2& left, const cocos2d::Vec2& right)
{
    _patrolEnabled = true;
    _patrolLeft = left;
    _patrolRight = right;
    _patrolDir = 1;
}

bool MonsterBase::hasAggro() const
{
    if (!_target) return false;
    if (_aggroRadius <= 0.0f) return true;
    return distanceTo(_target) <= _aggroRadius;
}
#pragma endregion
