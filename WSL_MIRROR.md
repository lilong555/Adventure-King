# WSL 镜像工作副本（给 Codex/Claude 用）

目标：保留 Windows 盘下的仓库作为 VS 编译用主目录，同时在 WSL 的 Linux 文件系统里放一份“工作副本”给 Codex/Claude 使用；两边用 `rsync` 做增量同步（只传改动）。

本仓库当前路径约定：

- Windows 主目录（WSL 下路径）：`/mnt/e/code/fansqim`
- WSL 工作副本：`~/code/fansqim`

## 使用规则（强制）

1. **WSL 工作副本（`~/code/fansqim`）是唯一编辑入口**：Codex/Claude、脚本、日常改代码都在这里进行。
2. **Windows 主目录（`/mnt/e/code/fansqim`）是编译入口**：VS/MSBuild 等编译只在 Windows 这份目录进行。
3. **只要在 WSL 改了代码，就必须同步回 Windows**：每次改动完成后立刻执行 `./scripts/wsl-mirror.sh push`，再去 Windows 编译/运行/提交。
4. **不要双端同时修改同一批文件**：需要在 Windows 做 `git pull`/切分支后，用 `./scripts/wsl-mirror.sh pull` 拉回 WSL 再继续改。

## 快速用法（WSL 执行）

第一次初始化（从 Windows 拉到 WSL）：

```bash
cd /mnt/e/code/fansqim
./scripts/wsl-mirror.sh init
```

日常：在 WSL 工作副本里改代码（推荐）：

```bash
cd ~/code/fansqim
```

改完同步回 Windows（强制，用于 VS 编译）：

```bash
./scripts/wsl-mirror.sh push
```

如果你在 Windows 主目录做了 `git pull`/切分支等，需要把变化拉回 WSL：

```bash
./scripts/wsl-mirror.sh pull
```

## 命令说明

- `init`：首次建立 WSL 工作副本（Windows → WSL）。
- `pull`：把 Windows 变化同步到 WSL（Windows → WSL）。默认**不**带 `--delete`（避免误删 WSL 侧文件）；需要完全镜像时再加 `pull --delete`。
- `push`：把 WSL 变化同步回 Windows（WSL → Windows），并使用 `--delete` 保持镜像一致（排除项不会被删）。
- `--dry-run`：预览将要发生的增量变更（推荐先跑一次确认）。
- `--force`：当脚本检测到目标目录存在未提交的 tracked 改动时，允许强制覆盖（谨慎使用）。

## 路径可配置

如果你的 Windows 盘路径或 WSL 目标路径不是默认值，可以临时指定：

```bash
WIN_ROOT=/mnt/d/work/MyGame WSL_ROOT=~/code/MyGame ./scripts/wsl-mirror.sh init
```

## 同步会跳过/保护的目录

脚本默认排除（不会同步，也不会被 `push --delete` 删除）：

- `.vs/`
- `Debug.win32/`、`Release.win32/`
- `build/`、`bin/`、`obj/`
- `node_modules/`、`.venv/`、`dist/`
- `*.vcxproj.user`
