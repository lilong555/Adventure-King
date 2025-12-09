# PROJECT SUMMARY

本文件用于向 AI（ChatGPT / Cursor / Copilot 等）提供项目的整体结构、模块职责、文件关系和组织方式，使其能够正确理解、分析并协助开发本项目。

项目类型：基于 Cocos2d-x 的游戏项目  
主要功能：角色系统、场景管理、转场效果、音乐管理、存档与设置界面等  
语言：C++  

---

# 1. 项目目录结构（来自 project_tree.txt）

```
Classes:.
│  AppDelegate.cpp
│  AppDelegate.h
│  
├─Character
│  ├─Base
│  │      CharacterBase.cpp
│  │      CharacterBase.h
│  │      CharacterData.h
│  │      
│  ├─components
│  │      AttributeComponent.cpp
│  │      AttributeComponent.h
│  │      SkillComponent.cpp
│  │      SkillComponent.h
│  │      StateMachineComponent.cpp
│  │      StateMachineComponent.h
│  │      
│  ├─Monster
│  │      MonsterBase.cpp
│  │      MonsterBase.h
│  │      
│  └─Player
│          PlayerCharacter.cpp
│          PlayerCharacter.h
│          
├─Managers
│      MusicManager.cpp
│      MusicManager.h
│      SceneTransitionManager.cpp
│      SceneTransitionManager.h
│      
└─Scenes
    │  DebugScene.cpp
    │  DebugScene.h
    │  GameScene.cpp
    │  GameScene.h
    │  HelloWorldScene.cpp
    │  HelloWorldScene.h
    │  HomeScene.cpp
    │  HomeScene.h
    │  MapScene.cpp
    │  MapScene.h
    │  
    └─Layers
            SaveMenuLayer.cpp
            SaveMenuLayer.h
            SetMenuLayer.cpp
            SetMenuLayer.h
```

---

# 2. 顶层模块说明

本项目主要由以下几大模块组成：

| 模块 | 说明 |
|------|------|
| Character | 游戏中所有角色的逻辑，包括玩家、怪物、组件系统 |
| Managers | 全局管理器，如音乐管理、场景转场管理 |
| Scenes | 游戏内所有场景（Scene），如主界面、游戏界面、地图界面等 |
| Scenes/Layers | 场景下的 UI 或子层，如存档菜单、设置菜单 |
| AppDelegate | 游戏主入口、初始化配置 |

---

# 3. 各目录与文件职责说明（AI 重点参考区）

## 3.1 Character（角色系统）

### ▶ Base/
基础角色类，包含角色公用的基本属性、状态与数据。

- **CharacterBase.cpp/h**  
  - 提供角色的通用行为，如移动、动画、生命相关属性等。

- **CharacterData.h**  
  - 定义角色基础数据结构，如 HP、MP、速度、属性等。

### ▶ components/
角色的可组合组件系统（类似 ECS 风格）。

- **AttributeComponent**：负责处理角色属性（HP/MAX HP/攻击/防御等）。  
- **SkillComponent**：负责技能逻辑、技能冷却、技能施放等。  
- **StateMachineComponent**：负责角色状态机（Idle/Move/Attack/Die 等）。

组件系统让 Monster 和 Player 可以复用逻辑。

### ▶ Monster/
- **MonsterBase.cpp/h**  
  - 怪物的基础逻辑，继承自 CharacterBase  
  - 控制怪物 AI、攻击逻辑、死亡处理等

### ▶ Player/
- **PlayerCharacter.cpp/h**  
  - 玩家角色类  
  - 包含玩家专属逻辑，如操作输入、菜单触发、互动控制等

---

# 3.2 Managers（全局管理系统）

### MusicManager
- 管理 BGM 和音效的播放、停止、淡入淡出等

### SceneTransitionManager
- 统一管理场景切换  
- 提供转场特效（淡入淡出、移动、模糊等）  
- 所有场景跳转都通过此类实现

---

# 3.3 Scenes（所有场景）

每个场景是一个独立的游戏界面（Scene）。

- **DebugScene**：用于调试  
- **GameScene**：游戏主场景（战斗/玩法核心）  
- **HelloWorldScene**：基础示例（可能用于测试）  
- **HomeScene**：家的场景 / 主界面  
- **MapScene**：地图场景，可能用于选择关卡或探索  

---

# 3.4 Scenes/Layers（场景子层）

### SaveMenuLayer
- 存档界面逻辑（显示存档槽、保存按钮等）

### SetMenuLayer
- 设置界面（音量、画质、按键设置等）

---

# 4. 项目入口逻辑

### AppDelegate.cpp/h

负责：

- Cocos 引擎初始化  
- 资源加载  
- 设置游戏分辨率、FPS  
- 初始化第一个场景（通常是 HomeScene / HelloWorldScene）  
- 处理游戏生命周期（进入后台、恢复等）

---

# 5. 模块间关系（AI 生成代码时必须遵守）

```
Scenes → 调用 Managers 进行场景切换、音乐控制
Scenes → 调用 Character 和组件系统更新与显示角色

CharacterBase → 使用 components/ 下的组件进行属性、技能、状态管理

MonsterBase / PlayerCharacter → 继承自 CharacterBase

Layers → 被场景创建，用于 UI 子界面
```

---

# 6. 对 AI 的使用说明（最关键）

在进行代码生成或分析时，请遵循：

1. **不要打破现有目录结构**  
2. **角色逻辑必须通过组件系统扩展，不要写在 Scene 里**  
3. **场景切换必须使用 SceneTransitionManager**  
4. **音乐播放必须通过 MusicManager**  
5. **UI 层必须在 Layers 目录内生成**  
6. **任何新增模块请放在合适的目录（Character / Managers / Scenes / Layers）**

---

# 7. 后续扩展建议（AI 也基于此思路作答）

- 添加更多组件（BuffComponent、InventoryComponent 等）  
- 扩展状态机（如 Jump、Roll、Hurt）  
- 添加更多场景（如战斗场景、背包场景）  
- 添加怪物 AI 脚本系统  
- 使用数据驱动（JSON 角色属性）  

---

# 8. AI 可以做的辅助任务

- 自动生成新场景模板  
- 扩展角色系统的组件  
- 修复角色状态机逻辑  
- 生成 UI 交互代码  
- 优化性能（如减少 draw call）  
- 编写技能系统、怪物 AI  
- 生成游戏设计文档、场景说明文档等  

---

> 本 PROJECT_SUMMARY.md 将作为 AI 理解本项目的核心索引文件。  
> 与 AI 对话时，只需引用本文件即可使 AI 获得完整上下文。
