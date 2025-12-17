#pragma once
//将 GameScene.cpp 中 doThrowBomb 和 explodeBomb 的核心逻辑搬运过来
#ifndef __BOMB_H__
#define __BOMB_H__

#include "cocos2d.h"
#include "Character/Base/CharacterBase.h" // 引用CharacterBase以便造成伤害

class Bomb : public cocos2d::Sprite
{
public:
    // 静态创建方法
    static Bomb* create(const std::string& filename);

    // 初始化物理属性
    bool initPhysics();

    // 投掷方法 (由外部调用，给予初始速度)
    void throwAt(const cocos2d::Vec2& velocity);

    // 爆炸方法
    void explode();

    // 设置攻击者 (用于判定伤害来源，防止炸到队友等逻辑)
    void setAttacker(CharacterBase* attacker) { _attacker = attacker; }

    // 获取攻击者
    CharacterBase* getAttacker() const { return _attacker; }

    // 状态查询
    bool isExploded() const { return _isExploded; }

protected:
    // 爆炸范围伤害计算
    void applyAoEDamage();

private:

    bool _isExploded = false;
    CharacterBase* _attacker = nullptr; // 持有攻击者的引用

    // --- 物理与伤害常量 ---
        // 你可以将这些值放入配置文件，这里为了演示直接定义
    const float BOMB_EXPLOSION_RADIUS = 80.0f;
    const float BOMB_DAMAGE_BASE = 150.0f;
};

#endif // __BOMB_H__
