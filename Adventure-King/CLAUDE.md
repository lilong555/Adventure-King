# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

基于 C++ 和 Cocos2d-x 引擎开发的横版动作冒险游戏《冒险王之神兵传奇》。

## 构建与运行

```bash
# Windows 命令行构建 (Debug)
"C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" Adventure-King/proj.win32/Adventure-King.sln /p:Configuration=Debug /p:Platform=Win32 /m /t:Build

# 清理后重新构建（修改头文件后必须）
MSBuild.exe ... /t:Clean && MSBuild.exe ... /t:Build

# 运行游戏
Adventure-King/proj.win32/Debug.win32/Adventure-King.exe
```

**VSCode**: `Ctrl+Shift+B` → "Build Debug (MSBuild)"

**Visual Studio**: 打开 `Adventure-King/proj.win32/Adventure-King.sln`，选择 `Debug/Win32` 构建

**WSL2 开发**: `.vscode/c_cpp_properties.json` 选择 `Linux` 配置；修改代码后需执行 `./scripts/wsl-mirror.sh push` 同步到 Windows 目录

## 代码架构

### 核心目录
```
Adventure-King/Classes/
├── Character/           # 角色系统（组件化架构）
│   ├── Base/           # CharacterBase 基类
│   ├── Player/         # PlayerCharacter
│   ├── Monster/        # MonsterBase + 具体怪物 (GoblinMonster, GobluMonster)
│   └── components/     # AttributeComponent, StateMachineComponent, SkillComponent, StatusEffectVfxComponent
├── Scenes/             # 场景系统 (GameScene, HelloWorldScene, MapScene, OriginMushroomScene, MysteryForestScene)
├── Objects/            # 投掷物 (ExplosiveProjectile)
├── Configs/            # GameConfigs, GamePhysicsCategory
├── Save/               # SaveManager, SaveData, JsonSerializer
├── UI/                 # PlayerStatusBar, SkillBar, PauseMenu, SaveMenuLayer
├── Managers/           # SceneTransitionManager, MusicManager
└── Utils/              # SpriteFrameCacheHelper
```

### 组件化角色系统

角色继承自 `CharacterBase`（`cocos2d::Sprite` 子类），通过组件扩展功能：

- **AttributeComponent** - 属性计算（基础/装备/技能/状态效果），管理 DOT 叠层
- **StateMachineComponent** - 状态切换与动画播放
- **SkillComponent** - 技能学习、装备、冷却
- **StatusEffectVfxComponent** - 状态效果视觉（燃烧粒子等）

### 场景流程

`AppDelegate` → `HelloWorldScene`（主菜单）→ `MapScene`（地图选择）→ `GameScene`（关卡基类）→ 具体关卡

### 战斗系统

- 碰撞结算在 `GameScene::onContactBegin`
- 投掷物逻辑在 `PlayerCharacter`（`spawnBombProjectile`, `spawnFireballProjectile`）
- 伤害统一用 `DamageInfo`；DOT 必须设置 `causesHitStun=false`
- 状态效果通过 `ExplosiveProjectile::addOnHitStatusEffect` 添加

### 物理碰撞

碰撞类型定义在 `Classes/Configs/GamePhysicsCategory.h`：`PLAYER`, `PLATFORM`, `COLLISION`, `TRIGGER`, `MONSTER`, `PLAYER_ATTACK`, `MONSTER_ATTACK`, `BOMB`

## 粒子效果系统

### 燃烧特效（StatusEffectVfxComponent）
- 触发条件：角色拥有 `StatusEffectType::BURNING` 状态
- 参数调整：修改 `StatusEffectVfxComponent.h` 中的 `BurningVfxParams` 结构体
```cpp
struct BurningVfxParams {
    static constexpr float EMITTER_OFFSET_Y_RATIO = 0.15f;  // 发射器Y偏移
    static constexpr float POS_VAR_X_RATIO = 0.12f;         // X散布比例
    static constexpr float POS_VAR_Y_RATIO = 0.10f;         // Y散布比例
    static constexpr float POS_VAR_X_MAX = 70.0f;           // X散布最大值
    static constexpr float POS_VAR_Y_MAX = 110.0f;          // Y散布最大值
    static constexpr float MAX_START_SIZE = 18.0f;          // 粒子最大尺寸
};
```

### 受击粒子（CharacterBase::spawnHurtVfx）
- 触发条件：`takeDamage()` 且 `causesHitStun=true`
- 资源文件：`Resources/Particle/par_chararcter_hurt_L/R.plist`

### 粒子资源
```
Resources/Particle/
├── par_chararcter_hurt_L.plist  # 受击粒子（左）
├── par_chararcter_hurt_R.plist  # 受击粒子（右）
├── par_warfire.plist            # 主菜单战火背景
├── par_GobluRemoteHit.plist     # Goblu远程命中（预留）
├── par_Poison.plist             # 中毒效果（预留）
├── par_Restore_health.plist     # 恢复生命（预留）
└── particle_texture.png         # 通用粒子纹理
```

## 开发约定

### 编码规范
- 4 空格缩进，LF 换行
- 类名/文件名 `PascalCase`，方法/变量 `camelCase`，私有成员 `_` 前缀
- 遵循 `Adventure-King/.editorconfig`

### 关键约定
- **贴图加载**: 用 `SpriteFrameCacheHelper::getOrCreateSpriteFrame` 复用帧
- **可调参数**: 统一放 `Classes/Configs/GameConfigs.h`；粒子参数放对应组件头文件
- **物理分类**: 只用 `GamePhysicsCategory.h` 中的枚举
- **玩家动画**: 场景调用 `setMoving`/`attackAnimated`/`castSkillAnimated`，不直接操作动作
- **引擎代码**: `Adventure-King/cocos2d/` 不要修改

### TMX 地图约定
- `collisions` 图层：物理碰撞体
- `born` 对象组：玩家出生点
- `gate` 对象组：传送门
- `enemy_g` 对象组：刷怪点（`class/type`=怪物类型，`name`=数量）

## 存档系统

```cpp
// 保存
SaveManager::getInstance()->saveGame(slotIndex, player, sceneName, playerPos);

// 加载
SaveSlotData data;
if (SaveManager::getInstance()->loadGame(slotIndex, data)) {
    SaveManager::getInstance()->applyPlayerData(player, data.playerData);
}
```

存档位置：`FileUtils::getWritablePath() + "saves/save_X.json"`

## 测试

无单元测试框架。通过构建并运行相关场景进行验证。
