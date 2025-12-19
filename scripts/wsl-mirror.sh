#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/wsl-mirror.sh init [--dry-run]
  scripts/wsl-mirror.sh pull [--dry-run] [--delete] [--force]
  scripts/wsl-mirror.sh push [--dry-run]

Environment (optional):
  WIN_ROOT=/mnt/e/code/fansqim
  WSL_ROOT=$HOME/code/fansqim

Notes:
  - Designed to run inside WSL.
  - "push" uses --delete (mirror WSL -> Windows).
  - "pull" does NOT use --delete unless you pass --delete.
EOF
}

die() {
  echo "error: $*" >&2
  exit 1
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "missing command: $1"
}

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
repo_name="$(basename "$repo_root")"

WIN_ROOT="${WIN_ROOT:-/mnt/e/code/$repo_name}"
WSL_ROOT="${WSL_ROOT:-$HOME/code/$repo_name}"

EXCLUDES=(
  --exclude '.vs'
  --exclude 'build'
  --exclude 'bin'
  --exclude 'obj'
  --exclude 'Debug.win32'
  --exclude 'Release.win32'
  --exclude 'node_modules'
  --exclude '.venv'
  --exclude 'dist'
  --exclude '*.vcxproj.user'
)

cmd="${1:-}"
shift || true

dry_run=0
force=0
use_delete=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    -n|--dry-run) dry_run=1 ;;
    --force) force=1 ;;
    --delete) use_delete=1 ;;
    *) die "unknown arg: $1" ;;
  esac
  shift
done

require_cmd rsync

rsync_args=(rsync -a --info=progress2 "${EXCLUDES[@]}")
if [[ $dry_run -eq 1 ]]; then
  rsync_args+=(--dry-run --itemize-changes)
fi

ensure_clean_git_or_force() {
  local path="$1"
  if [[ $force -eq 1 ]]; then
    return 0
  fi
  if [[ -d "$path/.git" ]]; then
    require_cmd git
    if [[ -n "$(git -C "$path" status --porcelain --untracked-files=no)" ]]; then
      die "working copy has uncommitted changes: $path (use --force to override)"
    fi
  fi
}

sync_dirs() {
  local src="$1"
  local dst="$2"
  local delete_flag="$3"

  [[ -d "$src" ]] || die "source not found: $src"
  mkdir -p "$dst"

  if [[ "$src" -ef "$dst" ]]; then
    die "source and destination are the same directory: $src"
  fi

  local args=("${rsync_args[@]}")
  if [[ "$delete_flag" -eq 1 ]]; then
    args+=(--delete)
  fi

  args+=("$src/" "$dst/")
  "${args[@]}"
}

case "$cmd" in
  init)
    sync_dirs "$WIN_ROOT" "$WSL_ROOT" 0
    ;;
  pull)
    ensure_clean_git_or_force "$WSL_ROOT"
    sync_dirs "$WIN_ROOT" "$WSL_ROOT" "$use_delete"
    ;;
  push)
    ensure_clean_git_or_force "$WIN_ROOT"
    sync_dirs "$WSL_ROOT" "$WIN_ROOT" 1
    ;;
  ""|-h|--help)
    usage
    ;;
  *)
    die "unknown command: $cmd (run with --help)"
    ;;
esac
