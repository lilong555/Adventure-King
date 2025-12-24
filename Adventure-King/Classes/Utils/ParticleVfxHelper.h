#pragma once

#include "Utils/PhysicsBodyLocalInfoHelper.h"
#include "cocos2d.h"
#include <functional>
#include <string>

// 粒子特效统一封装：
// - 统一 ParticleSystemQuad 的创建/挂载/常用参数设置，减少各处重复代码
// - 推荐配合 Utils/ParticlePreloadHelper.h 在 LoadingScene / 预加载阶段预热 plist，
//   把“首次解码内嵌纹理/上传 GPU”的卡顿挪到加载阶段完成
namespace ParticleVfxHelper
{
    struct PlayOptions
    {
        int zOrder = 999; ///< addChild 层级
        cocos2d::ParticleSystem::PositionType positionType = cocos2d::ParticleSystem::PositionType::GROUPED; ///< 跟随角色移动更自然
        bool autoRemoveOnFinish = true; ///< 播放结束自动销毁
        bool resetSystem = true; ///< 添加后是否 resetSystem（保证立即发射）
        bool useBodyCenter = true; ///< 是否默认对齐到物理体中心
        cocos2d::Vec2 position = cocos2d::Vec2::ZERO; ///< useBodyCenter=false 时使用的本地坐标
        std::string name; ///< 可选：节点名（便于查找/防重复）
    };

    using ParticleCustomizer = std::function<void(cocos2d::ParticleSystemQuad* particle)>;

    inline cocos2d::ParticleSystemQuad* playOnce(cocos2d::Node* owner,
                                                 const std::string& plistPath,
                                                 const PlayOptions& options = {},
                                                 const ParticleCustomizer& customizer = nullptr)
    {
        if (!owner || plistPath.empty())
        {
            return nullptr;
        }

        auto particle = cocos2d::ParticleSystemQuad::create(plistPath);
        if (!particle)
        {
            return nullptr;
        }

        if (!options.name.empty())
        {
            particle->setName(options.name);
        }

        particle->setAutoRemoveOnFinish(options.autoRemoveOnFinish);
        particle->setPositionType(options.positionType);

        cocos2d::Vec2 pos = options.position;
        if (options.useBodyCenter)
        {
            const auto bodyInfo = PhysicsBodyLocalInfoHelper::getBodyLocalInfo(owner);
            pos = bodyInfo.center;
        }
        particle->setPosition(pos);

        if (customizer)
        {
            customizer(particle);
        }

        owner->addChild(particle, options.zOrder);

        if (options.resetSystem)
        {
            particle->resetSystem();
        }

        return particle;
    }
} // namespace ParticleVfxHelper

