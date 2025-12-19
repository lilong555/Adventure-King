#pragma once

#include "Character/Base/CharacterData.h"
#include "cocos2d.h"

class AttributeComponent;

class StatusEffectVfxComponent : public cocos2d::Component
{
public:
    /// @brief 创建组件实例
    CREATE_FUNC(StatusEffectVfxComponent);

    /// @brief 构造函数
    StatusEffectVfxComponent();
    /// @brief 析构函数
    ~StatusEffectVfxComponent() override;

    /// @brief 初始化组件
    bool init() override;
    /// @brief 组件挂载时回调
    void onAdd() override;
    /// @brief 每帧更新特效状态
    void update(float dt) override;

private:
    /// @brief 更新燃烧特效
    void updateBurningVfx(cocos2d::Node* owner, AttributeComponent* attr);
    /// @brief 获取指定状态的叠层数
    int getStacks(AttributeComponent* attr, StatusEffectType type) const;

    static const char* const BURNING_VFX_NAME;
};
