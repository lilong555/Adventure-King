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

source .venv/bin/activate
python -m pip install -U pip >/dev/null
python -m pip install -r requirements.txt

exec python ak_blessing_server.py serve --host "${HOST}" --port "${PORT}"

