# CLAUDE.md

该文件为 Claude Code (claude.ai/code) 在此代码库中工作时提供指导。

## 项目概述

本项目是一款基于 C++ 和 Cocos2d-x 引擎开发的横版动作冒险游戏，名为《冒险王之神兵传奇》。游戏的核心是扮演一名冒险者，通过探索世界、击败怪物和收集装备来提升实力。

## 如何构建和运行

1.  **环境要求**:
    *   Windows 操作系统
    *   Visual Studio (建议使用 2019 或更高版本)
2.  **构建步骤**:
    *   使用 Visual Studio 打开位于 `Adventure-King/proj.win32/Adventure-King.sln` 的解决方案文件。
    *   在 Visual Studio 中，选择 `Debug` 或 `Release` 配置。
    *   选择 `x86` 或 `x64` 平台（根据你的环境）。
    *   点击“生成” > “生成解决方案”(Build > Build Solution) 来编译项目。
3.  **运行项目**:
    *   编译成功后，点击“调试” > “开始执行(不调试)”(Debug > Start Without Debugging) 或按 `Ctrl+F5` 来运行游戏。

## 代码架构和组件功能

### 目录结构

-   **`Adventure-King/Classes`**: 存放游戏的核心逻辑代码，包括所有场景 (Scene) 和自定义游戏对象的实现。
-   **`Adventure-King/Resources`**: 存放所有游戏资源，如图片、字体、音频等。
-   **`Adventure-King/cocos2d`**: Cocos2d-x 引擎的源代码。
-   **`Adventure-King/proj.win32`**: 适用于 Windows 平台的项目文件和入口点 (`main.cpp`)。

### 关键类与功能详解

#### 1. `AppDelegate` (应用代理)

-   **职责**: 作为整个应用的入口和生命周期管理器。
-   **已实现功能**:
    -   `applicationDidFinishLaunching()`:
        -   初始化游戏导演 (`Director`) 和 OpenGL 视图。
        -   设置游戏窗口标题为 "Adventure-King"。
        -   设定固定的设计分辨率为 `1520x840`，并采用 `ResolutionPolicy::SHOW_ALL` 策略以保证内容等比缩放，适应不同屏幕。
        -   加载并运行初始场景 `HelloWorldScene`。
    -   `applicationDidEnterBackground()` / `applicationWillEnterForeground()`:
        -   处理应用的后台切换逻辑，例如在应用进入后台时暂停动画和音乐，返回前台时恢复。

#### 2. `HelloWorldScene` (主菜单场景)

-   **职责**: 作为游戏的主菜单界面。
-   **已实现功能**:
    -   显示游戏标题。
    -   包含两个主要菜单项：
        1.  **开始游戏**: 点击后，会通过过渡动画切换到 `MapScene` (地图选择场景)。
        2.  **退出游戏**: 点击后，关闭应用程序。
    -   作为场景栈的根场景，可以通过 `Director::popToRootScene()` 返回到此界面。

#### 3. `HomeScene` (玩家之家场景)

-   **职责**: 一个简单的过渡或占位场景，目前定义为“冒险王之家”。
-   **已实现功能**:
    -   在屏幕中央显示 "进入冒险王之家..." 的文本。
    -   左上角有一个返回按钮，点击后会通过蓝色淡出效果切换回 `HelloWorldScene`。
    -   目前在主游戏流程中的作用比较独立。

#### 4. `MapScene` (地图选择场景)

-   **职责**: 提供一个可视化的世界地图，让玩家选择要进入的关卡。
-   **已实现功能**:
    -   **背景和布局**: 加载 `MapBackground.png`作为全屏背景，并创建一个容器节点 `contentContainer` 来统一管理所有地图元素，实现整体缩放以适应屏幕。
    -   **动态关卡标记**:
        -   通过 `_markerInfos` 结构体数组定义了多个关卡数据，包括普通状态图标、选中状态图标、在地图上的位置、缩放比例和关卡名称。
        -   在 `init()` 方法中，遍历 `_markerInfos` 来动态创建每个关卡的 `Sprite` 标记，并将关卡名称 `Label` 作为其子节点显示在下方。
    -   **交互逻辑**:
        -   **鼠标悬停**: 通过 `EventListenerMouse` 的 `onMouseMove` 事件实现。当鼠标指针悬停在某个关卡标记上时，该标记的纹理会切换为高亮状态的图片 (`selectedImage`)。
        -   **鼠标点击**: 通过 `onMouseDown` 事件实现。当玩家点击一个关卡标记时，会调用 `onMapMarkerClicked(mapId)` 函数。
    -   **场景切换**:
        -   `onMapMarkerClicked(mapId)`: 根据传入的 `mapId`，调用 `createDestinationScene` 函数来创建对应的关卡场景。
        -   `createDestinationScene(mapId)`: 使用 `switch` 语句，根据 `mapId` 返回一个新创建的场景实例（例如，`mapId` 为 1 时创建 `OriginMushroomScene`）。
        -   创建成功后，使用黑色淡出效果 (`TransitionFade`) 切换到目标关卡场景。

#### 5. `DebugScene` (角色功能调试场景)

-   **职责**: 一个用于独立测试和调试角色核心功能的开发场景。它不属于主游戏流程，但对开发至关重要。
-   **已实现功能**:
    -   **UI 调试面板**:
        -   实时显示角色的详细属性（等级、经验、HP/MP、力量、防御等）。
        -   显示角色的当前状态（待机、行走、跳跃、攻击等）。
        -   提供一系列控制按钮，用于模拟各种游戏事件，如“受击”、“治疗”、“攻击”、“升级”和“重置”。
    -   **物理与环境**:
        -   包含多个平台，用于测试跳跃和重力。
        -   实现了一个独立的物理循环，处理重力、跳跃和平台碰撞检测。
    -   **战斗测试**:
        -   可以触发角色的攻击和技能（如投掷炸弹）。
        -   场景中放置了一个木桩 (`TargetDummy`) 作为攻击目标，可以承受伤害并显示血条和伤害数字，方便测试伤害计算。
    -   **键盘控制**: 支持使用 `A`/`D` 键移动，`W`/`Space` 键跳跃，以及其他快捷键来触发调试功能。

### 场景流程

1.  **启动**: `AppDelegate` 加载 `HelloWorldScene`。
2.  **主菜单**: 在 `HelloWorldScene` 中，玩家点击“开始游戏”。
3.  **选择关卡**: 切换到 `MapScene`，玩家在地图上点击一个关卡标记。
4.  **进入游戏**: 从 `MapScene` 切换到对应 `mapId` 的游戏场景（如 `OriginMushroomScene` 或 `MysteryForestScene`）。
5.  **返回**: 在 `MapScene` 中点击关闭按钮可以返回到 `HelloWorldScene`。
6.  **调试流程**: (独立于主流程) 开发者可以直接修改 `AppDelegate.cpp`，将 `HelloWorldScene::createScene()` 替换为 `DebugScene::createScene()` 来直接启动调试场景。
