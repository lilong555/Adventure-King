# 角色系统设计文档

## 1. 核心目标

本文档旨在为《冒险王之神兵传奇》游戏设计一个全面、可扩展的角色系统。该系统将涵盖角色的所有核心方面，包括属性、状态、技能、装备和战斗逻辑，为后续的具体代码实现提供清晰的蓝图。

## 2. 所需文件结构

为了保持代码的整洁和模块化，建议在 `Classes` 目录下创建一个新的 `Character` 文件夹来存放与角色相关的所有文件。

```
Adventure-King/Classes/
└── Character/
    ├── CharacterBase.h         // 角色基类，定义所有角色的通用接口和属性
    ├── CharacterBase.cpp
    ├── CharacterData.h         // 存放所有与角色相关的枚举（Enums）和结构体（Structs）
    ├── components/             // 组件文件夹，用于存放可复用的功能模块
    │   ├── AttributeComponent.h  // 属性组件
    │   ├── AttributeComponent.cpp
    │   ├── StateMachineComponent.h // 状态机组件
    │   ├── StateMachineComponent.cpp
    │   ├── SkillComponent.h      // 技能组件
    │   ├── SkillComponent.cpp
    │   └── ...                 // 其他组件
    └── PlayerCharacter.h       // 玩家角色类，继承自 CharacterBase
    └── PlayerCharacter.cpp
```

## 3. 核心数据结构 (`CharacterData.h`)

在实现具体类之前，首先定义好需要用到的数据结构。

### 3.1. 枚举 (Enums)

```cpp
// 角色职业
enum class CharacterRole {
    WARRIOR,
    MAGE,
    ASSASSIN,
    TANK
};

// 角色核心状态
enum class CharacterState {
    IDLE,
    WALKING,
    RUNNING,
    JUMPING,
    DOUBLE_JUMPING,
    ATTACKING,
    CLIMBING,
    HURT,   // 受击硬直
    DEAD
};

// 基础属性类型
enum class AttributeType {
    STRENGTH,       // 力量
    DEFENSE,        // 防御
    CRITICAL_RATE,  // 暴击率
    MOVE_SPEED,     // 移动速度
    MAX_HP,         // 最大生命值
    MAX_MP          // 最大能量值
};

// 武器类型
enum class WeaponType {
    SWORD,
    STAFF,
    DAGGER // 为刺客等职业预留
};

// 装备槽位
enum class EquipmentSlot {
    WEAPON,
    HELMET,
    ARMOR,
    BOOTS
};

// 状态效果
enum class StatusEffect {
    POISONED, // 中毒
    EXCITED,  // 亢奋
    STUNNED   // 眩晕
};
```

### 3.2. 结构体 (Structs)

```cpp
#include <map>
#include <string>

// 基础属性结构体
struct Attributes {
    std::map<AttributeType, float> values;
};

// 技能基类
struct Skill {
    int id;
    std::string name;
    std::string description;
    bool isPassive; // 是否为被动技能
};

// 主动技能
struct ActiveSkill : public Skill {
    float cooldown;
    float manaCost;
};

// 被动技能
struct PassiveSkill : public Skill {
    Attributes attributeBonus; // 提供的属性加成
};

// 装备基类
struct Equipment {
    int id;
    std::string name;
    std::string description;
    EquipmentSlot slot;
    Attributes attributeBonus; // 装备提供的属性加成
};

// 武器类
struct Weapon : public Equipment {
    WeaponType type;
    float attackDamage;
    float attackRange;
};
```

## 4. 核心类设计

### 4.1. `CharacterBase` (角色基类)

这个类将作为玩家、敌人和 NPC 的父类，继承自 `cocos2d::Sprite` 以便在场景中显示。

**`CharacterBase.h`**

```cpp
#include "cocos2d.h"
#include "CharacterData.h"

class AttributeComponent;
class StateMachineComponent;
class SkillComponent;

class CharacterBase : public cocos2d::Sprite {
public:
    // 创建方法
    static CharacterBase* create(const std::string& spriteFrameName);

    virtual bool init() override;

    // 组件 getter
    AttributeComponent* getAttributeComponent() const { return _attributeComponent; }
    StateMachineComponent* getStateMachineComponent() const { return _stateMachineComponent; }
    SkillComponent* getSkillComponent() const { return _skillComponent; }

    // 核心战斗方法
    virtual void attack() = 0;
    virtual void takeDamage(float damage);
    virtual void die();

protected:
    // 组件实例
    AttributeComponent* _attributeComponent;
    StateMachineComponent* _stateMachineComponent;
    SkillComponent* _skillComponent;
    // ... 其他通用组件

    // 基础信息
    int _level;
    int _experience;
    float _currentHP;
    float _currentMP;
};
```

### 4.2. `PlayerCharacter` (玩家角色类)

继承自 `CharacterBase`，并添加玩家特有的逻辑。

**`PlayerCharacter.h`**

```cpp
#include "Character/CharacterBase.h"

class PlayerCharacter : public CharacterBase {
public:
    static PlayerCharacter* create(CharacterRole role);

    virtual bool init(CharacterRole role);

    void addExperience(int amount);
    void levelUp();

    // 装备管理
    void equip(Equipment* item);
    void unequip(EquipmentSlot slot);

    // 技能管理
    void useSkill(int slotIndex);

private:
    CharacterRole _role;
    int _skillPoints;
    std::map<EquipmentSlot, Equipment*> _equippedItems;

    // 技能槽
    std::vector<ActiveSkill*> _activeSkillSlots;
    std::vector<PassiveSkill*> _passiveSkillSlots;
};
```

## 5. 组件设计 (Component-Based Design)

将不同的功能模块拆分成独立的组件，可以方便地复用和管理。

### 5.1. `AttributeComponent` (属性组件)

- **职责**: 管理角色的所有属性，包括基础属性、装备加成和最终属性的计算。
- **功能**:
  - 存储角色的基础属性 (`_baseAttributes`)。
  - 存储来自装备、技能和状态效果的临时属性加成。
  - 提供一个 `recalculateFinalAttributes()` 方法，用于在装备或状态变化时重新计算最终属性。
  - 提供 `getAttributeValue(AttributeType type)` 方法来获取最终的属性值。

### 5.2. `StateMachineComponent` (状态机组件)

- **职责**: 管理角色的当前状态 (`CharacterState`)，并处理状态切换的逻辑。
- **功能**:
  - `changeState(CharacterState newState)`: 切换到新状态，并执行进入新状态的逻辑（如播放动画、启用/禁用物理效果）。
  - `update(float dt)`: 在每帧更新时，执行当前状态的逻辑（如 `IDLE` 状态下检查输入，`ATTACKING` 状态下检测攻击碰撞）。
  - `getCurrentState()`: 返回当前状态。
  - 管理与每个状态相关联的动画 (`cocos2d::Animation`)。

### 5.3. `SkillComponent` (技能组件)

- **职责**: 管理角色已学习的技能、技能槽以及技能的使用。
- **功能**:
  - 存储一个所有已学习技能的列表 (`_learnedSkills`)。
  - 管理主动技能槽 (`_activeSlots`) 和被动技能槽 (`_passiveSlots`)。
  - `learnSkill(Skill* skill)`: 学习一个新技能。
  - `equipSkill(Skill* skill, int slot)`: 将一个已学习的技能装备到槽位上。
  - `useActiveSkill(int slot)`: 使用指定槽位的主动技能，处理冷却和能量消耗。

## 6. 总结

该设计方案通过“基类 + 组件”的模式，构建了一个高度模块化和可扩展的角色系统。

- **`CharacterData.h`** 统一管理了所有数据结构，方便维护。
- **`CharacterBase`** 提供了所有角色的通用框架。
- **`PlayerCharacter`** 实现了玩家的特定逻辑。
- **组件** (`AttributeComponent`, `StateMachineComponent` 等) 将复杂的功能解耦，使得每个部分都易于实现和测试。

下一步，您可以根据这份文档，逐一实现 `CharacterData.h` 中的结构，然后是各个组件，最后是 `CharacterBase` 和 `PlayerCharacter` 类。
