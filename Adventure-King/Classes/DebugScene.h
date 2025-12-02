#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class PlayerCharacter;

/**
 * @brief 角色功能调试场景
 * 用于测试角色的各种功能：受击、攻击、属性、状态机等
 */
class DebugScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(DebugScene);

private:
    // 初始化方法
    void initBackground();
    void initPlayer();
    void initDebugUI();
    void initControlButtons();

    // 更新方法
    virtual void update(float dt) override;
    void updateDebugInfo();

    // 按钮回调
    void onTakeDamageClicked(cocos2d::Ref *sender);
    void onTakeCriticalDamageClicked(cocos2d::Ref *sender);
    void onHealClicked(cocos2d::Ref *sender);
    void onAttackClicked(cocos2d::Ref *sender);
    void onLevelUpClicked(cocos2d::Ref *sender);
    void onResetClicked(cocos2d::Ref *sender);
    void onBackClicked(cocos2d::Ref *sender);

    // 键盘输入处理
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);

    // 成员变量
    PlayerCharacter *_player = nullptr;

    // 调试信息标签
    cocos2d::Label *_infoLabel = nullptr;
    cocos2d::Label *_stateLabel = nullptr;
    cocos2d::Label *_damageLogLabel = nullptr;

    // HP/MP 进度条
    cocos2d::ui::LoadingBar *_hpBar = nullptr;
    cocos2d::ui::LoadingBar *_mpBar = nullptr;

    // 伤害日志
    std::vector<std::string> _damageLog;
    static const size_t MAX_LOG_LINES = 5;

    void addDamageLog(const std::string &log);
};
