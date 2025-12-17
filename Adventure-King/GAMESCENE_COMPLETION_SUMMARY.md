# GameScene 完善工作总结（现状版）

## 最后更新
2025-12-16

## 工作概述
`GameScene` 已形成可跑通的“地图 → 刷怪 → 战斗 → 受击反馈”的闭环：场景侧负责地图/输入/碰撞与刷怪，角色侧（`PlayerCharacter`）负责动画与投掷物逻辑，怪物侧（`MonsterBase`/`GoblinMonster`）负责 AI/攻击。

## 已完成的功能

### 1) 地图与场景框架
- TMX 地图加载、相机跟随、物理世界配置
- `collisions` 图层解析为物理阻挡体
- `born` 出生点、`gate` 传送门区域

### 2) 刷怪系统（enemy_g）
- 从 TMX 对象组 `enemy_g` 读取生成点
  - `class/type` = 怪物类型（例如 `goblin`）
  - `name` = 生成数量（例如 `3`）
- 生成触发：生成点第一次进入人物视野（水平距离 < 半屏宽）时触发一次
- 分批生成：约每 `0.4s` 生成一个，避免瞬间刷屏/卡顿

### 3) 战斗闭环（碰撞 + 伤害 + 反馈）
- 碰撞分类：`GamePhysicsCategory::{PLAYER, MONSTER, PLAYER_ATTACK, MONSTER_ATTACK, PLATFORM, COLLISION...}`
- `GameScene::onContactBegin` 处理：
  - `MONSTER_ATTACK ↔ PLAYER`：以攻击判定框 `PhysicsBody::tag` 作为 rawDamage，对玩家 `takeDamage`
  - `PLAYER_ATTACK ↔ MONSTER`：用于近战判定框伤害；投掷物本体通常 `tag=0`，由爆炸结算伤害
  - 投掷物命中/落地：转交 `PlayerCharacter::handleProjectileContact`
- 受击飘字：`CharacterBase::showDamageNumber`

### 4) 玩家（Klee / KELL）技能与投掷物
- 普攻（`J/4`）：扔炸弹（TNT），落地/命中爆炸并对范围内敌人造成伤害
- 技能1（`E/K`）：发射导弹（素材在 `Klee/rpg`），命中爆炸（`spr_vfx_explosion_flash_x`）
- 动画统一：行走/攻击/技能动画在 `PlayerCharacter` 内管理，`GameScene` 仅做状态与回调

### 5) 性能与日志优化
- 引入 `SpriteFrameCacheHelper::getOrCreateSpriteFrame`：对文件路径帧按需加载并写入 `SpriteFrameCache`，后续复用，减少重复创建与日志噪音
- `CharacterBase` 使用 `scheduleUpdateWithPriority(1)` 保证基类更新执行且避免重复 schedule warning

## 按键映射（默认）
| 按键 | 功能 |
|---|---|
| `A/D` / `←/→` | 移动 |
| `Shift` | 跑步 |
| `W` | 交互/跳跃（在 gate 上交互，否则跳跃） |
| `Space` | 仅跳跃 |
| `J` / `4` | 普攻：扔炸弹 |
| `E` / `K` | 技能1：导弹（火球） |
| `Esc` | 暂停菜单 |

## 地图（Tiled）约定
- `collisions`：碰撞对象（多边形/折线/矩形）用于生成阻挡体
- `born`：玩家出生点（取第一个对象）
- `gate`：传送门区域（矩形对象）
- `enemy_g`：刷怪点（`class/type`=怪物类型，`name`=数量）

## 相关文件
- `Adventure-King/Classes/Scenes/GameScene.cpp`
- `Adventure-King/Classes/Scenes/GameScene.h`
- `Adventure-King/Classes/Character/Player/PlayerCharacter.cpp`
- `Adventure-King/Classes/Character/Monster/MonsterBase.cpp`
- `Adventure-King/Classes/Character/Monster/Monsters/GoblinMonster.cpp`
- `Adventure-King/Classes/Physics/GamePhysicsCategory.h`
- `Adventure-King/Classes/Utils/SpriteFrameCacheHelper.h`
