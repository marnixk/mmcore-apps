#!/usr/bin/env bash
# Cloud Agent environment: mmcore native SDL build + Python tooling for games.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

log() { printf '\n\033[1;34m==>\033[0m %s\n' "$*"; }

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
  SUDO="sudo"
fi

log "Installing apt packages (SDL2, build tools, Python)"
$SUDO DEBIAN_FRONTEND=noninteractive apt-get update -qq
$SUDO DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
  build-essential git pkg-config libsdl2-dev python3 python3-pip python3-venv

log "Python venv + Pillow"
if [ ! -x "$REPO_ROOT/.venv/bin/python" ]; then
  python3 -m venv "$REPO_ROOT/.venv"
fi
"$REPO_ROOT/.venv/bin/pip" install -q --upgrade pip
"$REPO_ROOT/.venv/bin/pip" install -q Pillow

log "Building mmcore (pinned ref + patches)"
bash "$REPO_ROOT/tools/mmcore/build-mmcore.sh"

log "Environment ready"
"$REPO_ROOT/.venv/bin/python" -c "import PIL; print('Pillow', PIL.__version__)"
"$REPO_ROOT/.mmcore/native/mmcore" --help 2>&1 | head -1 || true
