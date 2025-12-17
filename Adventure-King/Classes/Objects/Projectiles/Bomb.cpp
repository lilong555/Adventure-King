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
    
    auto physicsBody = PhysicsBody::createCircle(15.0f, GameConfig::Material::BOMB);

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
            float critRate = attr->getAttributeValue(AttributeType::CRITICAL_RATE);
            float strength = attr->getAttributeValue(AttributeType::STRENGTH);
            damageAmount += (strength * 5.0f);

            if ((rand() % 100) < static_cast<int>(critRate * 100))
            {
                isCrit = true;
                damageAmount *= 1.5f;
            }
        }
    }

    DamageInfo damageInfo;
    damageInfo.amount = damageAmount;
    damageInfo.attacker = _attacker;
    damageInfo.isCritical = isCrit;

    // --- 范围检测与伤害施加 ---

    // 【关键修复 1】: 不要使用引用 (auto&)，而是创建一个副本 (auto)
    // 如果使用 auto& children = ...，当怪物死亡并从父节点移除时，children 向量会变动，
    // 导致正在进行的 for 循环崩溃 (vector iterators incompatible)。
    auto children = parentLayer->getChildren();

    for (const auto& child : children)
    {
        // 尝试转换为 CharacterBase
        auto target = dynamic_cast<CharacterBase*>(child);

        // 排除自己(攻击者) 和 空指针
        if (target && target != _attacker && target->getCurrentHP() > 0)
        {
            Vec2 explodePos = this->getPosition();
            float dist = target->getPosition().distance(explodePos);

            // 【关键修复 2】: 绝对不要调用 target->getScale()
            // 如果怪物为了面向左边设置了 scaleX = -1，调用 getScale() 会直接断言崩溃。
            // 使用 std::abs(getScaleX()) 是安全的替代方案。
            float scaleFactor = std::abs(target->getScaleX());
            float targetRadius = target->getContentSize().width * scaleFactor * 0.5f;

            // 容错处理
            if (targetRadius <= 0) targetRadius = 20.0f;

            if (dist - targetRadius <= BOMB_EXPLOSION_RADIUS)
            {
                target->takeDamage(damageInfo);

                // 注意：如果 takeDamage 导致怪物死亡并由 parentLayer 移除，
                // 因为我们上面遍历的是 children 的副本，所以这里不会崩溃。
                CCLOG("Bomb hit target: %s for %.1f damage", target->getName().c_str(), damageAmount);
            }
        }
    }
}
