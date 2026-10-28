#!/usr/bin/env bash
# Fetch (or update) marnixk/mmcore, apply local patches, build native/mmcore.
# Idempotent. Pin the interpreter with tools/mmcore/MMCORE_REF (branch, tag, or SHA).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="${MMCORE_SRC:-$ROOT/.mmcore}"
REF_FILE="$ROOT/tools/mmcore/MMCORE_REF"
PATCH_DIR="$ROOT/tools/mmcore/patches"
REF="$(tr -d '[:space:]' < "$REF_FILE")"
REF="${REF:-master}"

log() { printf '\n\033[1;34m==>\033[0m %s\n' "$*"; }

need() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "missing required command: $1" >&2
    exit 1
  }
}

need git
need make
need cc
need pkg-config
pkg-config --exists sdl2 || {
  echo "SDL2 dev package missing (install libsdl2-dev)" >&2
  exit 1
}

if [ ! -d "$SRC/.git" ]; then
  log "Cloning mmcore into $SRC"
  git clone --filter=blob:none https://github.com/marnixk/mmcore.git "$SRC"
fi

log "Checking out mmcore ref: $REF"
git -C "$SRC" fetch origin "$REF" --depth 1 2>/dev/null || git -C "$SRC" fetch origin
git -C "$SRC" checkout -q FETCH_HEAD 2>/dev/null || git -C "$SRC" checkout -q "$REF"

apply_patch() {
  local p="$1"
  if git -C "$SRC" apply --reverse --check "$p" >/dev/null 2>&1; then
    log "Already applied $(basename "$p")"
    return 0
  fi
  if git -C "$SRC" apply --check "$p" >/dev/null 2>&1; then
    log "Applying $(basename "$p")"
    git -C "$SRC" apply "$p"
    return 0
  fi
  # Upstream may already contain the hunk (the harness poll landed in
  # platform_sdl.c). Treat that as applied so a newer mmcore still builds.
  if grep -q "sdl_harness_poll" "$SRC/native/platform_sdl.c" 2>/dev/null; then
    log "Already upstream $(basename "$p")"
    return 0
  fi
  echo "patch does not apply cleanly: $p" >&2
  exit 1
}

if [ -d "$PATCH_DIR" ]; then
  for p in $(ls "$PATCH_DIR"/*.patch 2>/dev/null | sort); do
    apply_patch "$p"
  done
fi

log "Building native/mmcore"
"$SRC/scripts/build-native.sh" sdl

printf '\n\033[1;32mok:\033[0m %s\n' "$SRC/native/mmcore"
echo "$SRC/native/mmcore"
