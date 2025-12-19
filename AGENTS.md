# Repository Guidelines

## Project Structure & Module Organization
- Root contains the Cocos2d-x game under `Adventure-King/`.
- `Adventure-King/Classes/` holds all gameplay C++ code:
  - `Character/` (player, monsters, component system)
  - `Scenes/` (HelloWorldScene, GameScene, MapScene, etc.)
  - `Configs/` (GameConfigs, GamePhysicsCategory, GameSceneConfig)
  - `UI/`, `Managers/`, `Save/`
- `Adventure-King/Resources/` stores runtime assets (sprites, TMX maps, audio, fonts).
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
- 贴图加载：对文件路径（`Sprites/...`）优先用 `Adventure-King/Classes/Utils/SpriteFrameCacheHelper.h` 走 `SpriteFrameCache` 复用，避免重复创建与 “Frame isn't found” 日志噪音。
- 物理分类定义：使用 `Adventure-King/Classes/Configs/GamePhysicsCategory.h` 中的枚举，避免在其它位置重复定义掩码。
- 可调参数统一放在 `Adventure-King/Classes/Configs/GameConfigs.h`（包含 App/Save/UI/MainMenu/Debug 等），代码中避免重复写死数值。

## Combat & Status Effects
- 伤害统一走 `DamageInfo`；持续伤害（DOT）必须设置 `causesHitStun=false`，避免锁玩家操作。
- 状态效果（燃烧等）通过 `AttributeComponent` 的叠层/结算逻辑管理，表现走 `StatusEffectVfxComponent`。
- 投掷物/爆炸附加状态效果请使用 `ExplosiveProjectile::addOnHitStatusEffect`，避免在 Scene 内硬编码。

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
