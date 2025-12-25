#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__
#include "cocos2d.h"
#include "Character/Base/CharacterData.h"
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
    // implement the "static create()" method manually
    static std::vector<std::string> getPreloadResourcePaths();
    static void setupRegistry();
    CREATE_FUNC(HelloWorld);

private:
    // 主菜单会话内的职业选择：用于“新开局”
    CharacterRole _selectedRole = CharacterRole::MAGE;
    cocos2d::Label* _roleHintLabel = nullptr;

    void updateRoleHintLabel();

    // 点击“开始游戏”后弹出的职业选择层
    cocos2d::LayerColor* _roleSelectLayer = nullptr;
    cocos2d::Sprite* _rolePreviewSprite = nullptr;
    cocos2d::MenuItem* _pendingStartMenuItem = nullptr; // 用于取消时恢复按钮可点

    void showRoleSelectLayer(cocos2d::MenuItem* startMenuItem);
    void hideRoleSelectLayer(bool restoreStartButton);
    void refreshRolePreview();
    void startGameWithSelectedRole();
};

#endif // __HELLOWORLD_SCENE_H__
