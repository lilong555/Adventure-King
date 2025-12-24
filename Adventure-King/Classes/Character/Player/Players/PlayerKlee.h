#pragma once
//所有klee特有的方法都放在这里
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/StateMachineComponent.h"
#include "cocos2d.h"
#include "Character/Player/SkillSets/PlayerSkillSet.h"

class PlayerKlee : public PlayerCharacter {
public:
    // 显式定义构造函数
    PlayerKlee() : PlayerCharacter() {}

    static PlayerKlee* create(const std::string& spriteFrameName) {
        auto player = new (std::nothrow) PlayerKlee();
        // 调用 init，传入 MAGE 职业标识
        if (player && player->init(CharacterRole::MAGE, spriteFrameName)) {
            player->autorelease();
            return player;
        }
        CC_SAFE_DELETE(player);
        return nullptr;
    }

    /**
     * @brief 封装后的 Klee 资源预加载函数
     */
    static std::vector<std::string> getPreloadResourcePaths();

//protected:
//    // 重写 Klee 独有的攻击逻辑
//    virtual void attack() override;
};
