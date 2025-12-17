#include "Bomb.h"

#include "Physics/GamePhysicsCategory.h"
#include "Character/components/AttributeComponent.h" // 引入属性组件，用于计算伤害

USING_NS_CC;

Bomb* Bomb::create(const std::string& filename)
{
    Bomb* bomb = new (std::nothrow) Bomb();
    if (bomb && bomb->initWithFile(filename))
    {
        bomb->autorelease();
        bomb->initPhysics(); // 创建时自动初始化物理
        return bomb;
    }
    CC_SAFE_DELETE(bomb);
    return nullptr;
}

bool Bomb::initPhysics()
{
    PhysicsMaterial bombMaterial(0.5f, 0.3f, 0.2f);
    auto physicsBody = PhysicsBody::createCircle(15.0f, bombMaterial);

    physicsBody->setDynamic(true);
    physicsBody->setMass(0.5f);
    physicsBody->setRotationEnable(true);

    // ==========================================================
    // 使用 GamePhysicsCategory 进行设置
    // ==========================================================

    // 1. 我是谁？ (CategoryBitmask)
    // 我是炸弹
    physicsBody->setCategoryBitmask(ToMask(GamePhysicsCategory::BOMB));

    // 2. 我会撞到谁？ (CollisionBitmask)
    // 我会被“平台”、“通用碰撞体”、“怪物”挡住，产生物理反弹
    // 注意：这里使用了 operator| 将多个类别组合
    physicsBody->setCollisionBitmask(ToMask(
        GamePhysicsCategory::PLATFORM |
        GamePhysicsCategory::COLLISION |
        GamePhysicsCategory::MONSTER
    ));

    // 3. 撞到谁会通知我？ (ContactTestBitmask)
    // 当碰到“平台”、“怪物”时，我要收到回调（执行 explode）
    physicsBody->setContactTestBitmask(ToMask(
        GamePhysicsCategory::PLATFORM |
        GamePhysicsCategory::MONSTER
    ));

    this->addComponent(physicsBody);
    return true;
}

void Bomb::throwAt(const Vec2& velocity)
{
    auto body = this->getPhysicsBody();
    if (body)
    {
        // 根据质量应用冲量 F = m * v
        Vec2 impulse(velocity.x * body->getMass(), velocity.y * body->getMass());
        body->applyImpulse(impulse);
    }
}

void Bomb::explode()
{
    if (_isExploded) return;
    _isExploded = true;
    CCLOG("Bomb exploded at position (%.1f, %.1f)", this->getPositionX(), this->getPositionY());
    // 1. 视觉特效 (从 DebugScene::explodeBomb 移动过来)
    auto boomSprite = Sprite::create("Sprites/Characters/Player/Klee/BOOM_1.png");
    if (boomSprite)
    {
        // 如果炸弹有父节点，特效加到父节点上；否则加到自身(不推荐，因为自身要被移除了)
        if (this->getParent()) {
            boomSprite->setPosition(this->getPosition());
            this->getParent()->addChild(boomSprite, 6);
        }

        // 爆炸动画：放大 + 淡出
        boomSprite->setScale(0.4f);
        auto scaleUp = ScaleTo::create(0.2f, 1.2f);
        auto fadeOut = FadeOut::create(0.3f);
        auto seq = Sequence::create(Spawn::create(scaleUp, fadeOut, nullptr), RemoveSelf::create(), nullptr);
        boomSprite->runAction(seq);
    }

    // 2. 计算伤害
    applyAoEDamage();

    // 3. 移除自身
    this->removeFromParent();
    CCLOG("Bomb logic execution finished.");
}

void Bomb::applyAoEDamage()
{
    auto parentLayer = this->getParent();
    if (!parentLayer) return;

    

    // --- 准备攻击者数据 ---
    float damageAmount = BOMB_DAMAGE_BASE;
    bool isCrit = false;

    if (_attacker)
    {
        auto attr = _attacker->getAttributeComponent();
        if (attr)
        {
            // 读取属性
            float critRate = attr->getAttributeValue(AttributeType::CRITICAL_RATE);
            float strength = attr->getAttributeValue(AttributeType::STRENGTH);

            // 计算基础伤害: 150 + 力 * 5
            damageAmount += (strength * 5.0f);

            // 判定暴击
            if ((rand() % 100) < static_cast<int>(critRate * 100))
            {
                isCrit = true;
                damageAmount *= 1.5f; // 1.5倍暴击伤害
            }
        }
    }

    DamageInfo damageInfo;
    damageInfo.amount = damageAmount;
    damageInfo.attacker = _attacker;
    damageInfo.isCritical = isCrit;

    // --- 范围检测与伤害施加 ---
    // 遍历父节点的所有子节点 (这比只检测 _targetDummy 更通用)
    auto& children = parentLayer->getChildren();
    for (const auto& child : children)
    {
        // 尝试转换为 CharacterBase (通过 dynamic_cast 判断是否是可受伤的角色)
        auto target = dynamic_cast<CharacterBase*>(child);

        // 排除自己(攻击者) 和 空指针
        if (target && target != _attacker && target->getCurrentHP() > 0)
        {
            Vec2 explodePos = this->getPosition();
            // 计算距离 (考虑目标的体积半径，模拟更精准的判定)
            float dist = target->getPosition().distance(explodePos);
            // 这里简单估算目标半径为宽度的1/4，或者你可以直接用中心点距离
            float targetRadius = target->getContentSize().width * std::abs(target->getScaleX()) * 0.5f;

            if (dist - targetRadius <= BOMB_EXPLOSION_RADIUS)
            {
                target->takeDamage(damageInfo);
                CCLOG("Bomb hit target: %s for %.1f damage", target->getName().c_str(), damageAmount);
            }
        }
    }
}
