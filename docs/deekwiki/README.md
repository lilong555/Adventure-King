# lilong555-Adventure-King-DeepWiki（中文）

> 《Adventure King》项目的完整技术文档与架构说明  
> 覆盖运行框架、角色与战斗、世界与关卡、UI、输入、存档、配置及高级主题

---

## 一、项目总览与架构（Project Overview & Architecture）

- [概述](<概述.md>)
- [系统架构](<系统架构.md>)
- [关键概念与术语](<关键概念与术语.md>)

---

## 二、核心运行框架（Core Runtime Framework）

### 2.1 场景与生命周期

- [场景系统](<场景系统.md>)
- [GameScene（游戏场景）](<GameScene（游戏场景）.md>)
- [初始化流程](<初始化流程.md>)
- [更新循环与运行时逻辑](<更新循环与运行时逻辑.md>)
- [场景切换](<场景切换.md>)
- [调试场景（DebugScene）](<调试场景（DebugScene）.md>)

### 2.2 物理与底层机制

- [物理与战斗接触回调](<物理与战斗接触回调.md>)
- [物理分类系统](<物理分类系统.md>)

---

## 三、角色与战斗系统（Characters & Combat）

### 3.1 角色基础架构

- [角色基类（CharacterBase）](<角色基类（CharacterBase）.md>)
- [组件架构](<组件架构.md>)

### 3.2 玩家系统

- [玩家角色（PlayerCharacter）](<玩家角色（PlayerCharacter）.md>)
- [升级与成长](<升级与成长.md>)
- [装备与背包](<装备与背包.md>)
- [技能系统](<技能系统.md>)

### 3.3 怪物系统

- [怪物基类（MonsterBase）](<怪物基类（MonsterBase）.md>)
- [怪物 AI 与行为](<怪物 AI 与行为.md>)
- [怪物战斗](<怪物战斗.md>)
- [具体怪物类型](<具体怪物类型.md>)
- [Boss 机制](<Boss 机制.md>)

### 3.4 战斗机制

- [伤害系统](<伤害系统.md>)

---

## 四、世界与关卡系统（World & Level）

- [世界与关卡系统](<世界与关卡系统.md>)
- [关卡地图（LevelMap）](<关卡地图（LevelMap）.md>)
- [TMX 加载与碰撞](<TMX 加载与碰撞.md>)
- [敌人生成系统](<敌人生成系统.md>)
- [竞技场战斗系统](<竞技场战斗系统.md>)
- [关卡进度](<关卡进度.md>)

---

## 五、输入与交互系统（Input & Interaction）

- [游戏输入控制器（GameInputController）](<游戏输入控制器（GameInputController）.md>)
- [输入优先级与上下文](<输入优先级与上下文.md>)

---

## 六、用户界面系统（UI System）

### 6.1 UI 框架与控制

- [用户界面](<用户界面.md>)
- [游戏 UI 控制器（GameUIController）](<游戏 UI 控制器（GameUIController）.md>)
- [UI 状态管理](<UI 状态管理.md>)

### 6.2 具体 UI 模块

- [菜单系统](<菜单系统.md>)
- [玩家 UI 组件](<玩家 UI 组件.md>)
- [HUD 元素](<HUD 元素.md>)
- [背包层（InventoryLayer）](<背包层（InventoryLayer）.md>)

---

## 七、存档与持久化系统（Save & Persistence）

### 7.1 本地存档

- [存档与持久化](<存档与持久化.md>)
- [存档管理器（SaveManager）](<存档管理器（SaveManager）.md>)
- [存档数据结构](<存档数据结构.md>)
- [存储层](<存储层.md>)
- [存取档流程](<存取档流程.md>)

### 7.2 云存档

- [云存档服务](<云存档服务.md>)
- [CloudSyncService 客户端](<CloudSyncService 客户端.md>)
- [云存档服务器](<云存档服务器.md>)
- [云端认证](<云端认证.md>)

---

## 八、配置与数据驱动（Configuration & Data）

- [配置系统](<配置系统.md>)
- [玩家与怪物配置](<玩家与怪物配置.md>)
- [技能与装备配置](<技能与装备配置.md>)
- [战斗与世界配置](<战斗与世界配置.md>)

---

## 九、高级主题与扩展（Advanced Topics）

- [高级主题](<高级主题.md>)
- [技能集实现](<技能集实现.md>)
- [动画系统](<动画系统.md>)
- [性能优化](<性能优化.md>)

---
