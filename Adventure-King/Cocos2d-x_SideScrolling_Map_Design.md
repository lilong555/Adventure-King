# Cocos2d-x 横版卷轴2D地图技术实现文档

## 结论先行

Cocos2d-x 对卷轴式2D地图 **有强大且成熟的原生支持**，但它并非提供一个“一键式”的 `SideScrollingMap` 类。相反，引擎提供了实现该功能所需的所有核心组件（“积木”），开发者需要通过组合这些原生类来构建功能完整的卷轴地图系统。

三大核心原生支撑点：

1.  **`TMXTiledMap`**: 用于加载、渲染和交互由 Tiled Map Editor 创建的大型瓦片地图。这是构建可滚动关卡的基础。
2.  **`Follow` 动作 / 手动相机**: 用于实现镜头跟主角移动的视觉效果，这是“卷轴”效果的核心。
3.  **`ParallaxNode`**: 用于实现多层背景以不同速度移动的“视差滚动”效果，极大地增强了地图的深度感和视觉表现力。

---

## 1. 卷轴 2D 地图在 Cocos2d-x 中的原生支撑点

### 1.1 `TMXTiledMap`：瓦片地图系统

TMX 是一种基于 XML 的瓦片地图格式，其标准编辑器是 **Tiled Map Editor**。Cocos2d-x 提供了 `cocos2d::TMXTiledMap`, `TMXLayer`, `TMXObjectGroup` 等一系列类来完整支持 TMX 格式。

-   **功能**:
    -   直接加载 `.tmx` 地图文件并高效渲染。
    -   支持正交（orthogonal）、等角（isometric）、六边形（hexagonal）三种地图类型，横版卷轴游戏通常使用 **正交** 类型。
    -   完整支持图层、对象层、自定义属性等 Tiled 编辑器的所有核心功能。
-   **用途**: 实现远大于屏幕尺寸的地图滚动。相比于使用一张巨大的 PNG 图片作为背景，`TMXTiledMap` 通过分块渲染，极大地节省了内存和显存。

### 1.2 `Follow` 动作 / 摄像机跟随

Cocos2d-x 内置了 `cocos2d::Follow` 动作类。其核心作用是让一个节点（通常是包含地图的 `Layer`）“跟随”另一个节点（通常是玩家角色 `PlayerCharacter`）移动。

-   **原理**: `Follow` 动作会反向移动执行该动作的节点。当玩家向右移动时，`Layer` 会向左移动，从而在视觉上形成“相机锁定玩家”的效果。
-   **用途**: 这是实现视口（Viewport）滚动的最便捷方式。

### 1.3 `ParallaxNode`：视差背景

`cocos2d::ParallaxNode` 是一个特殊节点，专门用于模拟 **视差滚动（Parallax Scrolling）**。

-   **原理**: `ParallaxNode` 的每个子节点都可以设置一个 `parallaxRatio`（视差系数）。当 `ParallaxNode` 移动时，子节点会根据 `(父节点移动距离 * 视差系数)` 来计算自己的移动距离。
-   **用途**: 创建多层背景（如远山、近树、天空）并让它们以不同速度移动，产生强烈的远近纵深感，这是高质量卷轴游戏的标配。

---

## 2. 整体设计：用原生类实现横版卷轴地图

### 2.1 地图架构概览

推荐将所有与地图相关的元素封装在一个独立的 `MapLayer` 类中。

```cpp
// MapLayer.h
class MapLayer : public cocos2d::Layer
{
public:
    CREATE_FUNC(MapLayer);
    virtual bool init() override;

    void setPlayer(cocos2d::Node* player);
    // 可选：每帧更新，用于手动滚动
    virtual void update(float dt) override;

private:
    cocos2d::TMXTiledMap* _tileMap = nullptr;
    cocos2d::TMXLayer* _groundLayer = nullptr;
    cocos2d::TMXObjectGroup* _objectGroup = nullptr;

    cocos2d::Node* _player = nullptr; // 你的 PlayerCharacter

    cocos2d::Size _mapSizeInPixels;
    cocos2d::Size _visibleSize;

    void loadMap(const std::string& tmxFile);
    void updateViewPoint(); // 根据玩家位置手动更新视口
};
```

**核心流程**:

1.  在 `MapLayer` 中，使用 `TMXTiledMap` 加载一张横向很长的 TMX 地图。
2.  将主角（`PlayerCharacter`）添加为 `MapLayer` 或 `TMXTiledMap` 的子节点。
3.  通过 `Follow` 动作或手动更新 `MapLayer` 的 `position`，实现镜头跟随主角滚动的效果。

---

## 3. 使用 `TMXTiledMap` 搭建卷轴地图

### 3.1 资源准备：用 Tiled 制作 TMX

1.  **创建地图**: 在 Tiled 中选择 **Orthogonal（正交）** 类型，设置合适的地图尺寸（如 200x15 tiles）和瓦片尺寸（如 32x32 pixels）。
2.  **建议分层**:
    -   `background`: 纯装饰性背景。
    -   `ground`: 玩家可以行走的地面和平台。
    -   `collision`: (可选)一个专门用于标记碰撞区域的图层。
    -   `objects`: 对象层，用于标记玩家出生点、敌人刷新点、传送门等逻辑对象。
3.  **添加属性**:
    -   可以在图块集（Tileset）中为某类瓦片添加自定义属性，如 `collidable = true`。
    -   也可以在对象层直接绘制矩形来标记碰撞区域。

> **注意**: Cocos2d-x 版本对 TMX 文件的版本有一定要求，如果加载失败，请检查 Tiled 导出 TMX 格式时的版本兼容性。

### 3.2 在 Cocos2d-x 中加载地图

```cpp
// MapLayer.cpp
bool MapLayer::init()
{
    if (!Layer::init())
        return false;

    _visibleSize = Director::getInstance()->getVisibleSize();
    loadMap("maps/level01.tmx"); // 加载你的地图文件
    scheduleUpdate();
    return true;
}

void MapLayer::loadMap(const std::string& tmxFile)
{
    _tileMap = TMXTiledMap::create(tmxFile);
    this->addChild(_tileMap, 0);

    // 按需获取图层和对象组
    _groundLayer = _tileMap->getLayer("ground");
    _objectGroup = _tileMap->getObjectGroup("objects");

    // 提前计算地图像素总尺寸，用于边界判断
    auto mapTileSize = _tileMap->getMapSize();
    auto tileSize = _tileMap->getTileSize();
    _mapSizeInPixels = Size(mapTileSize.width * tileSize.width,
                           mapTileSize.height * tileSize.height);
}
```

---

## 4. 视口滚动方式一：手动更新 `position`

这是最灵活的方式，其原理是：**保持玩家在屏幕中心附近，反向移动地图节点**。

```cpp
void MapLayer::setPlayer(Node* player)
{
    _player = player;
    // 将玩家添加为地图的子节点，其坐标系相对于地图左下角
    _tileMap->addChild(_player, 10);
}

void MapLayer::update(float dt)
{
    if (!_player || !_tileMap) return;
    updateViewPoint();
}

void MapLayer::updateViewPoint()
{
    auto playerPos = _player->getPosition();

    // 1. 计算理论上的镜头中心点，并clamp在地图边界内
    float x = std::max(playerPos.x, _visibleSize.width / 2);
    x = std::min(x, _mapSizeInPixels.width - _visibleSize.width / 2);

    float y = std::max(playerPos.y, _visibleSize.height / 2);
    y = std::min(y, _mapSizeInPixels.height - _visibleSize.height / 2);

    // 2. 计算地图需要移动到的位置
    Vec2 actualCenter(_visibleSize.width / 2, _visibleSize.height / 2);
    Vec2 viewPoint = actualCenter - Vec2(x, y);

    // 3. 设置地图（或整个 MapLayer）的位置
    this->setPosition(viewPoint);
}
```

---

## 5. 视口滚动方式二：使用 `Follow` 原生动作

这是更简洁的方式，利用了 Cocos2d-x 的 Action 系统。

```cpp
// 在你的主场景或 GameController 中
auto mapLayer = MapLayer::create();
this->addChild(mapLayer);

auto player = PlayerCharacter::create();
mapLayer->setPlayer(player); // 这里 setPlayer 仅用于建立引用和 addChild

// 定义地图的世界边界
Rect worldBound(0, 0, mapLayer->getMapWidthInPixels(), mapLayer->getMapHeightInPixels());

// 关键：让 mapLayer 这个节点去跟随 player 节点
auto followAction = Follow::create(player, worldBound);
mapLayer->runAction(followAction);
```

-   **优点**: 代码非常简洁，不用自己处理边界计算。
-   **缺点**: 灵活性稍差。若想在过场动画等场景中临时禁用跟随，需要手动 `stopAction`。

---

## 6. 结合 `ParallaxNode` 实现多层视差

```cpp
// 在主场景或 MapLayer 中创建
auto parallaxNode = ParallaxNode::create();
this->addChild(parallaxNode, -1); // 确保在地图层后面

// 1. 添加背景层（z-order, 视差系数, 位置）
auto sky = Sprite::create("bg_sky.png");
parallaxNode->addChild(sky, 0, Vec2(0.2f, 1.0f), Vec2::ZERO); // 移动最慢

auto mountains = Sprite::create("bg_mountains.png");
parallaxNode->addChild(mountains, 1, Vec2(0.5f, 1.0f), Vec2::ZERO);

// 2. 将 TMXTiledMap 作为最前景层（视差系数为1）
parallaxNode->addChild(_tileMap, 2, Vec2(1.0f, 1.0f), Vec2::ZERO);

// 3. 滚动 ParallaxNode 而不是 TMXTiledMap
auto followAction = Follow::create(player, worldBound);
parallaxNode->runAction(followAction); // 让视差节点跟随玩家
```

---

## 7. 瓦片地图的碰撞与逻辑

### 7.1 坐标转换

TMX 地图原点在左上角，Cocos2d-x 在左下角，因此坐标转换是必需的。

```cpp
// 将像素坐标（Cocos坐标系）转换为瓦片坐标（TMX坐标系）
Vec2 MapLayer::tileCoordForPosition(const Vec2& posInPixels)
{
    auto tileSize = _tileMap->getTileSize();
    auto mapSize  = _tileMap->getMapSize();

    int x = posInPixels.x / tileSize.width;
    int y = (mapSize.height * tileSize.height - posInPixels.y) / tileSize.height;
    return Vec2(x, y);
}

// 将瓦片坐标转换为像素坐标（瓦片中心点）
Vec2 MapLayer::positionForTileCoord(const Vec2& tileCoord)
{
    auto tileSize = _tileMap->getTileSize();
    auto mapSize  = _tileMap->getMapSize();

    float x = tileCoord.x * tileSize.width + tileSize.width / 2;
    float y = (mapSize.height - tileCoord.y - 1) * tileSize.height + tileSize.height / 2;
    return Vec2(x, y);
}
```

### 7.2 基于瓦片属性的碰撞检测

```cpp
bool MapLayer::isCollidable(const Vec2& posInPixels)
{
    // 1. 将玩家的像素位置转换为瓦片坐标
    Vec2 tileCoord = tileCoordForPosition(posInPixels);

    // 2. 获取该坐标下的瓦片 GID (Global Identifier)
    auto tileGID = _collisionLayer->getTileGIDAt(tileCoord);
    if (tileGID == 0) return false; // GID为0表示没有瓦片

    // 3. 从地图中查询该 GID 对应的属性
    auto properties = _tileMap->getPropertiesForGID(tileGID).asValueMap();
    auto it = properties.find("collidable");

    // 4. 检查属性是否存在且为 "true"
    return (it != properties.end() && it->second.asString() == "true");
}
```

---

## 8. 性能与注意事项

-   **地图尺寸**: 注意图块集（Tileset）的图片尺寸不要超过目标设备支持的最大纹理尺寸（例如 2048x2048 或 4096x4096）。
-   **图层数量**: 保持渲染的图层数量在合理范围内（通常建议少于4-5层），过多的图层会增加 Draw Call，影响性能。
-   **滚动抖动**: 如果滚动时出现像素抖动或裂缝，通常与设计分辨率（Design Resolution）的缩放策略有关，请检查 `ResolutionPolicy` 并尽量确保坐标为整数。
