# Repository Guidelines

## Project Structure & Module Organization
- Root contains the Cocos2d-x game under `Adventure-King/`.
- `Adventure-King/Classes/` holds all gameplay C++ code:
  - `Character/` (player, monsters, component system)
  - `Scenes/` (HelloWorldScene, GameScene, MapScene, etc.)
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

## Coding Style & Naming Conventions
- Follow `Adventure-King/.editorconfig`: 4-space indents in `.cpp/.h`, spaces not tabs.
- Line endings: repository enforces LF for code/docs/assets; keep editors set to LF. Only `.bat`/`.cmd` scripts use CRLF.
- Classes and files use `PascalCase` (`PlayerCharacter.h/.cpp`); methods/vars use `camelCase`; private members use leading `_`.
- Keep new logic inside `Adventure-King/Classes/` and reuse existing components (Attribute/StateMachine/Skill) where possible.
- Player animations are managed by `PlayerCharacter`; scenes should call `setMoving`, `attackAnimated`, and `castSkillAnimated` instead of running actions directly.

## Testing Guidelines
- No dedicated unit test suite. Validate changes by building and playtesting relevant scenes.
- For bug fixes, add a short repro note in the PR and, if needed, update design docs in `Adventure-King/*.md`.

## Commit & Pull Request Guidelines
- Commits loosely follow Conventional Commits seen in history: `feat:`, `fix:`, `docs:`, `refactor:`, `chore:` plus a short summary (Chinese or English).
- PRs should include: purpose, linked issue/feature, scenes affected, and screenshots/GIFs for gameplay/UI changes. Note any new assets and their paths under `Resources/`.
