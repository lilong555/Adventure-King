# Adventure‑King《冒险王之神兵传奇》项目介绍（对外展示版｜超详细）

> 这份文档是“介绍我们自己”的材料，面向不熟悉 C++/Cocos2d‑x 的读者。  
> 我们会用尽量直观的语言解释：项目做了什么、为什么这样做、以及我们在工程实现上踩过哪些坑、如何解决。

---

## 目录（建议按需阅读）

- 0. 阅读指南与术语表
- 1. 项目概览：一句话 + 玩法闭环
- 2. 快速体验：从启动到战斗（含键位）
- 3. 功能清单：你能在游戏里看到什么
- 4. 工程实现详解（每个系统怎么落地）
  - 4.1 场景与资源：SceneRegistry / LoadingScene / 预加载与预热
  - 4.2 输入：GameInputController / 输入法(IME)禁用
  - 4.3 暂停与死亡：真正“冻结世界”的实现
  - 4.4 地图系统：TMX / collisions / enemy_g / gate
  - 4.5 战斗系统：Hitbox / DamageInfo / 碰撞与结算
  - 4.6 技能系统：职业 SkillSet / 主动与被动 / 装备槽 / CD
  - 4.7 状态效果：燃烧/中毒/回血/增伤（结算与表现分离）
  - 4.8 粒子系统：纹理来源、预热、方向、节流
  - 4.9 背包与装备：装备/技能/属性页面 + 装备特效接口
  - 4.10 掉落物：生成、自动拾取、拾取反馈动画
  - 4.11 Boss：击破条 + Boss 血条的连击统计与受击反馈
  - 4.12 存档：本地 SQLite‑KV + JSON 备份 + 云同步接口（演示）
- 5. 扩展指南：如何加新怪物/技能/装备/粒子/地图（按步骤）
- 6. 踩坑记录：我们遇到的真实问题、根因与修复方式
- 7. 作者与入口索引

---

## 0. 阅读指南与术语表

### 0.1 三种读法（选一种即可）

- **只想知道“项目是什么”**：读第 1～3 章（产品视角）。
- **想看“工程做得扎实吗”**：读第 4 章 + 第 6 章（实现细节 + 踩坑）。
- **想二次开发**：读第 5 章（扩展指南），再按“入口索引”看代码。

### 0.2 术语表（行外人也能理解）

- **Scene（场景）**：一个完整页面/关卡（主菜单、加载页、战斗关卡都算）。
- **Node（节点）**：画面中的一个对象（角色、UI、粒子、判定框都属于 Node）。
- **Action（动作）**：Cocos2d‑x 的时间轴系统（延迟、序列、循环动画等）。
- **Scheduler（调度器）**：按时间调用的回调（`scheduleOnce`/`schedule`）。
- **PhysicsWorld / PhysicsBody（物理世界/物理体）**：负责碰撞与物理模拟（重力、坡面滑动）。
- **Hitbox（命中判定框）**：攻击瞬间生成的“伤害区域”。
- **Hurtbox（受击框）**：角色自身的碰撞/受击范围（通常是 PhysicsBody）。
- **DOT（持续伤害）**：燃烧/中毒等，每隔一段时间结算一次，通常不打断动作。
- **击破（Break）**：Boss 的“破防条”，满了会倒地，形成输出窗口。
- **预加载（Preload）**：提前把贴图/粒子/音频读入缓存，避免“第一次出现卡一下”。
- **预热（Preheat）**：提前让粒子/动画跑一帧，避免首次播放卡顿/停一拍。

---

## 1. 项目概览：一句话 + 玩法闭环

### 1.1 一句话概括

我们用 **Cocos2d‑x** 做了一款**横版动作冒险**原型：  
**选职业 → 进关卡 → 战斗与成长（技能/装备/状态效果）→ Boss 机制（击破）→ 存档（本地 DB + 云同步演示）**。

### 1.2 我们要做的不是“演示”，而是“可扩展原型”

很多课程/原型项目会停在“能跑起来”。但真实游戏开发里，最难的是：
- 功能增加时不崩（系统解耦、配置集中、逻辑复用）
- 体验稳定（不卡顿、暂停彻底、判定可靠、视觉一致）
- 可回归（有 Debug 场景、训练木桩、日志可定位）

所以我们把重点放在“闭环 + 工程”：
- **闭环**：能打怪、能升级、能装备、能进下一关、能存档。
- **工程**：预加载体系、粒子预热、统一伤害结算、可扩展配置、云同步演示。

---

## 2. 快速体验：从启动到战斗（含键位）

### 2.1 启动后的主流程

主菜单由 `Adventure-King/Classes/Scenes/HelloWorldScene.cpp` 驱动，典型流程：

1. 进入主菜单（HelloWorldScene）
2. 点击开始游戏
3. 进入角色选择（法师/战士/刺客）
4. 进入 Home/地图入口
5. 进入 LoadingScene（若资源未准备好）
6. 进入战斗关卡 GameScene

### 2.2 键位（以代码为准）

键位在 `Adventure-King/Classes/Scenes/GameInputController.cpp`：

- 移动：`A/D` 或 `←/→`
- 跳跃：`W`（不在门区时）或 `Space`
- 跑步：`Shift`
- 普攻：`J` 或 `4`
- 技能槽：`E/K`（槽 0）、`Q`（槽 1）、`R`（槽 2）、`F`（槽 3）
- 暂停：`Esc`
- 门区交互：`W`（在 gate 区域时优先进入传送门）

一个关键细节（容易被忽略但决定手感）：
- 我们不会在“动作锁（攻击/施法）”期间把水平输入彻底冻结；否则会出现动作游戏常见的“锁死方向、不跟手”。
- 同时我们在 `update()` 中持续对齐动画，避免“受击回到 IDLE 但仍在按住移动键 → 人在跑却播 idle”。

---

## 3. 功能清单：你能在游戏里看到什么

（以 `main` 分支为准）

### 3.1 两张关卡地图（TMX）

- 起源之菇（Origin_Mushroom）
- 神秘之森（Mystery_Forest）

每张地图都包含：
- `collisions`：碰撞对象层（多边形/折线/矩形）
- `born`：玩家出生点
- `gate`：传送门区域
- `enemy_g`：刷怪点（怪物类型与数量）

### 3.2 职业与技能

- 法师（Klee / Mage）：远程投掷与爆炸、燃烧 DOT
- 战士（Warrior）：近战 + 范围火焰技能（偏稳定与覆盖）
- 刺客（Assassin）：近战快节奏、连段斩击 + 高风险强化状态

技能系统支持：
- 学习/装备/冷却/消耗
- 主动技能点与被动技能点（升级获得）
- 技能独立配置击破值（为 Boss 击破条服务）

### 3.3 背包系统

背包页面分为多个子页（按玩家认知组织，而非按代码组织）：
- 装备：装备/卸下、已装备高亮提示、双击装备
- 主动技能：未学习/已学习/可装备，显示技能名字与详情
- 被动技能：学习后可装备/卸下（不做槽位限制）
- 属性：使用属性点提升单项属性，点数不足提示

### 3.4 状态效果与粒子

状态效果（数值）与粒子表现（视觉）分离：
- 数值：燃烧、中毒、回血、反伤、增伤等（可叠层/可 tick）
- 视觉：按物理体匹配尺寸与位置（避免 Boss 的火焰太宽/漂浮）
- 节流：例如回血粒子 0.5 秒内多次触发只播一次，避免刷屏与性能浪费

### 3.5 Boss 机制：击破条 + 血条反馈

- Boss 击破条：普攻/技能按配置累积击破值，满后倒地、停留、起身
- Boss 血条优化：
  - 右侧显示“连击总伤害”（1 秒容差聚合）
  - 非 DOT 伤害触发受击反馈动画（放大→回弹）

### 3.6 掉落物

- 怪物死亡 30% 概率掉落血瓶/蓝瓶（各 50%）
- 玩家路过自动拾取
- 拾取反馈：漂浮 + 淡出（更符合动作游戏的“拿到东西”的感觉）

### 3.7 存档与云同步（演示）

- 本地存档：SQLite‑KV（`cocos2d::localStorage`，底层 sqlite3）作为主存储 + JSON 备份
- 云同步：客户端 HTTP 同步接口 + WSL 可运行的 C++ 后端 + `/admin` 可视化管理（删除/回滚）

---

## 4. 工程实现详解（每个系统怎么落地）

这一章开始进入“工程细节”。我们会给出：
- 关键文件入口
- 运行时流程（像“把代码翻译成中文”）
- 为什么这么做（取舍）
- 典型坑点（会在第 6 章再集中复盘）

### 4.1 场景与资源：SceneRegistry / LoadingScene / 预加载与预热

关键文件：
- `Adventure-King/Classes/Managers/SceneRegistry.h`
- `Adventure-King/Classes/Scenes/LoadingScene.cpp`
- `Adventure-King/Classes/Scenes/GameScene.cpp`

核心思想（很像“菜单 → 装箱 → 进门”）：
- SceneRegistry 是“场景说明书”：每个 SceneID 需要哪些资源、怎么创建场景、资源加载后要做什么预热。
- LoadingScene 是“装箱工”：根据说明书把资源都准备好，并把过程显示成进度条。

典型卡顿来自两类“第一次”：
- 第一次进入关卡：TMX 解析 + 贴图加载 + 物理体创建集中爆发
- 第一次生成怪物/粒子：动画帧首次进入缓存、粒子首次创建 GPU 资源

我们做了两件事：
- **预加载（Preload）**：提前加载贴图/粒子/音频
- **预热（Preheat）**：把关键粒子跑一帧（避免首帧粒子“停一拍”）

> 重要取舍：  
> 我们宁愿在 LoadingScene 有进度条“等一下”，也不要让玩家点进关卡时无反馈地卡死 0.5～1 秒。

### 4.2 输入：GameInputController / 输入法(IME)禁用

#### 4.2.1 输入控制（按键 → 速度 → 动画）

关键文件：
- `Adventure-King/Classes/Scenes/GameInputController.cpp`

输入系统做两件事：
- 通过按键状态设置玩家 PhysicsBody 的水平速度（让移动与物理/重力一致）
- 在非动作锁情况下持续对齐动画（避免状态机把动画切回 IDLE 后出现错位）

#### 4.2.2 禁用输入法：解决“按键失灵”类问题

关键文件：
- `Adventure-King/Classes/Utils/ImeHelper.cpp`
- 使用点：`GameScene.cpp` / `LoadingScene.cpp` / `DebugScene.cpp` / `HelloWorldScene.cpp`

问题背景：
- Windows 中文输入法可能会抢按键，导致游戏快捷键“偶尔无效”

解决方式：
- 进入战斗/加载等阶段 `ImeHelper::pushDisableIme()` 禁用 IME
- 离开时 `ImeHelper::popDisableIme()` 恢复
- 计数式 push/pop，避免嵌套调用失衡

这类功能对玩家来说“应该天生就对”，但对工程来说必须显式实现。

### 4.3 暂停与死亡：真正“冻结世界”的实现

关键文件：
- `Adventure-King/Classes/Scenes/GamePauseHelper.cpp`

最常见的“假暂停”：
- 只让 `update()` 早退
- 但 PhysicsWorld 仍在 `autoStep`，所以坡面滑动/投掷物飞行仍会发生

我们的做法：
- 只暂停游戏内容层（gameLayer），UI 仍可交互
- 同时关闭 PhysicsWorld 的 autoStep，并把 speed 置 0
- 恢复时把 autoStep/speed 恢复到暂停前缓存值

死亡菜单的实现原则：
- 死亡不是“直接退出”，而是进入暂停态并显示选择
- 选择“重开本关”或“回地图”

### 4.4 地图系统：TMX / collisions / enemy_g / gate

关键文件：
- `Adventure-King/Classes/Scenes/LevelMap.cpp`
- 配置结构：`Adventure-King/Classes/Configs/GameSceneConfig.h` 的 `LevelConfig`

地图读取的核心对象层：
- `collisions`：生成物理碰撞体（多边形/折线/矩形）
- `born`：玩家出生点
- `gate`：传送门区域（W 交互）
- `enemy_g`：刷怪点（怪物类型/数量/触发）

刷怪点状态是一个隐藏但重要的“存档点”：
- 玩家离开地图再回来时，刷怪点是否已触发、剩余数量、是否激活，都决定体验一致性
- 这也是为什么我们把存档做成“结构化数据”，而不是只存一个“玩家等级”

### 4.5 战斗系统：Hitbox / DamageInfo / 碰撞与结算

#### 4.5.1 统一的伤害数据结构：DamageInfo

关键文件：
- `Adventure-King/Classes/Character/Base/CharacterBase.h`

`DamageInfo` 解决的是“语义传递”问题：  
一次伤害不只是一个数字，还包含：
- 是否暴击、暴击倍率
- 是否打断（DOT 不应打断）
- 击破值（Boss 机制）
- 攻击来源（用于受击方向、反伤、仇恨统计等）

为什么“击破值”必须进入 DamageInfo？
- 因为击破本质是“攻击的一部分效果”，它应该随着伤害从攻击者流向受击者，而不是在 Boss 或 UI 里硬编码推测。

#### 4.5.2 Hitbox 从哪里来？（玩家与怪物两条链）

怪物侧：
- `MonsterBase::spawnAttackHitboxAt`（落点/远程）
- `MonsterBase::spawnMeleeHitbox`（近战）
- 文件：`Adventure-King/Classes/Character/Monster/MonsterBase.cpp`

玩家侧：
- 各职业 SkillSet 在“关键帧”生成玩家 hitbox
- 文件：`Adventure-King/Classes/Character/Player/SkillSets/*.cpp`

碰撞与结算：
- `Adventure-King/Classes/Scenes/CombatContactHelper.cpp`

调参入口（最常用）：
- 全局数值：`Adventure-King/Classes/Configs/GameConfig.h`
- 玩家技能：对应职业 SkillSet 文件
- 怪物技能：怪物类实现 + 对应 GameConfig 的怪物配置段

#### 4.5.3 Action 与判定的“不同步坑”

动作游戏里，“动画在播”和“判定生效”是两件事：
- 动画：Action 系统
- 判定：物理节点（hitbox）的生命周期

如果把“生成判定框”写成 `runAction(Delay + CallFunc)`，就会遇到一个经典坑：
- 受击打断时调用 `stopAllActions()`
- 导致 CallFunc 永远不会执行
- 玩家看到“怪物在施法/在挥刀”，但实际上没有判定框 → 体验上像“判定丢了”

工程化解法：
- 关键判定逻辑使用 `scheduleOnce`（带 key）进行管理，不依赖 Action 序列；必要时可在受击中明确取消/保留。

### 4.6 技能系统：职业 SkillSet / 主动与被动 / CD

关键文件：
- `Adventure-King/Classes/Character/components/SkillComponent.*`
- `Adventure-King/Classes/Character/Player/SkillSets/PlayerSkillSet.h`
- `Adventure-King/Classes/Character/Player/SkillSets/KleeSkillSet.cpp`
- `Adventure-King/Classes/Character/Player/SkillSets/WarriorSkillSet.cpp`
- `Adventure-King/Classes/Character/Player/SkillSets/AssassinSkillSet.cpp`
- 配置：`Adventure-King/Classes/Configs/GameConfig.h`

核心原则：
- 职业差异尽量下沉到 SkillSet（而不是堆在 PlayerCharacter）
- 技能点、击破值、CD、消耗尽量配置化（GameConfig）

一个对体验非常关键的细节：
- DOT 技能必须设置 `causesHitStun=false`，否则持续伤害会不停打断动作（玩家与怪物都会“抽搐”）。

### 4.7 状态效果：结算与表现分离（StatusEffect / VfxComponent）

关键文件：
- 结算：`Adventure-King/Classes/Character/Base/StatusEffect.h`
- 工厂：`Adventure-King/Classes/Character/StatusEffects/StatusEffectFactory.*`
- 表现：`Adventure-King/Classes/Character/components/StatusEffectVfxComponent.*`
- hooks：`Adventure-King/Classes/Character/components/AttributeComponent.*`

为什么要分离？
- 如果数值结算与粒子表现绑死在一起，改一个效果会牵扯到另一个，后期维护成本非常高。

我们的结构是：
- StatusEffect：只负责“持续多久、多久结算一次、怎么改伤害/属性”
- StatusEffectVfxComponent：只负责“粒子怎么摆、怎么跟随、按物理体对齐”

### 4.8 粒子系统：纹理来源、预热、方向、节流

粒子系统踩坑频率极高，因此我们把“规则”写得很明确：

1) **plist 是否嵌入纹理？**
- 很多粒子 plist 自带纹理信息（嵌入纹理）
- 不要强行 `setTexture("particle_texture.png")`，否则会出现“方块粒子”

2) **嵌入纹理也要预载**
- 即使纹理嵌入，仍需要在预加载阶段创建一次粒子，让纹理进入缓存

3) **粒子方向与左右判定**
- 受击粒子需要按攻击方向区分 L/R（看起来才像“从受击侧喷出”）
- 左右判断要基于攻击来源与受击者位置，而不是基于 sprite 翻转状态猜测

4) **节流**
- 回血等高频触发粒子必须节流（例如 0.5s 内多次触发只播一次）

关键入口：
- 受击粒子：`Adventure-King/Classes/Character/Base/CharacterBase.cpp`
- 状态粒子：`Adventure-King/Classes/Character/components/StatusEffectVfxComponent.cpp`
- 粒子资源：`Adventure-King/Resources/Particle/`

### 4.9 背包与装备：装备/技能/属性页面 + 装备特效接口

我们在迭代中明确：装备系统必须支持“机制”，否则只加数值很快无聊。

配置集中在：
- `Adventure-King/Classes/Configs/GameConfig.h`（Equipment / EquipmentEffect / Skill::Passive）

工程原则：
- 不把装备特效硬写在 PlayerCharacter 战斗核心函数里
- 通过 AttributeComponent hooks / StatusEffect 回调统一触发点（更易扩展）

粒子命名建议（武器命中特效）：
- 目录：`Adventure-King/Resources/Particle/`
- 命名：`par_weapon_<weaponId>_hit.plist` 或 `par_<weaponName>_hit.plist`
- 规则：能从名字看出“属于哪个装备/用途是什么”

### 4.10 掉落物：生成、自动拾取、拾取反馈动画

关键文件：
- `Adventure-King/Classes/Objects/DropItem.cpp`
- `Adventure-King/Classes/Scenes/CombatContactHelper.cpp`

掉落策略：
- 概率与种类在配置中统一管理（避免散落 magic number）
- 拾取动画做成漂浮 + 淡出，减少“瞬间消失”的突兀感

### 4.11 Boss：击破条 + 连击伤害 + 受击反馈

关键文件：
- `Adventure-King/Classes/UI/BossHealthBar.*`
- `Adventure-King/Classes/Character/Base/CharacterBase.h`（击破接口）

击破值的来源不是写死的：
- 技能/普攻在 DamageInfo 里携带 breakDamage
- Boss 只需要读 breakDamage 并更新自己的 break meter

血条反馈的坑：
- 连击很密集时，缩放动画容易“回不来”（结束点必须回到初始 scale）
- 因此动画必须以“初始状态”为收敛点，而不能以“当前 scale”为收敛点

### 4.12 存档：本地 SQLite‑KV + JSON 备份 + 云同步接口（演示）

关键文件（客户端）：
- `Adventure-King/Classes/Save/SaveManager.*`
- `Adventure-King/Classes/Save/Cloud/CloudSyncService.*`

本地存档策略：
- SQLite‑KV 作为主存储（跨平台、便于增量更新）
- JSON 作为备份（容灾与排查）
- 优先读 DB，没有则回退读 JSON，并在读取成功后“回写 DB”完成迁移

云同步（演示）：
- 服务端在 `tools/cloud_save_server/`（WSL 启动）
- 提供账号、同步、历史回滚、删除等能力（对外展示工程能力）
- 不写死任何公网 IP（避免把私人信息写进仓库）

---

## 5. 扩展指南：如何加新怪物/技能/装备/粒子/地图（按步骤）

这一章写给想继续扩展项目的人：按步骤做就能接入框架。

### 5.1 加新怪物

1) 新建怪物类：`Adventure-King/Classes/Character/Monster/Monsters/<New>.{h,cpp}`
2) 继承 `MonsterBase`，实现 init/attack/资源预载/动画缓存
3) 把关键资源加入 SceneRegistry 的 imagePaths 或 onResourcesLoaded（避免首次生成卡顿）
4) 在 TMX 的 `enemy_g` 中配置刷怪点（type/count）
5) 在刷怪映射表里注册该 type（按现有 LevelMap 逻辑补齐）

### 5.2 加新主动技能

1) 在职业 SkillSet 内创建 ActiveSkill（id/name/描述/消耗/cd/breakDamage）
2) learnSkill + equipActiveSkill 到某槽位（Q/E/R/F）
3) 在释放逻辑中：
   - 播动画
   - 在关键帧生成 hitbox
   - 设置 DamageInfo（包括 breakDamage、是否打断等）

### 5.3 加新被动技能或装备特效

原则：不要在 PlayerCharacter 核心战斗函数里堆 if‑else。

推荐路径：
- 用 AttributeComponent hooks 或 StatusEffect 回调实现触发
- 配置参数放在 `GameConfig.h`，不要散落 magic number

### 5.4 加新粒子（plist）

1) 放到 `Adventure-King/Resources/Particle/`
2) 如果是嵌入纹理：不要覆写 texture
3) 在 Loading/SceneRegistry 中预载（创建一次即可）
4) 在需要的时机创建并播放（注意 zOrder/位置/锚点）

### 5.5 加新地图（TMX）

1) 放到 `Adventure-King/Resources/Map/<MapName>/`
2) 在关卡 Scene 提供 `LevelConfig`（tmxMapPath、背景、playerSpritePath）
3) 在 SceneRegistry 注册 SceneID
4) 在入口场景（MapScene/HomeScene）加跳转

---

## 6. 踩坑记录：真实问题、根因与修复方式

这一章是我们最希望对外展示的部分：  
我们不回避 bug，而是把“定位与修复思路”讲清楚。

### 6.1 粒子变成方块：误覆写了嵌入纹理

现象：
- `par_levelup` / `par_Restore_health` 等粒子变成方块

根因：
- 这些 plist 自带纹理信息，强行 setTexture 会覆盖成统一纹理（通常是白色方块）

修复原则：
- 不覆写纹理；但要预载与预热，让纹理与粒子在第一次触发时不卡顿

### 6.2 hurt 粒子“完全不可见”

常见根因三件套：
- 粒子数太少/寿命太短
- 资源未加载成功（create 失败）
- 层级/位置被遮挡

工程化修法：
- 参数集中管理（而不是散落在 3 个文件里各改一次）
- 添加创建失败的可观测性（日志/断言/兜底）

### 6.3 hurt L/R 方向老是反：判断依据必须是“攻击来源”

错误做法：
- 根据角色朝向（flippedX）猜测受击方向

正确做法：
- 用攻击来源 worldX 与受击者 worldX 比较决定左右
- 这同样服务于“受击图镜像”“受击粒子方向”

### 6.4 燃烧特效太宽：contentSize 与 physicsSize 的错配

现象：
- Boss 燃烧特效覆盖到身外很远或漂浮

根因：
- 用 contentSize 推导粒子范围，而不是用 PhysicsBody 的真实碰撞范围
- 或者缩放换算做了两次（先除后乘）

修复原则：
- 以 PhysicsBody shape 尺寸为准
- 缩放只做一次（并明确“哪里是局部尺寸、哪里是世界尺寸”）

### 6.5 暂停不彻底：PhysicsWorld 仍在 autoStep

现象：
- 暂停时角色在坡上滑、投掷物继续飞、怪物继续攻击

根因：
- 只停 update 不等于停物理

修复：
- 暂停 gameLayer + 关闭 autoStep + speed=0（见 `GamePauseHelper`）

### 6.6 动画在播但没判定：stopAllActions 杀掉了 CallFunc

这是动作游戏里最典型的“看起来攻击了但没打到”：
- 判定框生成挂在 Action 序列里
- 受击时 stopAllActions
- 判定框永远不生成

修复方向：
- 关键判定逻辑用 scheduleOnce（带 key）管理，不依赖 Action 序列
- 或把判定逻辑挂到不会被 stopAllActions 影响的节点上（例如父节点/场景）

### 6.7 角色漂移/飞起来：SpriteFrame offset 单位用错（points vs pixels）

这是我们遇到过最隐蔽的问题之一，最终被固化成“工具函数 + 注释规则”：
- `SpriteFrame::createWithTexture` 的 rect/offset/originalSize 用的是像素单位
- 如果混用了 points，会出现“整体漂移/飞起”

我们在 `SpriteFrameCacheHelper` 里写了非常明确的注释与实现兜底：
- `Adventure-King/Classes/Utils/SpriteFrameCacheHelper.h`

### 6.8 云存 /admin 404、401：端口冲突与会话失效

现象：
- `/` 看起来正常，但 `/admin` 或某些 API 404
- 游戏端频繁提示登录失效（401）

常见根因：
- 端口被旧进程占用，实际访问的是另一个服务
- 演示系统 token 过期或服务重启后失效（权衡）

工程化排查：
- 先验证 `http://127.0.0.1:<port>/` 返回特征 JSON
- 再验证 `/admin` 是否同源
- 若不一致，先解决端口占用，而不是在客户端“重试到天荒地老”

### 6.9 WSL/Windows 同步：push --force 的破坏性

我们采用：
- WSL（Linux 文件系统）编辑代码
- Windows 目录用 VS 编译运行
- `scripts/wsl-mirror.sh` 同步两边

最危险的坑：
- `push --force` 可能覆盖 Windows 端未提交的改动

规避方式：
- Windows 端 pull/切分支后，先 `wsl-mirror.sh pull` 再继续改
- 规则写在 `WSL_MIRROR.md`

---

## 7. 作者与入口索引

### 7.1 作者

- 元梓浩
- 李胤龙

### 7.2 关键入口（按系统）

- 主菜单/角色选择：`Adventure-King/Classes/Scenes/HelloWorldScene.cpp`
- Loading/预热：`Adventure-King/Classes/Scenes/LoadingScene.cpp`
- 战斗关卡：`Adventure-King/Classes/Scenes/GameScene.cpp`
- 输入：`Adventure-King/Classes/Scenes/GameInputController.cpp`
- 暂停冻结：`Adventure-King/Classes/Scenes/GamePauseHelper.cpp`
- 地图加载：`Adventure-King/Classes/Scenes/LevelMap.cpp`
- 玩家：`Adventure-King/Classes/Character/Player/PlayerCharacter.cpp`
- 怪物基类：`Adventure-King/Classes/Character/Monster/MonsterBase.cpp`
- 职业技能：`Adventure-King/Classes/Character/Player/SkillSets/*.cpp`
- 状态效果：`Adventure-King/Classes/Character/Base/StatusEffect.h`
- 状态工厂：`Adventure-King/Classes/Character/StatusEffects/StatusEffectFactory.cpp`
- 受击与粒子：`Adventure-King/Classes/Character/Base/CharacterBase.cpp`
- 存档：`Adventure-King/Classes/Save/SaveManager.cpp`
- 云同步客户端：`Adventure-King/Classes/Save/Cloud/CloudSyncService.cpp`
- 云存服务端：`tools/cloud_save_server/README.md`

