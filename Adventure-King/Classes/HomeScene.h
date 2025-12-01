#ifndef __GAMESCENE_H__
#define __GAMESCENE_H__

#include "cocos2d.h"

class HomeScene : public cocos2d::Scene
{
public:
    // 静态创建方法：用于生成场景实例
    static cocos2d::Scene *createScene();

    // 初始化方法：用于设置场景内容
    virtual bool init();
    void menuReturnCallback(Ref *pSender);
    // 创建宏
    CREATE_FUNC(HomeScene);

private:
    // 可以在这里添加游戏逻辑或变量
};

#endif // __GAMESCENE_H__