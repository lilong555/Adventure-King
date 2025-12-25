# 粒子资源说明

## 粒子文件清单

| 文件名 | 用途 | 说明 |
|--------|------|------|
| `par_chararcter_hurt_L.plist` | 受击粒子（左） | 攻击来自右侧时播放 |
| `par_chararcter_hurt_R.plist` | 受击粒子（右） | 攻击来自左侧时播放 |
| `par_warfire.plist` | 主菜单战火背景 | 循环播放 |
| `par_warfire1.plist` | 战火变体1 | 备用 |
| `par_warfire_2.plist` | 战火变体2 | 备用 |
| `par_fire.plist` | 战士 Fire 技能 | 火焰冲击特效 |
| `par_dragon_fire.plist` | 龙焰特效 | 预留 |
| `par_levelup.plist` | 升级特效 | 角色升级时播放 |
| `par_Restore_health.plist` | 恢复生命 | 回血时播放（有节流） |
| `par_GobluRemoteHit.plist` | Goblu远程命中 | Boss远程攻击命中特效 |
| `par_Poison.plist` | 中毒效果 | 中毒状态视觉 |
| `par_nap.plist` | 睡眠/眩晕效果 | 预留 |
| `particle_texture.png` | 通用粒子纹理 | 部分粒子使用 |

## 武器命中判定粒子（Hitbox VFX）

**用途**：玩家在"生成攻击命中判定（hitbox）瞬间"播放一次粒子特效（可无）。

- **存放目录**：`Adventure-King/Resources/Particle/`
- **命名规则**：`par_weapon_hitbox_<weaponId>.plist`
  - `weaponId` 来源：[GameConfig.h](../../Classes/Configs/GameConfig.h) 中 `GameConfig::Equipment::Weapon::*`
  - 示例：
    - 新手剑 `5001` → `par_weapon_hitbox_5001.plist`
    - 血契短剑 `5004` → `par_weapon_hitbox_5004.plist`

**预加载/预热**：

- `LoadingScene` 预加载阶段会自动扫描 `Particle` 目录并预热所有 `par_weapon_hitbox_*.plist`，用于避免首次触发时的卡顿。
- 若未放置对应 plist，将不会播放，也不会刷缺失日志。

**实现入口（供程序定位）**：

- 播放逻辑：`Adventure-King/Classes/Character/Player/PlayerCharacter.cpp` 的 `PlayerCharacter::spawnPlayerAttackHitbox`
- 路径解析：`Adventure-King/Classes/Utils/WeaponHitboxVfxHelper.h`
- 预热逻辑：`Adventure-King/Classes/Utils/ParticlePreloadHelper.h`

## 新增粒子指南

1. 将 `.plist` 文件放入 `Resources/Particle/` 目录
2. 如需预热，在 `ParticlePreloadHelper::getCommonParticlePlists()` 中添加路径
3. 使用 `ParticleVfxHelper::playOnce` 播放一次性粒子
4. 如需自定义位置/层级，使用 `PlayOptions` 参数

详细规范参见：[VFX.md](../../VFX.md)
