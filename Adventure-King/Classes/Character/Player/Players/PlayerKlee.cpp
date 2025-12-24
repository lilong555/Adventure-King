#include "PlayerKlee.h"
#include<vector>
#include<string>
#include"cocos2d.h"

std::vector<std::string> PlayerKlee::getPreloadResourcePaths() {
    std::vector<std::string> paths;

    // 1. 路径前缀与基础变量准备
    const std::string kleeBase = "Sprites/Characters/Player/Klee/default/";
    const std::string kleeRpg = "Sprites/Characters/Player/Klee/rocket/";
    const std::string kKey = "klee";

    // 2. 普通攻击与基础动作 (Normal Attack & Base)
    // 攻击动作帧 1-3
    for (int i = 1; i <= 3; ++i) {
        paths.push_back(cocos2d::StringUtils::format("%sspr_%s_attack_%d.png", kleeBase.c_str(), kKey.c_str(), i));
    }
    paths.push_back(kleeBase + "TNT.png");    // 炸弹模型
    paths.push_back(kleeBase + "BOOM_1.png"); // 基础爆炸特效

    // 3. 火球术技能资源 (Fireball Skill / Rocket Dir)
    // A. 技能动作帧 (根据 framePlan：1, 4, 5, 6)
    std::vector<int> fireballActionFrames = { 1, 4, 5, 6 };
    for (int f : fireballActionFrames) {
        paths.push_back(cocos2d::StringUtils::format("%sspr_%s_attack_%d.png", kleeRpg.c_str(), kKey.c_str(), f));
    }

    // B. 火球飞行尾迹特效 (rocket_trail_long 1-4)
    for (int i = 1; i <= 4; ++i) {
        paths.push_back(cocos2d::StringUtils::format("%sspr_vfx_rocket_trail_long_%d.png", kleeRpg.c_str(), i));
    }

    // C. 火球爆炸闪光特效 (explosion_flash 0-4)
    for (int i = 0; i <= 4; ++i) {
        paths.push_back(cocos2d::StringUtils::format("%sspr_vfx_explosion_flash_%d.png", kleeRpg.c_str(), i));
    }

    // 4. 补充基础状态帧 (确保 idle 和 beattacked 也在列表内)
    paths.push_back(kleeBase + "spr_klee_run.png");
    paths.push_back(kleeBase + "spr_klee_beattacked.png");

    return paths;
}
