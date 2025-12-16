#pragma once

#include "cocos2d.h"
#include <cctype>
#include <string>

namespace SpriteFrameCacheHelper
{
    inline bool looksLikeFilePath(const std::string &nameOrPath)
    {
        if (nameOrPath.find('/') != std::string::npos || nameOrPath.find('\\') != std::string::npos)
        {
            return true;
        }

        auto dotPos = nameOrPath.rfind('.');
        if (dotPos == std::string::npos)
        {
            return false;
        }

        std::string ext = nameOrPath.substr(dotPos);
        for (auto &c : ext)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
    }

    inline cocos2d::SpriteFrame *getOrCreateSpriteFrame(const std::string &frameNameOrFile)
    {
        auto cache = cocos2d::SpriteFrameCache::getInstance();
        if (auto cached = cache->getSpriteFrameByName(frameNameOrFile))
        {
            return cached;
        }

        if (!looksLikeFilePath(frameNameOrFile))
        {
            return nullptr;
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
        return frame;
    }
} // namespace SpriteFrameCacheHelper
