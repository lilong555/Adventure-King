# GameScene 完善实现指南（现状版）

> 更新于 2025-12-16  
> 本文档最初用于指导将 DebugScene 的战斗/技能迁移到 GameScene。当前 main 已完成迁移并进一步重构：  
> - 动画与投掷物/爆炸判定已下沉到 `PlayerCharacter`  
> - `GameScene` 负责地图/输入/碰撞结算/刷怪与 UI  

## 关键入口
- `Adventure-King/Classes/Scenes/GameScene.cpp`
- `Adventure-King/Classes/Character/Player/PlayerCharacter.cpp`
- `Adventure-King/Classes/Character/Base/CharacterBase.cpp`
- `Adventure-King/Classes/Physics/GamePhysicsCategory.h`
- `Adventure-King/Classes/Utils/SpriteFrameCacheHelper.h`

## 输入与技能（默认）
| 按键 | 功能 |
|---|---|
| `A/D` / `←/→` | 移动 |
| `Shift` | 跑步 |
| `W` | 交互/跳跃（在 gate 上交互，否则跳跃） |
| `Space` | 仅跳跃 |
| `J` / `4` | 普攻：扔炸弹 |
| `E` / `K` | 技能1：导弹（火球） |
| `Esc` | 暂停 |

## 战斗闭环

### 角色侧（PlayerCharacter）
- 普攻：`spawnBombProjectile(gameLayer)` 创建 TNT 投掷物并注册物理体
- 技能1：`spawnFireballProjectile(gameLayer)` 创建导弹并循环播放尾迹动画
- 命中/落地爆炸：`handleProjectileContact(...)` + `explodeProjectile(...)`
  - 爆炸范围伤害会遍历 `gameLayer` 下的 `CharacterBase` 子节点并调用 `takeDamage`

### 场景侧（GameScene）
- `onContactBegin` 负责：
  - `MONSTER_ATTACK ↔ PLAYER`：从攻击判定框 `PhysicsBody::tag` 读取 rawDamage，对玩家 `takeDamage`
  - `PLAYER_ATTACK ↔ MONSTER`：用于近战判定框；投掷物通常 `tag=0`，爆炸伤害由玩家逻辑处理
  - 投掷物触发：转交 `PlayerCharacter::handleProjectileContact`

## 伤害飘字
- `CharacterBase::showDamageNumber`：默认怪物开启，玩家可按需开启/关闭  
  使用 `setDamageNumbersEnabled(true/false)` 控制。

## 地图与刷怪（Tiled）

### 图层/对象组约定
- `collisions`：碰撞对象（多边形/折线/矩形）→ 生成 `COLLISION` 物理体
- `born`：玩家出生点（取第一个对象）
- `gate`：传送门区域（矩形对象）
- `enemy_g`：敌人生成点
  - `class`（或 `type`）= 怪物类型（如 `goblin`）
  - `name` = 数量（如 `3`）

### 触发规则
- 首次进入视野（水平距离 < `getEnemySpawnViewDistance()`，默认半屏宽）触发一次生成
- 分批生成：约每 `0.4s` 生成一个（避免瞬间刷屏/卡顿）
- 类型映射：`GameScene::createMonsterByType`

## 资源组织（Klee）
- `Resources/Sprites/Characters/Player/Klee/defalt/`
  - 行走、普攻等默认动画
  - `TNT.png`、炸弹爆炸素材
- `Resources/Sprites/Characters/Player/Klee/rpg/`
  - 技能1施放动画 `spr_klee_attack_x`
  - 导弹尾迹 `spr_vfx_rocket_trail_long_x`
  - 爆炸闪光 `spr_vfx_explosion_flash_x`

## 性能：SpriteFrameCacheHelper
- 规则：对文件路径（`Sprites/...`）用 `SpriteFrameCacheHelper::getOrCreateSpriteFrame` 获取 `SpriteFrame*`，再用 `Sprite::createWithSpriteFrame` 创建精灵
- 目的：减少重复的 `SpriteFrame` 创建与 Texture 反复加载，同时避免 `SpriteFrameCache: Frame 'Sprites/...' isn't found` 噪音

## 常见调整入口
- 投掷物体积/判定：`PlayerCharacter::spawnBombProjectile` / `spawnFireballProjectile` 中的 `PhysicsBody::createCircle(...)` 半径
- 刷怪节奏：`GameScene::updateEnemySpawns` 的 `SPAWN_INTERVAL_SECONDS` / `SPAWN_SPACING_X`
- 新增怪物：实现 `MonsterBase` 子类，并在 `GameScene::createMonsterByType` 增加映射
