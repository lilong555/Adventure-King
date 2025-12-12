# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

本项目是一款基于 C++ 和 Cocos2d-x 引擎开发的横版动作冒险游戏，名为《冒险王之神兵传奇》。游戏的核心是扮演一名冒险者，通过探索世界、击败怪物和收集装备来提升实力。

## 最新进展
- GameScene 已接入战斗/技能：炸弹技能绑定默认槽位，设定能量消耗与冷却，行走/攻击/技能动画用不同 Tag，输入包含移动、跳跃、普攻与技能；炸弹带物理体，碰平台即爆炸，详见 `GAMESCENE_COMPLETION_SUMMARY.md` 与 `Classes/Scenes/GameScene.*`。
- 怪物系统落地：新增 `MonsterBase` 驱动 AI/移动/攻击/受击（持有 Attribute/StateMachine/Skill 组件，依赖外部设置 `_target`），首个敌人 `GoblinMonster`（`Classes/Character/Monster/Monsters`）设置基础生命、防御、力量、暴击和移动速度，较远感知、默认近战攻距，动画名 goblin_idle/walk/attack/hurt/dead，攻击优先技能槽否则普攻。
- 修复 TMX 多边形/折线碰撞在高分屏下的缩放偏差：parseTMXObjectVertices 解析 points/polylinePoints 时按 CC_CONTENT_SCALE_FACTOR() 做同样缩放，保证碰撞与地图一致。
- 动画体系统一：人物走路/攻击/技能动画已迁移到 PlayerCharacter（setMoving/attackAnimated/castSkillAnimated），GameScene 与 DebugScene 仅负责输入/状态与回调。
- 代码行尾标准化：仓库源码/资源统一使用 LF（.bat/.cmd 例外为 CRLF），通过 .gitattributes/.editorconfig 固定提交行尾，避免 CRLF 抖动。

## 构建与运行

### 环境要求
- Windows 操作系统
- Visual Studio 2022 (或更高版本)
- Cocos2d-x 引擎 (已包含在 `Adventure-King/cocos2d` 目录)

### VSCode 构建任务 (推荐)
项目配置了 VSCode 构建任务，可通过 `Ctrl+Shift+B` 执行：
- **Build Debug (MSBuild)** - 默认构建任务，编译 Debug 版本
- **Build Release (MSBuild)** - 编译 Release 版本
- **Clean (MSBuild)** - 清理构建产物
- **Rebuild Debug (MSBuild)** - 重新编译 Debug 版本
- **Run Game** - 运行编译后的游戏

### 命令行构建
```bash
# Debug 构建
"C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" Adventure-King/proj.win32/Adventure-King.sln /p:Configuration=Debug /p:Platform=Win32 /m /t:Build

# 运行游戏
Adventure-King/proj.win32/Debug.win32/Adventure-King.exe
```

### Visual Studio 构建
1. 打开 `Adventure-King/proj.win32/Adventure-King.sln`
2. 选择 `Debug` 或 `Release` 配置，平台选择 `Win32`
3. 生成 > 生成解决方案 (或按 `Ctrl+Shift+B`)
4. 调试 > 开始执行(不调试) (或按 `Ctrl+F5`)

## 代码架构

### 目录结构
- **`Adventure-King/Classes`** - 游戏核心逻辑代码
  - **`Classes/Character/`** - 角色系统（玩家、怪物）
  - **`Classes/Scenes/`** - 场景系统
  - **`Classes/UI/`** - 游戏内 UI 组件
  - **`Classes/Managers/`** - 全局管理器
  - **`Classes/Save/`** - 存档系统（SaveManager、JsonSerializer、SaveData）
- **`Adventure-King/Resources`** - 游戏资源 (图片、字体、音频、TMX 地图)
- **`Adventure-King/cocos2d`** - Cocos2d-x 引擎源代码
- **`Adventure-King/proj.win32`** - Windows 平台项目文件

### 角色系统 (组件化架构)

角色系统采用 **组件化架构 (Component-Based Architecture)**，核心文件位于 `Classes/Character/`：

- **`CharacterBase`** - 所有角色的基类，继承自 `cocos2d::Sprite`，作为组件容器
- **`PlayerCharacter`** - 玩家角色实现，包含经验升级、装备管理等逻辑
- **`CharacterData.h`** - 数据字典，定义所有枚举 (`CharacterRole`, `CharacterState`, `AttributeType`) 和数据结构

核心组件 (`Classes/Character/components/`)：
- **`AttributeComponent`** - 属性组件，计算战斗数值 (基础/装备/技能/状态效果)
- **`StateMachineComponent`** - 状态机组件，管理角色状态和动画播放
- **`SkillComponent`** - 技能组件，管理技能学习、装备、使用和冷却

### 怪物系统
- **`MonsterBase`** (`Classes/Character/Monster/MonsterBase.{h,cpp}`) - 组件化怪物基类，内置 `_target`、攻击计时与移动/攻击距离；`update()` 调用 `updateAI`/`updateMovement`/`updateAttack`，默认攻击走技能槽，`takeDamage`/`die` 会切换 HURT/DEAD 状态并可延迟移除；工具函数 `faceTarget`/`distanceTo`/`inAttackRange`。
- **`GoblinMonster`** (`Classes/Character/Monster/Monsters/GoblinMonster.{h,cpp}`) - 设置基础生命、防御、力量、暴击与感知/攻距；注册动画名 goblin_idle/walk/attack/hurt/dead（需预先加载到 AnimationCache）；`attack()` 优先用技能组件槽，失败则对目标造成基础伤害。
- AI 依赖外部把玩家节点传入 `_target`，未找到目标则停留 Idle；移动使用 MonsterBase 的移动速度（如需匹配属性移速需手动同步）。

### 场景系统

场景流程：`AppDelegate` → `HelloWorldScene` (主菜单) → `MapScene` (地图选择) → 关卡场景

**场景导航：**
- 主菜单 (`HelloWorldScene`) 提供存档加载功能，可直接加载存档进入对应关卡
- 暂停菜单 (`PauseMenu`) 的"返回主菜单"按钮会返回到 `HelloWorldScene`
- 关卡内按 ESC 可打开暂停菜单，支持保存/加载游戏

关卡场景架构 (`Classes/Scenes/GameScene.h`)：
- **`GameScene`** - 关卡场景基类，提供 TMX 地图加载、物理碰撞、玩家控制、传送门系统
  - 提供 `getPlayer()` 方法用于获取玩家角色指针
  - 支持从存档恢复玩家数据和位置
- **`OriginMushroomScene`** - 起源之菇关卡
- **`MysteryForestScene`** - 神秘之森关卡
- **`LevelConfig`** / **`PlayerConfig`** - 关卡和玩家配置结构体

辅助类：
- **`SceneTransitionManager`** - 场景切换管理器，处理淡入淡出过渡效果
- **`GameUI`** - 游戏内 UI (血量、能量、技能图标等)
- **`MusicManager`** - 音乐管理器

其他场景：
- **`HomeScene`** - 主页场景
- **`DebugScene`** - 调试场景，用于测试角色功能，包含实时属性查看和修改面板

### 战斗与技能（GameScene）
- 关卡已接入 DebugScene 的战斗/技能逻辑：炸弹技能绑定默认槽位，配置能量消耗与冷却，命中/落地即爆炸。
- 动画：行走/攻击/技能分别用不同 Action Tag，`_isAttacking`/`_isCastingSkill` 防止动画冲突，行走动画自动随左右移动启停。
- 输入映射：移动、跳跃、攻击、技能释放与暂停；炸弹技能走独立按键。
- 碰撞：新增 `GamePhysicsCategory::BOMB`，炸弹带圆形刚体，碰平台触发爆炸特效；与场景碰撞/传送门逻辑并行。
- 参考：`GAMESCENE_COMPLETION_SUMMARY.md`、`GAMESCENE_IMPLEMENTATION_GUIDE.md`、`Classes/Scenes/GameScene.*`。

### 物理系统

使用 Cocos2d-x 内置物理引擎，碰撞类型定义在 `GamePhysicsCategory` 枚举：
- `PLAYER` - 玩家
- `PLATFORM` - 平台/地面
- `COLLISION` - 碰撞体 (多边形)
- `TRIGGER` - 触发器 (不产生物理碰撞)

### 资源组织
- `Resources/Map/` - TMX 瓦片地图和图块集
- `Resources/Scene/` - 场景背景、UI 素材、音乐
- `Resources/Sprites/` - 角色和敌人精灵图
- `Resources/fonts/` - 字体文件 (包含中文字体 NotoSansSC)

### 存档系统

存档系统位于 `Classes/Save/` 目录，提供完整的游戏存档功能：

**核心组件：**
- **`SaveData.h`** - 存档数据结构定义
  - `SaveSlotData` - 完整存档槽位数据
  - `PlayerSaveData` - 玩家数据（等级、经验、属性、装备、技能）
  - `GameProgressSaveData` - 游戏进度（场景、位置、解锁关卡）
  - `SettingsSaveData` - 游戏设置

- **`JsonSerializer.h/.cpp`** - JSON 序列化工具
  - 使用 Cocos2d-x 内置 rapidjson 库
  - 支持存档数据和设置数据的序列化/反序列化

- **`SaveManager.h/.cpp`** - 存档管理器（单例）
  - 支持 5 个存档槽位
  - 提供保存、加载、删除存档功能
  - 支持自动存档（可配置间隔）
  - 支持设置持久化
  - 存档文件位置：`FileUtils::getWritablePath() + "saves/"`

**UI 组件：**
- **`SaveMenuLayer.h/.cpp`** - 存档菜单界面
  - 支持保存/加载模式切换
  - 显示存档信息（等级、时间戳）
  - 提供删除存档功能
  - 确认对话框

- **`PauseMenu.h/.cpp`** - 暂停菜单（已集成存档功能）
  - 添加"保存游戏"和"加载游戏"按钮

**使用方法：**

```cpp
// 保存游戏
auto saveManager = SaveManager::getInstance();
saveManager->saveGame(slotIndex, player, sceneName, playerPos);

// 加载游戏
SaveSlotData loadedData;
if (saveManager->loadGame(slotIndex, loadedData)) {
    // 应用存档数据到玩家
    saveManager->applyPlayerData(player, loadedData.playerData);
}

// 显示存档菜单
auto saveMenu = SaveMenuLayer::create(
    SaveMenuLayer::Mode::SAVE,  // 或 Mode::LOAD
    player,
    sceneName,
    playerPos
);
this->addChild(saveMenu);
```

**存档文件格式：**
- JSON 格式，便于调试和修改
- 文件名：`save_0.json` 到 `save_4.json`
- 包含元数据（槽位索引、时间戳、游戏版本）
- 包含完整的玩家数据和游戏进度

**存档加载流程：**
1. 在 `HelloWorldScene` 中选择存档槽位
2. `SaveManager` 加载存档数据
3. 根据 `currentSceneName` 创建对应的关卡场景
4. 使用 `scheduleOnce` 延迟应用玩家数据，确保场景完全初始化
5. 通过 `SaveManager::applyPlayerData()` 恢复玩家状态和位置

## 项目维护记录

### GameScene 战斗/技能完善
- 将 DebugScene 的战斗/技能移植到 GameScene：炸弹技能绑定默认槽位，配置能量消耗与冷却；行走/攻击/技能动画用独立 Tag，新增 `_isAttacking`、`_isCastingSkill` 状态。
- 输入映射覆盖移动/跳跃/攻击/技能/暂停；行走动画随按键自动播放/停止。
- 物理与碰撞：新增 `GamePhysicsCategory::BOMB`，炸弹带刚体，落地/撞平台即爆炸并播放特效；详见 `GAMESCENE_COMPLETION_SUMMARY.md` 与 `Classes/Scenes/GameScene.*`。

### 文件结构清理
- **问题**：发现 `Classes/GameScene.cpp` 和 `Classes/Scenes/GameScene.cpp` 重复
- **原因**：旧文件使用了错误的 include 路径 `Character/PlayerCharacter.h`
- **解决**：删除 `Classes/GameScene.cpp`，保留 `Classes/Scenes/GameScene.cpp`
- **验证**：所有 vcxproj 引用的文件都存在，所有 cpp 文件都被正确引用

### 存档系统修复
- **问题1**：暂停菜单"返回主菜单"跳转到 `MapScene` 而非 `HelloWorldScene`
- **解决**：修改 `GameScene.cpp` 中的 `setMainMenuCallback` 回调
- **问题2**：主菜单加载存档功能未实现
- **解决**：在 `HelloWorldScene.cpp` 中实现完整的加载回调逻辑
  - 根据场景名称创建对应场景
  - 延迟应用玩家数据
  - 恢复玩家位置
- **新增**：`GameScene::getPlayer()` 公共方法用于访问玩家角色

### 怪物框架与哥布林落地
- 新增组件化怪物基类 `MonsterBase`：AI/移动/攻击分离，默认攻击走技能槽，`takeDamage` 会切换 HURT/DEAD 并可延迟移除，需外部设置 `_target`。
- 首个敌人 `GoblinMonster`：配置基础属性与感知/攻距；注册动画 goblin_idle/walk/attack/hurt/dead；攻击优先用技能槽，否则对目标造成基础伤害。


### TMX 碰撞缩放修复
- parseTMXObjectVertices 解析 points/polylinePoints 时按 CC_CONTENT_SCALE_FACTOR() 缩放，解决高分屏多边形/折线碰撞偏移问题。

### 动画逻辑迁移
- 行走/攻击/技能动画迁移到 PlayerCharacter；Scene 侧调用 setMoving/attackAnimated/castSkillAnimated，避免手写动作逻辑。
