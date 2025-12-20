#pragma once

#include "Character/Base/CharacterData.h"
#include "cocos2d.h"

class AttributeComponent;

class StatusEffectVfxComponent : public cocos2d::Component
{
public:
    /// @brief 创建组件实例
    CREATE_FUNC(StatusEffectVfxComponent);

    // 燃烧特效调参（在头文件修改即可生效）
    struct BurningVfxParams
    {
        static constexpr float EMITTER_OFFSET_Y_RATIO = 0.15f;
        static constexpr float POS_VAR_X_RATIO = 0.12f;
        static constexpr float POS_VAR_Y_RATIO = 0.10f;
        static constexpr float POS_VAR_X_MAX = 70.0f;
        static constexpr float POS_VAR_Y_MAX = 110.0f;
        static constexpr float MAX_START_SIZE = 18.0f;
    };

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
