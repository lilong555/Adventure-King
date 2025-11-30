#ifndef __MAP_SCENE_H__
#define __MAP_SCENE_H__

#include "cocos2d.h"
#include <vector>
#include <string>
// 地图信息结构体
struct MapInfo
{
    int id;                     // 地图ID
    std::string name;           // 地图名称
    std::string backgroundPath; // 背景图片路径
    std::string iconPath;       // 地图图标路径
    bool isUnlocked;            // 是否解锁
    int difficulty;             // 难度等级
};
class MapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene *createScene();
    enum NodeTags
    { 
        TAG_CONTENT_CONTAINER = 5,
        TAG_MAP_MENU = 10,
    };
    cocos2d::MenuItemImage *createMenuItem(
        const char *normal,
        const char *selected,
        const cocos2d::ccMenuCallback &callback);

    virtual bool init();

    // 回调函数
    void mapCloseCallback(cocos2d::Ref *pSender);
    void mapSelectCallback(cocos2d::Ref *pSender);

    // 地图操作方法
    MapInfo *getMapById(int id);              // 根据ID获取地图
    void unlockMap(int id);                   // 解锁地图
    const std::vector<MapInfo> &getAllMaps(); // 获取所有地图
    CREATE_FUNC(MapScene);

private:
    std::vector<MapInfo> _mapList; // 地图数组
    int _currentMapIndex;          // 当前选中的地图索引
};

#endif // __MAP_SCENE_H__
