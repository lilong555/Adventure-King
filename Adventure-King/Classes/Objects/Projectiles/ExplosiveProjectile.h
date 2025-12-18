#pragma once

#include "Character/Base/CharacterBase.h"
#include "cocos2d.h"
#include <string>
#include <vector>

class ExplosiveProjectile : public cocos2d::Sprite
{
public:
    struct StatusEffectTemplate
    {
        StatusEffectType type = StatusEffectType::POISONED;
        float duration = 0.0f;
        Attributes attributeBonus;

        int stacks = 1;
        int maxStacks = 0;       // 0 = 不限制
        bool stackable = false;  // true 时按 type 合并并叠加 stacks
        bool refreshOnAdd = true;

        float tickInterval = 0.0f;
        float baseDamageScale = 0.0f;
        float perStackDamageScale = 0.0f;
    };

    void setAttacker(CharacterBase *attacker) { _attacker = attacker; }
    CharacterBase *getAttacker() const { return _attacker; }

    void setBaseDamage(float damage) { _baseDamage = damage; }
    float getBaseDamage() const { return _baseDamage; }

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
