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
        static std::unordered_set<std::string> s_cachedFileFrames;
        if (s_cachedFileFrames.find(frameNameOrFile) != s_cachedFileFrames.end())
        {
            auto cached = cache->getSpriteFrameByName(frameNameOrFile);
            if (cached)
            {
                return cached;
            }
            // 缓存可能被清理（例如 removeUnusedSpriteFrames），允许重建
            s_cachedFileFrames.erase(frameNameOrFile);
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
        s_cachedFileFrames.insert(frameNameOrFile);
        return frame;
    }
} // namespace SpriteFrameCacheHelper
