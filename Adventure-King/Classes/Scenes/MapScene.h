#pragma once
#ifndef __MAP_SCENE_H__
#define __MAP_SCENE_H__

#include "cocos2d.h"
#include "Configs/GameSceneConfig.h"
#include <vector>
#include <map>

class MapScene : public cocos2d::Scene
{
public:

    static cocos2d::Scene* createScene();
    virtual bool init() override;
    CREATE_FUNC(MapScene);

    struct MapMarkerInfo {
        SceneID id;
        std::string name;
        std::string normalImage;
        std::string selectedImage;
        cocos2d::Vec2 position;
        float scale;
    };

private:
    void onMapMarkerClicked(SceneID id);
    void enterMap(SceneID id);
    void mapCloseCallback(cocos2d::Ref* pSender);
    void updateMarkerTextures(const cocos2d::Vec2& mousePos);

    std::vector<cocos2d::Sprite*> _mapMarkers;
    std::map<SceneID, MapMarkerInfo> _markerMap;
    bool _isTransitioning = false;
};

#endif
