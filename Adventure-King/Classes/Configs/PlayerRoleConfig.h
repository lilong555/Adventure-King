#pragma once

#include "Character/Base/CharacterData.h"
#include "Configs/GameSceneConfig.h"

// 玩家职业与资源/展示名映射
// 说明：
// - 当前项目素材命名尚未完全统一，因此这里先做“职业 -> 默认贴图路径”的集中管理，避免散落在各处写死。
// - 后续如果补齐标准命名（spr_<key>_run.png / spr_<key>_attack_x.png 等），只需要在这里调整默认入口即可。
namespace PlayerRoleConfig
{
    inline const char* getDefaultSpritePath(CharacterRole role)
    {
        switch (role)
        {
        case CharacterRole::WARRIOR:
            return "Sprites/Characters/Player/man/1765093576488638789_6025_01.png";
        case CharacterRole::ASSASSIN:
            return "Sprites/Characters/Player/maaer/spr_male_run_1.png";
        case CharacterRole::MAGE:
        default:
            // 目前“法师”沿用 Klee 的素材与技能实现
            return GameSceneConfig::Scene::DEFAULT_PLAYER_SPRITE;
        }
    }

    inline const char* getDisplayName(CharacterRole role)
    {
        switch (role)
        {
        case CharacterRole::WARRIOR:
            return "战士";
        case CharacterRole::ASSASSIN:
            return "刺客";
        case CharacterRole::MAGE:
            return "法师";
        case CharacterRole::TANK:
            return "坦克";
        default:
            return "未知职业";
        }
    }
} // namespace PlayerRoleConfig

