#pragma once

#include "Character/Base/CharacterBase.h"
#include "cocos2d.h"
#include <string>
#include <vector>

class ExplosiveProjectile : public cocos2d::Sprite
{
public:
    // 命中附加状态模板：用于投掷物/爆炸在命中目标时施加通用状态效果（可叠层、可带 DOT）
    struct StatusEffectTemplate
    {
        // 状态类型（同 type 的效果在 stackable=true 时会合并）
        StatusEffectType type = StatusEffectType::POISONED;
        // 持续时间（秒）；当 refreshOnAdd=true 时，再次施加会刷新计时
        float duration = 0.0f;
        // 纯属性加成（不依赖 DOT）
        Attributes attributeBonus;

        // 叠层配置
        int stacks = 1;
        int maxStacks = 0;       // 0 = 不限制
        bool stackable = false;  // true 时按 type 合并并叠加 stacks
        bool refreshOnAdd = true;

        // DOT 配置：tickInterval>0 时，每 tick 造成 (baseDamageScale + perStackDamageScale * stacks) * sourceAttackPower
        float tickInterval = 0.0f;
        float baseDamageScale = 0.0f;
        float perStackDamageScale = 0.0f;
    };

    // 设置施法者/攻击者
    void setAttacker(CharacterBase *attacker) { _attacker = attacker; }
    // 获取施法者/攻击者
    CharacterBase *getAttacker() const { return _attacker; }

    // 设置基础伤害
    void setBaseDamage(float damage) { _baseDamage = damage; }
    // 获取基础伤害
    float getBaseDamage() const { return _baseDamage; }

    // 伤害倍率：最终伤害会额外叠加 attackerAttackPower * scale
    void setAttackPowerDamageScale(float scale) { _attackPowerDamageScale = scale; }
    // 获取伤害倍率
    float getAttackPowerDamageScale() const { return _attackPowerDamageScale; }

    // 设置爆炸半径
    void setExplosionRadius(float radius) { _explosionRadius = radius; }
    // 获取爆炸半径
    float getExplosionRadius() const { return _explosionRadius; }

    // 设置击破值：用于 Boss 的“击破条/韧性条”等机制（每次命中/爆炸结算都会累计）
    void setBreakDamage(int breakDamage) { _breakDamage = (breakDamage < 0) ? 0 : breakDamage; }
    // 获取击破值
    int getBreakDamage() const { return _breakDamage; }

    // 设置是否触碰即爆炸
    void setExplodeOnContact(bool explodeOnContact) { _explodeOnContact = explodeOnContact; }
    // 获取触碰爆炸开关
    bool getExplodeOnContact() const { return _explodeOnContact; }

    // 设置爆炸精灵特效参数
    void setExplosionSpriteVfx(const std::string &spritePath,
                               float scale,
                               float scaleUpDuration,
                               float scaleUpFactor,
                               float fadeOutDuration);

    // 设置爆炸帧动画特效
    void setExplosionFrameVfx(const std::vector<std::string> &framePaths,
                              float frameDelay,
                              float frameScale);

    // 设置飞行中的循环动画
    void setLoopAnimation(const std::vector<std::string> &framePaths, float frameDelay);

    // 添加命中附加状态效果
    void addOnHitStatusEffect(const StatusEffectTemplate &effect);
    // 清理所有命中附加状态
    void clearOnHitStatusEffects();

    // 是否已经爆炸
    bool isExploded() const { return _isExploded; }
    // 触发爆炸与伤害结算
    void explode();

protected:
    ExplosiveProjectile() = default;
    virtual ~ExplosiveProjectile() = default;

    // 处理爆炸范围伤害与附加状态
    virtual void applyAoEDamage();

    // 播放爆炸特效
    void playExplosionVfx();

private:
    bool _isExploded = false;
    bool _explodeOnContact = true;

    float _baseDamage = 0.0f;
    float _attackPowerDamageScale = 0.0f;
    float _explosionRadius = 0.0f;
    int _breakDamage = 0;
    CharacterBase *_attacker = nullptr;

    std::vector<StatusEffectTemplate> _onHitStatusEffects;

    std::vector<std::string> _loopAnimationPaths;
    float _loopAnimationDelay = 0.08f;

    std::vector<std::string> _explosionFramePaths;
    float _explosionFrameDelay = 0.05f;
    float _explosionFrameScale = 1.0f;

    std::string _explosionSpritePath;
    float _explosionSpriteScale = 1.0f;
    float _explosionSpriteScaleUpDuration = 0.2f;
    float _explosionSpriteScaleUpFactor = 1.2f;
    float _explosionSpriteFadeOutDuration = 0.3f;
};
