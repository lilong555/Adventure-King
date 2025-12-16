#include "MonsterBase.h"
#include "cocos2d.h"
USING_NS_CC;

MonsterBase::MonsterBase()
{
}

MonsterBase::~MonsterBase()
{
}

bool MonsterBase::init(const std::string& spriteFrameName)
{
    if (!initWithSpriteFrameName(spriteFrameName))
    {
        if (!initWithFile(spriteFrameName))
            return false;
    }

    // 默认缩放：与玩家比例匹配（统一怪物体型）
    setScale(0.6f);
    _baseScaleX = 0.6f;

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

    addComponent(_physicsBody);
    scheduleUpdate();
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

    auto state = getStateMachineComponent()->getCurrentState();

    if (state == CharacterState::WALKING)
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
        if (_target) faceTarget(_target);
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
    if (!_target) return;

    // 1. 基础数据
    float dist = distanceTo(_target);
    auto sm = getStateMachineComponent();

    // 2. 超出仇恨范围 -> 待机
    if (_aggroRadius > 0.0f && dist > _aggroRadius)
    {
        sm->changeState(CharacterState::IDLE);
        return;
    }

    // 3. 在攻击范围内
    if (dist <= _attackRange)
    {
        // ★ 关键修正 ★
        // 只有当“冷却好了”才切 ATTACKING
        // 否则切 IDLE (这样才能触发 MonsterBase::update 里的 faceTarget 转身逻辑)
        if (_attackTimer >= _attackInterval)
        {
            sm->changeState(CharacterState::ATTACKING);
        }
        else
        {
            sm->changeState(CharacterState::IDLE);
        }
    }
    // 4. 不在范围 -> 追击
    else
    {
        // 这里最好更新一下 _currentTargetPos，虽然这里是基类默认实现
        // 如果你的 updateMovement 依赖 _currentTargetPos，这行必须加
        // _currentTargetPos = _target->getPosition(); 

        sm->changeState(CharacterState::WALKING);
    }
}

#pragma endregion


#pragma region 移动
void MonsterBase::updateMovement(float dt)
{
    if (!_physicsBody) return;

    float moveSpeed = 0.0f;
    if (auto attr = getAttributeComponent())
    {
        moveSpeed = attr->getAttributeValue(AttributeType::MOVE_SPEED);
    }

    // 无目标 → 不移动
    if (!_target)
    {
        _physicsBody->setVelocity(Vec2::ZERO);
        return;
    }

    float dist = distanceTo(_target);
    if (dist <= _attackRange)
    {
        // 停止移动时，也要保留 Y 轴速度（防止怪物在空中攻击时突然定住不掉下来）
        float currentVy = _physicsBody->getVelocity().y;
        _physicsBody->setVelocity(Vec2(0, currentVy));
        return;
    }

    // 1. 获取当前物理引擎计算出的 Y 轴速度 (包含重力影响)
    float currentVy = _physicsBody->getVelocity().y;

    // 2. 只计算水平方向 (X轴) 的向量
    Vec2 targetPos = _target->getPosition();
    Vec2 myPos = getPosition();

    // 当水平距离非常接近时，由于物理抖动可能导致方向频繁反转
    const float kChaseDeadzoneX = 8.0f;
    float dx = targetPos.x - myPos.x;
    if (fabs(dx) <= kChaseDeadzoneX)
    {
        _physicsBody->setVelocity(Vec2(0, currentVy));
        return;
    }

    // 判断在左边还是右边
    float dirX = (dx > 0.0f) ? 1.0f : -1.0f;

    // 3. 组合新速度：
    // X轴 = 我们想要的移动速度
    // Y轴 = 物理引擎原本的速度 (让重力继续拉着它)
    _physicsBody->setVelocity(Vec2(dirX * moveSpeed, currentVy));

    faceTarget(_target);
}
#pragma endregion


#pragma region 攻击

void MonsterBase::updateAttack(float dt)
{
    // 如果没有目标，直接返回
    if (!_target) return;

    // 获取状态机
    auto sm = getStateMachineComponent();

    // =========================================================
    // 1. 如果正在攻击中 (ATTACKING)
    // =========================================================
    if (sm->getCurrentState() == CharacterState::ATTACKING)
    {
        // ★ 这里什么都不用做！★
        // 不要调用 faceTarget，让它保持攻击开始时的朝向。
        // 等动作播放完，回调函数会自动把状态切回 IDLE。
        return;
    }

    // =========================================================
    // 2. 如果是其他状态 (IDLE/WALKING) -> 处于攻击间隔中
    // =========================================================

    // 累加冷却时间
    _attackTimer += dt;

    // ★ 关键点：在攻击间隔期间，怪物需要盯着玩家 ★
    // 如果你在 updateAI 或 updateMovement 里已经调用了 faceTarget，这里可以省略。
    // 但为了保险，可以在这里加一句（或者确保 IDLE 状态下也有人负责转身）：
    if (sm->getCurrentState() == CharacterState::IDLE)
    {
        faceTarget(_target);
    }

    // 检查距离
    float dist = distanceTo(_target);
    if (dist > _attackRange) return;

    // 冷却完毕，且在范围内 -> 发动攻击
    if (_attackTimer >= _attackInterval)
    {
        // 切状态
        sm->changeState(CharacterState::ATTACKING);
        // 执行攻击 (此时方向被锁死在上一帧的朝向)
        attack();
    }
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

void MonsterBase::faceTarget(Node* target)
{
    if (!target) return;

    const float kFaceDeadzoneX = 8.0f;
    float dx = target->getPositionX() - getPositionX();
    if (fabs(dx) <= kFaceDeadzoneX)
    {
        return;
    }

    float sign = (dx < 0.0f) ? -1.0f : 1.0f;

    // 只改符号，不改大小
    setScaleX(sign * fabs(_baseScaleX));
}

// MonsterBase.cpp

float MonsterBase::distanceTo(cocos2d::Node* target) const
{
    if (!target) return 999999.0f;

    // 1. 获取怪物在屏幕上的绝对位置 (世界坐标)
    Vec2 myWorldPos = this->getParent()->convertToWorldSpace(this->getPosition());

    // 2. 获取目标在屏幕上的绝对位置 (世界坐标)
    // 注意：如果 target 没有父节点，它自己就是世界坐标，需要判空
    Vec2 targetWorldPos = target->getPosition();
    if (target->getParent())
    {
        targetWorldPos = target->getParent()->convertToWorldSpace(target->getPosition());
    }

    // 3. 计算这一刻的真实距离
    return myWorldPos.distance(targetWorldPos);
}

bool MonsterBase::inAttackRange(Node* target)
{
    return distanceTo(target) <= _attackRange;
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
