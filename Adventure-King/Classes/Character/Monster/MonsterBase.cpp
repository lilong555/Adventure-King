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

void MonsterBase::update(float dt)
{
    if (isDead()) return;

    _attackTimer += dt;

    // 状态机更新
    if (getStateMachineComponent())
        getStateMachineComponent()->update(dt);

    updateAI(dt);
    updateMovement(dt);
    updateAttack(dt);

    cocos2d::Sprite::update(dt);
}

#pragma region AI
//默认 AI 行为
void MonsterBase::updateAI(float dt)
{
    if (!_target) return;

    float dist = distanceTo(_target);

    if (_aggroRadius > 0.0f && dist > _aggroRadius)
    {
        getStateMachineComponent()->changeState(CharacterState::IDLE);
        return;
    }

    if (dist <= _attackRange)
    {
        getStateMachineComponent()->changeState(CharacterState::ATTACKING);
    }
    else
    {
        getStateMachineComponent()->changeState(CharacterState::WALKING);
    }
}

#pragma endregion


#pragma region 移动

void MonsterBase::updateMovement(float dt)
{
    float speed = 0.0f;

    if (auto attr = getAttributeComponent())
    {
        speed = attr->getAttributeValue(AttributeType::MOVE_SPEED) * dt;
    }

    if (!_target)
    {
        if (_patrolEnabled)
        {
            cocos2d::Vec2 pos = getPosition();
            if (_patrolDir > 0)
            {
                pos.x += speed;
                if (pos.x >= _patrolRight.x)
                {
                    pos.x = _patrolRight.x;
                    _patrolDir = -1;
                }
            }
            else
            {
                pos.x -= speed;
                if (pos.x <= _patrolLeft.x)
                {
                    pos.x = _patrolLeft.x;
                    _patrolDir = 1;
                }
            }
            setPosition(pos);
        }
        return;
    }

    float dist = distanceTo(_target);
    if (dist <= _attackRange) return;

    if (_leashRadius > 0.0f && _hasHome)
    {
        float dHome = _homePos.distance(getPosition());
        if (dHome > _leashRadius)
        {
            Vec2 dirHome = (_homePos - getPosition()).getNormalized();
            setPosition(getPosition() + dirHome * speed);
            faceTarget(_target);
            return;
        }
    }

    Vec2 dir = (_target->getPosition() - getPosition()).getNormalized();
    setPosition(getPosition() + dir * speed);

    faceTarget(_target);
}

#pragma endregion


#pragma region 攻击

void MonsterBase::updateAttack(float dt)
{
    if (!_target) return;

    float dist = distanceTo(_target);
    if (dist > _attackRange) return;

    if (_attackTimer >= _attackInterval)
    {
        _attackTimer = 0;
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

    setCurrentHP(hp);

    if (hp <= 0)
    {
        die();
        return;
    }

    getStateMachineComponent()->changeState(CharacterState::HURT);
}

void MonsterBase::die()
{
    getStateMachineComponent()->changeState(CharacterState::DEAD);

    if (getAutoRemoveOnDeath())
    {
        runAction(Sequence::create(
            DelayTime::create(0.5f),
            RemoveSelf::create(),
            nullptr
        ));
    }
}

#pragma endregion


#pragma region 工具函数

void MonsterBase::faceTarget(Node* target)
{
    if (!target) return;

    float sign = (target->getPositionX() < getPositionX()) ? -1.0f : 1.0f;

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
