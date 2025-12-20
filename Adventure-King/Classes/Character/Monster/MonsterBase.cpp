#include "MonsterBase.h"
#include "Character/components/StatusEffectVfxComponent.h"
#include "Configs/GameConfigs.h"
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

void MonsterBase::setHpBarScale(float scale)
{
    _hpBarScale = std::max(0.1f, scale);
    updateHpBar();
}

bool MonsterBase::init(const std::string& spriteFrameName)
{
    // 1. 视觉初始化
    bool initSuccess = initWithSpriteFrameName(spriteFrameName);
    if (!initSuccess)
    {
        initSuccess = initWithFile(spriteFrameName);
    }

    if (!initSuccess)
    {
        return false;
    }

    // ---------------------------------------------------------
    // ✅ 【核心修复】手动挂载必要组件
    // ---------------------------------------------------------
    // 属性组件 (必须有，用于存血量/攻击力)
    if (!getAttributeComponent()) {
        auto attr = AttributeComponent::create();
        attr->setName("AttributeComponent"); // 必须设置名字，基类通过名字查找
        this->addComponent(attr);
    }

    // 状态机组件 (必须有，用于播放受击/死亡/攻击动画)
    if (!getStateMachineComponent()) {
        auto sm = StateMachineComponent::create();
        sm->setName("StateMachineComponent");
        this->addComponent(sm);
    }

    // 技能组件 (可选，如果怪物也用技能系统就加上)
    if (!getSkillComponent()) {
        auto skill = SkillComponent::create();
        skill->setName("SkillComponent");
        this->addComponent(skill);
    }

    // 状态效果VFX组件（燃烧等表现）
    if (!getComponent("StatusEffectVfxComponent")) {
        auto vfx = StatusEffectVfxComponent::create();
        vfx->setName("StatusEffectVfxComponent");
        this->addComponent(vfx);
    }

    // ---------------------------------------------------------

    // 默认缩放
    setScale(GameConfig::Monster::Base::SCALE);
    _baseScaleX = GameConfig::Monster::Base::SCALE;

    // 默认锚点：底部对齐（更符合横版地面站立表现）
    setAnchorPoint(Vec2(GameConfig::Monster::Base::ANCHOR_X,
                        GameConfig::Monster::Base::ANCHOR_Y));

    // 怪物默认开启受击飘字
    setDamageNumbersEnabled(true);

    // === 创建怪物物理体 ===
    Size size = getContentSize();
    Size boxSize(size.width * GameConfig::Monster::Base::PHYSICS_BOX_RATIO_W,
                 size.height * GameConfig::Monster::Base::PHYSICS_BOX_RATIO_H);

    PhysicsMaterial material = GameConfig::Material::MONSTER;

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

    setPhysicsBody(_physicsBody);

    _aiUpdateInterval = GameConfig::Monster::Base::AI_UPDATE_INTERVAL;
    _inactiveAiUpdateInterval = GameConfig::Monster::Base::AI_INACTIVE_UPDATE_INTERVAL;
    _moveUpdateInterval = GameConfig::Monster::Base::MOVE_UPDATE_INTERVAL;
    _attackUpdateInterval = GameConfig::Monster::Base::ATTACK_UPDATE_INTERVAL;

    // 性能：默认按屏幕宽度设置“活跃更新范围”，避免大量离屏怪物每帧跑 AI 逻辑
    if (_activeUpdateDistanceX <= 0.0f)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        if (visibleSize.width > 0.0f)
        {
            _activeUpdateDistanceX = visibleSize.width *
                                     GameConfig::Monster::Base::ACTIVE_UPDATE_DISTANCE_MULTIPLIER;
        }
    }

    // 生成错峰：避免同帧生成多个怪物时，节流 tick 同帧集中触发造成尖刺
    // 取 [0.5*interval, interval]：保证首次响应不会等待过久，同时分散负载
    auto staggerAccumulator = [](float intervalSeconds) -> float {
        if (intervalSeconds <= 0.0f)
            return 0.0f;

        return cocos2d::random(intervalSeconds * 0.5f, intervalSeconds);
    };

    _aiUpdateAccumulator = staggerAccumulator(_aiUpdateInterval);
    _moveUpdateAccumulator = staggerAccumulator(_moveUpdateInterval);
    _attackUpdateAccumulator = staggerAccumulator(_attackUpdateInterval);
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

    auto sm = getStateMachineComponent();
    if (!sm)
        return;

    if (isDead())
    {
        sm->changeState(CharacterState::DEAD);
        return;
    }

    // 硬直/眩晕期间不做 AI/移动/攻击逻辑，避免节流导致的“硬直还在走”
    if (_isStunned)
    {
        sm->changeState(CharacterState::IDLE);
        _hasMoveGoal = false;
        if (_physicsBody)
        {
            cocos2d::Vec2 v = _physicsBody->getVelocity();
            v.x = 0;
            _physicsBody->setVelocity(v);
        }
        return;
    }

    // 受击硬直期间：打断移动/攻击逻辑，并冻结水平速度，等待 StateMachineComponent 自动回到 IDLE
    auto state = sm->getCurrentState();
    if (state == CharacterState::HURT)
    {
        _hasMoveGoal = false;
        _returningHome = false;

        if (_physicsBody)
        {
            cocos2d::Vec2 v = _physicsBody->getVelocity();
            v.x = 0;
            _physicsBody->setVelocity(v);
        }
        return;
    }

    const bool withinActiveRange = isWithinActiveUpdateRange();
    const float aiInterval = withinActiveRange ? _aiUpdateInterval : _inactiveAiUpdateInterval;

    if (aiInterval <= 0.0f)
    {
        updateAI(dt);
    }
    else
    {
        _aiUpdateAccumulator += dt;
        if (_aiUpdateAccumulator >= aiInterval)
        {
            // 传入累计 dt：即使当前 AI 不依赖 dt，也方便后续扩展时间相关逻辑
            updateAI(_aiUpdateAccumulator);
            _aiUpdateAccumulator = 0.0f;
        }
    }

    state = sm->getCurrentState();

    // 离屏（或远离玩家）时：冻结水平速度并跳过移动/攻击计算，减少 CPU 占用
    if (!withinActiveRange)
    {
        if (_physicsBody)
        {
            cocos2d::Vec2 v = _physicsBody->getVelocity();
            v.x = 0;
            _physicsBody->setVelocity(v);
        }

        // 避免离开激活范围时卡在攻击等中间状态（离屏不需要继续保持攻击状态）
        if (state == CharacterState::ATTACKING)
        {
            sm->changeState(CharacterState::IDLE);
        }
        return;
    }

    // 攻击冷却计时：按帧累加，攻击动作期间不计时（保持“冷却从攻击结束开始”）
    if (state != CharacterState::ATTACKING)
    {
        _attackTimer += dt;
    }

    if (state == CharacterState::WALKING || state == CharacterState::STATE_PATROL)
    {
        if (_moveUpdateInterval <= 0.0f)
        {
            updateMovement(dt);
        }
        else
        {
            _moveUpdateAccumulator += dt;
            if (_moveUpdateAccumulator >= _moveUpdateInterval)
            {
                updateMovement(_moveUpdateAccumulator);
                _moveUpdateAccumulator = 0.0f;
            }
        }
    }
    else if (state == CharacterState::IDLE)
    {
        if (_physicsBody)
        {
            cocos2d::Vec2 v = _physicsBody->getVelocity();
            v.x = 0;
            _physicsBody->setVelocity(v);
        }

        if (_target)
        {
            faceTarget(_target);
        }
    }

    // 攻击状态下 (ATTACKING) 不调用 faceTarget，也就是“锁死方向”
    if (_attackUpdateInterval <= 0.0f)
    {
        updateAttack(dt);
    }
    else
    {
        _attackUpdateAccumulator += dt;
        if (_attackUpdateAccumulator >= _attackUpdateInterval)
        {
            updateAttack(_attackUpdateAccumulator);
            _attackUpdateAccumulator = 0.0f;
        }
    }
}

#pragma region AI
//设置索敌、追击、巡逻
void MonsterBase::setAIConfig(float AR, float LR, bool PTL) {
    _aggroRadius = AR;  // 仇恨范围 (必须 > AttackRange)
    _leashRadius = LR;    // 牵引范围 (0 代表死追到底，不回家)

    // --- 巡逻设置 ---
    _patrolEnabled = PTL;  // 是否巡逻
}

void MonsterBase::setUpdateTickIntervals(float aiIntervalSeconds, float movementIntervalSeconds, float attackIntervalSeconds)
{
    _aiUpdateInterval = std::max(0.0f, aiIntervalSeconds);
    _moveUpdateInterval = std::max(0.0f, movementIntervalSeconds);
    _attackUpdateInterval = std::max(0.0f, attackIntervalSeconds);
}

void MonsterBase::setInactiveAiUpdateInterval(float seconds)
{
    _inactiveAiUpdateInterval = std::max(0.0f, seconds);
}

void MonsterBase::setActiveUpdateDistanceX(float distanceX)
{
    _activeUpdateDistanceX = distanceX;
}

bool MonsterBase::isWithinActiveUpdateRange() const
{
    auto distanceTarget = _primaryTarget ? _primaryTarget : _target;
    if (!distanceTarget)
        return true;

    if (_activeUpdateDistanceX <= 0.0f)
        return true;

    return horizontalDistanceTo(distanceTarget) <= _activeUpdateDistanceX;
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

    if (sm->getCurrentState() == CharacterState::ATTACKING)
    {
        return;
    }

    // 重新索敌：当 _target 因超出仇恨范围被清空时，玩家重新进入仇恨范围应当恢复
    // 注意：若正在 leash 回家（_returningHome），不允许立即重新索敌，避免永远回不了家。
    if (!_target && !_returningHome && _primaryTarget)
    {
        if (_aggroRadius <= 0.0f || distanceTo(_primaryTarget) <= _aggroRadius)
        {
            _target = _primaryTarget;
        }
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
            const float kPatrolReachEpsilon = GameConfig::Monster::Base::PATROL_REACH_EPSILON;
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
        _returningHome = false;
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
            _returningHome = true;
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
    _returningHome = false;
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
    const float kChaseDeadzoneX = GameConfig::Monster::Base::CHASE_DEADZONE_X;
    float dx = targetPos.x - myPos.x;
    if (fabs(dx) <= kChaseDeadzoneX)
    {
        _physicsBody->setVelocity(Vec2(0, currentVy));

        // 到达移动目标（回家/巡逻）后清空目标
        if (!_target && _hasMoveGoal)
        {
            _hasMoveGoal = false;
            _returningHome = false;
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
    CC_UNUSED_PARAM(dt);
    if (isDead() || _isStunned)
        return;

    auto sm = getStateMachineComponent();
    if (!sm)
        return;

    if (sm->getCurrentState() == CharacterState::HURT)
        return;

    // 正在攻击：保持朝向不变
    if (sm->getCurrentState() == CharacterState::ATTACKING)
        return;

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

    if (_attackInterval > 0.0f)
    {
        _attackTimer = std::fmod(_attackTimer, _attackInterval);
        if (_attackTimer < 0.0f)
        {
            _attackTimer += _attackInterval;
        }
    }
    else
    {
        _attackTimer = 0.0f;
    }
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
    spawnHurtVfx(info);

    setCurrentHP(hp);
    updateHpBar();

    if (hp <= 0)
    {
        die();
        return;
    }

    // DOT 等持续伤害：不触发硬直/打断，避免频繁停机
    if (!info.causesHitStun)
    {
        return;
    }

    // 受击：打断当前动作（尤其是攻击），并进入受击状态
    stopAllActions();
    stopVisualActions();
    _hasMoveGoal = false;
    _returningHome = false;

    if (_physicsBody)
    {
        cocos2d::Vec2 v = _physicsBody->getVelocity();
        v.x = 0;
        _physicsBody->setVelocity(v);
    }

    // 受击方向：beattacked png 有方向；
    // 当攻击来自“面向方向”（正向受击）时，需要镜像受击图。
    if (info.hasHitWorldPos || (info.attacker && info.attacker != this))
    {
        float myX = getWorldPosition(this).x;
        float attackerX = info.hasHitWorldPos ? info.hitWorldPos.x : getWorldPosition(info.attacker).x;
        bool attackerOnLeft = attackerX < myX;

        bool facingLeft = getScaleX() < 0.0f;

        // beattacked png 有方向：以“攻击来源在左侧”为基准决定最终镜像状态。
        // 怪物朝向由 scaleX 的正负实现，避免改动它；用 Sprite::setFlippedX 作为“额外镜像层”。
        //
        // 目标：最终镜像状态 = attackerOnLeft
        // 最终镜像状态 = (scaleX<0) XOR flippedX  =>  flippedX = facingLeft XOR attackerOnLeft
        bool hurtOverlayFlip = facingLeft ^ attackerOnLeft;
        setFlippedX(hurtOverlayFlip);
        runAction(Sequence::create(
            DelayTime::create(0.3f),
            CallFunc::create([this]() { setFlippedX(false); }),
            nullptr));
    }

    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::HURT);
    }
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

    float uiScale = std::max(0.1f, _hpBarScale);

    float maxHp = _maxHP;
    if (auto attr = getAttributeComponent())
    {
        maxHp = attr->getAttributeValue(AttributeType::MAX_HP);
    }
    if (maxHp <= 0.0f)
        return;

    float barWidth = GameConfig::Monster::Base::HP_BAR_WIDTH * uiScale;
    float barHeight = GameConfig::Monster::Base::HP_BAR_HEIGHT * uiScale;
    float yOffset = getContentSize().height + GameConfig::Monster::Base::HP_BAR_Y_OFFSET;

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
    if (!node)
        return Vec2::ZERO;

    auto myParent = getParent();
    auto nodeParent = node->getParent();
    if (myParent && nodeParent == myParent)
    {
        return node->getPosition();
    }

    Vec2 worldPos = getWorldPosition(node);
    return myParent ? myParent->convertToNodeSpace(worldPos) : worldPos;
}

float MonsterBase::horizontalDistanceTo(const Node *target) const
{
    if (!target)
        return 999999.0f;

    auto myParent = getParent();
    if (myParent && target->getParent() == myParent)
    {
        return std::fabs(target->getPositionX() - getPositionX());
    }

    Vec2 myWorldPos = getWorldPosition(this);
    Vec2 targetWorldPos = getWorldPosition(target);
    return std::fabs(targetWorldPos.x - myWorldPos.x);
}

void MonsterBase::faceToX(float targetWorldX)
{
    const float kFaceDeadzoneX = GameConfig::Monster::Base::FACE_DEADZONE_X;
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
    // 记录攻击来源，用于受击方向判断（例如按攻击左右决定 beattacked png 镜像）
    // 使用 userObject（Ref*）避免 userData(void*) 的不安全类型转换
    attackNode->setUserObject(this);
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
    if (!target)
        return;

    auto myParent = getParent();
    if (myParent && target->getParent() == myParent)
    {
        const float kFaceDeadzoneX = GameConfig::Monster::Base::FACE_DEADZONE_X;
        float dx = target->getPositionX() - getPositionX();
        if (std::fabs(dx) <= kFaceDeadzoneX)
            return;

        float sign = (dx < 0.0f) ? -1.0f : 1.0f;
        setScaleX(sign * std::fabs(_baseScaleX));
        return;
    }

    faceToX(getWorldPosition(target).x);
}

// MonsterBase.cpp

float MonsterBase::distanceTo(cocos2d::Node* target) const
{
    if (!target)
        return 999999.0f;

    auto myParent = getParent();
    if (myParent && target->getParent() == myParent)
    {
        return getPosition().distance(target->getPosition());
    }

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
    _primaryTarget = target;
    _returningHome = false;
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
