# CLAUDE.md

该文件为 Claude Code (claude.ai/code) 在此代码库中工作时提供指导。

## 项目概述

本项目是一款基于 C++ 和 Cocos2d-x 引擎开发的横版动作冒险游戏，名为《冒险王之神兵传奇》。游戏的核心是扮演一名冒险者，通过探索世界、击败怪物和收集装备来提升实力。

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

### 场景流程

1.  **启动**: `AppDelegate` 加载 `HelloWorldScene`。
2.  **主菜单**: 在 `HelloWorldScene` 中，玩家点击“开始游戏”。
3.  **选择关卡**: 切换到 `MapScene`，玩家在地图上点击一个关卡标记。
4.  **进入游戏**: 从 `MapScene` 切换到对应 `mapId` 的游戏场景（如 `OriginMushroomScene` 或 `MysteryForestScene`）。
5.  **返回**: 在 `MapScene` 中点击关闭按钮可以返回到 `HelloWorldScene`。
