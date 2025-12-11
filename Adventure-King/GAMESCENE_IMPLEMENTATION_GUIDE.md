# GameScene 完善实现指南

本文档说明如何参考 DebugScene 完善 GameScene 的战斗系统和技能系统。

## 已完成的工作

### 1. GameScene.h 修改
- ✅ 添加了 `Bomb` 结构体定义
- ✅ 在 `GamePhysicsCategory` 枚举中添加了 `BOMB` 类别
- ✅ 添加了战斗状态成员变量：`_isAttacking`, `_isCastingSkill`
- ✅ 添加了炸弹系统成员变量：`std::vector<Bomb> _bombs`
- ✅ 添加了炸弹系统常量：`BOMB_THROW_SPEED_X`, `BOMB_THROW_SPEED_Y`, `BOMB_DAMAGE`, `BOMB_EXPLOSION_RADIUS`
- ✅ 添加了技能配置常量：`BOMB_SKILL_SLOT`, `BOMB_SKILL_ID`, `BOMB_SKILL_MP_COST`, `BOMB_SKILL_COOLDOWN`
- ✅ 添加了方法声明：
  - `initPlayerSkills()`
  - `playAttackAnimation()`, `onAttackAnimationFinished()`
  - `playSkillAnimation()`, `onSkillAnimationFinished()`
  - `throwBomb()`, `doThrowBomb()`, `explodeBomb()`
  - `startWalkAnimation()`, `stopWalkAnimation()`

### 2. GameScene.cpp 修改
- ✅ 实现了 `initPlayerSkills()` 方法
- ✅ 在 `initPlayer()` 中调用 `initPlayerSkills()`

## 待实现的功能

### 1. 行走动画系统

在 GameScene.cpp 的 `update()` 方法后添加：

```cpp
/**
 * @brief 开始播放行走动画
 */
void GameScene::startWalkAnimation()
{
    if (!_player || _isWalkAnimationPlaying)
        return;

    _isWalkAnimationPlaying = true;

    // 加载行走动画纹理
    auto textureCache = Director::getInstance()->getTextureCache();
    auto texture1 = textureCache->addImage("Sprites/Characters/Player/Klee/spr_klee_run_1.png");
    auto texture2 = textureCache->addImage("Sprites/Characters/Player/Klee/spr_klee_run_2.png");
    auto texture3 = textureCache->addImage("Sprites/Characters/Player/Klee/spr_klee_run.png");

    if (texture1 && texture2 && texture3)
    {
        // 从纹理创建精灵帧
        Vector<SpriteFrame *> frames;
        frames.pushBack(SpriteFrame::createWithTexture(texture1,
                                                       Rect(0, 0, texture1->getContentSize().width, texture1->getContentSize().height)));
        frames.pushBack(SpriteFrame::createWithTexture(texture2,
                                                       Rect(0, 0, texture2->getContentSize().width, texture2->getContentSize().height)));
        frames.pushBack(SpriteFrame::createWithTexture(texture3,
                                                       Rect(0, 0, texture3->getContentSize().width, texture3->getContentSize().height)));

        // 创建并运行循环动画
        auto animation = Animation::createWithSpriteFrames(frames, 0.15f);
        auto animate = Animate::create(animation);
        auto repeatAnimate = RepeatForever::create(animate);
        repeatAnimate->setTag(999);

        _player->runAction(repeatAnimate);
        CCLOG("Walk animation started");
    }
    else
    {
        CCLOG("Failed to load walk animation textures");
        _isWalkAnimationPlaying = false;
    }
}

/**
 * @brief 停止行走动画
 */
void GameScene::stopWalkAnimation()
{
    if (!_player || !_isWalkAnimationPlaying)
        return;

    _isWalkAnimationPlaying = false;

    // 保存翻转状态
    bool wasFlippedX = _player->isFlippedX();

    // 停止动画
    _player->stopActionByTag(999);

    // 恢复默认纹理
    auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_run.png");
    if (defaultTexture)
    {
        _player->setTexture(defaultTexture);
        _player->setTextureRect(Rect(0, 0,
                                     defaultTexture->getContentSize().width,
                                     defaultTexture->getContentSize().height));
        _player->setFlippedX(wasFlippedX); // 恢复翻转状态
    }

    CCLOG("Walk animation stopped");
}
```

### 2. 攻击动画系统

```cpp
/**
 * @brief 播放攻击动画
 */
void GameScene::playAttackAnimation()
{
    if (!_player)
        return;

    // 停止行走动画
    if (_isWalkAnimationPlaying)
        stopWalkAnimation();

    _isAttacking = true;

    // 获取武器信息
    auto equippedWeapon = _player->getEquippedWeapon();

    // 计算动画速度（攻击速度越高越快）
    float animSpeed = 0.15f;
    if (equippedWeapon)
    {
        animSpeed = 0.15f / equippedWeapon->attackSpeed;
    }

    // 加载攻击动画纹理
    auto texture1 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_1.png");
    auto texture2 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_2.png");
    auto texture3 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_3.png");

    if (texture1 && texture2 && texture3)
    {
        // 创建动画帧
        Vector<SpriteFrame *> frames;
        frames.pushBack(SpriteFrame::createWithTexture(texture1,
                                                       Rect(0, 0, texture1->getContentSize().width, texture1->getContentSize().height)));
        frames.pushBack(SpriteFrame::createWithTexture(texture2,
                                                       Rect(0, 0, texture2->getContentSize().width, texture2->getContentSize().height)));
        frames.pushBack(SpriteFrame::createWithTexture(texture3,
                                                       Rect(0, 0, texture3->getContentSize().width, texture3->getContentSize().height)));

        auto animation = Animation::createWithSpriteFrames(frames, animSpeed);
        auto animate = Animate::create(animation);

        // 停止之前的攻击动画
        _player->stopActionByTag(1000);

        // 创建动画序列：播放 -> 回调
        auto callbackAction = CallFunc::create([this]()
                                               { this->onAttackAnimationFinished(); });
        auto sequence = Sequence::create(animate, callbackAction, nullptr);
        sequence->setTag(1000);

        _player->runAction(sequence);

        CCLOG("Attack animation started");
    }
    else
    {
        CCLOG("Failed to load attack sprites");
        _isAttacking = false;
    }
}

/**
 * @brief 攻击动画结束回调
 */
void GameScene::onAttackAnimationFinished()
{
    _isAttacking = false;

    // 保存翻转状态
    bool wasFlippedX = _player ? _player->isFlippedX() : false;

    // 恢复角色状态
    if (_isMovingLeft || _isMovingRight)
    {
        startWalkAnimation();
    }
    else
    {
        // 恢复默认纹理
        auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
            "Sprites/Characters/Player/Klee/spr_klee_run.png");
        if (defaultTexture && _player)
        {
            _player->setTexture(defaultTexture);
            _player->setTextureRect(Rect(0, 0,
                                         defaultTexture->getContentSize().width,
                                         defaultTexture->getContentSize().height));
            _player->setFlippedX(wasFlippedX);
        }
    }

    CCLOG("Attack animation finished");
}
```

### 3. 技能动画系统

```cpp
/**
 * @brief 播放技能施放动画
 */
void GameScene::playSkillAnimation()
{
    if (!_player)
        return;

    // 停止行走动画（如果在播放）
    if (_isWalkAnimationPlaying)
    {
        stopWalkAnimation();
    }

    _isCastingSkill = true;

    // 加载3张攻击图片（技能动画暂时使用攻击动画）
    auto texture1 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_1.png");
    auto texture2 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_2.png");
    auto texture3 = Director::getInstance()->getTextureCache()->addImage(
        "Sprites/Characters/Player/Klee/spr_klee_attack_3.png");

    if (texture1 && texture2 && texture3)
    {
        // 创建精灵帧
        auto frame1 = SpriteFrame::createWithTexture(texture1,
                                                     Rect(0, 0, texture1->getContentSize().width, texture1->getContentSize().height));
        auto frame2 = SpriteFrame::createWithTexture(texture2,
                                                     Rect(0, 0, texture2->getContentSize().width, texture2->getContentSize().height));
        auto frame3 = SpriteFrame::createWithTexture(texture3,
                                                     Rect(0, 0, texture3->getContentSize().width, texture3->getContentSize().height));

        // 创建动画帧序列
        Vector<SpriteFrame *> frames;
        frames.pushBack(frame1);
        frames.pushBack(frame2);
        frames.pushBack(frame3);

        // 每帧0.13秒
        auto animation = Animation::createWithSpriteFrames(frames, 0.13f);
        auto animate = Animate::create(animation);

        // 停止之前的技能动画
        _player->stopActionByTag(1001);

        // 创建动画序列：播放动画 -> 回调结束
        auto callbackAction = CallFunc::create([this]()
                                               { this->onSkillAnimationFinished(); });
        auto sequence = Sequence::create(animate, callbackAction, nullptr);
        sequence->setTag(1001);

        _player->runAction(sequence);

        CCLOG("Skill animation started");
    }
    else
    {
        CCLOG("Failed to load skill sprites");
        _isCastingSkill = false;
    }
}

/**
 * @brief 技能动画播放完成回调
 */
void GameScene::onSkillAnimationFinished()
{
    _isCastingSkill = false;

    // 动画结束后实际丢出炸弹
    doThrowBomb();

    // 保存当前翻转状态
    bool wasFlippedX = _player ? _player->isFlippedX() : false;

    // 如果玩家仍在移动，恢复行走动画
    if (_isMovingLeft || _isMovingRight)
    {
        startWalkAnimation();
    }
    else
    {
        // 恢复到默认静止图片
        auto defaultTexture = Director::getInstance()->getTextureCache()->addImage(
            "Sprites/Characters/Player/Klee/spr_klee_run.png");
        if (defaultTexture && _player)
        {
            _player->setTexture(defaultTexture);
            _player->setTextureRect(Rect(0, 0, defaultTexture->getContentSize().width,
                                         defaultTexture->getContentSize().height));
            _player->setFlippedX(wasFlippedX);
        }
    }

    CCLOG("Skill animation finished");
}
```

### 4. 炸弹技能系统

```cpp
/**
 * @brief 释放炸弹技能
 */
void GameScene::throwBomb()
{
    if (!_player || _player->isDead())
        return;

    // 如果正在施放技能或攻击中，禁用技能
    if (_isCastingSkill || _isAttacking)
    {
        return;
    }

    // 通过技能组件释放技能（会自动检查 MP、冷却，并扣除 MP）
    auto skillComp = _player->getSkillComponent();
    if (!skillComp)
    {
        CCLOG("Skill component not found");
        return;
    }

    // 尝试使用槽位 0 的技能（炸弹技能）
    if (!skillComp->useActiveSkill(BOMB_SKILL_SLOT))
    {
        // 技能释放失败（可能是 MP 不足或冷却中）
        CCLOG("Skill cast failed - MP insufficient or on cooldown");
        return;
    }

    // 技能释放成功，播放技能动画
    playSkillAnimation();
    CCLOG("Skill started: Throw Bomb");
}

/**
 * @brief 实际创建并投掷炸弹
 */
void GameScene::doThrowBomb()
{
    if (!_player || _player->isDead())
        return;

    // 创建炸弹精灵
    auto bombSprite = Sprite::create("Sprites/Characters/Player/Klee/TNT.png");
    if (!bombSprite)
    {
        CCLOG("Failed to create bomb sprite");
        return;
    }

    // 创建炸弹对象
    Bomb bomb;
    bomb.isExploded = false;
    bomb.sprite = bombSprite;

    // 根据角色朝向决定炸弹方向
    bool facingLeft = _player->isFlippedX();
    float throwDirX = facingLeft ? -1.0f : 1.0f;

    // 设置炸弹初始位置（角色上方）
    Vec2 playerPos = _player->getPosition();
    float offsetX = throwDirX * bomb.sprite->getContentSize().width;
    float offsetY = bomb.sprite->getContentSize().height;
    bombSprite->setPosition(playerPos + Vec2(offsetX, offsetY));
    bombSprite->setScale(0.5f);

    // 创建炸弹物理刚体
    PhysicsMaterial bombMaterial(0.5f, 0.3f, 0.2f);                    // 密度、弹性、摩擦
    auto physicsBody = PhysicsBody::createCircle(15.0f, bombMaterial); // 圆形碰撞体
    physicsBody->setDynamic(true);
    physicsBody->setMass(0.5f);
    physicsBody->setRotationEnable(true); // 允许旋转

    // 设置碰撞掩码
    physicsBody->setCategoryBitmask(static_cast<int>(GamePhysicsCategory::BOMB));
    physicsBody->setCollisionBitmask(static_cast<int>(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION));
    physicsBody->setContactTestBitmask(static_cast<int>(GamePhysicsCategory::PLATFORM | GamePhysicsCategory::COLLISION));

    bombSprite->addComponent(physicsBody);
    _gameLayer->addChild(bombSprite, 4);

    // 施加初始速度（冲量）
    Vec2 impulse(throwDirX * BOMB_THROW_SPEED_X * physicsBody->getMass(),
                 BOMB_THROW_SPEED_Y * physicsBody->getMass());
    physicsBody->applyImpulse(impulse);

    _bombs.push_back(bomb);

    CCLOG("Bomb thrown with physics!");
}

/**
 * @brief 处理炸弹爆炸
 */
void GameScene::explodeBomb(Bomb &bomb)
{
    if (!bomb.sprite)
        return;

    Vec2 explodePos = bomb.sprite->getPosition();

    // 移除炸弹精灵
    bomb.sprite->removeFromParent();

    // 创建爆炸效果
    auto boomSprite = Sprite::create("Sprites/Characters/Player/Klee/BOOM_1.png");
    if (boomSprite)
    {
        boomSprite->setPosition(explodePos);
        boomSprite->setScale(0.8f);
        _gameLayer->addChild(boomSprite, 6);

        // 爆炸动画：放大 + 淡出
        auto scaleUp = ScaleTo::create(0.2f, 1.2f);
        auto fadeOut = FadeOut::create(0.3f);
        auto spawn = Spawn::create(scaleUp, fadeOut, nullptr);
        auto remove = RemoveSelf::create();
        auto sequence = Sequence::create(spawn, remove, nullptr);
        boomSprite->runAction(sequence);
    }

    bomb.sprite = nullptr;

    CCLOG("Bomb exploded at (%.0f, %.0f)", explodePos.x, explodePos.y);
}
```

### 5. 完善输入处理

在 `onKeyPressed` 方法中添加攻击和技能按键：

```cpp
void GameScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event *event)
{
    // ESC 键始终响应（用于暂停/恢复）
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        togglePauseMenu();
        return;
    }

    // 暂停时忽略其他输入
    if (_isPaused || !_player)
        return;

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _isMovingLeft = true;
        _player->setFlippedX(true);
        if (!_isAttacking && !_isCastingSkill)
            startWalkAnimation();
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = true;
        _player->setFlippedX(false);
        if (!_isAttacking && !_isCastingSkill)
            startWalkAnimation();
        break;

    case EventKeyboard::KeyCode::KEY_W:
        // 先检查传送门交互
        if (!handleGateInteraction())
        {
            // 没有传送门交互则执行跳跃
            handleJump();
        }
        break;

    case EventKeyboard::KeyCode::KEY_SPACE:
        // 空格键只跳跃，不触发传送门
        handleJump();
        break;

    // 新增：攻击按键
    case EventKeyboard::KeyCode::KEY_J:
    case EventKeyboard::KeyCode::KEY_4:
        if (!_isAttacking && !_isCastingSkill && _player)
        {
            _player->attack();
            playAttackAnimation();
        }
        break;

    // 新增：技能按键
    case EventKeyboard::KeyCode::KEY_E:
    case EventKeyboard::KeyCode::KEY_K:
        throwBomb();
        break;

    default:
        break;
    }
}

void GameScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *event)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _isMovingLeft = false;
        break;

    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _isMovingRight = false;
        break;

    default:
        break;
    }

    // 所有方向键释放时停止动画
    if (!_isMovingLeft && !_isMovingRight && !_isAttacking && !_isCastingSkill)
    {
        stopWalkAnimation();
    }
}
```

### 6. 更新碰撞处理

在 `onContactBegin` 方法中添加炸弹碰撞处理：

```cpp
bool GameScene::onContactBegin(PhysicsContact &contact)
{
    auto nodeA = contact.getShapeA()->getBody()->getNode();
    auto nodeB = contact.getShapeB()->getBody()->getNode();

    if (!nodeA || !nodeB)
        return true;

    int categoryA = contact.getShapeA()->getBody()->getCategoryBitmask();
    int categoryB = contact.getShapeB()->getBody()->getCategoryBitmask();

    // 检测玩家与平台/碰撞体的接触
    bool playerIsA = (categoryA & GamePhysicsCategory::PLAYER);
    bool playerIsB = (categoryB & GamePhysicsCategory::PLAYER);
    bool platformContact =
        (playerIsA && ((categoryB & GamePhysicsCategory::PLATFORM) || (categoryB & GamePhysicsCategory::COLLISION))) ||
        (playerIsB && ((categoryA & GamePhysicsCategory::PLATFORM) || (categoryA & GamePhysicsCategory::COLLISION)));

    if (platformContact)
    {
        // 获取碰撞法向量判断是否从上方落下
        auto contactData = contact.getContactData();
        if (contactData)
        {
            Vec2 normal = contactData->normal;

            // 如果玩家是 B，法向量需要反转
            if (playerIsB)
            {
                normal = -normal;
            }

            // normal 是从玩家指向平台的向量
            // 如果 normal.y < GROUND_NORMAL_THRESHOLD，说明平台在玩家下方
            if (normal.y < GROUND_NORMAL_THRESHOLD)
            {
                _groundContactCount++;
                _isGrounded = true;
                CCLOG("Player grounded (normal.y=%.2f), contacts: %d", normal.y, _groundContactCount);
            }
        }
    }

    // 新增：炸弹与平台碰撞 - 触发爆炸
    bool bombIsA = (categoryA & GamePhysicsCategory::BOMB);
    bool bombIsB = (categoryB & GamePhysicsCategory::BOMB);
    bool bombPlatformContact =
        (bombIsA && ((categoryB & GamePhysicsCategory::PLATFORM) || (categoryB & GamePhysicsCategory::COLLISION))) ||
        (bombIsB && ((categoryA & GamePhysicsCategory::PLATFORM) || (categoryA & GamePhysicsCategory::COLLISION)));

    if (bombPlatformContact)
    {
        Node *bombNode = bombIsA ? nodeA : nodeB;

        // 查找对应的炸弹并爆炸
        for (auto &bomb : _bombs)
        {
            if (bomb.sprite == bombNode && !bomb.isExploded)
            {
                bomb.isExploded = true;
                explodeBomb(bomb);
                break;
            }
        }
    }

    return true;
}
```

## 使用说明

### 按键映射
- **A/D 或 左/右箭头**：左右移动
- **W 或 空格**：跳跃
- **J 或 4**：攻击
- **E 或 K**：释放炸弹技能
- **ESC**：暂停菜单

### 注意事项
1. 确保所有精灵资源文件存在于正确的路径
2. 炸弹技能需要消耗 10 MP，冷却时间 1 秒
3. 攻击和技能动画会互斥，不能同时播放
4. 行走动画在攻击或施法时会自动停止

## 编译测试

完成所有实现后：
1. 使用 VSCode 的 Build Debug 任务编译项目
2. 使用 Run Game 任务运行游戏
3. 进入任意关卡测试新功能

## 参考文件
- `Adventure-King/Classes/Scenes/DebugScene.cpp` - 完整的参考实现
- `Adventure-King/Classes/Scenes/DebugScene.h` - 头文件参考
