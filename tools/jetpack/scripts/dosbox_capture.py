#!/usr/bin/env python3
"""Capture in-game Jetpack playfields via DOSBox-X + Xvfb (not minimap / title)."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "_download" / "extracted" / "jetpak15"
OUT = ROOT / "reference"
CONF = ROOT / "_dosbox" / "capture_level0.conf"


def _run_dosbox(env: dict) -> subprocess.Popen:
    return subprocess.Popen(
        ["dosbox-x", "-conf", str(CONF), "-fastlaunch"],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def _focus_game(env: dict) -> str:
    wid = subprocess.check_output(
        ["xdotool", "search", "--name", "DOSBox"],
        env=env,
        text=True,
    ).strip().split("\n")[0]
    subprocess.run(["xdotool", "windowactivate", "--sync", wid], env=env, check=False)
    subprocess.run(
        ["xdotool", "mousemove", "--window", wid, "320", "240", "click", "1"],
        env=env,
        check=False,
    )
    return wid


def set_start_level(env: dict, level: int) -> None:
    """F3 on title menu adds 10 per press (0, 10, 20, …)."""
    presses = level // 10
    for _ in range(presses):
        subprocess.run(["xdotool", "key", "F3"], env=env, check=False)
        time.sleep(0.35)


def start_gameplay(env: dict) -> None:
    subprocess.run(["xdotool", "key", "s"], env=env, check=False)
    time.sleep(0.8)
    subprocess.run(["xdotool", "key", "s"], env=env, check=False)
    time.sleep(2.0)
    for key in ("Return", "space", "KP_Enter"):
        subprocess.run(["xdotool", "key", key], env=env, check=False)
        time.sleep(0.25)
    time.sleep(3.5)


def capture_play(out: Path, level: int, env: dict) -> Path:
    dest = out / f"dosbox_play_level{level:02d}.png"
    subprocess.run(["scrot", str(dest)], env=env, check=False)
    return dest


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=Path, default=OUT)
    ap.add_argument(
        "--levels",
        type=int,
        nargs="*",
        default=[0, 10, 20, 30, 40, 50, 60, 70, 80, 90],
        help="Campaign levels to capture (multiples of 10 via F3)",
    )
    ap.add_argument("--wait-title", type=float, default=12.0)
    args = ap.parse_args()
    out = args.out.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)
    if not (GAME / "JETPACK.EXE").is_file():
        print(f"Missing {GAME / 'JETPACK.EXE'}", file=sys.stderr)
        return 1

    env = {
        "SDL_VIDEODRIVER": "x11",
        "SDL_AUDIODRIVER": "dummy",
        "DISPLAY": ":199",
    }
    xvfb = subprocess.Popen(
        ["Xvfb", ":199", "-screen", "0", "640x480x24", "-ac"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(0.4)
    openbox = subprocess.Popen(["openbox"], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.4)

    db = _run_dosbox(env)
    time.sleep(args.wait_title)
    try:
        if not shutil.which("xdotool"):
            print("xdotool required for automation", file=sys.stderr)
            return 1
        _focus_game(env)
        for level in args.levels:
            if level > 0:
                db.terminate()
                try:
                    db.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    db.kill()
                time.sleep(0.5)
                db = _run_dosbox(env)
                time.sleep(args.wait_title)
                _focus_game(env)
            set_start_level(env, level)
            start_gameplay(env)
            path = capture_play(out, level, env)
            print(f"Wrote {path}")
    finally:
        db.terminate()
        try:
            db.wait(timeout=3)
        except subprocess.TimeoutExpired:
            db.kill()
        openbox.terminate()
        xvfb.terminate()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
