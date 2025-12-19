#include "Character/Base/CharacterBase.h"
#include "Character/components/AttributeComponent.h"
#include "Character/components/StateMachineComponent.h"
#include "Character/components/SkillComponent.h"
#include "Configs/GameConfigs.h"
#include "Utils/SpriteFrameCacheHelper.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

USING_NS_CC;

namespace
{
    const char* const DEFAULT_DAMAGE_FONT_PATH = "fonts/ZCOOLKuaiLe-Regular.ttf";
}

CharacterBase::CharacterBase() = default;
CharacterBase::~CharacterBase() = default;

AttributeComponent* CharacterBase::getAttributeComponent()
{
    return static_cast<AttributeComponent*>(this->getComponent("AttributeComponent"));
}

StateMachineComponent* CharacterBase::getStateMachineComponent()
{
    return static_cast<StateMachineComponent*>(this->getComponent("StateMachineComponent"));
}

SkillComponent* CharacterBase::getSkillComponent()
{
    return static_cast<SkillComponent*>(this->getComponent("SkillComponent"));
}

cocos2d::Sprite* CharacterBase::getVisualSprite() const
{
    return _visualSprite ? _visualSprite : const_cast<CharacterBase*>(this);
}

float CharacterBase::getAttackPower()
{
    auto attr = getAttributeComponent();
    if (!attr)
    {
        return 0.0f;
    }
    return attr->getAttributeValue(AttributeType::STRENGTH);
}

// 子类在 create 中调用，用于初始化贴图和组件
bool CharacterBase::initWithSpriteFrameName(const std::string& spriteFrameName)
{
    // 优先从 SpriteFrameCache 获取；若缺失且看起来是文件路径，则按文件加载并加入缓存
    auto spriteFrame = SpriteFrameCacheHelper::getOrCreateSpriteFrame(spriteFrameName);
    if (!spriteFrame)
    {
        CCLOG("CharacterBase::initWithSpriteFrameName - SpriteFrame '%s' not found", spriteFrameName.c_str());
        return false;
    }

    if (!Sprite::initWithSpriteFrame(spriteFrame))
    {
        return false;
    }

    setVisualSprite(this);

    _level = 1;
    _experience = 0;
    _currentHP = 0.0f;
    _currentMP = 0.0f;

    // 默认更新优先级设为 1
    scheduleUpdateWithPriority(1);
    return true;
}

// 使用普通文件路径初始化
bool CharacterBase::initWithFile(const std::string& filename)
{
    if (!Sprite::initWithFile(filename))
    {
        return false;
    }

    // 将文件贴图加入 SpriteFrameCache，便于后续复用
    SpriteFrameCacheHelper::getOrCreateSpriteFrame(filename);

    setVisualSprite(this);

    _level = 1;
    _experience = 0;
    _currentHP = 0.0f;
    _currentMP = 0.0f;

    scheduleUpdateWithPriority(1);
    return true;
}

// 每帧更新
void CharacterBase::update(float dt)
{
    // 1. 必选：调用父类 update
    Sprite::update(dt);

    // 2. 可选：在这里处理全局的角色逻辑（例如中毒掉血、BUFF计时）

    // 【重要修改】
    // 不需要手动调用组件的 update！
    // 只要组件被 addComponent 且 owner 开启了 scheduleUpdate，
    // Cocos2d-x 引擎会自动调用所有组件的 update(dt)。

    // 如果你要在这里手动调用 attributeComponent->updateStatusEffects(dt)，
    // 请确保 AttributeComponent 内部的 update(dt) 没有重复做这件事。
    // 为了保持清晰，建议把 updateStatusEffects 放在 AttributeComponent::update 里面调用。
}

// 受击
void CharacterBase::takeDamage(const DamageInfo& info)
{
    if (isDead()) return;

    float finalDamage = info.amount;
    auto attr = getAttributeComponent(); // 获取组件

    // ---------------------------------------------------------
    // 1. 暴击计算 (Critical Hit)
    // ---------------------------------------------------------
    if (info.isCritical)
    {
        finalDamage *= info.critMultiplier;
    }

    // ---------------------------------------------------------
    // 2. 防御计算 (Defense Mitigation - MOBA Style)
    // ---------------------------------------------------------
    if (attr)
    {
        float defense = attr->getAttributeValue(AttributeType::DEFENSE);

        // 引入"护甲穿透"计算：防御力 = 原始防御 - 穿透值
        float effectiveDefense = std::max(0.0f, defense - info.penetration);

        // 使用乘法公式：防御越高，减伤越高
        float reductionFactor = GameConfig::Combat::ARMOR_CONST /
                                (GameConfig::Combat::ARMOR_CONST + effectiveDefense);

        finalDamage *= reductionFactor;
    }

    // ---------------------------------------------------------
    // 3. 随机浮动 (Random Variance)
    // ---------------------------------------------------------
    // 修复：rand() 是整数，除以 100.0f 之前需要转型
    // 范围 0.95 ~ 1.05
    float variance = 0.95f + (static_cast<float>(rand() % 11) / 100.0f);
    finalDamage *= variance;

    // ---------------------------------------------------------
    // 4. 最终结算
    // ---------------------------------------------------------
    finalDamage = std::max(1.0f, std::floor(finalDamage));

    showDamageNumber(finalDamage, info.isCritical);

    _currentHP -= finalDamage;

    // 获取最大生命值用于受击动画阈值
    float maxHP = _maxHP;
    if (attr) {
        maxHP = attr->getAttributeValue(AttributeType::MAX_HP);
    }

    if (_currentHP <= 0.0f)
    {
        _currentHP = 0.0f;
        die();
    }
    else
    {
        // 只有伤害超过一定阈值才播放受击动作
        if (auto sm = getStateMachineComponent())
        {
            if (info.causesHitStun && maxHP > 0 && finalDamage > (maxHP * 0.05f))
            {
                sm->changeState(CharacterState::HURT);
            }
        }
    }
}

void CharacterBase::showDamageNumber(float damage, bool isCritical)
{
    if (!_damageNumbersEnabled) return;
    if (damage <= 0.0f) return;

    auto parent = getParent();
    if (!parent) return;

    std::string damageText = StringUtils::format("%.0f", damage);
    if (isCritical)
    {
        damageText = "暴击 " + damageText + "!";
    }

    // 使用 createWithSystemFont 作为备选，防止 TTF 文件缺失导致崩溃
    Label* label = nullptr;
    if (FileUtils::getInstance()->isFileExist(DEFAULT_DAMAGE_FONT_PATH)) {
        label = Label::createWithTTF(damageText, DEFAULT_DAMAGE_FONT_PATH, isCritical ? 28 : 22);
    }
    else {
        label = Label::createWithSystemFont(damageText, "Arial", isCritical ? 28 : 22);
    }

    if (!label) return;

    label->setColor(isCritical ? Color3B(255, 50, 50) : Color3B(255, 200, 50));
    label->enableOutline(Color4B::BLACK, 2);

    Rect bbox = getBoundingBox();
    float offsetX = static_cast<float>((rand() % 31) - 15);
    float offsetY = 20.0f + static_cast<float>((rand() % 11) - 5);
    Vec2 pos = Vec2(bbox.getMidX(), bbox.getMaxY()) + Vec2(offsetX, offsetY);
    label->setPosition(pos);

    // 添加到 Scene 层而不是 Character 的子节点，防止随 Character 移动
    // 这里使用了 parent (GameLayer)，这通常是正确的
    parent->addChild(label, 9999);

    auto moveUp = MoveBy::create(0.8f, Vec2(0, 60));
    auto fadeOut = FadeOut::create(0.5f);
    // Sequence 1: 飘起 -> 消失
    // Sequence 2: 同时移除自己
    label->runAction(Sequence::create(
        Spawn::create(moveUp, fadeOut, nullptr),
        RemoveSelf::create(),
        nullptr
    ));
}

void CharacterBase::setVisualSprite(cocos2d::Sprite* sprite)
{
    _visualSprite = sprite ? sprite : this;
    if (_visualSprite)
    {
        _visualBaseScaleX = _visualSprite->getScaleX();
        _visualBaseScaleY = _visualSprite->getScaleY();
    }
}

void CharacterBase::stopVisualActions()
{
    auto visual = getVisualSprite();
    if (visual && visual != this)
    {
        visual->stopAllActions();
        visual->setScaleX(_visualBaseScaleX);
        visual->setScaleY(_visualBaseScaleY);
    }
}

void CharacterBase::die()
{
    // 切换状态
    if (auto sm = getStateMachineComponent())
    {
        sm->changeState(CharacterState::DEAD);
    }

    // 根据设置决定是否自动移除
    if (_autoRemoveOnDeath)
    {
        setCascadeOpacityEnabled(true);
        runAction(Sequence::create(
            FadeOut::create(0.5f),
            RemoveSelf::create(),
            nullptr));
    }
    else
    {
        // 不自动移除，尸体保留 (闪烁变灰效果)
        setCascadeOpacityEnabled(true);
        // 变灰而不是变红，通常更能代表死亡
        setColor(Color3B::GRAY);
    }
}

void CharacterBase::setCurrentHP(float hp)
{
    float maxHp = hp; // 默认值
    if (auto attr = getAttributeComponent())
    {
        maxHp = attr->getAttributeValue(AttributeType::MAX_HP);
    }
    // 确保 maxHp 至少为 1，防止除零或逻辑错误
    maxHp = std::max(1.0f, maxHp);

    _currentHP = clampf(hp, 0.0f, maxHp);
}

void CharacterBase::setCurrentMP(float mp)
{
    float maxMp = mp;
    if (auto attr = getAttributeComponent())
    {
        maxMp = attr->getAttributeValue(AttributeType::MAX_MP);
    }
    // MP 可以为 0
    maxMp = std::max(0.0f, maxMp);

    _currentMP = clampf(mp, 0.0f, maxMp);
}
