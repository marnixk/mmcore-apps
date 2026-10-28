#!/usr/bin/env python3
"""Throwaway FRAMEPROFILE of the Jetpack play loop.

Copies games/jetpack, caps the copy at 48 logic frames, and runs mmcore
with OPTION FRAMEPROFILE (MMCORE_FRAME_PROFILE=1). Does not modify
games/jetpack sources. Prints a short summary of the CSV mmcore writes
when the program ends.

A profile "frame" is one present (page copy or a blit onto the display),
not one BASIC logic tick. Statement counts are the portable number;
microseconds are this machine, not a Pi.
"""
from __future__ import annotations

import os
import shutil
import statistics
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
GAME = os.path.join(ROOT, "games", "jetpack")
MMCORE = os.environ.get("MMCORE", os.path.join(ROOT, ".mmcore", "native", "mmcore"))


def patch(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise SystemExit(f"patch miss: {label}")
    return text.replace(old, new, 1)


def main() -> int:
    if not os.path.isfile(MMCORE):
        print("mmcore missing", MMCORE, file=sys.stderr)
        return 1
    tmp = tempfile.mkdtemp(prefix="jp-fp-")
    drv = os.path.join(tmp, "drv")
    croot = os.path.join(tmp, "croot")
    game = os.path.join(drv, "g")
    os.makedirs(os.path.join(croot, "C"))
    shutil.copytree(GAME, game, ignore=shutil.ignore_patterns("art"))
    hook = os.path.join(game, "includes", "testhook.inc")
    with open(hook, "w", encoding="utf-8") as fh:
        fh.write(
            "const TH_START = 0\n"
            "const TH_KILL = 0\n"
            "const TH_GEMS = 0\n"
            "const TH_PHASE = 0\n"
            "const TH_LIVES = 5\n"
            "const TH_FAST = 0\n"
        )
    play = os.path.join(game, "includes", "play.inc")
    with open(play, encoding="utf-8") as fh:
        src = fh.read()
    src = patch(
        src,
        "    gm.draw()\n",
        "    if gm.frm >= 48 then gm.res = GE_TITLE\n    gm.draw()\n",
        "play cap",
    )
    with open(play, "w", encoding="utf-8") as fh:
        fh.write(src)
    main_bas = os.path.join(game, "main.bas")
    with open(main_bas, encoding="utf-8") as fh:
        src = fh.read()
    src = patch(
        src,
        "  gm.run()\nloop\n",
        "  gm.run()\n  if gm.frm >= 48 then exit do\nloop\n",
        "main cap",
    )
    with open(main_bas, "w", encoding="utf-8") as fh:
        fh.write(src)
    hs = os.path.join(tmp, "run.hs")
    with open(hs, "w", encoding="utf-8") as fh:
        fh.write('wait 800\nfeed MODE 20,16\nwait 400\nfeed RUN "D:/g/main.bas"\nwait 90000\nquit\n')
    env = os.environ.copy()
    env.update(
        {
            "SDL_VIDEODRIVER": "dummy",
            "SDL_AUDIODRIVER": "dummy",
            "MMB_DRIVE_ROOT": croot,
            "MMB_SDL_HARNESS": hs,
            "MMB_SDL_SERIAL": "1",
            "MMCORE_FRAME_PROFILE": "1",
        }
    )
    log = os.path.join(tmp, "serial.log")
    with open(log, "w", encoding="utf-8") as fh:
        proc = subprocess.run(
            ["timeout", "-s", "KILL", "100", MMCORE, "--drive", drv],
            stdin=subprocess.DEVNULL,
            stdout=fh,
            stderr=subprocess.STDOUT,
            env=env,
        )
    csv_path = None
    for dirpath, _, names in os.walk(tmp):
        for name in names:
            if name.endswith("frame-profile.csv"):
                csv_path = os.path.join(dirpath, name)
    print("rc", proc.returncode, "csv", csv_path or "missing", "tmp", tmp)
    if not csv_path:
        tail = open(log, encoding="utf-8", errors="replace").read()[-1500:]
        print(tail)
        return 1
    rows = []
    header = []
    with open(csv_path, encoding="utf-8") as fh:
        for line in fh:
            if line.startswith("#") or not line.strip():
                header.append(line.rstrip())
                continue
            if line.startswith("index,"):
                continue
            parts = line.split(",")
            if len(parts) < 8:
                continue
            rows.append(tuple(int(p) for p in parts[:8]))
    print("\n".join(header))
    print("parsed", len(rows))
    if not rows:
        return 1
    # Steady play frames are the short ones after the first paint.
    # Keep frames whose statement count is under the median of the
    # upper half, and report both the tail and the whole file.
    stmts = [r[6] for r in rows]
    interp = [r[2] for r in rows]
    draw = [r[3] for r in rows]
    present = [r[4] for r in rows]
    wait = [r[5] for r in rows]

    def brief(label, subset):
        if not subset:
            print(label, "empty")
            return
        def avg(i):
            return statistics.mean(r[i] for r in subset)
        print(
            f"{label} n={len(subset)} "
            f"stmts_avg={avg(6):.0f} stmts_med={statistics.median(r[6] for r in subset):.0f} "
            f"interp_us={avg(2):.0f} draw_us={avg(3):.0f} "
            f"present_us={avg(4):.0f} wait_us={avg(5):.0f} wall_us={avg(1):.0f}"
        )

    brief("all", rows)
    tail = rows[-80:] if len(rows) > 80 else rows
    brief("tail80", tail)
    small = [r for r in rows if r[6] < 800]
    brief("stmts<800", small)
    print(
        "sums stmts", sum(stmts),
        "interp", sum(interp),
        "draw", sum(draw),
        "present", sum(present),
        "wait", sum(wait),
    )
    # Keep the csv next to the script output for the write-up.
    out = "/tmp/jp-playtest1-frameprofile.csv"
    shutil.copy(csv_path, out)
    print("copied", out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
