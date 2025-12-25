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
│   ├── Base/           # CharacterBase 基类、CharacterData、StatusEffect
│   ├── Player/         # PlayerCharacter
│   │   ├── Players/    # 具体玩家实现（PlayerKlee）
│   │   └── SkillSets/  # 职业技能集（KleeSkillSet, WarriorSkillSet, AssassinSkillSet）
│   ├── Monster/        # MonsterBase + 具体怪物 (GoblinMonster, GobluMonster, ObscurMonster)
│   ├── components/     # AttributeComponent, StateMachineComponent, SkillComponent, StatusEffectVfxComponent, InventoryComponent
│   └── StatusEffects/  # StatusEffectFactory、装备特效实现
├── Scenes/             # 场景系统 (GameScene, HelloWorldScene, MapScene, LevelMap)
│   └── LevelScenes/    # 具体关卡 (OriginMushroomScene, MysteryForestScene)
├── Objects/            # 游戏对象
│   └── Projectiles/    # 投掷物 (ExplosiveProjectile, Fireball)
├── Configs/            # GameConfig, GamePhysicsCategory, ArenaConfig
├── Save/               # SaveManager, SaveData, JsonSerializer
├── UI/                 # PlayerStatusBar, SkillBar, PauseMenu, SaveMenuLayer, InventoryLayer
├── Managers/           # SceneTransitionManager, MusicManager
└── Utils/              # SpriteFrameCacheHelper, ParticlePreloadHelper, ParticleVfxHelper, WeaponHitboxVfxHelper
```

### 组件化角色系统

角色继承自 `CharacterBase`（`cocos2d::Sprite` 子类），通过组件扩展功能：

- **AttributeComponent** - 属性计算（基础/装备/技能/状态效果），管理 DOT 叠层
- **StateMachineComponent** - 状态切换与动画播放
- **SkillComponent** - 技能学习、装备、冷却
- **StatusEffectVfxComponent** - 状态效果视觉（燃烧粒子等）
- **InventoryComponent** - 背包与装备管理，装备特效的装卸

### 职业技能系统

职业技能通过 `PlayerSkillSet` 抽象，具体实现：
- **KleeSkillSet** - 法师：炸弹普攻 + 火球技能（附加燃烧 DOT）
- **WarriorSkillSet** - 战士：近战普攻 + Fire 火焰冲击（智能选点 AoE）
- **AssassinSkillSet** - 刺客：近战普攻 + Slash 斩击

新增职业技能步骤：
1. 在 `GameConfig.h` 添加技能参数 namespace
2. 继承 `PlayerSkillSet` 实现 `initSkills`、`tryNormalAttack`、`tryUseSkill`
3. 在 `PlayerCharacter` 中注册新的 SkillSet

### 装备特效系统

装备特效通过 `StatusEffect` 子类实现：
- **ThornsArmorEffect** - 荆棘甲：受击反伤
- **EmergencyMaskEffect** - 急救面罩：低血量自动回复
- **HunterBootsEffect** - 追猎之靴：击杀后移速加成
- **BloodPactSwordEffect** - 血契短剑：吸血
- **EmberStaffEffect** - 焰纹法杖：命中附加燃烧

装备特效参数在 `GameConfig::EquipmentEffect` 中配置。

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
├── par_fire.plist               # 战士 Fire 技能
├── par_dragon_fire.plist        # 龙焰特效
├── par_levelup.plist            # 升级特效
├── par_Restore_health.plist     # 恢复生命
├── par_GobluRemoteHit.plist     # Goblu远程命中
├── par_Poison.plist             # 中毒效果
├── par_nap.plist                # 睡眠/眩晕效果
└── particle_texture.png         # 通用粒子纹理
```

粒子播放工具：
- `ParticleVfxHelper::playOnce` - 一次性粒子播放
- `ParticlePreloadHelper` - 粒子预热（避免首次播放卡顿）

## 开发约定

### 编码规范
- 4 空格缩进，LF 换行
- 类名/文件名 `PascalCase`，方法/变量 `camelCase`，私有成员 `_` 前缀
- 遵循 `Adventure-King/.editorconfig`

### 关键约定
- **贴图加载**: 用 `SpriteFrameCacheHelper::getOrCreateSpriteFrame` 复用帧
- **可调参数**: 统一放 `Classes/Configs/GameConfig.h`；粒子参数放对应组件头文件；装备特效参数放 `GameConfig::EquipmentEffect`
- **物理分类**: 只用 `GamePhysicsCategory.h` 中的枚举
- **玩家动画**: 场景调用 `setMoving`/`attackAnimated`/`castSkillAnimated`，不直接操作动作
- **职业技能**: 通过 `PlayerSkillSet` 子类实现，不在 `PlayerCharacter` 中硬编码
- **装备特效**: 通过 `StatusEffect` 子类实现，在 `StatusEffectFactory` 注册
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
