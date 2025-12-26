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

```bash
mkdir -p build
cmake -S . -B build
cmake --build build -j
./build/ak_cloud_save_server --root ./cloud_data --host 127.0.0.1 --port 5173
```

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

## 游戏端配置（环境变量）

在运行游戏前设置：

- `AK_CLOUD_SYNC_URL`：例如 `http://127.0.0.1:5173`
- `AK_CLOUD_SYNC_USER`：用户名
- `AK_CLOUD_SYNC_PASS`：密码

这样“云存/云同步”按钮才会生效。  
（出于安全考虑：仓库不提交任何公网 IP，请自行在本机环境变量里配置。）

