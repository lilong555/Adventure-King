#pragma once

#include "cocos2d.h"

class PlayerCharacter;

enum class DropItemType
{
    HEALTH = 0,
    MANA = 1,
};

// 掉落物：怪物死亡时生成，玩家路过自动拾取
class DropItem : public cocos2d::Sprite
{
public:
    static DropItem* create(DropItemType type);

    bool init(DropItemType type);

    DropItemType getType() const { return _type; }

    // 执行拾取逻辑（恢复血量/蓝量），并销毁自身
    void pickUp(PlayerCharacter* player);

private:
    DropItemType _type = DropItemType::HEALTH;
    bool _pickedUp = false;
};

