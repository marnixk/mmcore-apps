#!/usr/bin/env bash
# smoke.sh GAME_DIR
# Runs the standard smoke harness at all five target resolutions.
set -euo pipefail

TDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$TDIR/../.." && pwd)"

if [ "$#" -lt 1 ]; then
  echo "usage: smoke.sh games/<name>" >&2
  exit 2
fi

GAME="$(cd "$ROOT" && cd "$1" && pwd)"
name="$(basename "$GAME")"
profile="$TDIR/profiles/$name.body.hs"
if [ ! -f "$profile" ]; then
  echo "no smoke profile: $profile" >&2
  echo "add tools/gametest/profiles/<game>.body.hs" >&2
  exit 2
fi

summary="$TDIR/out/smoke-$name/summary.txt"
rm -rf "$TDIR/out/smoke-$name"
mkdir -p "$TDIR/out/smoke-$name"

export TMO="${TMO:-90}"

fail=0
while IFS='|' read -r id mode _label; do
  [ -z "$id" ] && continue
  [[ "$id" =~ ^# ]] && continue
  runid="smoke-$name-$id"
  echo "== $name @ $mode ($id) =="
  if bash "$TDIR/run.sh" "$runid" "$GAME" "$profile" "$mode"; then
    echo "PASS $id" | tee -a "$summary"
  else
    echo "FAIL $id" | tee -a "$summary"
    fail=1
  fi
done < "$TDIR/modes.txt"

if [ "$fail" -ne 0 ]; then
  echo "smoke FAILED for $name" >&2
  exit 1
fi
echo "smoke PASSED for $name"
exit 0
