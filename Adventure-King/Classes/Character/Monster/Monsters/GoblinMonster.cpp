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
	CC_SAFE_RELEASE(_attackAnimate);
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

    // 2. 初始化 AI 参数 (关卡策划关注这一块)
    setAIConfig(700,0,true);
		// === 设置缩放比例 ===
	    setScale(0.36f);
	    _baseScaleX = 0.36f;

    // === 设置怪物属性 ===
    initAttributes();

    // === 刷新 HP / MP ===
    setCurrentHP(_maxHP);
    _currentMP = 0;
    updateHpBar();

    // === 注册状态动画（需要动画名已加入 AnimationCache）===
    initStateAnimations();

    // === 设置物理掩码 ===
    if (_physicsBody)
    {
        _physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::MONSTER));
        _physicsBody->setCollisionBitmask(
            ToMask(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::PLAYER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB)
        );
        _physicsBody->setContactTestBitmask(
            ToMask(GamePhysicsCategory::PLAYER | GamePhysicsCategory::PLAYER_ATTACK | GamePhysicsCategory::BOMB)
        );
    }
    this->setAnchorPoint(Vec2(0.5f, 0.0f));
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
    base.set(AttributeType::ATTACK_RANGE, 300.0f); // 发动攻击的距离

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
    cocos2d::Vector<cocos2d::SpriteFrame*> frames;
    char str[200] = { 0 };

    // 按序加载攻击帧：从 1 开始，遇到缺失文件就停止（避免刷屏报错）
    auto fileUtils = cocos2d::FileUtils::getInstance();
    bool oldPopupNotify = fileUtils->isPopupNotify();
    fileUtils->setPopupNotify(false);

    for (int i = 1; i <= 20; i++)
    {
        // 1. 拼接路径
        // 你的资源根目录是 Resources，所以路径从 Sprites/... 开始
        std::sprintf(str, "Sprites/Enemies/Goblin/Goblin_attack_%d.png", i);

        // 2. ★ 核心修改：使用 TextureCache 直接加载硬盘上的图片 ★
        auto texture = cocos2d::Director::getInstance()->getTextureCache()->addImage(str);

        if (!texture)
        {
            break;
        }

        // 3. 如果加载成功，用这张纹理创建一个 SpriteFrame
        auto size = texture->getContentSize();
        auto frame = cocos2d::SpriteFrame::createWithTexture(texture, cocos2d::Rect(0, 0, size.width, size.height));
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
    CCLOG("Goblin Attack Triggered!");

    // 1. 重置计时器
    _attackTimer = 0.0f;

    // -----------------------------------------------------------
    // 2. 物理速度处理 (关键修复)
    // -----------------------------------------------------------
    // 只停止 X 轴移动，保留 Y 轴速度 (让怪物受重力影响自然下落)
    if (_physicsBody)
    {
        cocos2d::Vec2 v = _physicsBody->getVelocity();
        v.x = 0;
        _physicsBody->setVelocity(v);
    }

    // -----------------------------------------------------------
    // 3. 准备动画动作
    // -----------------------------------------------------------
    // ★ 安全保险：如果没有动画，创建一个 1秒的延时动作代替
    // 否则如果 _attackAnimate 为空，函数直接 return，状态机永远卡在 ATTACKING
    cocos2d::FiniteTimeAction* animateAction = nullptr;

    if (_attackAnimate)
    {
        animateAction = _attackAnimate->clone();
    }
    else
    {
        animateAction = cocos2d::DelayTime::create(1.0f); // 假动作
    }

    // -----------------------------------------------------------
    // 4. 计算出刀时间
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
    // 5. 判定框逻辑 (Hitbox)
    // -----------------------------------------------------------
    auto logicSequence = cocos2d::Sequence::create(
        cocos2d::DelayTime::create(hitTime),
        cocos2d::CallFunc::create([this]() {

            // 安全检查
            if (!this || !this->getParent()) return;

            auto parent = this->getParent();

	            // 计算朝向
	            float direction = (this->getScaleX() > 0) ? 1.0f : -1.0f;

	            // 缩放适配：攻击判定基于旧的 0.6 缩放值调过，这里按当前缩放同比例缩放偏移/尺寸
	            const float kAttackTuningScale = 0.6f;
	            float scaleRatio = 1.0f;
	            if (kAttackTuningScale > 0.0f)
	            {
	                scaleRatio = fabs(this->getScaleX()) / kAttackTuningScale;
	            }

	            // 计算偏移
	            cocos2d::Vec2 offset(100.f * direction * scaleRatio, 170.f * scaleRatio);
	            cocos2d::Size hitboxSize(300.0f * scaleRatio, 20.0f * scaleRatio);

	            // ★ 核心：计算世界/父节点坐标 ★
	            cocos2d::Vec2 worldPos = this->getPosition() + offset;

	            auto attackNode = cocos2d::Node::create();
	            attackNode->setPosition(worldPos);
	            attackNode->setContentSize(hitboxSize);
	            attackNode->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));

            // ★ 核心：加到父节点，避免物理质心偏移 ★
            parent->addChild(attackNode);

	            // 物理属性
	            auto body = cocos2d::PhysicsBody::createBox(hitboxSize);
	            body->setDynamic(false);
	            body->setGravityEnable(false);
	            body->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER));
            body->setCategoryBitmask(ToMask(GamePhysicsCategory::MONSTER_ATTACK));
            body->setCollisionBitmask(0);
            body->setTag(10); // 伤害值

            attackNode->setPhysicsBody(body);

            // 0.1秒后销毁
            attackNode->runAction(
                cocos2d::Sequence::create(
                    cocos2d::DelayTime::create(0.1f),
                    cocos2d::RemoveSelf::create(),
                    nullptr
                )
            );

            // 调试日志
            // CCLOG("Hitbox spawned at: %.1f, %.1f", worldPos.x, worldPos.y);
            }),
        nullptr
    );

    // -----------------------------------------------------------
    // 6. 组合并运行 (Spawn = 并行)
    // -----------------------------------------------------------
    // 动画 和 逻辑 同时跑
    auto spawn = cocos2d::Spawn::create(animateAction, logicSequence, nullptr);

    // -----------------------------------------------------------
    // 7. 结束回调 (恢复 IDLE)
    // -----------------------------------------------------------
    auto finalSequence = cocos2d::Sequence::create(
        spawn,
        cocos2d::CallFunc::create([this]() {
            auto sm = getStateMachineComponent();
            if (sm && sm->getCurrentState() != CharacterState::DEAD)
            {
                sm->changeState(CharacterState::IDLE);
            }
            }),
        nullptr
    );

    this->runAction(finalSequence);
}

#pragma endregion
