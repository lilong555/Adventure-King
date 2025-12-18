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

    void setAttacker(CharacterBase *attacker) { _attacker = attacker; }
    CharacterBase *getAttacker() const { return _attacker; }

    void setBaseDamage(float damage) { _baseDamage = damage; }
    float getBaseDamage() const { return _baseDamage; }

    // 伤害倍率：最终伤害会额外叠加 attackerAttackPower * scale
    void setAttackPowerDamageScale(float scale) { _attackPowerDamageScale = scale; }
    float getAttackPowerDamageScale() const { return _attackPowerDamageScale; }

    void setExplosionRadius(float radius) { _explosionRadius = radius; }
    float getExplosionRadius() const { return _explosionRadius; }

    void setExplodeOnContact(bool explodeOnContact) { _explodeOnContact = explodeOnContact; }
    bool getExplodeOnContact() const { return _explodeOnContact; }

    void setExplosionSpriteVfx(const std::string &spritePath,
                               float scale,
                               float scaleUpDuration,
                               float scaleUpFactor,
                               float fadeOutDuration);

    void setExplosionFrameVfx(const std::vector<std::string> &framePaths,
                              float frameDelay,
                              float frameScale);

    void setLoopAnimation(const std::vector<std::string> &framePaths, float frameDelay);

    void addOnHitStatusEffect(const StatusEffectTemplate &effect);
    void clearOnHitStatusEffects();

    bool isExploded() const { return _isExploded; }
    void explode();

protected:
    ExplosiveProjectile() = default;
    virtual ~ExplosiveProjectile() = default;

    virtual void applyAoEDamage();

    void playExplosionVfx();

private:
    bool _isExploded = false;
    bool _explodeOnContact = true;

    float _baseDamage = 0.0f;
    float _attackPowerDamageScale = 0.0f;
    float _explosionRadius = 0.0f;
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
