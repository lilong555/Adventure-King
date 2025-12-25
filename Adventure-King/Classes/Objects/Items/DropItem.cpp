#include "Objects/Items/DropItem.h"

#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "Configs/GameConfig.h"
#include "Configs/GamePhysicsCategory.h"

USING_NS_CC;

namespace
{
    void setupItemPhysicsBody(DropItem* item)
    {
        if (!item)
        {
            return;
        }

        // 仅用于拾取检测：不产生真实碰撞
        const float pickupBox = std::max(1.0f, GameConfig::DropItem::PICKUP_BOX_SIZE);
        auto body = PhysicsBody::createBox(Size(pickupBox, pickupBox));
        body->setDynamic(false);
        body->setGravityEnable(false);
        body->setRotationEnable(false);
        // 锚点在底部：把拾取判定框上移半个高度，贴近瓶子主体
        body->setPositionOffset(Vec2(0.0f, pickupBox * 0.5f));

        // 设置为传感器：只触发接触事件，不阻挡玩家
        for (auto shape : body->getShapes())
        {
            if (shape)
            {
                shape->setSensor(true);
            }
        }

        body->setCategoryBitmask(ToMask(GamePhysicsCategory::ITEM));
        body->setCollisionBitmask(0);
        body->setContactTestBitmask(ToMask(GamePhysicsCategory::PLAYER));
        item->setPhysicsBody(body);
    }

    float getAttributeOrDefault(PlayerCharacter* player, AttributeType type, float fallback)
    {
        if (!player)
        {
            return fallback;
        }

        if (auto attr = player->getAttributeComponent())
        {
            const float v = attr->getAttributeValue(type);
            if (v > 0.0f)
            {
                return v;
            }
        }
        return fallback;
    }
}

DropItem* DropItem::create(DropItemType type)
{
    auto ret = new(std::nothrow) DropItem();
    if (ret && ret->init(type))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DropItem::init(DropItemType type)
{
    _type = type;

    const char* spritePath = nullptr;
    switch (_type)
    {
    case DropItemType::HEALTH:
        spritePath = GameConfig::DropItem::HP_SPRITE_PATH;
        break;
    case DropItemType::MANA:
        spritePath = GameConfig::DropItem::MP_SPRITE_PATH;
        break;
    default:
        spritePath = GameConfig::DropItem::HP_SPRITE_PATH;
        break;
    }

    if (!spritePath || !Sprite::initWithFile(spritePath))
    {
        CCLOG("DropItem: 创建失败，资源不存在：%s", spritePath ? spritePath : "(null)");
        return false;
    }

    // 以“固定显示高度”适配不同分辨率的原始 PNG（当前资源尺寸很大）
    setAnchorPoint(Vec2(0.5f, 0.0f));
    const float targetHeight = std::max(1.0f, GameConfig::DropItem::VISUAL_HEIGHT);
    const float srcHeight = getContentSize().height;
    if (srcHeight > 1.0f)
    {
        setScale(targetHeight / srcHeight);
    }

    setupItemPhysicsBody(this);

    return true;
}

void DropItem::pickUp(PlayerCharacter* player)
{
    if (_pickedUp || !player)
    {
        return;
    }
    _pickedUp = true;

    // 先禁用物理，避免重复触发
    if (auto body = getPhysicsBody())
    {
        body->setEnabled(false);
        body->setCategoryBitmask(ToMask(GamePhysicsCategory::NONE));
        body->setCollisionBitmask(0);
        body->setContactTestBitmask(0);
    }

    // 应用效果
    if (_type == DropItemType::HEALTH)
    {
        const float maxHp = getAttributeOrDefault(player, AttributeType::MAX_HP, 100.0f);
        const float healAmount = std::max(0.0f, maxHp * GameConfig::DropItem::HP_RESTORE_RATIO);
        player->heal(healAmount);
    }
    else if (_type == DropItemType::MANA)
    {
        const float maxMp = getAttributeOrDefault(player, AttributeType::MAX_MP, 100.0f);
        const float restoreAmount = std::max(0.0f, maxMp * GameConfig::DropItem::MP_RESTORE_RATIO);
        player->setCurrentMP(player->getCurrentMP() + restoreAmount);
    }

    // 轻量表现：快速缩放消失（不影响游戏逻辑）
    setCascadeOpacityEnabled(true);
    runAction(Sequence::create(
        Spawn::create(ScaleTo::create(0.08f, 0.0f), FadeOut::create(0.08f), nullptr),
        RemoveSelf::create(),
        nullptr));
}
