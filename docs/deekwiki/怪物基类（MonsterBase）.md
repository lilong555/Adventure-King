# 怪物基类（MonsterBase）

> **相关源文件**
> * [Adventure-King/Classes/Character/Base/CharacterData.h](https://github.com/lilong555/Adventure-King/blob/60df0f40/Adventure-King/Classes/Character/Base/CharacterData.h)
> * [Adventure-King/Classes/Character/Monster/MonsterBase.cpp](https://github.com/lilong555/Adventure-King/blob/60df0f40/Adventure-King/Classes/Character/Monster/MonsterBase.cpp)
> * [Adventure-King/Classes/Character/Monster/MonsterBase.h](https://github.com/lilong555/Adventure-King/blob/60df0f40/Adventure-King/Classes/Character/Monster/MonsterBase.h)
> * [Adventure-King/Classes/Character/Monster/Monsters/GoblinMonster.cpp](https://github.com/lilong555/Adventure-King/blob/60df0f40/Adventure-King/Classes/Character/Monster/Monsters/GoblinMonster.cpp)
> * [Adventure-King/Classes/Character/Monster/Monsters/GoblinMonster.h](https://github.com/lilong555/Adventure-King/blob/60df0f40/Adventure-King/Classes/Character/Monster/Monsters/GoblinMonster.h)

## 目的与范围

`MonsterBase` 是 Adventure-King 中所有敌对怪物的抽象基类。它提供一套完整的 AI 驱动战斗实体系统，包含仇恨/牵引（aggro/leash）机制、巡逻行为、攻击协调以及基于物理的移动。该类继承自 `CharacterBase`（[3.3](角色基类（CharacterBase）.md)），并在其基础上扩展怪物专属 AI 逻辑与生命周期管理。

玩家角色实现请参见 [PlayerCharacter](玩家角色（PlayerCharacter）.md)。AI 行为细节请参见 [Monster AI and Behavior](怪物 AI 与行为.md)。战斗机制请参见 [Monster Combat](怪物战斗.md)。

**来源：** [Classes/Character/Monster/MonsterBase.h L1-L184](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L1-L184)

 [Classes/Character/Monster/MonsterBase.cpp L1-L1111](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L1-L1111)

---

## 类架构与继承关系

```mermaid
classDiagram
    class CharacterBase {
        «abstract»
        #_currentHP
        #_currentMP
        #_maxHP
        +takeDamage(DamageInfo)
        +die()
        +attack()
    }
    class MonsterBase {
        «abstract»
        #_target
        #_aggroRadius
        #_leashRadius
        #_patrolEnabled
        #_aiUpdateInterval
        +init(spriteFrameName)
        +update(dt)
        +attack()
        +takeDamage(DamageInfo)
        +die()
        +setTarget(Node*)
        +setHome(Vec2)
        +enablePatrol(Vec2, Vec2)
        +applyHpScalingForPlayerLevel(int, bool)
        #updateAI(dt)
        #updateMovement(dt)
        #updateAttack(dt)
        #spawnMeleeHitbox(...)
    }
    class GoblinMonster {
        -_attackAnimate
        +attack()
        #getExpReward(int)
        #initAnimations()
        #initAttributes()
    }
    class AttributeComponent {
        +setBaseAttributes(Attributes)
        +recalculateFinalAttributes()
        +getAttributeValue(AttributeType)
    }
    class StateMachineComponent {
        +changeState(CharacterState)
        +getCurrentState()
        +registerStateAnimation(state, key)
    }
    class SkillComponent {
        +useActiveSkill(slotIndex)
    }
    class StatusEffectVfxComponent {
        «visual effects»
    }
    CharacterBase <|-- MonsterBase : "uses"
    MonsterBase <|-- GoblinMonster : "uses"
    AttributeComponent --o MonsterBase : "uses"
    StateMachineComponent --o MonsterBase : "uses"
    SkillComponent --o MonsterBase
    StatusEffectVfxComponent --o MonsterBase
```

**来源：** [Classes/Character/Monster/MonsterBase.h L10-L184](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L10-L184)

 [Classes/Character/Monster/Monsters/GoblinMonster.h L7-L39](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.h#L7-L39)

 [Classes/Character/Base/CharacterData.h L1-L247](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Base/CharacterData.h#L1-L247)

---

## 初始化流水线

### 组件挂载与初始化

`init()` 会遵循严格的初始化顺序，确保所有组件在使用前已正确挂载：

| 步骤 | 动作 | 行号 |
| --- | --- | --- |
| 1 | 加载 sprite 贴图 | [MonsterBase.cpp L68-L78](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L68-L78) |
| 2 | 挂载 `AttributeComponent` | [MonsterBase.cpp L84-L90](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L84-L90) |
| 3 | 挂载 `StateMachineComponent` | [MonsterBase.cpp L93-L98](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L93-L98) |
| 4 | 挂载 `SkillComponent` | [MonsterBase.cpp L101-L106](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L101-L106) |
| （原文截断：`…2662 chars truncated…`） | 将更新间隔在 `[0.5*interval, interval]` 之间随机化，避免多个怪物同时生成时出现帧率尖峰 | [MonsterBase.cpp L170-L179](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L170-L179) |

> 说明：上表中包含导出时生成的截断占位 `…2662 chars truncated…`，这里按原样保留以避免遗漏。

**来源：** [Classes/Character/Monster/MonsterBase.cpp L66-L181](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L66-L181)

 [Classes/Character/Monster/MonsterBase.h L18-L19](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L18-L19)

---

## 物理配置

### 分类与碰撞掩码

怪物的 physics body 会配置特定的 category 与 collision/contact 掩码，以实现与其它实体的正确交互：

```mermaid
flowchart TD

Monster["MonsterBase<br>PhysicsBody"]
MONSTER["MONSTER"]
Platform["PLATFORM"]
Player["PLAYER"]
PlayerAttack["PLAYER_ATTACK"]
Bomb["BOMB"]
Collision["COLLISION"]
Player2["PLAYER"]
PlayerAttack2["PLAYER_ATTACK"]
Bomb2["BOMB"]

Monster -.->|"CollisionBitmask"| MONSTER
Monster -.->|"CollisionBitmask"| Platform
Monster -.->|"CollisionBitmask"| Player
Monster -.-> PlayerAttack
Monster -.->|"CollisionBitmask"| Bomb
Monster -.-> Collision
Monster -.->|"ContactTestBitmask(damage events)"| Player2
Monster -.-> PlayerAttack2
Monster -.->|"ContactTestBitmask"| Bomb2
```

| 掩码类型 | 类别 | 作用 |
| --- | --- | --- |
| **CategoryBitmask** | `MONSTER` | 标识该 body 属于怪物 |
| **CollisionBitmask** | `PLATFORM \| PLAYER \| PLAYER_ATTACK \| BOMB \| COLLISION` | 参与物理碰撞解算（避免重叠/穿透） |
| **ContactTestBitmask** | `PLAYER \| PLAYER_ATTACK \| BOMB` | 触发接触回调，用于伤害处理 |

**来源：** [Classes/Character/Monster/MonsterBase.cpp L132-L150](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L132-L150)

 [Classes/Configs/GamePhysicsCategory.h](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Configs/GamePhysicsCategory.h)

---

## 更新循环与节流系统

### 多层更新架构

`MonsterBase::update()` 实现了一套较复杂的节流机制，用于在响应性与 CPU 效率之间取平衡：不同子系统按不同时间间隔更新，取决于重要性与性能需求。

```mermaid
stateDiagram-v2
    [*] --> CheckDead : "alive"
    CheckDead --> Dead : "_isStunned"
    CheckDead --> CheckStunned : "alive"
    Dead --> [*] : "changeState(IDLE)"
    CheckStunned --> Stunned : "state == HURT"
    CheckStunned --> CheckHurt : "not stunned"
    Stunned --> StopMovement : "freeze velocity"
    StopMovement --> [*] : "wait for SM to return to IDLE"
    CheckHurt --> Hurt : "state == HURT"
    CheckHurt --> CheckActiveRange : "not hurt"
    Hurt --> FreezeVelocity : "wait for SM to return to IDLE"
    FreezeVelocity --> [*] : "wait for SM to return to IDLE"
    CheckActiveRange --> UpdateAI : "isWithinActiveUpdateRange()"
    CheckActiveRange --> InactivePath : "not active"
    InactivePath --> FreezeVelocity2 : "set velocity.x = 0"
    FreezeVelocity2 --> [*] : "set velocity.x = 0"
    UpdateAI --> ThrottledAI : "_aiUpdateInterval > 0"
    UpdateAI --> DirectAI : "interval == 0"
    ThrottledAI --> AICheck : "accumulator >= interval"
    ThrottledAI --> SkipAI : "interval == 0"
    AICheck --> CallUpdateAI : "updateAI(accumulator)"
    CallUpdateAI --> UpdateMovement : "_moveUpdateInterval > 0"
    SkipAI --> UpdateMovement
    DirectAI --> CallUpdateAI : "updateAI(accumulator)"
    UpdateMovement --> ThrottledMove : "_moveUpdateInterval > 0"
    UpdateMovement --> DirectMove : "interval == 0"
    ThrottledMove --> MoveCheck : "accumulator >= interval"
    ThrottledMove --> UpdateAttack : "updateMovement(accumulator)"
    MoveCheck --> CallUpdateMovement : "updateMovement(accumulator)"
    CallUpdateMovement --> UpdateAttack : "updateMovement(accumulator)"
    DirectMove --> CallUpdateMovement
    UpdateAttack --> ThrottledAttack : "_attackUpdateInterval > 0"
    UpdateAttack --> DirectAttack : "interval == 0"
    ThrottledAttack --> AttackCheck : "accumulator >= interval"
    ThrottledAttack --> [*] : "set velocity.x = 0"
    AttackCheck --> CallUpdateAttack : "updateAttack(accumulator)"
    CallUpdateAttack --> [*] : "accumulator < interval"
    DirectAttack --> CallUpdateAttack : "updateAttack(accumulator)"
```

### 更新间隔配置

| 系统 | 成员变量 | 默认值 | 作用 |
| --- | --- | --- | --- |
| **AI 逻辑** | `_aiUpdateInterval` | 0.1s | 决策（仇恨、追击、巡逻等） |
| **非活跃 AI** | `_inactiveAiUpdateInterval` | 0.3s | 远离玩家时的 AI |
| **移动** | `_moveUpdateInterval` | 0.033s（约 30fps） | 速度更新 |
| **攻击** | `_attackUpdateInterval` | 0.05s（20fps） | 攻击触发逻辑 |
| **活跃范围** | `_activeUpdateDistanceX` | `screenWidth * 1.5` | 完整 AI 的距离阈值 |

**配置示例：**

```
// In derived monster class:
setUpdateTickIntervals(
    0.1f,  // AI every 100ms
    0.05f, // Movement every 50ms
    0.05f  // Attack checks every 50ms
);
setActiveUpdateDistanceX(800.0f); // Full AI within 800 pixels
```

**来源：** [Classes/Character/Monster/MonsterBase.cpp L281-L414](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L281-L414)

 [Classes/Character/Monster/MonsterBase.h L165-L173](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L165-L173)

---

## AI 决策系统

### 状态机与目标选择

```mermaid
flowchart TD

Start["updateAI(dt)"]
CheckDead["isDead()"]
SetDead["changeState(DEAD)"]
CheckStun["_isStunned?"]
SetIdle["changeState(IDLE)<br>clear _hasMoveGoal"]
CheckAttacking["state == ATTACKING?"]
Return1["return (no AI during attack)"]
CheckTarget["_target exists?"]
CheckReturning["_returningHome?"]
NoTarget["No Target"]
TryReacquire["_primaryTarget<br>in aggro radius?"]
ReacquireTarget["_target = _primaryTarget"]
HasTarget["Target exists"]
CalcDist["distToTarget =<br>distanceTo(_target)"]
CheckAggroLost["distToTarget ><br>_aggroRadius * 1.2?"]
LoseAggro["_target = nullptr<br>_hasMoveGoal = false<br>_returningHome = false"]
CheckPatrol1["_patrolEnabled?"]
SetPatrol1["changeState(STATE_PATROL)"]
SetIdle1["changeState(IDLE)"]
CheckLeash["_leashRadius > 0<br>AND distFromHome ><br>_leashRadius?"]
InitLeash["_target = nullptr<br>_moveGoalPos = _homePos<br>_hasMoveGoal = true<br>_returningHome = true"]
SetWalking1["changeState(WALKING)"]
CheckInAttackRange["horizontalDistanceTo(_target)<br><= _attackRange?"]
StopForAttack["_hasMoveGoal = false<br>changeState(IDLE)"]
ChaseTarget["_moveGoalPos = target pos<br>_hasMoveGoal = true<br>_returningHome = false"]
SetWalking2["changeState(WALKING)"]
CheckMoveGoal["_hasMoveGoal?"]
SetWalking3["changeState(WALKING)"]
CheckPatrol2["_patrolEnabled?"]
CalcPatrolDir["Check patrol boundary<br>reverse _patrolDir if reached"]
SetPatrolGoal["_moveGoalPos = patrol endpoint<br>_hasMoveGoal = true"]
SetPatrolState["changeState(STATE_PATROL)"]
SetIdle2["changeState(IDLE)"]

Start -.->|"false"| CheckDead
CheckDead -.->|"true"| SetDead
CheckDead -.->|"true"| CheckStun
CheckStun -.->|"false"| SetIdle
CheckStun -.->|"true"| CheckAttacking
CheckAttacking -.->|"false"| Return1
CheckAttacking -.->|"false"| CheckTarget
CheckTarget -.->|"false"| CheckReturning
CheckReturning -.->|"true"| NoTarget
CheckReturning -.->|"no"| TryReacquire
TryReacquire -.->|"yes"| ReacquireTarget
TryReacquire -.->|"yes"| NoTarget
ReacquireTarget -.->|"true"| HasTarget
CheckTarget -.-> HasTarget
HasTarget -.-> CalcDist
CalcDist -.->|"no"| CheckAggroLost
CheckAggroLost -.->|"yes"| LoseAggro
LoseAggro -.->|"yes"| CheckPatrol1
CheckPatrol1 -.->|"yes"| SetPatrol1
CheckPatrol1 -.->|"no"| SetIdle1
CheckAggroLost -.->|"no"| CheckLeash
CheckLeash -.->|"yes"| InitLeash
InitLeash -.-> SetWalking1
CheckLeash -.->|"no"| CheckInAttackRange
CheckInAttackRange -.-> StopForAttack
CheckInAttackRange -.-> ChaseTarget
ChaseTarget -.-> SetWalking2
NoTarget -.->|"no"| CheckMoveGoal
CheckMoveGoal -.->|"yes"| SetWalking3
CheckMoveGoal -.->|"no"| CheckPatrol2
CheckPatrol2 -.-> CalcPatrolDir
CalcPatrolDir -.-> SetPatrolGoal
SetPatrolGoal -.-> SetPatrolState
CheckPatrol2 -.-> SetIdle2
```

### AI 配置参数

| 参数 | 类型 | 作用 | 备注 |
| --- | --- | --- | --- |
| `_aggroRadius` | `float` | 发现/追击距离 | 若为 0，则对 `_primaryTarget` 总是仇恨 |
| `_leashRadius` | `float` | 离家追击最大距离 | 若为 0，则无牵引（可无限追击） |
| `_attackRange` | `float` | 开始攻击距离 | 一般应小于 `_aggroRadius` |
| `_patrolEnabled` | `bool` | 空闲时是否巡逻 | 需要 `_patrolLeft` 与 `_patrolRight` |
| `_primaryTarget` | `Node*` | 主目标引用（通常是玩家） | 通过 `setTarget()` 设置 |
| `_target` | `Node*` | 当前追击目标 | 仇恨丢失会自动清空 |
| `_returningHome` | `bool` | 是否正牵引回家 | 防止途中再次仇恨 |

**关键行为：**

1. **仇恨丢失滞回（Hysteresis）：** 在 `1.2 * _aggroRadius` 才判定丢失目标，避免边界抖动（[MonsterBase.cpp L521](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L521-L521)）。
2. **回家期间禁止再仇恨：** `_returningHome == true` 时不会重新锁定目标，直到回到 home（[MonsterBase.cpp L482](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L482-L482)）。
3. **巡逻边界死区：** 巡逻使用 `PATROL_REACH_EPSILON` 作为端点死区，避免端点来回震荡（[MonsterBase.cpp L500-L505](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L500-L505)）。

**来源：** [Classes/Character/Monster/MonsterBase.cpp L454-L560](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L454-L560)

---

## 移动系统（追击/回家/巡逻）

> 说明：本节中包含导出时生成的截断占位 `MonsterBa…739 chars truncated…`，按原样保留以避免遗漏。

```mermaid
flowchart TD

Start["updateMovement(dt)"]
CheckBody["_physicsBody exists?"]
Return1["return"]
GetSpeed["moveSpeed = attr(MOVE_SPEED)"]
CheckTargetGoal["_target? OR _hasMoveGoal?"]
StopMove["setVelocity(0, currentVy)"]
CheckAttackRange["_target? AND<br>horizontalDistanceTo(_target) <= _attackRange?"]
CalcTarget["targetPos = _target?<br>getPositionInParentSpace(_target)<br>: _moveGoalPos"]
CalcDelta["dx = targetPos.x - myPos.x"]
CheckDeadzone["abs(dx) <=<br>CHASE_DEADZONE_X?"]
StopAtGoal["setVelocity(0, currentVy)"]
CheckClearGoal["!_target AND<br>_hasMoveGoal?"]
ClearGoal["_hasMoveGoal = false<br>_returningHome = false"]
Return2["return"]
CalcDir["dirX = (dx > 0) ? 1.0 : -1.0"]
SetVel["setVelocity(dirX * moveSpeed, currentVy)"]
CheckFace["_target exists?"]
FaceTarget["faceTarget(_target)"]
FaceX["faceToX(targetWorldX)"]
Return3["return"]

Start -.->|"yes"| CheckBody
CheckBody -.->|"no"| Return1
CheckBody -.-> GetSpeed
GetSpeed -.->|"yes"| CheckTargetGoal
CheckTargetGoal -.->|"no"| StopMove
CheckTargetGoal -.-> CheckAttackRange
CheckAttackRange -.->|"yes"| StopMove
CheckAttackRange -.->|"no"| CalcTarget
CalcTarget -.-> CalcDelta
CalcDelta -.->|"no"| CheckDeadzone
CheckDeadzone -.->|"yes"| StopAtGoal
StopAtGoal -.->|"yes"| CheckClearGoal
CheckClearGoal -.->|"no"| ClearGoal
CheckClearGoal -.->|"yes"| Return2
CheckDeadzone -.-> CalcDir
CalcDir -.-> SetVel
SetVel -.->|"no"| CheckFace
CheckFace -.-> FaceTarget
CheckFace -.-> FaceX
FaceTarget -.-> Return3
FaceX -.-> Return3
ClearGoal -.-> Return2
StopMove -.-> Return2
```

### 坐标系处理

移动系统会处理多个坐标空间：

| 函数 | 作用 | 返回类型 |
| --- | --- | --- |
| `getWorldPosition(node)` | 将节点位置转换为世界坐标 | `Vec2` |
| `getPositionInParentSpace(node)` | 将目标位置转换为怪物父节点坐标 | `Vec2` |
| `horizontalDistanceTo(node)` | 只计算 X 轴水平距离 | `float` |

**实现示例：**

```
// From MonsterBase.cpp:598-623
Vec2 targetPos = _target ? getPositionInParentSpace(_target) : _moveGoalPos;
Vec2 myPos = getPosition();
float dx = targetPos.x - myPos.x;

if (fabs(dx) <= kChaseDeadzoneX) {
    _physicsBody->setVelocity(Vec2(0, currentVy));
    return;
}

float dirX = (dx > 0.0f) ? 1.0f : -1.0f;
_physicsBody->setVelocity(Vec2(dirX * moveSpeed, currentVy));
```

**来源：** [Classes/Character/Monster/MonsterBase.cpp L566-L638](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L566-L638)

 [Classes/Character/Monster/MonsterBase.h L106-L137](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L106-L137)

---

## 战斗系统

### 攻击流程

```mermaid
sequenceDiagram
  participant p1 as update(dt)
  participant p2 as updateAttack(dt)
  participant p3 as StateMachineComponent
  participant p4 as MonsterBase
  participant p5 as Attack Hitbox
  participant p6 as PhysicsWorld

  note over p1: Every Frame
  p1->>p1: state != ATTACKING?<br/>_attackTimer += dt
  p1->>p2: Call (throttled by _attackUpdateInterval)
  p2->>p2: Check: !isDead() && !_isStunned
  p2->>p2: Check: state != HURT && != ATTACKING
  p2->>p2: Check: _target exists
  p2->>p2: Check: horizontalDistanceTo(_target) <= _attackRange
  p2->>p2: Check: _attackTimer >= _attackInterval
  alt All Checks Pass
    p2->>p4: Stop horizontal velocity
    p2->>p4: _attackTimer = fmod(_attackTimer, _attackInterval)
    p2->>p3: changeState(ATTACKING)
    p2->>p4: attack()
    note over p4: Default Implementation
    p4->>p4: getSkillComponent()->useActiveSkill(0)
    note over p4: Derived Class Override
    p4->>p4: Run attack animation
    p4->>p4: scheduleOnce(spawn hitbox at hit frame)
    p4->>p5: spawnMeleeHitbox(offset, size, damageTag, life)
    p5->>p6: Add PhysicsBody with MONSTER_ATTACK category
    note over p6: Contact Event Triggered
    p6->>p6: onContactBegin(hitbox, playerBody)
    p6->>p4: takeDamage callback (on player)
    p4->>p3: Attack animation complete
    p3->>p3: changeState(IDLE)
  end
```

### 伤害处理

当调用 `takeDamage()` 时，会执行如下步骤：

| 步骤 | 动作 | 行号参考 |
| --- | --- | --- |
| 1 | 检查是否已死亡（防止重复死亡/重复结算） | [MonsterBase.cpp L709-L712](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L709-L712) |
| 2 | 若为暴击则应用暴击倍率 | [MonsterBase.cpp L717-L720](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L717-L720) |
| 3 | 计算最终 HP 变化 | [MonsterBase.cpp L722-L723](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L722-L723) |
| 4 | 为 Boss UI 记录非 DOT 伤害 | [MonsterBase.cpp L726](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L726-L726) |
| 5 | 显示伤害数字 | [MonsterBase.cpp L728](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L728-L728) |
| 6 | 生成受击特效 | [MonsterBase.cpp L729](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L729-L729) |
| 7 | 执行“受伤后”钩子（装备效果等） | [MonsterBase.cpp L736-L740](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L736-L740) |
| 8 | 通知攻击者“造成伤害” | [MonsterBase.cpp L743-L750](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L743-L750) |
| 9 | 更新血条 | [MonsterBase.cpp L752](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L752-L752) |
| 10 | 若死亡：发放经验并调用 `die()` | [MonsterBase.cpp L754-L759](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L754-L759) |
| 11 | 若存活：进入受击状态并停止移动 | [MonsterBase.cpp L762-L807](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L762-L807) |

**暴击实现：**

```
// From MonsterBase.cpp:717-720
if (info.isCritical) {
    dmg *= info.critMultiplier; // Default 1.5x
}
```

**受击朝向逻辑：**
系统会把 `setFlippedX()` 当成额外的“镜像层”，叠加在 `scaleX` 之上，以保证受击贴图朝向攻击者方向（[MonsterBase.cpp L783-L801](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L783-L801)）。

**来源：** [Classes/Character/Monster/MonsterBase.cpp L706-L808](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L706-L808)

 [Classes/Character/Monster/MonsterBase.h L29-L30](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L29-L30)

---

## 死亡与奖励

### 死亡流程

```mermaid
flowchart TD

Die["die()"]
CheckLoot["_lootDropped<br>flag set?"]
RollDrop["random() <<br>DROP_CHANCE?"]
DisablePhysics["_physicsBody->setVelocity(ZERO)<br>setDynamic(false)<br>setCategoryBitmask(NONE)"]
RollType["random() <<br>HP_DROP_RATIO?"]
CreateHP["DropItem::create(HEALTH)"]
CreateMP["DropItem::create(MANA)"]
CalcPos["pos = getPosition() +<br>Vec2(randX, SPAWN_OFFSET_Y)"]
AddToParent["parent->addChild(item,<br>DEFAULT_CHARACTER_Z_ORDER)"]
SetFlag["_lootDropped = true"]
HideHP["_hpBar->setVisible(false)"]
StopActions["stopAllActions()"]
CallBase["CharacterBase::die()"]
Return["return"]

Die -.->|"no"| CheckLoot
CheckLoot -.->|"yes"| RollDrop
CheckLoot -.->|"yes"| DisablePhysics
RollDrop -.-> RollType
RollDrop -.->|"no"| DisablePhysics
RollType -.->|"no"| CreateHP
RollType -.-> CreateMP
CreateHP -.->|"yes"| CalcPos
CreateMP -.-> CalcPos
CalcPos -.-> AddToParent
AddToParent -.-> SetFlag
SetFlag -.-> DisablePhysics
DisablePhysics -.-> HideHP
HideHP -.-> StopActions
StopActions -.-> CallBase
CallBase -.-> Return
```

### 经验奖励系统

经验奖励由子类通过 `getExpReward(playerLevel)` 实现：

```javascript
// MonsterBase default implementation (returns 0)
int MonsterBase::getExpReward(int playerLevel) const {
    return 0;
}

// GoblinMonster override
int GoblinMonster::getExpReward(int playerLevel) const {
    return GameConfig::Monster::Goblin::EXP_REWARD_BASE +
           (playerLevel - 1) * GameConfig::Monster::Goblin::EXP_REWARD_PER_LEVEL;
}
```

`grantKillExperience()` 会按如下优先级解析玩家引用：

1. `info.attacker`（若其为 `PlayerCharacter*`）
2. `_primaryTarget`（兜底）
3. `_target`（最终兜底）

**来源：** [Classes/Character/Monster/MonsterBase.cpp L810-L853](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L810-L853)

 [Classes/Character/Monster/MonsterBase.cpp L11-L49](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L11-L49)

 [Classes/Character/Monster/Monsters/GoblinMonster.cpp L116-L125](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.cpp#L116-L125)

---

## HP 缩放系统

### 基于等级的 HP 缩放

`applyHpScalingForPlayerLevel()` 会按玩家等级缩放怪物 HP，以保持不同等级区间的难度一致：

```mermaid
flowchart TD

Start["applyHpScalingForPlayerLevel(playerLevel, isBoss)"]
Clamp["playerLevel = max(1, playerLevel)"]
GetBase["baseHp = attr->getBaseAttributes()<br>→ get(MAX_HP, 0)"]
CheckBase["baseHp > 0?"]
Fallback["baseHp = attr->getAttributeValue(MAX_HP)"]
ApplyBase["baseHp *= isBoss ?<br>BOSS_BASE_MULTIPLIER<br>: NORMAL_BASE_MULTIPLIER"]
CheckFallback["baseHp > 0?"]
Return1["return (skip scaling)"]
GetScaling["perLevel = isBoss ?<br>BOSS_HP_PER_LEVEL<br>: NORMAL_HP_PER_LEVEL"]
CalcMult["multiplier = 1.0 +<br>(playerLevel - 1) * perLevel"]
ClampMult["multiplier = clamp(multiplier,<br>1.0, maxMultiplier)"]
ApplyScale["attr->setBaseAttribute(MAX_HP,<br>baseHp * multiplier)"]
Refresh["refreshCacheAttributes()"]
Heal["setCurrentHP(_maxHP)"]
UpdateBar["updateHpBar()"]
Return2["return"]

Start -.-> Clamp
Clamp -.-> GetBase
GetBase -.->|"no"| CheckBase
CheckBase -.->|"yes"| Fallback
CheckBase -.->|"yes"| ApplyBase
Fallback -.->|"no"| CheckFallback
CheckFallback -.-> Return1
CheckFallback -.-> ApplyBase
ApplyBase -.-> GetScaling
GetScaling -.-> CalcMult
CalcMult -.-> ClampMult
ClampMult -.-> ApplyScale
ApplyScale -.-> Refresh
Refresh -.-> Heal
Heal -.-> UpdateBar
UpdateBar -.-> Return2
```

### 缩放公式表

| 怪物类型 | 基础倍率 | 每级 HP 系数 | 最大倍率 | 示例（10 级） |
| --- | --- | --- | --- | --- |
| **普通怪** | `NORMAL_BASE_MULTIPLIER` | `NORMAL_HP_PER_LEVEL` | `NORMAL_HP_MAX_MULTIPLIER` | `baseHP * baseMultiplier * (1 + 9 * perLevel)` |
| **Boss** | `BOSS_BASE_MULTIPLIER` | `BOSS_HP_PER_LEVEL` | `BOSS_HP_MAX_MULTIPLIER` | `baseHP * baseMultiplier * (1 + 9 * perLevel)` |

**配置常量**（来自 `GameConfig::Monster::LevelScaling`）：

* 定义位置：[Classes/Configs/GameConfig.h](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Configs/GameConfig.h)
* 应用位置：[Classes/Character/Monster/MonsterBase.cpp L225-L279](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L225-L279)

**用法模式：**

```sql
// In GameScene::createMonsterByType
auto monster = GoblinMonster::create("Sprites/Enemies/Goblin/Goblin_idle.png");
monster->applyHpScalingForPlayerLevel(playerLevel, false);
monster->setPosition(spawnPoint);
```

**来源：** [Classes/Character/Monster/MonsterBase.cpp L225-L279](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L225-L279)

 [Classes/Character/Monster/MonsterBase.h L39-L45](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L39-L45)

---

## 命中框生成工具

### 近战命中框（spawnMeleeHitbox）

`spawnMeleeHitbox()` 会在怪物位置的相对偏移处生成一个短生命周期的 physics hitbox：

```mermaid
flowchart TD

Monster["MonsterBase<br>position + offset"]
Hitbox["Attack Node"]
Body["Box PhysicsBody<br>size = hitboxSize"]
Tag["Damage stored in tag"]
Cat["MONSTER_ATTACK"]
Test["PLAYER"]
Source["Attack source reference"]
Cleanup["Auto-remove after lifeSeconds"]

Monster -.->|"spawnMeleeHitbox"| Hitbox
Hitbox -.->|"setUserObject(this)"| Body
Body -.->|"setTag(damageTag)"| Tag
Body -.->|"CategoryBitmask"| Cat
Body -.->|"ContactTestBitmask"| Test
Hitbox -.->|"DelayTime + RemoveSelf"| Source
Hitbox -.->|"PhysicsBody"| Cleanup
```

**方法签名：**

```javascript
Node* spawnMeleeHitbox(
    const Vec2 &offsetInParentSpace,
    const Size &hitboxSize,
    int damageTag,
    float lifeSeconds = 0.1f
);
```

**关键特性：**

* **伤害编码：** 伤害值存放在 `PhysicsBody::tag`（[MonsterBase.cpp L1019](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L1019-L1019)）
* **攻击源引用：** `setUserObject(this)` 让受击者可判定命中方向（[MonsterBase.cpp L1010](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L1010-L1010)）
* **自动清理：** 使用 `Sequence → DelayTime → RemoveSelf` 模式（[MonsterBase.cpp L1022-L1025](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L1022-L1025)）
* **物理分类：** `MONSTER_ATTACK`，collision mask 为 `0`（不做物理阻挡，仅做 contact test）（[MonsterBase.cpp L1017-L1018](https://github.com/lilong555/Adventure-King/blob/60df0f40/MonsterBase.cpp#L1017-L1018)）

### 绝对位置命中框（spawnAttackHitboxAt）

`spawnAttackHitboxAt()` 在父节点坐标的绝对位置生成 hitbox（用于投掷物、AoE 等）：

**方法签名：**

```javascript
Node* spawnAttackHitboxAt(
    const Vec2 &centerPosInParentSpace,
    const Size &hitboxSize,
    int damageTag,
    float lifeSeconds = 0.1f,
    int localZOrder = 0
);
```

**与近战命中框的区别：**

* 位置为父节点坐标绝对值（不是相对怪物偏移）
* 多了 `localZOrder` 以便控制渲染层级
* 物理配置与自动清理逻辑相同

**来源：** [Classes/Character/Monster/MonsterBase.cpp L986-L1028](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L986-L1028)

 [Classes/Character/Monster/MonsterBase.h L116-L129](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L116-L129)

---

## 属性管理

### 初始化模式

`setupCharacterStats()` 为子类提供统一的属性初始化流程：

```mermaid
sequenceDiagram
  participant p1 as GoblinMonster
  participant p2 as MonsterBase
  participant p3 as AttributeComponent
  participant p4 as HP Bar

  p1->>p1: Create Attributes struct
  p1->>p1: Set base values from GameConfig
  p1->>p2: setupCharacterStats(attributes)
  p2->>p3: setBaseAttributes(attributes)
  p2->>p3: recalculateFinalAttributes()
  p2->>p2: refreshCacheAttributes()
  note over p2: Cache to member variables
  p2->>p2: _moveSpeed = attr->get(MOVE_SPEED)
  p2->>p2: _attackInterval = attr->get(ATTACKINTERVAL)
  p2->>p2: _attackRange = attr->get(ATTACK_RANGE)
  p2->>p2: _maxHP = attr->get(MAX_HP)
  p2->>p2: ensureHpBar()
  p2->>p4: Create DrawNode if needed
  p2->>p2: updateHpBar()
  p2-->>p1: Initialization complete
```

**实现示例：**

```
// From GoblinMonster::initAttributes
void GoblinMonster::initAttributes() {
    Attributes base;
    base.set(AttributeType::STRENGTH, Conf::STRENGTH);
    base.set(AttributeType::DEFENSE, Conf::DEFENSE);
    base.set(AttributeType::MOVE_SPEED, Conf::MOVE_SPEED);
    base.set(AttributeType::MAX_HP, Conf::MAX_HP);
    base.set(AttributeType::ATTACKINTERVAL, Conf::ATTACK_INTERVAL);
    base.set(AttributeType::ATTACK_RANGE, Conf::ATTACK_RANGE);
    
    setupCharacterStats(base); // Delegates to MonsterBase
}
```

**来源：** [Classes/Character/Monster/MonsterBase.cpp L184-L222](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L184-L222)

 [Classes/Character/Monster/Monsters/GoblinMonster.cpp L182-L207](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.cpp#L182-L207)

---

## 血条渲染

### 基于 DrawNode 的血条

血条使用 `cocos2d::DrawNode` 绘制，并按血量比例动态改变颜色：

| HP 比例 | 颜色 | RGB |
| --- | --- | --- |
| > 50% | 绿色 | `(0.2, 0.8, 0.2)` |
| 25–50% | 黄色 | `(1.0, 0.8, 0.0)` |
| < 25% | 红色 | `(1.0, 0.2, 0.2)` |

**渲染算法：**

```
// From MonsterBase.cpp:873-928
void MonsterBase::updateHpBar() {
    _hpBar->clear();
    
    float hpRatio = clampf(getCurrentHP() / maxHp, 0.0f, 1.0f);
    float currentWidth = barWidth * hpRatio;
    
    // Background (dark gray)
    _hpBar->drawSolidRect(barPos, barPos + Vec2(barWidth, barHeight),
                          Color4F(0.2f, 0.2f, 0.2f, 1.0f));
    
    // Foreground (colored by HP ratio)
    _hpBar->drawSolidRect(barPos, barPos + Vec2(currentWidth, barHeight), hpColor);
    
    // Border (white outline)
    _hpBar->drawRect(barPos, barPos + Vec2(barWidth, barHeight), Color4F::WHITE);
}
```

**配置参数：**

* `HP_BAR_WIDTH`：基础宽度（会乘以 `_hpBarScale`）
* `HP_BAR_HEIGHT`：高度
* `HP_BAR_Y_OFFSET`：位于 sprite 上方的垂直偏移

**来源：** [Classes/Character/Monster/MonsterBase.cpp L861-L928](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L861-L928)

 [Classes/Character/Monster/MonsterBase.h L89-L93](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L89-L93)

---

## 坐标系工具

### 多父节点空间处理

坐标工具用于处理“怪物与目标不在同一父节点”的情况：

```mermaid
flowchart TD

GetPos["getPositionInParentSpace(target)"]
CheckParent["target->getParent()<br>== this->getParent()?"]
DirectPos["return target->getPosition()"]
ToWorld["worldPos = target->getParent()<br>→ convertToWorldSpace(target pos)"]
ToLocal["return this->getParent()<br>→ convertToNodeSpace(worldPos)"]
HorizDist["horizontalDistanceTo(target)"]
CheckParent2["same parent?"]
DirectDist["return abs(target.x - this.x)"]
WorldDist["myWorldX = getWorldPosition(this).x<br>targetWorldX = getWorldPosition(target).x"]
CalcDist["return abs(targetWorldX - myWorldX)"]

GetPos -.-> CheckParent
CheckParent -.->|"yes"| DirectPos
CheckParent -.->|"no"| ToWorld
ToWorld -.-> ToLocal
HorizDist -.-> CheckParent2
CheckParent2 -.->|"yes"| DirectDist
CheckParent2 -.->|"no"| WorldDist
WorldDist -.-> CalcDist
```

**实现对照表：**

| 函数 | 同父节点路径 | 不同父节点路径 |
| --- | --- | --- |
| `getWorldPosition()` | `parent->convertToWorldSpace(pos)` | 同上 |
| `getPositionInParentSpace()` | `target->getPosition()` | `parent->convertToNodeSpace(worldPos)` |
| `horizontalDistanceTo()` | `abs(target.x - this.x)` | `abs(worldX_target - worldX_this)` |
| `distanceTo()` | `pos.distance(target pos)` | `worldPos.distance(worldPos_target)` |

**来源：** [Classes/Character/Monster/MonsterBase.cpp L933-L1072](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L933-L1072)

 [Classes/Character/Monster/MonsterBase.h L106-L137](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L106-L137)

---

## 具体实现示例：GoblinMonster

### 类结构

```mermaid
classDiagram
    class MonsterBase {
        «abstract»
        +attack()
        #getExpReward(int)
    }
    class GoblinMonster {
        -_attackAnimate: Animate
        +create(spriteFrameName)
        +preloadResources()
        +init(spriteFrameName)
        +attack()
        #getExpReward(int)
        #initAnimations()
        #initAttributes()
        #initStateAnimations()
    }
    class AnimationCache {
        +getAnimation(key)
        +addAnimation(anim, key)
    }
    class GameConfig_Monster_Goblin {
        +STRENGTH: float
        +DEFENSE: float
        +MOVE_SPEED: float
        +MAX_HP: float
        +ATTACK_INTERVAL: float
        +VISION_RANGE: float
        +CHASE_RANGE: float
        +EXP_REWARD_BASE: int
    }
    MonsterBase <|-- GoblinMonster : "caches animations"
    GoblinMonster ..> AnimationCache : "reads config"
    GoblinMonster ..> GameConfig_Monster_Goblin
```

### 初始化顺序

```mermaid
sequenceDiagram
  participant p1 as GoblinMonster::create
  participant p2 as GoblinMonster::init
  participant p3 as MonsterBase::init
  participant p4 as AnimationCache
  participant p5 as GameConfig

  p1->>p2: new + init(spriteFrameName)
  p2->>p3: MonsterBase::init(spriteFrameName)
  note over p3: See "Initialization Pipeline" section
  p3->>p3: Attach components
  p3->>p3: Create physics body
  p3-->>p2: Component setup complete
  p2->>p5: Read Goblin::VISION_RANGE, etc.
  p2->>p2: setAIConfig(vision, chase, patrol)
  p2->>p2: initAttributes()
  note over p2: Create Attributes struct
  p2->>p2: setupCharacterStats(attributes)
  p2->>p2: setHpBarScale(Goblin::HP_BAR_SCALE)
  p2->>p2: setCurrentHP(_maxHP)
  p2->>p2: updateHpBar()
  p2->>p2: initStateAnimations()
  note over p2: Register IDLE, HURT, WALKING
  p2->>p4: Register "goblin_idle"
  p2->>p4: Register "goblin_hurt"
  p2->>p4: Register "goblin_walk"
  p2->>p2: initAnimations()
  p2->>p4: Get "goblin_attack" animation
  p2->>p2: _attackAnimate = Animate::create(animation)
  p2->>p2: _attackAnimate->retain()
  p2-->>p1: Initialized GoblinMonster*
```

**来源：** [Classes/Character/Monster/Monsters/GoblinMonster.cpp L139-L166](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.cpp#L139-L166)

 [Classes/Character/Monster/Monsters/GoblinMonster.h L7-L39](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.h#L7-L39)

---

## 攻击实现示例

### Goblin 攻击流程

`GoblinMonster::attack()` 的 override 展示了怪物攻击实现的标准模式：

```mermaid
sequenceDiagram
  participant p1 as MonsterBase::updateAttack
  participant p2 as GoblinMonster::attack
  participant p3 as Attack Animation
  participant p4 as Hitbox Logic Sequence
  participant p5 as spawnMeleeHitbox

  p1->>p1: All attack checks pass
  p1->>p2: attack()
  p2->>p2: Clone _attackAnimate
  p2->>p2: Calculate hitTime from animation
  note over p2: hitTime = frameTime * ATTACK_HIT_FRAME_INDEX
  p2->>p4: Create DelayTime(hitTime)
  p4->>p4: Wait for hit frame
  p4->>p4: CallFunc (spawn hitbox)
  note over p4: Inside CallFunc
  p4->>p4: Calculate direction from scaleX
  p4->>p4: Calculate offset with direction
  p4->>p4: Read STRENGTH from AttributeComponent
  p4->>p5: spawnMeleeHitbox(offset, size, damageTag, life)
  par Animation and Logic Run in Parallel
    p3->>p3: Play attack frames
  and
    p4->>p4: Wait then spawn hitbox
  end
  note over p2: Animation complete
  p2->>p2: CallFunc: changeState(IDLE)
```

### 配置驱动的命中框

命中框完全由 `GameConfig::Monster::Goblin` 配置：

```sql
// From GoblinMonster.cpp:294-326
auto logicSequence = Sequence::create(
    DelayTime::create(hitTime),
    CallFunc::create(<FileRef file-url="https://github.com/lilong555/Adventure-King/blob/60df0f40/this" undefined  file-path="this">Hii</FileRef> {
        float direction = (this->getScaleX() > 0) ? 1.0f : -1.0f;
        
        // Scale adaptation
        float scaleRatio = fabs(this->getScaleX()) / 
                          GameConfig::Monster::Goblin::HITBOX_TUNE_SCALE;
        
        Vec2 offset(
            GameConfig::Monster::Goblin::HITBOX_OFFSET_X * direction * scaleRatio,
            GameConfig::Monster::Goblin::HITBOX_OFFSET_Y * scaleRatio
        );
        
        Size hitboxSize(
            GameConfig::Monster::Goblin::HITBOX_WIDTH * scaleRatio,
            GameConfig::Monster::Goblin::HITBOX_HEIGHT * scaleRatio
        );
        
        int damageTag = static_cast<int>(
            round(getAttributeComponent()->getAttributeValue(AttributeType::STRENGTH))
        );
        
        spawnMeleeHitbox(offset, hitboxSize, damageTag,
                        GameConfig::Monster::Goblin::HITBOX_LIFE_SECONDS);
    }),
    nullptr
);
```

**来源：** [Classes/Character/Monster/Monsters/GoblinMonster.cpp L254-L354](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.cpp#L254-L354)

 [Classes/Character/Monster/Monsters/GoblinMonster.h L24-L25](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.h#L24-L25)

---

## 资源预热

### 静态预加载模式

`GoblinMonster::preloadResources()` 展示了预加载模式，用于避免第一次生成怪物时卡顿：

```
// From GoblinMonster.cpp:168-179
void GoblinMonster::preloadResources() {
    ensureSingleFrameAnimationCached("goblin_idle", 
                                     "Sprites/Enemies/Goblin/Goblin_idle.png");
    ensureSingleFrameAnimationCached("goblin_hurt", 
                                     "Sprites/Enemies/Goblin/Goblin_beattacked.png");
    ensureLoopAnimationCached("goblin_walk",
                             "Sprites/Enemies/Goblin/Goblin_walk_%d.png",
                             4,
                             GameConfig::Monster::Goblin::WALK_ANIM_FRAME_DELAY);
    ensureGoblinAttackAnimationCached();
}
```

**用法模式：**

```
// In LoadingScene or GameScene initialization
GoblinMonster::preloadResources();
// Later spawns will use cached animations
```

**来源：** [Classes/Character/Monster/Monsters/GoblinMonster.cpp L168-L179](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.cpp#L168-L179)

 [Classes/Character/Monster/Monsters/GoblinMonster.h L18-L19](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/Monsters/GoblinMonster.h#L18-L19)

---

## 性能注意事项

### 节流总结

| 系统 | 节流机制 | 收益 |
| --- | --- | --- |
| **AI** | `_aiUpdateAccumulator` | 降低寻路/决策开销 |
| **移动** | `_moveUpdateAccumulator` | 平滑速度变化，减少抖动 |
| **攻击** | `_attackUpdateAccumulator` | 避免过于频繁的攻击判定 |
| **非活跃 AI** | `_inactiveAiUpdateInterval` | 降低屏幕外怪物的 CPU 占用 |
| **活跃范围** | `_activeUpdateDistanceX` | 冻结很远的怪物以省 CPU |

### 错峰启动（Staggered Start）

为防止一次生成大量怪物导致同一帧峰值（frame spike），会把 accumulator 初始值随机化：

```markdown
// From MonsterBase.cpp:170-179
auto staggerAccumulator = [](float intervalSeconds) -> float {
    if (intervalSeconds <= 0.0f) return 0.0f;
    return cocos2d::random(intervalSeconds * 0.5f, intervalSeconds);
};

_aiUpdateAccumulator = staggerAccumulator(_aiUpdateInterval);
_moveUpdateAccumulator = staggerAccumulator(_moveUpdateInterval);
_attackUpdateAccumulator = staggerAccumulator(_attackUpdateInterval);
```

这样可让不同怪物在不同的子帧偏移上触发更新，把 CPU 压力分散到多帧。

**来源：** [Classes/Character/Monster/MonsterBase.cpp L152-L179](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L152-L179)

 [Classes/Character/Monster/MonsterBase.cpp L281-L414](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L281-L414)

---

## 与游戏系统的集成

### GameScene 生成流程

```mermaid
sequenceDiagram
  participant p1 as GameScene
  participant p2 as LevelMap
  participant p3 as Monster Factory
  participant p4 as MonsterBase
  participant p5 as PlayerCharacter

  p1->>p1: update(dt)
  p1->>p2: updateEnemySpawns(player, viewDistanceX)
  p2->>p2: Check proximity to spawn points
  alt Player in range
    p2->>p1: Trigger spawn callback
    p1->>p3: createMonsterByType(type)
    p3->>p4: GoblinMonster::create()
    p4-->>p3: monster instance
    p3->>p4: applyHpScalingForPlayerLevel(playerLevel)
    p3->>p4: setTarget(player)
    p3->>p4: setHome(spawnPos)
    p3->>p4: setAIConfig(aggro, leash, patrol)
    p3-->>p1: Configured monster
    p1->>p1: addChild(monster)
    p1->>p4: Register onDeath callback
  end
  note over p4: Monster now in active gameplay
  p4->>p4: update(dt) every frame
  p4->>p5: Chase, attack, etc.
```

### 死亡回调链

当怪物死亡时，通常会触发如下回调链：

1. **Monster → LevelMap：** `_currentActiveMonsters` 计数 -1
2. **LevelMap → Arena：** 判断当前波次是否完成
3. **Arena → LevelMap：** 触发下一波/解锁闸门
4. **LevelMap → GameScene：** 判断关卡是否完成
5. **GameScene → UI：** 若全部敌人击败则展示胜利界面

**来源：** [Classes/Scene/GameScene.cpp](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Scene/GameScene.cpp)

 [Classes/Level/LevelMap.cpp](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Level/LevelMap.cpp)

---

## 关键方法汇总表

| 方法 | 作用 | 是否必须覆写？ |
| --- | --- | --- |
| `init(spriteFrameName)` | 初始化怪物（组件、物理、AI） | 通常（先调用基类） |
| `update(dt)` | 逐帧更新（含节流） | 很少 |
| `updateAI(dt)` | AI 决策逻辑 | 有时（自定义 AI） |
| `updateMovement(dt)` | 物理速度更新 | 很少 |
| `updateAttack(dt)` | 攻击触发逻辑 | 很少 |
| `attack()` | 执行攻击动作 | **是（总是）** |
| `takeDamage(DamageInfo)` | 接收伤害 | 有时（特殊机制） |
| `die()` | 死亡处理 | 有时（Boss 机制） |
| `getExpReward(int)` | 经验计算 | **是（若需要经验奖励）** |
| `setupCharacterStats(Attributes)` | 属性初始化工具方法 | 否 |
| `applyHpScalingForPlayerLevel(int, bool)` | 按玩家等级缩放 HP | 否（外部调用） |

**来源：** [Classes/Character/Monster/MonsterBase.h L18-L100](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.h#L18-L100)

 [Classes/Character/Monster/MonsterBase.cpp L1-L1111](https://github.com/lilong555/Adventure-King/blob/60df0f40/Classes/Character/Monster/MonsterBase.cpp#L1-L1111)
