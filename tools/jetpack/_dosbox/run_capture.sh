#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/_download/extracted/jetpak15"
CAP="$ROOT/out/dosbox_capture"
mkdir -p "$CAP"
export SDL_VIDEODRIVER=x11
export DISPLAY=:99

Xvfb :99 -screen 0 640x480x24 &
XVFB_PID=$!
sleep 1
openbox &
sleep 1

cat > /tmp/dosbox_capture.conf <<EOF
[sdl]
fullscreen=false
windowposition=0,0
windowresolution=640x480
output=surface

[render]
aspect=false
scaler=normal2x

[cpu]
cycles=max

[dos]
memsize=16

[autoexec]
mount c $GAME
c:
jetpack.exe
EOF

dosbox -conf /tmp/dosbox_capture.conf &
DB_PID=$!
sleep 6
# Title screen: press S to start, then wait
xdotool search --name DOSBox windowactivate --sync key s
sleep 4
# F3 might be level select - skip, just capture gameplay level 0
scrot "$CAP/title_or_game.png" || true
kill $DB_PID 2>/dev/null || true
kill $XVFB_PID 2>/dev/null || true
