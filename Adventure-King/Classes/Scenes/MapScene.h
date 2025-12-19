#ifndef __MAP_SCENE_H__
#define __MAP_SCENE_H__

#include "cocos2d.h"
#include <vector>
#include <string>

class MapScene : public cocos2d::Scene
{
public:
    /**
     * @brief 创建地图选择场景
     */
    static cocos2d::Scene *createScene();
    /**
     * @brief 析构：清理地图标记资源
     */
    ~MapScene() override;
    enum NodeTags
    {
        TAG_CONTENT_CONTAINER = 5,
        TAG_MAP_MENU = 10,
    };
    /**
     * @brief 创建菜单项图片按钮
     */
    cocos2d::MenuItemImage *createMenuItem(
        const char *normal,
        const char *selected,
        const cocos2d::ccMenuCallback &callback);

    /**
     * @brief 初始化地图选择界面
     */
    virtual bool init();

    // 回调函数
    /// @brief 关闭地图场景
    void mapCloseCallback(cocos2d::Ref *pSender);
    /// @brief 选择地图（旧入口，保留兼容）
    void mapSelectCallback(cocos2d::Ref *pSender);

    // 地图操作方法
    /// @brief 解锁指定地图
    void unlockMap(int id);
    /// @brief 地标点击回调
    void onMapMarkerClicked(int mapId);
    CREATE_FUNC(MapScene);
    // 地图标记信息结构体
    struct MapMarkerInfo
    {
        std::string normalImage;   // 正常状态图片路径
        std::string selectedImage; // 选中状态图片路径
        cocos2d::Vec2 position;    // 相对位置
        int mapId;                 // 地图ID
        float scale;               // 缩放比例
        std::string name;          // 地图名称
    };

private:
    int _currentMapIndex;                       // 当前选中的地图索引
    std::vector<cocos2d::Sprite *> _mapMarkers; // 地标精灵列表
    std::vector<MapMarkerInfo> _markerInfos;    // 地图标记数据列表

    // 创建目标场景（进入地图后的场景）
    /// @brief 根据 mapId 创建目标关卡场景
    cocos2d::Scene *createDestinationScene(int mapId);
};

#endif // __MAP_SCENE_H__
