#pragma once
#pragma once
/**
 * @file CharacterAssetConfig.h
 * @brief 玩家各职业资源路径集中配置
 */
#include <string>
#include <vector>
#include "cocos2d.h"

namespace AssetRes {

    // ============================================================
    // 1. 法师 (Mage / Klee) 相关素材
    // ============================================================
    namespace Klee {
        const std::string BASE_DIR = "Sprites/Characters/Player/Klee/default/";
        const std::string RPG_DIR = "Sprites/Characters/Player/Klee/rocket/";
        const std::string KEY = "klee";

        inline std::vector<std::string> getAllPaths() {
            std::vector<std::string> p;
            // 基础动作
            for (int i = 1; i <= 3; ++i)
                p.push_back(cocos2d::StringUtils::format("%sspr_%s_attack_%d.png", BASE_DIR.c_str(), KEY.c_str(), i));

            p.push_back(BASE_DIR + "TNT.png");
            p.push_back(BASE_DIR + "BOOM_1.png");
            p.push_back(BASE_DIR + "spr_klee_run.png");
            p.push_back(BASE_DIR + "spr_klee_beattacked.png");

            // 火球技能 (序列帧 1, 4, 5, 6)
            for (int f : {1, 4, 5, 6})
                p.push_back(cocos2d::StringUtils::format("%sspr_%s_attack_%d.png", RPG_DIR.c_str(), KEY.c_str(), f));

            // 特效
            for (int i = 1; i <= 4; ++i)
                p.push_back(cocos2d::StringUtils::format("%sspr_vfx_rocket_trail_long_%d.png", RPG_DIR.c_str(), i));
            for (int i = 0; i <= 4; ++i)
                p.push_back(cocos2d::StringUtils::format("%sspr_vfx_explosion_flash_%d.png", RPG_DIR.c_str(), i));

            return p;
        }
    }

    // ============================================================
    // 2. 战士 (Warrior / Man) 相关素材
    // ============================================================
    namespace Warrior {
        const std::string BASE_DIR = "Sprites/Characters/Player/man/default/";

        inline std::vector<std::string> getAllPaths() {
            std::vector<std::string> p;
            p.reserve(15); // 预留空间

            // --- 基础状态与受击 ---
            p.push_back(BASE_DIR + "spr_man_beattacked.png"); // 受击
            p.push_back(BASE_DIR + "spr_man_run.png");        // 基础跑步/单帧参考

            // --- 序列帧动画 ---

            // 攻击动作 (1-3)
            for (int i = 1; i <= 3; ++i) {
                p.push_back(cocos2d::StringUtils::format("%sspr_man_attack_%d.png", BASE_DIR.c_str(), i));
            }

            // 跑步动作 (1-6)
            for (int i = 1; i <= 6; ++i) {
                p.push_back(cocos2d::StringUtils::format("%sspr_man_run_%d.png", BASE_DIR.c_str(), i));
            }

            // 如果后续有待机动作（如 spr_man_idle），建议也按此逻辑加入
            return p;
        }
    }

    // ============================================================
    // 3. 刺客 (Assassin / Maaer) 相关素材
    // ============================================================
    namespace Assassin {
        const std::string BASE_DIR = "Sprites/Characters/Player/maaer/default/";
        const std::string SLASH_DIR = "Sprites/Characters/Player/maaer/slash/";

        inline std::vector<std::string> getAllPaths() {
            std::vector<std::string> p;
            p.reserve(25); // 预留空间优化性能

            // --- 基础单图与状态 ---
            p.push_back(BASE_DIR + "15704_S.png");           // 特殊素材/头像
            p.push_back(BASE_DIR + "spr_maaer_beattacked.png"); // 受击
            p.push_back(BASE_DIR + "spr_maaer_run.png");        // 基础跑步帧

            // --- 序列帧动画 ---

            // 攻击动作 (1-4)
            for (int i = 1; i <= 4; ++i) {
                p.push_back(cocos2d::StringUtils::format("%sspr_maaer_attack_%d.png", BASE_DIR.c_str(), i));
            }

            // 待机动作 (1-4)
            for (int i = 1; i <= 4; ++i) {
                p.push_back(cocos2d::StringUtils::format("%sspr_maaer_idle_%d.png", BASE_DIR.c_str(), i));
            }

            // 跑步动作 (1-5)
            for (int i = 1; i <= 5; ++i) {
                p.push_back(cocos2d::StringUtils::format("%sspr_maaer_run_%d.png", BASE_DIR.c_str(), i));
            }

            // 技能：斩击 (Slash 1-4)
            for (int i = 1; i <= 4; ++i) {
                p.push_back(cocos2d::StringUtils::format("%sspr_maaer_slash_%d.png", SLASH_DIR.c_str(), i));
            }
            return p;
        }
    }

    inline std::vector<std::string> getSelectedRolePaths(CharacterRole role) {
        switch (role) {
        case CharacterRole::WARRIOR:
            return Warrior::getAllPaths();
        case CharacterRole::ASSASSIN:
            return Assassin::getAllPaths();
        case CharacterRole::MAGE:
        default:
            // 目前法师角色关联的是 Klee 的素材
            return Klee::getAllPaths();
        }
    }

} // namespace AssetRes
