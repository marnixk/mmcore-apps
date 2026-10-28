#!/usr/bin/env python3
"""Boot Jetpack (slot 0) and dump VGA DAC + planes via the resident hook."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.jetlev_swap import prepare_runtime_game_dir, write_slot0_level
from jetpack.native_vga import is_gameplay_frame

GAME_SRC = ROOT / "_download" / "extracted" / "jetpak15"
RUNTIME = ROOT / "_capture_runtime" / "vga_dump"
COM = Path("/tmp/vga_dump.com")


def grab(display: str, dest: Path) -> None:
    subprocess.run(
        [
            "python3",
            "-c",
            f"""
from mss import MSS
from PIL import Image
with MSS(display='{display}') as sct:
    mon=sct.monitors[1]
    shot=sct.grab({{'left': mon['left'], 'top': mon['top'], 'width': 640, 'height': 400}})
    im=Image.frombytes('RGB', shot.size, shot.bgra, 'raw', 'BGRX')
    im.save('{dest}')
""",
        ],
        check=True,
    )


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--level", type=int, default=0)
    ap.add_argument("--display", default=":240")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    if not COM.is_file():
        subprocess.check_call(
            ["nasm", "-f", "bin", "-o", str(COM), str(ROOT / "scripts" / "vga_dump.asm")]
        )
    if RUNTIME.is_dir():
        shutil.rmtree(RUNTIME)
    prepare_runtime_game_dir(GAME_SRC, RUNTIME)
    write_slot0_level(GAME_SRC / "JETLEV.DAT", RUNTIME / "JETLEV.DAT", args.level)
    shutil.copy(COM, RUNTIME / "DUMP.COM")
    for stale in ("GO.DMP", "VGA.BIN"):
        p = RUNTIME / stale
        if p.exists():
            p.unlink()

    conf = ROOT / "_dosbox" / "vga_dump.conf"
    conf.write_text(
        f"""[sdl]
fullscreen=false
windowposition=0,0
windowresolution=640x400
output=surface
showmenu=false
title=jetpackvga

[render]
aspect=false
scaler=none

[cpu]
cycles=max

[mixer]
nosound=true

[midi]
mpu401=none
mididevice=none

[dos]
memsize=16

[dosbox]
captures={ROOT / 'reference' / '_vga_shots'}

[autoexec]
@echo off
mount c {RUNTIME}
c:
dump.com
jetpack.exe
""",
        encoding="utf-8",
    )
    env = {
        "SDL_VIDEODRIVER": "x11",
        "SDL_AUDIODRIVER": "dummy",
        "DISPLAY": args.display,
        "PATH": "/usr/local/bin:/usr/bin:/bin",
    }
    subprocess.run(["pkill", "-f", f"Xvfb {args.display}"], check=False)
    time.sleep(0.2)
    xvfb = subprocess.Popen(
        ["Xvfb", args.display, "-screen", "0", "800x600x24", "-ac"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(0.3)
    openbox = subprocess.Popen(
        ["openbox"],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(0.25)
    db = subprocess.Popen(
        ["dosbox-x", "-conf", str(conf), "-fastlaunch"],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    out = args.out.expanduser().resolve()
    out.parent.mkdir(parents=True, exist_ok=True)
    shot = out.with_suffix(".png")
    try:
        time.sleep(8)
        from PIL import Image
        import importlib.util
        import numpy as np

        spec = importlib.util.spec_from_file_location(
            "slot0cap", ROOT / "scripts" / "dosbox_capture_slot0.py"
        )
        slot0cap = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(slot0cap)
        wid = slot0cap.wait_dosbox_window(env)
        native = np.array(
            Image.open(ROOT / "reference" / "native" / f"level_{args.level:02d}_f0.png").convert("RGB")
        )
        probe = out.parent / "_probe.png"

        def keys(key: str) -> None:
            subprocess.run(["xdotool", "windowactivate", "--sync", wid], env=env, check=False)
            subprocess.run(
                ["xdotool", "key", "--window", wid, "--clearmodifiers", key],
                env=env,
                check=False,
            )

        def playfield_match() -> float:
            grab(args.display, probe)
            im = np.array(Image.open(probe).convert("RGB"))
            if im.shape[0] < 80 or im.shape[1] < 640:
                return 0.0
            # Top and middle bands. The ready-card stays over the centre
            # after the top row of terrain is already drawn.
            best = 0.0
            for ox in (8, 9):
                got = im[:, ox : ox + 624].astype(np.int16)
                ref = native[:, 9:633].astype(np.int16)
                scores = []
                for y0, y1 in ((0, 80), (160, 240)):
                    d = np.abs(got[y0:y1] - ref[y0:y1]).max(axis=2)
                    scores.append(float((d == 0).mean()))
                best = max(best, min(scores))
            return best

        subprocess.run(
            ["xdotool", "windowactivate", "--sync", wid], env=env, check=False
        )
        subprocess.run(
            ["xdotool", "mousemove", "--window", wid, "320", "200", "click", "1"],
            env=env,
            check=False,
        )
        time.sleep(0.3)

        def purple_of(path: Path) -> float:
            im = np.array(Image.open(path).convert("RGB"))
            panel = im[150:250, 200:440].astype(np.int16)
            return float(
                np.mean((panel[:, :, 0] > 70) & (panel[:, :, 2] > 70) & (panel[:, :, 1] < 90))
            )

        # Credits swallow the first S. Wait until the menu is actually up,
        # press S once, then wait out the ready-card with no further keys.
        for i in range(30):
            playfield_match()
            if purple_of(probe) > 0.2:
                print("menu up", flush=True)
                break
            if i % 5 == 0:
                keys("s")
            time.sleep(0.4)
        keys("s")
        print("start sent", flush=True)
        entered = False
        dismissed = False
        for i in range(40):
            score = playfield_match()
            im = np.array(Image.open(probe).convert("RGB"))
            black = float(np.mean(im.max(axis=2) < 8))
            if i % 4 == 0:
                print(f"wait {i} match {score:.3f} black {black:.2f}", flush=True)
            if score > 0.85:
                entered = True
                break
            # Ready-card is letterboxed. One key dismisses it into the level.
            if black > 0.45 and not dismissed and i > 1:
                keys("space")
                dismissed = True
                print("dismiss ready", flush=True)
            time.sleep(0.4)
        if not entered:
            probe.replace(out.parent / f"level_{args.level:02d}_enter_failed.png")
            print("failed to reach gameplay", file=sys.stderr)
            return 1
        grab(args.display, shot)
        (RUNTIME / "GO.DMP").write_bytes(b"")
        deadline = time.time() + 25
        vga = RUNTIME / "VGA.BIN"
        while time.time() < deadline:
            if vga.is_file() and vga.stat().st_size >= 768 + 4 * 65536:
                break
            time.sleep(0.25)
        else:
            print(f"VGA.BIN missing or short ({vga.stat().st_size if vga.is_file() else 0})", file=sys.stderr)
            return 1
        mem = RUNTIME / "MEM.BIN"
        stable = -1
        while time.time() < deadline:
            if mem.is_file():
                sz = mem.stat().st_size
                if sz == 640 * 1024 and sz == stable:
                    break
                stable = sz
            time.sleep(0.25)
        else:
            print(f"MEM.BIN missing or short ({mem.stat().st_size if mem.is_file() else 0})", file=sys.stderr)
            return 1
        time.sleep(0.4)
        shutil.copy(vga, out)
        mem_out = out.with_name(out.stem + "_mem.bin")
        shutil.copy(mem, mem_out)
        print(f"wrote {out} ({out.stat().st_size} bytes) mem {mem_out} ({mem_out.stat().st_size}) shot {shot}")
        return 0
    finally:
        db.terminate()
        try:
            db.wait(timeout=3)
        except subprocess.TimeoutExpired:
            db.kill()
        openbox.terminate()
        xvfb.terminate()


if __name__ == "__main__":
    raise SystemExit(main())
