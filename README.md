# 冒险王之神兵传奇

**作者**: 元梓浩, 李胤龙

##  简介

参考同名游戏《冒险王之神兵传奇》，开发一款基于 Cocos2d-x 引擎的横版动作冒险游戏。玩家将扮演一名冒险者，探索未知世界、击败怪物、收集神兵利器，并通过升级与战斗不断提升自己的能力。游戏融合了角色成长、装备收集与战斗策略，强调流畅的操作手感与丰富的关卡设计。

##  项目介绍

- 如果你希望详细了解，建议读：[详细文档](https://lilong555.github.io/Adventure-King)

## 🛠️ 技术栈

-   **游戏引擎**: Cocos2d-x
-   **数据库**: SQLite
-   **AI相关架构**: Langchain 


### Windows 构建与运行
- Visual Studio：打开 `Adventure-King/proj.win32/Adventure-King.sln`，选择 `Debug/Win32` 构建并运行

<details>
  <summary>Windows 发布（安装包）</summary>
  > 
  目标：给外部用户一个“安装包（.exe）”，安装后即可运行游戏；同时把“赐福后端（Python/LangChain）”脚本一并安装到本机目录（可选启动）。

前置条件：
- 已安装 **Inno Setup 6**（用于生成安装包）
- 已安装 **Python 3.10+**（仅用于赐福后端；游戏本体不依赖 Python）

步骤：
1. 用 Visual Studio 构建 `Release|Win32`（会生成 `Adventure-King/proj.win32/Release.win32/Adventure-King.exe`，并把 `Resources/` 拷贝到输出目录）
2. 在 Windows PowerShell 执行：

```powershell
powershell -ExecutionPolicy Bypass -File tools\installer_win\build_installer.ps1
```

输出：
- 安装包会生成到 `dist/Adventure-King-Setup.exe`
- 安装后会提供 3 个快捷方式：
  - `Adventure-King`
  - `Adventure-King（启动赐福后端）`
  - `赐福后端（Blessing Server）`

说明：
- 安装包脚本：`tools/installer_win/AdventureKing.iss`
- Release|Win32 使用 `/MT` 静态链接 CRT，安装包不再额外安装 VC++ 运行库（体积会更大，但更易分发）。
- 启动赐福后端依赖 Python；第一次启动会创建 `.venv` 并安装依赖（需联网或本机有可用 pip 源）。
- 安装包构建脚本会尽量提前下载赐福后端依赖的离线 wheels（用于减少首次启动安装依赖失败/卡顿）。如果下载失败，会自动回退到“首次启动在线安装”。
</details>

## 云端存档（WSL 后端 + 游戏端登录/游客）

本项目提供一套**本地可跑的 C++ 云存后端**（HTTP），推荐在 **WSL** 等liunx内启动服务；Windows 端运行游戏通过 `localhost` 访问即可。  
服务端代码位于：`tools/cloud_save_server/`（详见该目录 README）。


<details>
  <summary>具体内容</summary>

  ### 1) 在 WSL 启动云存服务

```bash
cd ~/code/fansqim/tools/cloud_save_server
./run_wsl.sh
```

默认行为：
- 自动用 `g++` 编译生成 `ak_cloud_save_server`
- 监听 `0.0.0.0:5174`
- 数据落盘到 `tools/cloud_save_server/cloud_data/`

可选：把云存数据目录放到你已挂载的远端盘（例如 `~/mnt/ecs/...`），用于演示“云端落盘”：

```bash
export AK_CLOUD_SERVER_ROOT="$HOME/mnt/ecs/adventure-king-cloud"
export AK_CLOUD_SERVER_PORT=5174
./run_wsl.sh
```

### 2) Windows 端访问 WSL 服务

通常情况下，WSL2 在 Windows 上可直接通过：

`http://127.0.0.1:5174`

访问到 WSL 内监听 `0.0.0.0:5174` 的服务。  
若你的系统不支持 localhost 转发，请在 Windows 端改用 WSL 的 IP（自行在本机查询，不要写进仓库）：

- 在 WSL 执行：`hostname -I`
- 然后在游戏里输入：`http://<wsl-ip>:5174`

### 3) 游戏端使用方式（主菜单）

进入主菜单后左上角会出现账号入口：

- **游客登录（禁用云存）**：选择后云端功能会被禁用（存档菜单会显示“云端：游客模式”）
- **登录/注册**：弹出窗口输入：
  - 服务地址：`http://127.0.0.1:5174`（或你的 WSL IP）
  - 用户名：`3~32` 位，仅支持 `字母/数字/下划线`
  - 密码：`6~64` 位（演示用规则）

登录成功后，“存档菜单”内将可使用：
- 保存模式：槽位右侧 **云存**（先保存该槽位，再上传本地“全量存档包”覆盖云端） / 底部 **云存(全量)** / **云同步**
- 读取模式：槽位右侧 **云读**（先云同步再加载槽位） / 底部 **云同步**

### 4) 常见问题排查

- **提示“账号：未登录（云存不可用）”**：未启动服务或未登录/注册；先确认 `./run_wsl.sh` 正在运行
- **登录失败/注册失败**：检查服务端窗口日志；确认端口未被占用、URL 可访问
- **Windows 无法访问 WSL**：改用 WSL IP（见上文）

### 5) 可视化管理页面（删除/回滚）

云存服务端提供一个简单的管理页面（用于开发/演示）：

- 打开：`http://127.0.0.1:5174/admin`
- 页面文件：`tools/cloud_save_server/web/admin.html`（可按需调整样式/功能）
- 管理员 Token：服务端启动日志会打印  
  `Admin token (X-AK-Admin-Token): <token>`  
  把该 token 填到页面顶部输入框即可执行 **删除用户/回滚历史版本**。

### 6) 安全说明（重要）

当前实现用于开发/演示（强烈建议仅在本机/内网使用，不要暴露到公网）：
- HTTP 明文（未启用 TLS，登录/同步数据可被窃听）
- token 为内存态（服务重启后需重新登录）
- 密码落盘为 `salt + sha256`（仅为避免明文；生产应替换为 `bcrypt/argon2` 等专用口令哈希）
- 云存目录权限/备份若泄露，可能导致离线暴力破解与存档被篡改

</details>




## AI 赐福（LangChain 后端）

本项目的“赐福”是一个**开发阶段演示功能**：游戏通过 HTTP 请求本地后端，由后端调用 OpenAI 兼容接口，并通过 LangChain 的 **工具调用** 约束输出，最终返回可直接应用的赐福属性（覆盖旧 buff）。  
服务端代码位于：`tools/blessing_server/`（详见该目录 README）。

<details>
  <summary>点击展开查看更多</summary>

  ### 1) 在 WSL 启动赐福后端

```bash
cd ~/code/fansqim/tools/blessing_server
./run.sh
```

默认监听 `0.0.0.0:5181`，Windows 端通常可用：

- `http://127.0.0.1:5181`

可选环境变量：

- `AK_BLESSING_HOST`（默认 `0.0.0.0`）
- `AK_BLESSING_PORT`（默认 `5181`）
- `AK_OPENAI_BASE_URL`（默认 `https://elysiver.h-e.top/v1`；可自行替换为任何 OpenAI 兼容网关）

> 注意：如果你第一次运行报 `python3-venv` 缺失，请先安装 `python3-venv` 后再执行 `./run.sh`。

### 2) 游戏端配置方式

- 进入地图 `Home`，走到赐福 NPC（`spr_shutouj.png`）附近按 `W` 进入赐福界面
- 在赐福界面填写：
  - `Base URL`：例如 `http://127.0.0.1:5181`
  - `API Key`：你的 OpenAI 兼容 Bearer Token（展示阶段由用户手动填写；不要写进仓库/文档）
  - `Model`：默认 `gemini-3-flash-preview`（可按你的网关支持情况修改）

### 3) 接口概览（用于联调）

- `POST /api/blessing/question`：生成考验问题（中文，2~3 问）
- `POST /api/blessing/answer`：提交玩家回答并返回赐福结构

### 4) 常见问题排查

- **404/连接不上**：确认端口是否被占用；浏览器访问 `http://127.0.0.1:5181/` 应返回 `{ "ok": true, ... }`
- **WSL → Windows 访问异常**：把服务端 host 设为 `0.0.0.0`，并优先用 `127.0.0.1`；不支持转发时再改用 WSL IP
</details>




## 当前可玩内容（main）
- 关卡：`Origin_Mushroom`（TMX 地图 + 物理碰撞 + 传送门）、`Mystery_Forest`（含连战模式）
- 角色（可在主菜单选择）：
  - **Klee（法师）**：远程投掷型，擅长范围伤害
  - **战士（Warrior）**：近战型，擅长 AoE 火焰技能
  - **刺客（Assassin）**：近战型，擅长快速斩击
- 战斗：
  - **Klee**：
    - 普攻（`J`/`4`）：扔炸弹（TNT），落地/命中爆炸并对范围内敌人造成伤害
    - 技能1（`E`/`K`）：发射导弹，命中爆炸并触发燃烧 DOT（持续 5s、每 0.5s 结算、可叠层）
  - **战士**：
    - 普攻（`J`/`4`）：近战挥砍，生成命中判定框
    - 技能1（`E`/`K`）：Fire 火焰冲击，智能选点覆盖最多敌人，播放火焰粒子特效
  - **刺客**：
    - 普攻（`J`/`4`）：近战攻击
    - 技能1（`E`/`K`）：Slash 斩击，快速近战 AoE
  - 燃烧特效：橙红色火焰粒子，尺寸自适应角色体型
- 装备特效系统：
  - 血契短剑：吸血效果（按伤害比例回复生命）
  - 焰纹法杖：命中概率附加燃烧
  - 荆棘甲：受击反伤
  - 急救面罩：低血量自动回复
  - 追猎之靴：击杀后移速加成
- 刷怪：从 TMX 对象组 `enemy_g` 读取生成点，首次进入视野触发，约每 `0.4s` 生成一个
- 连战模式：`Mystery_Forest` 场景支持波次刷怪，击败当前波次后触发下一波
- 敌人：
  - 哥布林（Goblin）：HP 随玩家等级缩放
  - 哥布鲁（Goblu）：远近攻击判定，更强的属性
  - 黑暗法师（Obscur）：远近双模式攻击，释放冰系法术
- 存档系统：5 个存档槽位，支持保存/加载游戏进度

## 操作
| 按键 | 功能 |
|---|---|
| `A`/`←` | 向左移动 |
| `D`/`→` | 向右移动 |
| `Shift` | 跑步 |
| `W` | 交互/跳跃（在传送门上触发返回地图；否则跳跃） |
| `Space` | 仅跳跃 |
| `J`/`4` | 普攻：扔炸弹 |
| `E`/`K` | 技能1：导弹（火球） |
| `Esc` | 暂停菜单 |
| `B` | 背包菜单 |

## 地图
- 两个地图：起源之菇，神秘之森


## 游戏核心功能

### 角色系统

支持三种角色：

> 战士，法师，刺客

核心状态切换：

> Idle
>
> 行走
>
> 奔跑
>
> 跳跃
>
> 二段跳
>
> 攻击
>

-   角色有技能点系统，升级获得技能点解锁主动技能/被动技能
-   角色有属性系统，升级获得属性点提高属性（力量、防御、暴击率、移速……）
-   生命值与能量值系统
-   等级与经验值系统
-   受击硬直反馈与死亡判定

### 武器与装备系统

-   多种武器
-   武器可切换，具备不同攻击特效
-   装备属性影响角色属性

### 敌人系统

-   至少2种普通敌人（近战/远程）
-   1个Boss敌人，具备特殊技能
-   敌人具备基础AI：追击、巡逻
-   击杀敌人可能掉落血瓶、蓝瓶
-   击败敌人必定获取经验，等级越高掉得越多

### 关卡与场景

-   至少2个不同主题的关卡（如森林、地牢）
-   场景可交互元素（传送点）
-   支持场景音乐和动作特效

### UI 系统

-   开始菜单
-   存档界面
-   暂停菜单
-   背包界面
-   地图界面
-   游戏结束与重新开始界面
-   玩家状态栏（血量、能量、经验值、技能图标，状态栏）
-   Boss血条/击破条显示

## 扩展功能

-   AI赐福
-   云端存档，登录
-   更丰富的技能
-   剧情对话系统与NPC交互
-   被动技能或符文（build玩法）
