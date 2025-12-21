#pragma once

#include "cocos2d.h"
#include <string>
#include <vector>

// 粒子预热工具：
// - 对于使用 plist 内嵌纹理（textureImageData）的粒子，首次创建时仍会解码并上传贴图到 GPU；
// - 通过在 LoadingScene/地图预热阶段提前 create 一次，可把“首次卡顿”放到加载阶段完成。
namespace ParticlePreloadHelper
{
    inline std::vector<std::string> getCommonParticlePlists()
    {
        return {
            "Particle/par_chararcter_hurt_L.plist",
            "Particle/par_chararcter_hurt_R.plist",
            "Particle/par_levelup.plist",
            "Particle/par_Restore_health.plist",
            "Particle/par_Poison.plist",
            "Particle/par_GobluRemoteHit.plist",
            "Particle/par_dragon_fire.plist",
            "Particle/par_warfire.plist",
            "Particle/par_warfire1.plist",
            "Particle/par_warfire_2.plist",
        };
    }

    inline void preloadParticlePlists(const std::vector<std::string>& plistPaths)
    {
        auto fileUtils = cocos2d::FileUtils::getInstance();
        auto textureCache = cocos2d::Director::getInstance()->getTextureCache();
        if (!fileUtils || !textureCache)
        {
            return;
        }

        for (const auto& path : plistPaths)
        {
            if (path.empty())
            {
                continue;
            }

            // Cocos2d-x 在使用 textureImageData 时，会以“plist 的 fullPath”作为贴图 key（textureFileName 为空时）。
            const std::string fullPath = fileUtils->fullPathForFilename(path);
            if (fullPath.empty())
            {
                CCLOG("ParticlePreload: 粒子文件不存在：%s", path.c_str());
                continue;
            }

            if (textureCache->getTextureForKey(fullPath))
            {
                continue; // 已预热过
            }

            // 预热：创建一次粒子即可触发内嵌纹理解码 & 贴图入 TextureCache
            auto particle = cocos2d::ParticleSystemQuad::create(path);
            if (!particle)
            {
                CCLOG("ParticlePreload: 创建失败：%s", path.c_str());
                continue;
            }

            // 显式触发一次取纹理，确保贴图已创建（不需要 addChild）
            (void)particle->getTexture();
        }
    }

    inline void preloadCommonParticles()
    {
        preloadParticlePlists(getCommonParticlePlists());
    }
} // namespace ParticlePreloadHelper

