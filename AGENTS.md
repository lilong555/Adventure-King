# Repository Guidelines

## Project Structure & Module Organization
- Root contains the Cocos2d-x game under `Adventure-King/`.
- `Adventure-King/Classes/` holds all gameplay C++ code:
  - `Character/` - 角色系统（组件化架构）
    - `Base/` - 角色基类 `CharacterBase`、`CharacterData`（属性/状态效果类型定义）、`StatusEffect`（状态效果基类）
    - `Player/` - 玩家角色 `PlayerCharacter`
      - `Players/` - 具体玩家实现（PlayerKlee）
      - `SkillSets/` - 职业技能集（PlayerSkillSet 基类、KleeSkillSet、WarriorSkillSet、AssassinSkillSet）
    - `Monster/` - 怪物系统 `MonsterBase` 及具体怪物 (GoblinMonster, GobluMonster, ObscurMonster)
    - `components/` - 核心组件（AttributeComponent, StateMachineComponent, SkillComponent, StatusEffectVfxComponent, InventoryComponent）
    - `StatusEffects/` - 状态效果实现（StatusEffectFactory、装备特效如 ThornsArmorEffect、EmergencyMaskEffect 等）
  - `Scenes/` - 场景系统（HelloWorldScene, GameScene, MapScene, LevelMap）
    - `LevelScenes/` - 具体关卡（OriginMushroomScene, MysteryForestScene）
  - `Configs/` - 配置文件（GameConfig, GamePhysicsCategory, GameSceneConfig, ArenaConfig）
  - `Objects/` - 游戏对象
    - `Projectiles/` - 投掷物（ExplosiveProjectile, Fireball 等）
  - `UI/` - 界面组件（PlayerStatusBar, SkillBar, BossHealthBar, PauseMenu, SaveMenuLayer, InventoryLayer）
  - `Managers/` - 全局管理器（SceneTransitionManager, MusicManager）
  - `Save/` - 存档系统（SaveManager, SaveData, JsonSerializer）
  - `Utils/` - 工具类（SpriteFrameCacheHelper, ParticlePreloadHelper, ParticleVfxHelper, WeaponHitboxVfxHelper）
- `Adventure-King/Resources/` stores runtime assets (sprites, TMX maps, audio, fonts, particles).
- `Adventure-King/proj.*` are platform projects (`proj.win32`, `proj.android`, `proj.linux`, `proj.ios_mac`).
- `Adventure-King/cocos2d/` is the vendored engine; do not edit unless upgrading engine.
- `tempresource/` is for raw/WIP art; only move finalized assets into `Adventure-King/Resources/`.

## Build, Run, and Development
- Windows/VSCode: `Ctrl+Shift+B` → “Build Debug (MSBuild)” or “Build Release (MSBuild)”.
- Command line Debug build:
  `".../MSBuild.exe" Adventure-King/proj.win32/Adventure-King.sln /p:Configuration=Debug /p:Platform=Win32 /m /t:Build`
- Run: `Adventure-King/proj.win32/Debug.win32/Adventure-King.exe`
- CMake (`Adventure-King/CMakeLists.txt`) supports cross-platform builds if you prefer `cmake` workflows.
- VSCode IntelliSense: `.vscode/c_cpp_properties.json` 提供 `Win32` 与 `Linux` 两套配置；在 WSL2 打开工程请选择 `Linux`，避免系统头文件报错。

## Coding Style & Naming Conventions
- Follow `Adventure-King/.editorconfig`: 4-space indents in `.cpp/.h`, spaces not tabs.
- Line endings: repository enforces LF for code/docs/assets; keep editors set to LF. Only `.bat`/`.cmd` scripts use CRLF.
- Classes and files use `PascalCase` (`PlayerCharacter.h/.cpp`); methods/vars use `camelCase`; private members use leading `_`.
- Keep new logic inside `Adventure-King/Classes/` and reuse existing components (Attribute/StateMachine/Skill) where possible.
- Player animations are managed by `PlayerCharacter`; scenes should call `setMoving`, `attackAnimated`, and `castSkillAnimated` instead of running actions directly.
- 贴图加载：对文件路径（`Sprites/...`）优先用 `Adventure-King/Classes/Utils/SpriteFrameCacheHelper.h` 走 `SpriteFrameCache` 复用，避免重复创建与 "Frame isn't found" 日志噪音。
- 物理分类定义：使用 `Adventure-King/Classes/Configs/GamePhysicsCategory.h` 中的枚举，避免在其它位置重复定义掩码。
- 可调参数统一放在 `Adventure-King/Classes/Configs/GameConfigs.h`（包含 App/Save/UI/MainMenu/Debug 等），代码中避免重复写死数值。
- 粒子特效参数：燃烧特效参数放在 `StatusEffectVfxComponent.h` 的 `BurningVfxParams` 结构体中，便于调参。

## Combat & Status Effects
- 伤害统一走 `DamageInfo`；持续伤害（DOT）必须设置 `causesHitStun=false`，避免锁玩家操作。
- 状态效果（燃烧等）通过 `AttributeComponent` 的叠层/结算逻辑管理，表现走 `StatusEffectVfxComponent`。
- 投掷物/爆炸附加状态效果请使用 `ExplosiveProjectile::addOnHitStatusEffect`，避免在 Scene 内硬编码。
- 受击粒子（`CharacterBase::spawnHurtVfx`）仅在 `causesHitStun=true` 时触发。
- 装备特效通过 `StatusEffect` 子类实现，在 `StatusEffectFactory` 中注册，由 `InventoryComponent` 管理装卸。
- 装备特效回调：`onAfterDealDamage`（造成伤害后）、`onAfterReceiveDamage`（受到伤害后）用于触发型机制。

## Skill System
- 职业技能通过 `PlayerSkillSet` 抽象，具体实现在 `SkillSets/` 目录下（KleeSkillSet、WarriorSkillSet、AssassinSkillSet）。
- 新增职业技能：继承 `PlayerSkillSet`，实现 `initSkills`、`tryNormalAttack`、`tryUseSkill`。
- 技能参数统一放在 `GameConfig.h` 对应的 namespace 中（如 `GameConfig::Warrior::FireSkill`）。
- 技能动画使用 `PlayerCharacter::playOneShotAnimation` 或 `attackAnimated`/`castSkillAnimated`。
- 技能命中框使用 `PlayerCharacter::spawnPlayerAttackHitbox`，支持暴击判定。

## Arena (连战模式)
- 连战配置在 `ArenaConfig.h`，包含波次数据（`WaveData`）、触发区域、刷怪点。
- 连战逻辑在 `LevelMap` 中实现，通过 `ArenaConfig` 管理波次状态。
- 击败当前波次所有怪物后自动触发下一波。

## Particle Effects
- 粒子资源位于 `Resources/Particle/`，使用 plist 格式配置。
- 燃烧特效由 `StatusEffectVfxComponent` 代码生成，参数在头文件 `BurningVfxParams` 中调整。
- 受击粒子使用 `par_chararcter_hurt_L/R.plist`，根据攻击方向选择。
- 主菜单战火背景使用 `par_warfire.plist`。
- 技能粒子：`par_fire.plist`（战士 Fire 技能）、`par_dragon_fire.plist`（龙焰）。
- 一次性粒子播放使用 `ParticleVfxHelper::playOnce`，支持自定义位置、zOrder、挂载节点。
- 新增粒子后需加入 `ParticlePreloadHelper::getCommonParticlePlists()` 确保预热。

## Map (TMX) Conventions
- `collisions` 图层：多边形/折线/矩形对象用于生成物理碰撞体
- `born` 对象组：玩家出生点
- `gate` 对象组：传送门区域
- `enemy_g` 对象组：刷怪点（`class/type`=怪物类型，`name`=生成数量；首次进入视野触发，约每 `0.4s` 生成一个）

## Testing Guidelines
- No dedicated unit test suite. Validate changes by building and playtesting relevant scenes.
- For bug fixes, add a short repro note in the PR and, if needed, update design docs in `Adventure-King/*.md`.

## Commit & Pull Request Guidelines
- Commits loosely follow Conventional Commits seen in history: `feat:`, `fix:`, `docs:`, `refactor:`, `chore:` plus a short summary (Chinese or English).
- PRs should include: purpose, linked issue/feature, scenes affected, and screenshots/GIFs for gameplay/UI changes. Note any new assets and their paths under `Resources/`.
- `Adventure-King/Resources/` 下的任何改动都需要保留并提交。
