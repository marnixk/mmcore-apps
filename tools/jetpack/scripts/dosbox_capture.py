#!/usr/bin/env python3
"""Capture native 640×400 VGA frames for all campaign levels via DOSBox-X."""

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "_download" / "extracted" / "jetpak15"
OUT = ROOT / "reference" / "native"
CONF = ROOT / "_dosbox" / "native_capture.conf"


def _env(display: str) -> dict[str, str]:
    return {
        "SDL_VIDEODRIVER": "x11",
        "SDL_AUDIODRIVER": "dummy",
        "DISPLAY": display,
        "PATH": "/usr/local/bin:/usr/bin:/bin",
    }


def grab_window(display: str, wid: str, dest: Path) -> None:
    subprocess.run(
        [
            "python3",
            "-c",
            f"""
import subprocess
from mss import MSS
from PIL import Image
WID='{wid}'
geo=subprocess.check_output(['xdotool','getwindowgeometry','--shell',WID],env={{'DISPLAY':'{display}'}}).decode()
vals={{k:int(v) for k,v in (line.split('=',1) for line in geo.strip().split(chr(10)) if '=' in line)}}
with MSS(display='{display}') as sct:
    shot=sct.grab({{'left':vals['X'],'top':vals['Y'],'width':vals['WIDTH'],'height':vals['HEIGHT']}})
    im=Image.frombytes('RGB', shot.size, shot.bgra, 'raw', 'BGRX')
    im.save('{dest}')
""",
        ],
        check=True,
    )


def _frame_ok(path: Path) -> bool:
    from jetpack.native_vga import is_gameplay_frame

    from PIL import Image

    return is_gameplay_frame(Image.open(path))


def enter_first_level(env: dict, display: str, wid: str) -> bool:
    subprocess.run(["xdotool", "key", "s"], env=env, check=False)
    time.sleep(1.0)
    subprocess.run(["xdotool", "key", "s"], env=env, check=False)
    time.sleep(2.0)
    for key in ("Return", "space", "KP_Enter"):
        subprocess.run(["xdotool", "key", key], env=env, check=False)
        time.sleep(0.4)
    time.sleep(2.0)
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
        probe = Path(tmp.name)
    for _ in range(30):
        grab_window(display, wid, probe)
        if _frame_ok(probe):
            probe.unlink(missing_ok=True)
            return True
        subprocess.run(["xdotool", "key", "Return"], env=env, check=False)
        time.sleep(0.5)
        subprocess.run(["xdotool", "key", "space"], env=env, check=False)
        time.sleep(0.8)
    dest = ROOT / "reference" / "_enter_gameplay_failed.png"
    probe.replace(dest)
    return False


def skip_level(env: dict) -> None:
    subprocess.run(["xdotool", "key", "Tab"], env=env, check=False)
    time.sleep(0.12)
    subprocess.run(["xdotool", "key", "2"], env=env, check=False)
    time.sleep(0.12)
    subprocess.run(["xdotool", "key", "s"], env=env, check=False)
    time.sleep(1.5)
    subprocess.run(["xdotool", "key", "apostrophe"], env=env, check=False)
    time.sleep(2.5)


def set_title_start_level(env: dict, level: int) -> None:
    """F3 on title adds 10 per press (MANUAL / in-game UI)."""
    for _ in range(max(0, level // 10)):
        subprocess.run(["xdotool", "key", "F3"], env=env, check=False)
        time.sleep(0.35)


def restart_session(env: dict, display: str, start_level: int) -> str:
    """Restart DOSBox and enter campaign at ``start_level``."""
    subprocess.run(["pkill", "-f", "dosbox-x.*native_capture"], check=False)
    time.sleep(0.8)
    subprocess.Popen(
        ["dosbox-x", "-conf", str(CONF), "-fastlaunch"],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(14.0)
    wid = subprocess.check_output(
        ["xdotool", "search", "--name", "DOSBox"],
        env=env,
        text=True,
    ).strip().split("\n")[0]
    subprocess.run(["xdotool", "windowactivate", "--sync", wid], env=env, check=False)
    set_title_start_level(env, start_level)
    if not enter_first_level(env, display, wid):
        raise RuntimeError(f"could not enter gameplay at level {start_level}")
    for _ in range(start_level % 10):
        skip_level(env)
        wait_level_ready(env, display, wid)
    return wid


def wait_level_ready(env: dict, display: str, wid: str) -> bool:
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
        probe = Path(tmp.name)
    for _ in range(20):
        grab_window(display, wid, probe)
        if _frame_ok(probe):
            probe.unlink(missing_ok=True)
            return True
        for key in ("Return", "space"):
            subprocess.run(["xdotool", "key", key], env=env, check=False)
            time.sleep(0.35)
        time.sleep(0.8)
    probe.unlink(missing_ok=True)
    return False


def main() -> int:
    sys.path.insert(0, str(ROOT))
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=Path, default=OUT)
    ap.add_argument("--display", default=":199")
    ap.add_argument("--wait-title", type=float, default=14.0)
    ap.add_argument("--levels", type=int, nargs="*", default=list(range(100)))
    ap.add_argument("--frames-per-level", type=int, default=3)
    ap.add_argument(
        "--boot-level",
        type=int,
        default=0,
        help="Campaign level to start from on title (F3 + skip remainder)",
    )
    args = ap.parse_args()
    out = args.out.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)
    if not (GAME / "JETPACK.EXE").is_file():
        print(f"Missing {GAME / 'JETPACK.EXE'}", file=sys.stderr)
        return 1

    env = _env(args.display)
    subprocess.run(["pkill", "-f", f"Xvfb {args.display}"], check=False)
    time.sleep(0.3)
    xvfb = subprocess.Popen(
        ["Xvfb", args.display, "-screen", "0", "800x600x24", "-ac"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(0.4)
    openbox = subprocess.Popen(["openbox"], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.3)

    db = subprocess.Popen(
        ["dosbox-x", "-conf", str(CONF), "-fastlaunch"],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(args.wait_title)
    try:
        wid = subprocess.check_output(
            ["xdotool", "search", "--name", "DOSBox"],
            env=env,
            text=True,
        ).strip().split("\n")[0]
        subprocess.run(["xdotool", "windowactivate", "--sync", wid], env=env, check=False)
        subprocess.run(
            ["xdotool", "mousemove", "--window", wid, "320", "200", "click", "1"],
            env=env,
            check=False,
        )
        set_title_start_level(env, args.boot_level)
        if not enter_first_level(env, args.display, wid):
            fail = ROOT / "reference" / "_enter_gameplay_failed.png"
            grab_window(args.display, wid, fail)
            print(f"Failed to enter gameplay (saved {fail})", file=sys.stderr)
            return 1
        for _ in range(args.boot_level % 10):
            skip_level(env)
            wait_level_ready(env, args.display, wid)

        min_level = min(args.levels) if args.levels else 0
        max_level = max(args.levels) if args.levels else 99
        for lv in range(min_level, max_level + 1):
            if lv not in args.levels and args.levels != list(range(100)):
                if lv < max_level:
                    skip_level(env)
                    wait_level_ready(env, args.display, wid)
                continue
            if not wait_level_ready(env, args.display, wid):
                skip_level(env)
                if not wait_level_ready(env, args.display, wid):
                    print(f"level {lv:02d}: not in gameplay, skip", file=sys.stderr)
                    continue
            for fi in range(args.frames_per_level):
                dest = out / f"level_{lv:02d}_f{fi}.png"
                grab_window(args.display, wid, dest)
                if fi + 1 < args.frames_per_level:
                    time.sleep(0.45)
            print(f"level {lv:02d} captured ({args.frames_per_level} frames)")
            if lv < max_level:
                skip_level(env)
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
