# 粒子特效规范（Particle VFX）

## 目标

- 统一粒子创建/挂载方式，减少重复代码
- 将“首次创建粒子”的开销挪到加载阶段，避免战斗/触发瞬间卡顿

## 预热（Preload）

粒子 plist 可能包含 `textureImageData`（内嵌纹理）。首次 `ParticleSystemQuad::create()` 时会发生解码与贴图上传，
如果发生在战斗/技能触发瞬间容易掉帧。

- 在 `LoadingScene` / 场景预加载阶段调用：
  - `ParticlePreloadHelper::preloadCommonParticles();`
  - 或 `ParticlePreloadHelper::preloadParticlePlists({...});`

文件：`Adventure-King/Classes/Utils/ParticlePreloadHelper.h`

## 播放（Play）

一次性粒子（如升级、回血、命中提示）统一使用：

- `ParticleVfxHelper::playOnce(owner, "Particle/xxx.plist");`

默认行为：

- `PositionType::GROUPED`
- 对齐到物理体中心（`PhysicsBodyLocalInfoHelper`）
- `autoRemoveOnFinish = true`
- `zOrder = 999`

文件：`Adventure-King/Classes/Utils/ParticleVfxHelper.h`

## 节流（Throttle）

对“短时间可能多次触发”的粒子（例如回血）需要节流，避免刷屏与性能抖动。

- 当前实现：`CharacterBase::setCurrentHP` 内置 `0.5s` 冷却（`_lastRestoreHealthVfxMs`）
- 新增类似粒子建议遵循同样策略（冷却时间可按视觉效果与触发频率调节）

## 注意事项

- 不要手动指定 `particle_texture.png` 作为纹理：项目内多份粒子 plist 设计为内嵌纹理
- 新增粒子后按需加入 `ParticlePreloadHelper::getCommonParticlePlists()`，确保加载阶段预热

