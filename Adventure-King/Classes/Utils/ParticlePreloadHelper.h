#pragma once

#include "cocos2d.h"
#include <string>
#include <vector>

// 粒子预热工具：
// - 背景：粒子 plist 支持内嵌纹理（textureImageData）。首次创建粒子时仍可能发生“解码纹理 + 上传 GPU”，
//   若在战斗/技能触发瞬间发生，会造成明显掉帧或卡顿。
// - 用法：在 LoadingScene / 地图预加载阶段提前 create 一次，把首次开销挪到加载阶段完成。
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
        "Particle/par_warfire_2.plist"
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

            // 显式触发一次取纹理，确保内嵌纹理被解码并上传到 GPU/TextureCache，即使不渲染粒子（不需要 addChild）
            (void)particle->getTexture();
        }
    }

    inline void preloadCommonParticles()
    {
        preloadParticlePlists(getCommonParticlePlists());
    }
} // namespace ParticlePreloadHelper
