#!/usr/bin/env python3
"""
Test-only edits on a *copy* of a game tree (never the published sources).

Usage:
  hook.py <game_dir> [TH_START=0 TH_AUTO=1 ...]

If tools/gametest/hooks/<game_name>.py exists (game_name = basename of the
folder), it is imported and run instead. Otherwise this script is a no-op
unless TH_* constants are passed, in which case it writes includes/testhook.inc
with those CONST lines (games must include that file themselves via a hook
recipe — see hooks/README.md and circuitquest-style recipes).
"""
from __future__ import annotations

import importlib.util
import os
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))


def _noop_consts(game_dir: str, extra: str) -> None:
    if not extra.strip():
        return
    consts: dict[str, int] = {}
    for kv in extra.split():
        k, v = kv.split("=", 1)
        consts[k] = int(v)
    inc = os.path.join(game_dir, "includes", "testhook.inc")
    os.makedirs(os.path.dirname(inc), exist_ok=True)
    lines = ["const " + ", ".join(f"{k} = {v}" for k, v in consts.items())]
    with open(inc, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    game_dir = os.path.abspath(sys.argv[1])
    extra = sys.argv[2] if len(sys.argv) > 2 else ""
    name = os.path.basename(game_dir.rstrip("/"))
    custom = os.path.join(TOOLS, "hooks", f"{name}.py")
    if os.path.isfile(custom):
        spec = importlib.util.spec_from_file_location(f"gametest_hook_{name}", custom)
        mod = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(mod)
        mod.apply(game_dir, extra)
        return 0
    _noop_consts(game_dir, extra)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
