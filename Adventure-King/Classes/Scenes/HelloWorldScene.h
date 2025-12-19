#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__
#include "cocos2d.h"
class HelloWorld : public cocos2d::Scene
{
public:
    enum NodeTags
    { // 定义标签常量便于寻找结点
        TAG_CONTENT_CONTAINER = 5,
        TAG_MAP_MENU = 10,
        TAG_PLAYER = 15,
        TAG_SAVE_MENU = 20,
        //... 可以根据需要添加更多标签
    };
    /**
     * @brief 创建主菜单场景
     */
    static cocos2d::Scene *createScene();

    /**
     * @brief 创建带回调的菜单图片按钮
     */
    cocos2d::MenuItemImage *createMenuItem(
        const char *normal,
        const char *selected,
        const cocos2d::ccMenuCallback &callback);

    /**
     * @brief 初始化主菜单场景
     */
    virtual bool init();

    // a selector callback
    /// @brief 退出游戏
    void menuCloseCallback(cocos2d::Ref *pSender);
    /// @brief 进入游戏主场景
    void menuStartCallback(cocos2d::Ref *pSender);
    /// @brief 打开存档菜单
    void menuSaveCallback(cocos2d::Ref *pSender);
    /// @brief 打开地图选择
    void menuMapCallback(cocos2d::Ref *pSender);
    /// @brief 打开设置菜单
    void menuSetCallback(cocos2d::Ref *pSender);

    /// @brief 关闭存档菜单
    void menuSaveCloseCallback(cocos2d::Ref *pSender);
    // implement the "static create()" method manually
    CREATE_FUNC(HelloWorld);
};

#endif // __HELLOWORLD_SCENE_H__
