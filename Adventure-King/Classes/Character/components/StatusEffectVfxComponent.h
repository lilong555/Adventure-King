#pragma once

#include "Character/Base/CharacterData.h"
#include "cocos2d.h"

class AttributeComponent;

class StatusEffectVfxComponent : public cocos2d::Component
{
public:
    CREATE_FUNC(StatusEffectVfxComponent);

    StatusEffectVfxComponent();
    ~StatusEffectVfxComponent() override;

    bool init() override;
    void onAdd() override;
    void update(float dt) override;

private:
    void updateBurningVfx(cocos2d::Node* owner, AttributeComponent* attr);
    int getStacks(AttributeComponent* attr, StatusEffectType type) const;

    static const char* const BURNING_VFX_NAME;
};

