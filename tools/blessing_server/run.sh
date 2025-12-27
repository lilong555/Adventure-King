#!/usr/bin/env bash
set -euo pipefail

# Adventure-King 赐福后端一键启动脚本（WSL/Linux）
# - 自动创建 venv 并安装依赖
# - 默认监听 0.0.0.0:5181

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

HOST="${AK_BLESSING_HOST:-0.0.0.0}"
PORT="${AK_BLESSING_PORT:-5181}"

if [ ! -d ".venv" ]; then
  python3 -m venv .venv
fi

if [ ! -f ".venv/bin/activate" ]; then
  echo "[WARN] 检测到 .venv 不完整（缺少 bin/activate），可能是上次创建中断或环境异常，正在重建..." 1>&2
  rm -rf .venv
  python3 -m venv .venv || true
fi

if [ ! -f ".venv/bin/activate" ]; then
  echo "[ERROR] 创建虚拟环境失败：未生成 .venv/bin/activate。" 1>&2
  echo "请先安装 venv 依赖后重试，例如：sudo apt-get update && sudo apt-get install -y python3-venv" 1>&2
  exit 1
fi

source .venv/bin/activate
python -m pip install -U pip >/dev/null
python -m pip install -r requirements.txt

exec python ak_blessing_server.py serve --host "${HOST}" --port "${PORT}"
