#!/usr/bin/env bash
set -euo pipefail

# ============================================================
# Adventure-King 云端存档服务（WSL 一键启动）
# - 适用于：服务端运行在 WSL，游戏运行在 Win 端
# - 默认监听 0.0.0.0:5174（Win 端可用 http://127.0.0.1:5174 访问）
# - 默认数据目录：tools/cloud_save_server/cloud_data（可用环境变量覆盖）
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BIN="$SCRIPT_DIR/ak_cloud_save_server"
SRC="$SCRIPT_DIR/src/main.cpp"
RAPIDJSON_INC="$SCRIPT_DIR/../../Adventure-King/cocos2d/external"

if [[ ! -x "$BIN" || "$BIN" -ot "$SRC" ]]; then
  echo "[INFO] 编译 ak_cloud_save_server ..."
  g++ -std=c++17 -O2 -pthread \
    -I"$SCRIPT_DIR" \
    -I"$RAPIDJSON_INC" \
    "$SRC" \
    -o "$BIN"
fi

ROOT="${AK_CLOUD_SERVER_ROOT:-$SCRIPT_DIR/cloud_data}"
HOST="${AK_CLOUD_SERVER_HOST:-0.0.0.0}"
PORT="${AK_CLOUD_SERVER_PORT:-5174}"

echo "[INFO] 启动云存服务：root=$ROOT host=$HOST port=$PORT"
exec "$BIN" --root "$ROOT" --host "$HOST" --port "$PORT"
