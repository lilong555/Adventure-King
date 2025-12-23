#pragma once

#include "cocos2d.h"
#include <string>
#include <unordered_set>

namespace SpriteFrameCacheHelper
{
    // 判断是否为文件路径（而非 plist 帧名）
    inline bool isFilePath(const std::string &nameOrPath)
    {
        return nameOrPath.find('/') != std::string::npos || nameOrPath.find('\\') != std::string::npos;
    }

    // 从缓存获取帧；若为文件路径则在首次使用时创建并缓存
    inline cocos2d::SpriteFrame *getOrCreateSpriteFrame(const std::string &frameNameOrFile)
    {
        auto cache = cocos2d::SpriteFrameCache::getInstance();
        if (!isFilePath(frameNameOrFile))
        {
            // 对于精灵表（plist）里的帧名，直接走 SpriteFrameCache（缺失时打印日志有助于排错）
            return cache->getSpriteFrameByName(frameNameOrFile);
        }

        // 对于文件路径（Sprites/...），避免先 getSpriteFrameByName 触发 “Frame isn't found” 的噪音日志：
        // 首次使用时直接按文件加载并加入缓存，后续再从 SpriteFrameCache 获取。
        static std::unordered_set<std::string> s_cachedFrameKeys;
        if (s_cachedFrameKeys.find(frameNameOrFile) != s_cachedFrameKeys.end())
        {
            auto cached = cache->getSpriteFrameByName(frameNameOrFile);
            if (cached)
            {
                return cached;
            }
            // 缓存可能被清理（例如 removeUnusedSpriteFrames），允许重建
            s_cachedFrameKeys.erase(frameNameOrFile);
        }

        auto textureCache = cocos2d::Director::getInstance()->getTextureCache();
        auto texture = textureCache ? textureCache->addImage(frameNameOrFile) : nullptr;
        if (!texture)
        {
            return nullptr;
        }

        auto size = texture->getContentSize();
        auto frame = cocos2d::SpriteFrame::createWithTexture(texture, cocos2d::Rect(0, 0, size.width, size.height));
        if (!frame)
        {
            return nullptr;
        }

        cache->addSpriteFrame(frame, frameNameOrFile);
        s_cachedFrameKeys.insert(frameNameOrFile);
        return frame;
    }

    // 从文件创建帧，并强制使用固定原始尺寸（避免动画帧尺寸变化导致内容尺寸抖动）
    inline cocos2d::SpriteFrame *getOrCreateSpriteFrameWithOriginalSize(const std::string &filePath,
                                                                        const cocos2d::Size &originalSize,
                                                                        bool alignBottom = false,
                                                                        bool alignLeft = false)
    {
        if (!isFilePath(filePath))
        {
            return cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(filePath);
        }

        // 注意：SpriteFrame::createWithTexture(texture, rect, rotated, offset, originalSize)
        // 这里的 rect / offset / originalSize 需要使用“像素”单位（引擎内部会再换算为 points）。
        // 如果误用 points，会导致 offsetPosition 计算异常，从而出现“角色移动时整体飞起来/漂移”的现象。

        const std::string cacheKey = filePath + "#orig=" +
                                     std::to_string(static_cast<int>(CC_SIZE_POINTS_TO_PIXELS(originalSize).width)) + "x" +
                                     std::to_string(static_cast<int>(CC_SIZE_POINTS_TO_PIXELS(originalSize).height)) +
                                     (alignBottom ? "#bottom" : "") +
                                     (alignLeft ? "#left" : "");

        auto cache = cocos2d::SpriteFrameCache::getInstance();
        if (auto cached = cache->getSpriteFrameByName(cacheKey))
        {
            return cached;
        }

        auto textureCache = cocos2d::Director::getInstance()->getTextureCache();
        auto texture = textureCache ? textureCache->addImage(filePath) : nullptr;
        if (!texture)
        {
            return nullptr;
        }

        const auto sizeInPixels = texture->getContentSizeInPixels();
        cocos2d::Rect rectInPixels(0, 0, sizeInPixels.width, sizeInPixels.height);

        auto originalSizeInPixels = CC_SIZE_POINTS_TO_PIXELS(originalSize);
        if (originalSizeInPixels.width <= 0.0f || originalSizeInPixels.height <= 0.0f)
        {
            // 未提供稳定尺寸时，退回使用当前纹理尺寸
            originalSizeInPixels = rectInPixels.size;
        }

        cocos2d::Vec2 offsetInPixels = cocos2d::Vec2::ZERO;
        if (alignBottom)
        {
            offsetInPixels.y = (rectInPixels.size.height - originalSizeInPixels.height) * 0.5f;
        }
        if (alignLeft)
        {
            offsetInPixels.x = (rectInPixels.size.width - originalSizeInPixels.width) * 0.5f;
        }

        auto frame = cocos2d::SpriteFrame::createWithTexture(texture, rectInPixels, false, offsetInPixels, originalSizeInPixels);
        if (!frame)
        {
            return nullptr;
        }

        cache->addSpriteFrame(frame, cacheKey);
        return frame;
    }
} // namespace SpriteFrameCacheHelper
