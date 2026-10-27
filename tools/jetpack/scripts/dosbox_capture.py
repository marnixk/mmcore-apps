#!/usr/bin/env python3
"""Capture Jetpack DOS screenshots and VGA palette via DOSBox-X + Xvfb."""

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


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=Path, default=OUT)
    args = ap.parse_args()
    out = args.out.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)
    if not (GAME / "JETPACK.EXE").is_file():
        print(f"Missing {GAME / 'JETPACK.EXE'}", file=sys.stderr)
        return 1

    shot_dir = out / "_dosbox_shots"
    shot_dir.mkdir(parents=True, exist_ok=True)
    for old in shot_dir.glob("*.png"):
        old.unlink()

    conf_text = CONF.read_text(encoding="utf-8")
    conf_text += f"""
[dosbox]
captures={shot_dir}
"""
    tmp_conf = out / "_capture_run.conf"
    tmp_conf.write_text(conf_text, encoding="utf-8")

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
    time.sleep(0.5)
    openbox = subprocess.Popen(["openbox"], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.5)

    db = subprocess.Popen(
        ["dosbox-x", "-conf", str(tmp_conf), "-fastlaunch"],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    time.sleep(8)
    try:
        import shutil as sh

        if sh.which("xdotool"):
            wid = subprocess.check_output(
                ["xdotool", "search", "--name", "DOSBox"],
                env=env,
                text=True,
            ).strip().split("\n")[0]
            subprocess.run(["xdotool", "windowactivate", "--sync", wid], env=env, check=False)
            time.sleep(0.3)
            # Title: S = start, Enter through welcome, then in-game level 0
            subprocess.run(["xdotool", "key", "s"], env=env, check=False)
            time.sleep(2)
            subprocess.run(["scrot", "-u", str(out / "dosbox_title.png")], env=env, check=False)
            subprocess.run(["xdotool", "key", "Return"], env=env, check=False)
            time.sleep(3)
            subprocess.run(["scrot", "-u", str(out / "dosbox_level0_welcome.png")], env=env, check=False)
            subprocess.run(["xdotool", "key", "Return"], env=env, check=False)
            time.sleep(4)
            subprocess.run(["scrot", "-u", str(out / "dosbox_level0_gameplay.png")], env=env, check=False)
            subprocess.run(["xdotool", "key", "p"], env=env, check=False)
            time.sleep(0.5)
            subprocess.run(["scrot", "-u", str(out / "dosbox_level0_paused.png")], env=env, check=False)
    except Exception as exc:
        print("automation warning:", exc, file=sys.stderr)

    db.terminate()
    try:
        db.wait(timeout=3)
    except subprocess.TimeoutExpired:
        db.kill()
    openbox.terminate()
    xvfb.terminate()

    shots = sorted(shot_dir.glob("*.png"))
    for i, s in enumerate(shots[:5]):
        shutil.copy(s, out / f"dosbox_capture_{i:02d}.png")
    print(f"Wrote scrots to {out}; dosbox-x captures in {shot_dir} ({len(shots)} files)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
