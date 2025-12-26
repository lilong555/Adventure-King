# Adventure-King 云端存档服务（C++ / HTTP）

本目录提供一个**独立的 C++ 后端**，用于云端存档的注册/登录与同步（HTTP）。  
游戏客户端通过 `SaveMenuLayer` 里的“云存/云同步”按钮与该服务交互。

## 设计目标

- **用户级别管理**：注册、登录，按用户隔离存档
- **云存（上传）**：客户端上传“本地全部存档包”（所有槽位 + 设置）
- **云同步（一键）**：客户端拉取云端包，与本地按时间戳合并（取最新），再回传合并结果
- **不在代码中写死任何服务器地址/IP**：客户端用环境变量配置 URL

## 构建与运行（示例）

> 注意：本工具是后端服务，与游戏工程解耦；可在 Win/Linux 任意一端构建运行。

### WSL（推荐：服务端跑在 WSL）

在 `tools/cloud_save_server` 目录下：

```bash
./run_wsl.sh
```

默认会：
- 自动用 `g++` 编译生成 `ak_cloud_save_server`
- 监听 `0.0.0.0:5173`（Win 端游戏可直接用 `http://localhost:5173` 访问）
- 数据落盘到 `tools/cloud_save_server/cloud_data/`

你也可以用环境变量覆盖默认值：

```bash
export AK_CLOUD_SERVER_ROOT="$HOME/mnt/ecs/adventure-king-cloud"
export AK_CLOUD_SERVER_PORT=5173
./run_wsl.sh
```

#### WSL ↔ Windows 访问说明

通常 Windows 可通过 `http://localhost:5173` 访问到 WSL 内监听的服务。  
若你的系统不支持 localhost 转发，请在 Windows 端改用 WSL IP（自行查询，不要写进仓库）：

```bash
hostname -I
```

然后在游戏中输入 `http://<wsl-ip>:5173`。

### Windows（推荐：无需安装 CMake）

在 `tools/cloud_save_server` 目录下直接运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_win.ps1
```

会在当前目录生成 `ak_cloud_save_server.exe`，然后按脚本输出的示例命令启动即可。

> 如果你确定本地是 CRLF 行尾，也可以用 `.\build_win.bat`；  
> 但在 WSL 镜像同步场景下，`.bat` 可能会被同步成 LF 行尾导致 cmd 解析异常，此时请优先使用 `.ps1`。

### CMake（可选）

```bash
mkdir -p build
cmake -S . -B build
cmake --build build -j
./build/ak_cloud_save_server --root ./cloud_data --host 0.0.0.0 --port 5173
```

## 与游戏联动（推荐流程）

### 1) 启动服务端

WSL 里执行 `./run_wsl.sh` 后保持窗口运行即可。

### 2) 游戏端登录/注册（主菜单）

主菜单左上角提供两个入口：

- **游客登录（禁用云存）**：启用后云端功能直接禁用（存档菜单显示“云端：游客模式”）
- **登录/注册**：弹窗输入：
  - 服务地址：`http://localhost:5173`（或你的 WSL IP）
  - 用户名：`3~32` 位，仅支持 `字母/数字/下划线`
  - 密码：至少 `6` 位

登录成功后，存档菜单内即可使用“云存/云读/云同步”。

### 3) 存档菜单的云按钮说明

- 保存模式：
  - 槽位右侧 **云存**：先保存该槽位，再上传本地【全部存档+设置】（覆盖云端）
  - 底部 **云存(全量)**：直接上传本地【全部存档+设置】（覆盖云端）
  - 底部 **云同步**：双向同步（按时间戳取最新），并回传合并结果
- 读取模式：
  - 槽位右侧 **云读**：先云同步，再加载该槽位
  - 底部 **云同步**：双向同步（按时间戳取最新），并回传合并结果

> 说明：云同步的冲突策略是“同槽位按 `saveTimestamp` 取最新”。

## 数据落盘结构（root 下）

```
cloud_data/
  users.json                 # 用户表（含盐+哈希密码）
  users/
    <username>/
      package.json           # 最近一次上传的完整存档包
      history/               # 历史备份（按时间戳）
      saves/                 # 解析拆分后的槽位 JSON（便于管理/排查）
        save_0.json
        ...
      settings.json
```

## API 概览

- `POST /api/register` `{ "username": "...", "password": "..." }`
- `POST /api/login` `{ "username": "...", "password": "..." }` → `{ "token": "...", "expiresInSeconds": 3600 }`
- `POST /api/sync/push`（需 `Authorization: Bearer <token>`）body=存档包 JSON
- `GET /api/sync/pull`（需 `Authorization: Bearer <token>`）→ 存档包 JSON

## 可视化管理页面（删除/回滚）

服务启动后会打印管理员 token（用于开发/演示）：

```
Admin token (X-AK-Admin-Token): <token>
```

打开管理页面：

- `http://localhost:5173/admin`（或 `http://<wsl-ip>:5173/admin`）

将 token 填入页面顶部输入框后即可：
- 查看用户列表与当前存档包预览
- 查看历史版本列表
- **回滚**到某个历史版本（会覆盖当前 package.json，并自动备份当前版本到 history）
- **删除**用户（账号 + 云端数据，不可恢复）

### 常见问题：打开能进 /admin，但“刷新失败：HTTP 404”

这通常表示你访问到的 `5173` 端口**不是本服务**（端口被其它程序占用/代理到了别处），因此 `/api/admin/users` 不存在。

排查方法：
- 浏览器打开 `http://localhost:5173/`，应返回 JSON：`{"ok":true,"message":"Adventure-King Cloud Save Server"}`
- 查看响应头应包含：`X-AK-Server: ak_cloud_save_server`
- 若不是以上结果，请更换端口（例如 `--port 5174`）并更新游戏端服务地址

## API 快速验证（可选）

服务运行后，在 WSL 中可用 `curl` 进行快速验证：

```bash
curl -sS -X POST http://127.0.0.1:5173/api/register \
  -H 'Content-Type: application/json' \
  -d '{"username":"user_01","password":"123456"}'

curl -sS -X POST http://127.0.0.1:5173/api/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"user_01","password":"123456"}'
```

## 游戏端配置（环境变量）

如果你不想每次打开游戏都在 UI 里登录，也可以在运行游戏前设置环境变量（仅建议开发机使用）：

- `AK_CLOUD_SYNC_URL`：例如 `http://localhost:5173`
- `AK_CLOUD_SYNC_USER`：用户名
- `AK_CLOUD_SYNC_PASS`：密码

这样“云存/云同步”按钮会自动生效（无需主菜单登录）。  
（出于安全考虑：仓库不提交任何公网 IP，请自行在本机环境变量里配置。）

## 安全说明（重要）

当前实现用于开发/演示：
- HTTP 明文（未启用 TLS）
- token 为内存态（服务重启后需重新登录）
- 密码落盘为 `salt + sha256`（演示用；生产应替换为 `bcrypt/argon2` 等）
