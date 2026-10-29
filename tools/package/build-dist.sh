#!/usr/bin/env bash
# Build dist/<game>.app for every game folder that has main.bas.
# ZIP store layout matches mmcore PACKAGE (see tools/package/pack_app.py).
# Does not run the mmcore binary. Safe to re-run: identical inputs keep
# identical bytes, so an unchanged game does not rewrite its .app.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PY="$ROOT/tools/package/pack_app.py"
DIST="$ROOT/dist"

python3 "$PY" self-test

prune=0
if [ "$#" -eq 0 ]; then
  prune=1
  games=()
  for d in "$ROOT/games"/*/; do
    [ -d "$d" ] || continue
    games+=("$(basename "$d")")
  done
else
  games=("$@")
fi

if [ "${#games[@]}" -eq 0 ]; then
  echo "no games under $ROOT/games" >&2
  exit 1
fi

mkdir -p "$DIST"
built=()
for g in "${games[@]}"; do
  case "$g" in
    */* | .* | "")
      echo "bad game name: $g" >&2
      exit 1
      ;;
  esac
  if [ ! -f "$ROOT/games/$g/main.bas" ]; then
    echo "skip: games/$g has no main.bas" >&2
    exit 1
  fi
  python3 "$PY" pack --root "$ROOT" --game "$g" --out "$DIST/$g.app"
  python3 "$PY" check --app "$DIST/$g.app" --game "$g"
  built+=("$g")
done

if [ "$prune" -eq 1 ]; then
  shopt -s nullglob
  for app in "$DIST"/*.app; do
    base="$(basename "$app" .app)"
    keep=0
    for g in "${built[@]}"; do
      if [ "$g" = "$base" ]; then
        keep=1
        break
      fi
    done
    if [ "$keep" -eq 0 ]; then
      rm -f "$app"
      echo "removed dist/$base.app"
    fi
  done
fi
