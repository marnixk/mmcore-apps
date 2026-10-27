#!/usr/bin/env bash
# run.sh RUN_NAME GAME_DIR BODY_HS [MODECMD]
#   RUN_NAME   label for output under tools/gametest/out/<RUN_NAME>
#   GAME_DIR   path to games/<name> (source; copied to a scratch drive)
#   BODY_HS    harness fragment (wait/key/shot); @OUT@ -> output directory
#   MODECMD    optional MODE line (default MODE 19,16)
#
# Environment:
#   MMCORE      path to mmcore binary (default: ../../.mmcore/native/mmcore)
#   HOOK        hook.py path (default beside this script)
#   TH          "TH_START=0 ..." hook constants
#   CROOT       drive root with C/ inside (default: scratch per run)
#   TMO         timeout seconds (default 240)
#   MOVES       directory with moves_*.txt copied into the game copy
set -euo pipefail

TDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$TDIR/../.." && pwd)"

if [ "$#" -lt 3 ]; then
  echo "usage: run.sh RUN_NAME GAME_DIR BODY_HS [MODECMD]" >&2
  exit 2
fi

name="$1"
GAME="$2"
body="$3"
modecmd="${4:-MODE 19,16}"

if [ ! -d "$GAME" ] || [ ! -f "$body" ]; then
  echo "run.sh: missing game dir or body script" >&2
  exit 2
fi

MMCORE="${MMCORE:-$ROOT/.mmcore/native/mmcore}"
if [ ! -x "$MMCORE" ]; then
  echo "mmcore not built; run: bash tools/mmcore/build-mmcore.sh" >&2
  exit 1
fi

PY="${ROOT}/.venv/bin/python"
if [ ! -x "$PY" ]; then
  PY=python3
fi

out="$TDIR/out/$name"
drv="$TDIR/drv/$name"
rm -rf "$out" "$drv"
mkdir -p "$out" "$drv/g"

gamebase="$(basename "$GAME")"
mkdir -p "$drv/g"
cp -r "$GAME/." "$drv/g/"
if [ -n "${MOVES:-}" ]; then
  cp "$MOVES"/moves_*.txt "$drv/g/" 2>/dev/null || true
fi

"$PY" "${HOOK:-$TDIR/hook.py}" "$drv/g" "${TH:-}" || exit 1

croot="${CROOT:-$TDIR/croot/$name}"
if [ -z "${CROOT:-}" ]; then
  rm -rf "$croot"
  mkdir -p "$croot/C"
fi

main="${MAIN_BAS:-}"
if [ -z "$main" ]; then
  if [ -f "$drv/g/main.bas" ]; then
    main="main.bas"
  elif [ -f "$drv/g/$gamebase.bas" ]; then
    main="$gamebase.bas"
  else
    cand="$(find "$drv/g" -maxdepth 1 -name '*.bas' | head -1)"
    main="$(basename "$cand")"
  fi
fi

hs="$out/script.hs"
{
  echo "wait 1200"
  echo "feed $modecmd"
  echo "wait 600"
  echo "feed RUN \"D:/g/$main\""
  sed "s#@OUT@#$out#g" "$body"
  echo "wait 400"
  echo "quit"
} > "$hs"

set +e
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  MMB_DRIVE_ROOT="$croot" MMB_SDL_HARNESS="$hs" MMB_SDL_SERIAL=1 \
  timeout -s KILL "${TMO:-240}" "$MMCORE" --drive "$drv" < /dev/null > "$out/serial.log" 2>&1
rc=$?
set -e
echo "exit=$rc" >> "$out/serial.log"

cp "$drv/g/th.log" "$out/" 2>/dev/null || true

"$PY" "$TDIR/ppm_to_png.py" "$out"

"$PY" "$TDIR/check_serial.py" "$out/serial.log" > "$out/check.txt" 2>&1 || true

if grep -q "serial errors" "$out/check.txt" 2>/dev/null; then
  exit 1
fi
if [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ]; then
  echo "timeout (mmcore killed after ${TMO:-240}s)" >> "$out/check.txt"
  if [ -f "$out/title.png" ] && [ -f "$out/play0.png" ] && [ -f "$out/pause.png" ]; then
    echo "partial pass: required screenshots present" >> "$out/check.txt"
    exit 0
  fi
  exit 1
fi
exit 0
