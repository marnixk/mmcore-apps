#!/usr/bin/env bash
# Capture ground-truth PNGs from Jetpack v1.5 via DOSBox-X + Xvfb.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/_download/extracted/jetpak15"
OUT="$ROOT/reference"
mkdir -p "$OUT"

if [[ ! -f "$GAME/JETPACK.EXE" ]]; then
  echo "Missing $GAME/JETPACK.EXE (unpack jetpak15.zip first)" >&2
  exit 1
fi

CONF="$ROOT/_dosbox/cap2.conf"
xvfb-run -a -s "-screen 0 640x480x24 -ac" bash -c "
  openbox >/dev/null 2>&1 & sleep 1
  dosbox-x -conf '$CONF' -fastlaunch &
  sleep 16
  W=\$(xdotool search --name DOSBox | head -1)
  xdotool windowactivate --sync \"\$W\"
  scrot -u '$OUT/title_screen.png'
  xdotool key s
  sleep 8
  scrot -u '$OUT/level00_ready.png'
"

echo "Reference PNGs in $OUT"
