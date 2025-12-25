# 粒子特效规范（Particle VFX）

## 目标

- 统一粒子创建/挂载方式，减少重复代码
- 将"首次创建粒子"的开销挪到加载阶段，避免战斗/触发瞬间卡顿

## 预热（Preload）

粒子 plist 可能包含 `textureImageData`（内嵌纹理）。首次 `ParticleSystemQuad::create()` 时会发生解码与贴图上传，
如果发生在战斗/技能触发瞬间容易掉帧。

- 在 `LoadingScene` / 场景预加载阶段调用：
  - `ParticlePreloadHelper::preloadCommonParticles();`
  - 或 `ParticlePreloadHelper::preloadParticlePlists({...});`

文件：`Adventure-King/Classes/Utils/ParticlePreloadHelper.h`

## 播放（Play）

一次性粒子（如升级、回血、命中提示、技能特效）统一使用：

- `ParticleVfxHelper::playOnce(owner, "Particle/xxx.plist");`
- `ParticleVfxHelper::playOnce(owner, "Particle/xxx.plist", options);` - 带自定义选项

### PlayOptions 参数

```cpp
struct PlayOptions {
    int zOrder = 999;                                    // 渲染层级
    ParticleSystem::PositionType positionType = GROUPED; // 粒子位置类型
    std::string name;                                    // 节点名称（用于调试/查找）
    bool useBodyCenter = true;                           // 是否对齐到物理体中心
    Vec2 position;                                       // 自定义位置（useBodyCenter=false 时生效）
};
```

默认行为：

- `PositionType::GROUPED`
- 对齐到物理体中心（`PhysicsBodyLocalInfoHelper`）
- `autoRemoveOnFinish = true`
- `zOrder = 999`

文件：`Adventure-King/Classes/Utils/ParticleVfxHelper.h`

## 技能粒子

### 战士 Fire 技能
- 粒子文件：`Particle/par_fire.plist`
- 播放位置：命中框底部中点
- 挂载节点：`combatLayer`（世界层），不随 hitbox 销毁
- 使用示例：
```cpp
ParticleVfxHelper::PlayOptions options;
options.zOrder = 2;
options.useBodyCenter = false;
options.position = Vec2(center.x, center.y - hitboxSize.height * 0.5f);
ParticleVfxHelper::playOnce(combatLayer, "Particle/par_fire.plist", options);
```

### 升级特效
- 粒子文件：`Particle/par_levelup.plist`
- 播放位置：角色中心
- 触发时机：角色升级时

### 恢复生命特效
- 粒子文件：`Particle/par_Restore_health.plist`
- 播放位置：角色中心
- 节流：`CharacterBase::setCurrentHP` 内置 `0.5s` 冷却

## 节流（Throttle）

对"短时间可能多次触发"的粒子（例如回血）需要节流，避免刷屏与性能抖动。

- 当前实现：`CharacterBase::setCurrentHP` 内置 `0.5s` 冷却（`_lastRestoreHealthVfxMs`）
- 新增类似粒子建议遵循同样策略（冷却时间可按视觉效果与触发频率调节）

## 粒子资源清单

| 文件名 | 用途 | 触发位置 |
|--------|------|----------|
| `par_chararcter_hurt_L.plist` | 受击粒子（左） | `CharacterBase::spawnHurtVfx` |
| `par_chararcter_hurt_R.plist` | 受击粒子（右） | `CharacterBase::spawnHurtVfx` |
| `par_warfire.plist` | 主菜单战火背景 | `HelloWorldScene` |
| `par_fire.plist` | 战士 Fire 技能 | `WarriorSkillSet::tryUseSkill` |
| `par_dragon_fire.plist` | 龙焰特效 | 预留 |
| `par_levelup.plist` | 升级特效 | `CharacterBase::addExp` |
| `par_Restore_health.plist` | 恢复生命 | `CharacterBase::setCurrentHP` |
| `par_GobluRemoteHit.plist` | Goblu远程命中 | `GobluMonster` |
| `par_Poison.plist` | 中毒效果 | `StatusEffectVfxComponent` |
| `par_nap.plist` | 睡眠/眩晕效果 | 预留 |
| `par_weapon_hitbox_*.plist` | 武器命中判定粒子 | `PlayerCharacter::spawnPlayerAttackHitbox` |

## 注意事项

- 不要手动指定 `particle_texture.png` 作为纹理：项目内多份粒子 plist 设计为内嵌纹理
- 新增粒子后按需加入 `ParticlePreloadHelper::getCommonParticlePlists()`，确保加载阶段预热
- 技能粒子挂载到 `combatLayer` 而非 hitbox，避免粒子随 hitbox 销毁而提前结束

