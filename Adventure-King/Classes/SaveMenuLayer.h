#pragma once
#pragma once
#include "cocos2d.h"

class SaveMenuLayer : public cocos2d::Layer
{
public:
    static SaveMenuLayer* create();

    virtual bool init() override;

    void onClose(cocos2d::Ref* sender);

private:
    cocos2d::Sprite* _background = nullptr;
    virtual bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event) override;
    bool initBackground();
    bool initCloseButton();
    void layoutUI();
};
