# 粒子资源说明

## 武器命中判定粒子（Hitbox VFX）

**用途**：玩家在“生成攻击命中判定（hitbox）瞬间”播放一次粒子特效（可无）。

- **存放目录**：`Adventure-King/Resources/Particle/`
- **命名规则**：`par_weapon_hitbox_<weaponId>.plist`
  - `weaponId` 来源：[GameConfigs.h](../../Classes/Configs/GameConfigs.h) 中 `GameConfig::Equipment::Weapon::*`
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
