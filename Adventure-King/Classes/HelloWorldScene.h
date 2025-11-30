

#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"



class HelloWorld : public cocos2d::Scene
{
public:
    enum NodeTags {//定义标签常量便于寻找结点
        TAG_CONTENT_CONTAINER = 5,
        TAG_MAP_MENU = 10,
		TAG_PLAYER = 15,
		TAG_SAVE_MENU = 20,
		//... 可以根据需要添加更多标签
    };
    static cocos2d::Scene *createScene();

    cocos2d::MenuItemImage* createMenuItem(
        const char* normal,
        const char* selected,
        const cocos2d::ccMenuCallback& callback
    );

    virtual bool init();

    // a selector callback
    void menuCloseCallback(cocos2d::Ref *pSender);
    void menuStartCallback(cocos2d::Ref *pSender);
    void menuSaveCallback(cocos2d::Ref *pSender);
    void menuMapCallback(cocos2d::Ref *pSender);
    void menuSetCallback(cocos2d::Ref *pSender);

    void menuSaveCloseCallback(cocos2d::Ref* pSender);
    // implement the "static create()" method manually
    CREATE_FUNC(HelloWorld);
};

#endif // __HELLOWORLD_SCENE_H__
