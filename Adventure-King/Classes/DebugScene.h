#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class PlayerCharacter;

/**
 * @brief 炸弹结构体
 */
struct Bomb
{
    cocos2d::Sprite *sprite = nullptr;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    bool isExploded = false; // 是否已爆炸
};

/**
 * @brief 木桩（靶子）结构体
 */
struct TargetDummy
{
    cocos2d::Sprite *sprite = nullptr;
    float maxHP = 1000.0f;
    float currentHP = 1000.0f;
    cocos2d::DrawNode *hpBar = nullptr;
    cocos2d::Label *hpLabel = nullptr;
};

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
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event);

    // 移动相关
    void updatePlayerMovement(float dt);
    void startWalkAnimation();
    void stopWalkAnimation();

    // 物理系统相关
    void initPlatforms();
    void updateGravity(float dt);
    bool checkGrounded();
    void jump();

    // 攻击相关
    void playAttackAnimation();
    void onAttackAnimationFinished();

    // 技能相关
    void playSkillAnimation();
    void onSkillAnimationFinished();
    void throwBomb();
    void doThrowBomb(); // 实际丢出炸弹的逻辑
    void updateBombs(float dt);
    void explodeBomb(Bomb &bomb);

    // 木桩相关
    void initTargetDummy();
    void dealDamageToTarget(float damage, bool isCrit = false);
    void showDamageNumber(const cocos2d::Vec2 &pos, float damage, bool isCrit = false);
    void updateTargetHPBar();

    // 成员变量
    PlayerCharacter *_player = nullptr;

    // 移动状态
    bool _isMovingLeft = false;
    bool _isMovingRight = false;
    float _moveSpeed = 200.0f; // 移动速度（像素/秒）
    bool _isWalkAnimationPlaying = false;

    // 攻击状态
    bool _isAttacking = false;

    // 技能施放状态
    bool _isCastingSkill = false;

    // 物理系统
    float _velocityY = 0.0f;                    // 垂直速度
    bool _isGrounded = false;                   // 是否在地面上
    static constexpr float GRAVITY = -800.0f;   // 重力加速度
    static constexpr float JUMP_FORCE = 655.0f; // 跳跃力度
    static constexpr float GROUND_Y = 100.0f;   // 地面Y坐标

    // 角色碰撞箱 (相对于角色锚点的偏移)
    // 锚点在角色脚底中心 (0.5, 0)
    cocos2d::Rect _collisionBox;                     // 碰撞箱
    cocos2d::DrawNode *_collisionBoxDebug = nullptr; // 碰撞箱可视化（调试用）

    // 平台列表 (x, y, width, height)
    std::vector<cocos2d::Rect> _platforms;

    // 炸弹列表
    std::vector<Bomb> _bombs;
    static constexpr float BOMB_THROW_SPEED_X = 300.0f;   // 炸弹水平初速度
    static constexpr float BOMB_THROW_SPEED_Y = 350.0f;   // 炸弹垂直初速度
    static constexpr float BOMB_GRAVITY = -600.0f;        // 炸弹重力
    static constexpr float BOMB_DAMAGE = 150.0f;          // 炸弹伤害
    static constexpr float BOMB_EXPLOSION_RADIUS = 80.0f; // 爆炸半径

    // 木桩（靶子）
    TargetDummy _targetDummy;

    // 调试信息标签
    cocos2d::Label *_infoLabel = nullptr;
    cocos2d::Label *_stateLabel = nullptr;
    cocos2d::Label *_damageLogLabel = nullptr;
    cocos2d::Label *_hpLabel = nullptr;
    cocos2d::Label *_mpLabel = nullptr;

    // 伤害日志
    std::vector<std::string> _damageLog;
    static const size_t MAX_LOG_LINES = 5;

    void addDamageLog(const std::string &log);
};
