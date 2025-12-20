#pragma once

#include "cocos2d.h"
#include <algorithm>
#include <vector>

// 物理体范围工具：将 PhysicsBody 的所有 Shape 投影到 Owner 本地坐标系，
// 统一用于 VFX 定位/范围计算，避免在多个组件里重复实现。
namespace PhysicsBodyLocalInfoHelper
{
    struct BodyLocalInfo
    {
        cocos2d::Size size = cocos2d::Size::ZERO;
        cocos2d::Vec2 center = cocos2d::Vec2::ZERO;
    };

    inline void updateBounds(const cocos2d::Vec2& point,
                             bool& hasPoint,
                             cocos2d::Vec2& minPoint,
                             cocos2d::Vec2& maxPoint)
    {
        if (!hasPoint)
        {
            minPoint = point;
            maxPoint = point;
            hasPoint = true;
            return;
        }
        minPoint.x = std::min(minPoint.x, point.x);
        minPoint.y = std::min(minPoint.y, point.y);
        maxPoint.x = std::max(maxPoint.x, point.x);
        maxPoint.y = std::max(maxPoint.y, point.y);
    }

    inline void addLocalPointFromWorld(const cocos2d::Node* owner,
                                       const cocos2d::Vec2& worldPoint,
                                       bool& hasPoint,
                                       cocos2d::Vec2& minPoint,
                                       cocos2d::Vec2& maxPoint)
    {
        if (!owner)
        {
            return;
        }
        cocos2d::Vec2 localPoint = owner->convertToNodeSpace(worldPoint);
        updateBounds(localPoint, hasPoint, minPoint, maxPoint);
    }

    inline void addLocalPointFromBody(const cocos2d::Node* owner,
                                      cocos2d::PhysicsBody* body,
                                      const cocos2d::Vec2& bodyLocalPoint,
                                      bool& hasPoint,
                                      cocos2d::Vec2& minPoint,
                                      cocos2d::Vec2& maxPoint)
    {
        if (!owner || !body)
        {
            return;
        }
        cocos2d::Vec2 worldPoint = body->local2World(bodyLocalPoint);
        addLocalPointFromWorld(owner, worldPoint, hasPoint, minPoint, maxPoint);
    }

    inline BodyLocalInfo getBodyLocalInfo(cocos2d::Node* owner)
    {
        BodyLocalInfo info;
        if (!owner)
        {
            return info;
        }

        bool hasPoint = false;
        cocos2d::Vec2 minPoint;
        cocos2d::Vec2 maxPoint;

        if (auto body = owner->getPhysicsBody())
        {
            const auto& shapes = body->getShapes();
            for (auto shape : shapes)
            {
                if (!shape)
                {
                    continue;
                }

                if (auto circle = dynamic_cast<cocos2d::PhysicsShapeCircle*>(shape))
                {
                    const cocos2d::Vec2 center = circle->getCenter();
                    const float radius = circle->getRadius();
                    addLocalPointFromBody(owner, body, center + cocos2d::Vec2(radius, 0.0f), hasPoint, minPoint, maxPoint);
                    addLocalPointFromBody(owner, body, center + cocos2d::Vec2(-radius, 0.0f), hasPoint, minPoint, maxPoint);
                    addLocalPointFromBody(owner, body, center + cocos2d::Vec2(0.0f, radius), hasPoint, minPoint, maxPoint);
                    addLocalPointFromBody(owner, body, center + cocos2d::Vec2(0.0f, -radius), hasPoint, minPoint, maxPoint);
                    continue;
                }

                if (auto poly = dynamic_cast<cocos2d::PhysicsShapePolygon*>(shape))
                {
                    const int count = poly->getPointsCount();
                    if (count <= 0)
                    {
                        continue;
                    }
                    std::vector<cocos2d::Vec2> points(static_cast<size_t>(count));
                    poly->getPoints(points.data());
                    for (const auto& point : points)
                    {
                        addLocalPointFromBody(owner, body, point, hasPoint, minPoint, maxPoint);
                    }
                }
            }
        }

        if (!hasPoint)
        {
            cocos2d::Rect bbox = owner->getBoundingBox();
            cocos2d::Vec2 originWorld = bbox.origin;
            cocos2d::Vec2 topRightWorld = bbox.origin + bbox.size;
            if (auto parent = owner->getParent())
            {
                originWorld = parent->convertToWorldSpace(bbox.origin);
                topRightWorld = parent->convertToWorldSpace(bbox.origin + bbox.size);
            }
            addLocalPointFromWorld(owner, originWorld, hasPoint, minPoint, maxPoint);
            addLocalPointFromWorld(owner, topRightWorld, hasPoint, minPoint, maxPoint);
        }

        if (hasPoint)
        {
            info.size = cocos2d::Size(std::max(0.0f, maxPoint.x - minPoint.x),
                                      std::max(0.0f, maxPoint.y - minPoint.y));
            info.center = cocos2d::Vec2((minPoint.x + maxPoint.x) * 0.5f,
                                        (minPoint.y + maxPoint.y) * 0.5f);
        }
        return info;
    }
} // namespace PhysicsBodyLocalInfoHelper

