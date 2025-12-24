#pragma once

#include "cocos2d.h"

#include <string>

// 武器命中判定（hitbox）特效约定：
// - 需求：在“生成攻击 hitbox 的瞬间”播放一次粒子特效（可无）。
// - 存放位置：Adventure-King/Resources/Particle/
// - 默认命名：Particle/par_weapon_hitbox_<weaponId>.plist
//   例如：
//   - 新手剑(5001)：Particle/par_weapon_hitbox_5001.plist
//   - 血契短剑(5004)：Particle/par_weapon_hitbox_5004.plist
//
// 说明：
// - plist 支持内嵌纹理（textureImageData）。为避免首次触发卡顿，请在 LoadingScene 预热阶段调用
//   Utils/ParticlePreloadHelper.h 的预热接口（本项目已在 preloadCommonParticles 中自动尝试预热“已存在”的武器 hitbox plist）。
namespace WeaponHitboxVfxHelper
{
    inline std::string getDefaultHitboxVfxPlistPath(int weaponId)
    {
        return cocos2d::StringUtils::format("Particle/par_weapon_hitbox_%d.plist", weaponId);
    }

    inline std::string resolveHitboxVfxPlistPath(int weaponId)
    {
        if (weaponId <= 0)
        {
            return {};
        }

        auto fileUtils = cocos2d::FileUtils::getInstance();
        if (!fileUtils)
        {
            return {};
        }

        const std::string path = getDefaultHitboxVfxPlistPath(weaponId);
        if (!fileUtils->isFileExist(path))
        {
            return {};
        }
        return path;
    }
} // namespace WeaponHitboxVfxHelper

