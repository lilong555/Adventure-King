# 冒险王之神兵传奇 (Adventure-King)

## 项目概述

基于 Cocos2d-x 引擎的横版动作冒险游戏，使用 C++ 开发。

## 核心功能需求

### 角色系统

至少支持两种角色：

> 战士，法师，刺客，坦克

核心状态切换：

> Idle、行走、奔跑、跳跃、二段跳、攻击、攀爬

- 角色有技能点系统，升级获得技能点解锁主动技能
- 角色有属性系统，升级获得属性点提高属性（力量、防御、暴击率、移速……）
- 生命值与能量值系统
- 等级与经验值系统
- 受击硬直反馈与死亡判定

### 武器与装备系统

- 至少 2 种不同类型的武器（剑和法杖）
- 武器可切换，具备不同攻击方式与攻击特效
- 装备属性影响角色属性

### 敌人系统

- 至少 3 种普通敌人（近战/远程）/ 每场景
- 1 个 Boss 敌人，具备特殊技能，可具备二阶段
- 敌人具备基础 AI：追击、闪避、巡逻
- 击杀敌人可能掉落血瓶、蓝瓶、和不同品质的武器装备
- 击败敌人必定获取经验，等级越高掉得越多

### 关卡与场景

- 至少 2 个不同主题的关 ???（如森林、地牢）
- 场景可交互元素（如宝箱、陷阱、传送点）
- 支持场景音乐和动作特效

### UI 系统

- 开始菜单
- 存档界面
- 暂停菜单
- 背包界面
- 地图界面
- 游戏结束与重新开始界面
- 玩家状态栏（血量、能量、经验值、技能 ??? 标）
- Boss 血条显示

## 可选扩展功能

- 在 Android 系统上运行
- 双人游戏模式
- 武器幻化（同一个武器在不同职业呈现不同效果）
- 支持货币系统
- 宠物系统，可在特殊关卡中捕捉宠物
- 下劈障碍物进行跳跃
- 更丰富的技能
- 成就系统与收集图鉴
- 地图编辑器支持自定义地图、小怪和 boss
- 剧情对话系统与 NPC 交互
- 天气系统与动态光影
- 被动技能或符文（build 玩法）
- 多职业实时切换（类似于原神的队伍系统）

## 项目架构

### 目录结构

```
Adventure-King/
├── Classes/
│   ├── AppDelegate.cpp/h             # 游戏主入口
│   ├── GameUI.cpp/h                  # 游戏内 UI 管理器
│   │
│   ├── Character/                    # 角色系统（组件化架构）
│   │   ├── Base/
│   │   │   ├── CharacterBase.cpp/h   # 角色基类
│   │   │   └── CharacterData.h       # 数据字典（枚举、结构体）
│   │   ├── components/               # 组件系统
│   │   │   ├── AttributeComponent.cpp/h      # 属性组件
│   │   │   ├── StateMachineComponent.cpp/h   # 状态机组件
│   │   │   └── SkillComponent.cpp/h          # 技能组件
│   │   ├── Monster/                  # 怪物系统
│   │   │   ├── MonsterBase.cpp/h     # 怪物基类
│   │   │   └── Monsters/
│   │   │       └── GoblinMonster.cpp/h       # 哥布林怪物
│   │   └── Player/
│   │       └── PlayerCharacter.cpp/h # 玩家角色
│   │
│   ├── Scenes/                       # 场景系统
│   │   ├── GameScene.cpp/h           # 关卡场景基类
│   │   ├── DebugScene.cpp/h          # 调试场景
│   │   ├── HelloWorldScene.cpp/h     # 主菜单场景
│   │   ├── HomeScene.cpp/h           # 主页场景
│   │   ├── MapScene.cpp/h            # 地图选择场景
│   │   └── Layers/                   # UI 层
│   │       ├── SaveMenuLayer.cpp/h   # 存档菜单
│   │       └── SetMenuLayer.cpp/h    # 设置菜单
│   │
│   ├── UI/                           # 游戏内 UI 组件
│   │   ├── PlayerStatusBar.cpp/h     # 玩家状态栏（HP/MP/经验条）
│   │   ├── SkillBar.cpp/h            # 技能栏（技能图标+冷却）
│   │   ├── BossHealthBar.cpp/h       # Boss 血条（支持多阶段）
│   │   └── PauseMenu.cpp/h           # 暂停菜单
│   │
│   └── Managers/                     # 全局管理器
│       ├── MusicManager.cpp/h        # 音乐管理器
│       └── SceneTransitionManager.cpp/h  # 场景切换管理器
│
└── Resources/
    ├── fonts/                        # 字体文件
    │   └── NotoSansSC/               # 中文字体
    ├── Map/                          # TMX 地图
    │   ├── Map1.tmx                  # 基础地图
    │   ├── Origin_Mushroom/          # 起源之菇关卡
    │   └── Origin_Mushroom_2/        # 第二个蘑菇关卡
    ├── Scene/                        # 场景资源
    │   ├── Backgrounds/              # 背景图
    │   ├── MusicOfScene/             # 场景音乐
    │   └── UI/                       # UI 素材
    └── Sprites/                      # 精灵图
        ├── Characters/
        │   └── Player/               # 玩家角色精灵
        │       ├── Klee/             # 可莉角色
        │       ├── maaer/            # 其他角色
        │       └── man/              # 男性角色
        ├── Enemies/
        │   └── YuanSheMenJiang/      # 原神门将敌人
        └── Items/                    # 道具精灵
```

### 计划中的架构扩展

```
Classes/
├─ Character/
│   ├─ Components/
│   │    ├─ StateMachine/
│   │    │      ├─ State.h/.cpp         // 泛型状态类
│   │    │      └─ MonsterStates/       // 怪物状态
│   │    │            ├─ IdleState.h/.cpp
│   │    │            ├─ PatrolState.h/.cpp
│   │    │            ├─ ChaseState.h/.cpp
│   │    │            ├─ AttackState.h/.cpp
│   │    │            └─ HurtState.h/.cpp
│   │    ├─ MovementComponent.h/.cpp    // 统一移动逻辑
│   │    └─ ColliderComponent.h/.cpp    // 碰撞体、检测、范围
│   │
│   └─ Monster/
│        ├─ MonsterData.h               // 掉落表、仇恨范围
│        ├─ AI/
│        │     ├─ MonsterAI.h/.cpp      // 行为树/状态机桥接
│        │     └─ BehaviourTree/...
│        └─ Monsters/                   // 更多怪物类型
│              ├─ Slime.h/.cpp
│              └─ Boss1.h/.cpp
│
└─ Managers/
    ├─ MonsterManager.h/.cpp
    ├─ DropManager.h/.cpp
    └─ GameController.h/.cpp
```

## 当前实现状态

| 模块                 | 状态      | 完成度 |
| -------------------- | --------- | ------ |
| 角色系统 - 组件架构  | ? 完成   | 95%    |
| 角色系统 - 玩家角色  | ? 完成   | 95%    |
| 角色系统 - 敌人/怪物 | ?? 部分   | 40%    |
| 场景系统 - 关卡框架  | ? 完成   | 95%    |
| 场景系统 - 关卡内容  | ?? 部分   | 50%    |
| UI 系统 - 菜单界面   | ? 完成   | 90%    |
| UI 系统 - 游戏内 UI  | ? 完成   | 95%    |
| 存档系统             | ? 未实现 | 0%     |
| 武器装备系统         | ?? 部分   | 60%    |
| 掉落系统             | ? 未实现 | 0%     |

### 最近更新（2025-12）

- ? 完善游戏内 UI 系统，修复 UI 组件点击区域不随角色移动的问题
- ? 整合 Monster 分支，新增怪物系统基础框架
- ? 实现 MonsterBase 怪物基类和 GoblinMonster 哥布林怪物
- ? 更新 UI 组件的 include 路径以适配新目录结构

## 开发规范

### 代码风格

- 使用 C++11/14 标准
- 类名使用 PascalCase
- 方法名使用 camelCase
- 成员变量使用 `_` 前缀（如 `_health`）
- 使用智能指针管理组件生命周期

### 物理碰撞分类

```cpp
enum class GamePhysicsCategory {
    PLAYER = 1 << 0,    // 玩家
    PLATFORM = 1 << 1,  // 平台/地面
    COLLISION = 1 << 2, // 碰撞体
    TRIGGER = 1 << 3,   // 触发器
    ENEMY = 1 << 4,     // 敌人（待添加）
    ITEM = 1 << 5,      // 道具（待添加）
};
```

### 关卡配置

```cpp
struct LevelConfig {
    std::string tmxMapPath;       // TMX 地图文件路径
    std::string backgroundPath;   // 背景图片路径
    std::string playerSpritePath; // 玩家精灵路径
    float gravity = -1000.0f;     // 重力加速度
    bool enablePhysicsDebug = false;
};
```

不要执行编译，需要编译的时候向用户提出编译请求

## 关键文件路径

### 角色系统

- `Classes/Character/Base/CharacterBase.h` - 角色基类
- `Classes/Character/Base/CharacterData.h` - 数据字典
- `Classes/Character/Player/PlayerCharacter.h` - 玩家角色
- `Classes/Character/components/AttributeComponent.h` - 属性组件
- `Classes/Character/components/StateMachineComponent.h` - 状态机组件
- `Classes/Character/components/SkillComponent.h` - 技能组件

### 怪物系统

- `Classes/Character/Monster/MonsterBase.h` - 怪物基类
- `Classes/Character/Monster/Monsters/GoblinMonster.h` - 哥布林怪物

### 场景系统

- `Classes/Scenes/GameScene.h` - 关卡基类（重要参考）
- `Classes/Scenes/DebugScene.h` - 调试场景（功能测试参考）
- `Classes/Scenes/HelloWorldScene.h` - 主菜单场景
- `Classes/Scenes/HomeScene.h` - 主页场景
- `Classes/Scenes/MapScene.h` - 地图选择场景

### UI 系统

- `Classes/UI/PlayerStatusBar.h` - 玩家状态栏（HP/MP/经验条，带伤害延迟动画）
- `Classes/UI/SkillBar.h` - 技能栏（技能图标、冷却遮罩、快捷键提示）
- `Classes/UI/BossHealthBar.h` - Boss 血条（支持多阶段、阶段转换动画）
- `Classes/UI/PauseMenu.h` - 暂停菜单（继续、设置、返回主菜单、退出）
- `Classes/GameUI.h` - 游戏内 UI 管理器（集成所有 UI 组件）
- `Classes/Scenes/Layers/SaveMenuLayer.h` - 存档菜单
- `Classes/Scenes/Layers/SetMenuLayer.h` - 设置菜单

### 管理器

- `Classes/Managers/MusicManager.h` - 音乐管理器
- `Classes/Managers/SceneTransitionManager.h` - 场景切换管理器

### 资源

- `Resources/Map/` - TMX 地图文件
- `Resources/Sprites/` - 精灵图资源
- `Resources/fonts/NotoSansSC/` - 中文字体
