#ifndef _APP_DELEGATE_H_
#define _APP_DELEGATE_H_

#include "cocos2d.h"

/**
@brief    The cocos2d Application.

Private inheritance here hides part of interface from Director.
*/
class AppDelegate : private cocos2d::Application
{
public:
    // 创建应用代理实例
    AppDelegate();
    // 析构时清理音频系统
    virtual ~AppDelegate();

    // 初始化 OpenGL 上下文参数
    virtual void initGLContextAttrs();

    /**
    @brief    在此处实现Director和Scene的初始化代码。
    @return true    初始化成功，应用程序继续。
    @return false   初始化失败，应用程序终止。
    */
    virtual bool applicationDidFinishLaunching();

    /**
    @brief  Called when the application moves to the background
    @param  the pointer of the application
    */
    virtual void applicationDidEnterBackground();

    /**
    @brief  Called when the application reenters the foreground
    @param  the pointer of the application
    */
    virtual void applicationWillEnterForeground();
};

#endif // _APP_DELEGATE_H_
