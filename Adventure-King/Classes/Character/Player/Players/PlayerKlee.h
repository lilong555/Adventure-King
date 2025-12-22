#pragma once
//所有klee特有的方法都放在这里
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/StateMachineComponent.h"
#include "cocos2d.h"

class PlayerKlee : public PlayerCharacter {
public:
    static PlayerKlee* create(const std::string& spriteFrameName) {
        auto player = new PlayerKlee();
        if (player && player->init(CharacterRole::MAGE, spriteFrameName)) {
            player->autorelease();
            return player;
        }
        CC_SAFE_DELETE(player);
        return nullptr;
    }

protected:
    // 仅重写 Klee 独有的逻辑
    virtual void attack() override;
        // 实现 Klee 扔炸弹的逻辑，而不是父类的普通挥砍
};
