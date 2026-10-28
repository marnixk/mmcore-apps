#!/usr/bin/env python3
"""Capture all levels by swapping each into JETLEV slot 0 (no skip cheat)."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME_SRC = ROOT / "_download" / "extracted" / "jetpak15"
RUNTIME = ROOT / "_capture_runtime" / "jetpak15"
OUT = ROOT / "reference" / "native"
CAPTURES_DIR = ROOT / "reference" / "_dosbox_captures"
CONF_TEMPLATE = ROOT / "_dosbox" / "native_capture.conf"


def _env(display: str) -> dict[str, str]:
    return {
        "SDL_VIDEODRIVER": "x11",
        "SDL_AUDIODRIVER": "dummy",
        "DISPLAY": display,
        "PATH": "/usr/local/bin:/usr/bin:/bin",
    }


def write_conf(conf_path: Path, game_mount: Path, captures: Path) -> None:
    base = CONF_TEMPLATE.read_text(encoding="utf-8")
    lines = []
    for line in base.splitlines():
        if line.startswith("mount c "):
            lines.append(f"mount c {game_mount}")
        elif line.startswith("captures="):
            lines.append(f"captures={captures}")
        elif line.strip() == "fullscreen=false":
            lines.append("fullscreen=false")
            lines.append("windowposition=0,0")
            lines.append("windowsize=640,400")
        elif line.strip().startswith("windowposition=") or line.strip().startswith("windowsize="):
            continue
        else:
            lines.append(line)
    conf_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def grab_root(display: str, dest: Path, width: int = 640, height: int = 400) -> None:
    subprocess.run(
        [
            "python3",
            "-c",
            f"""
from mss import MSS
from PIL import Image
with MSS(display='{display}') as sct:
    mon=sct.monitors[1]
    shot=sct.grab({{'left': mon['left'], 'top': mon['top'], 'width': {width}, 'height': {height}}})
    im=Image.frombytes('RGB', shot.size, shot.bgra, 'raw', 'BGRX')
    im.save('{dest}')
""",
        ],
        check=True,
    )


def trigger_screenshot(env: dict, wid: str, captures_dir: Path, before: set[str]) -> Path | None:
    subprocess.run(
        ["xdotool", "key", "--window", wid, "ctrl+F5"],
        env=env,
        check=False,
    )
    for _ in range(30):
        time.sleep(0.2)
        after = {p.name for p in captures_dir.glob("*") if p.is_file()}
        new = after - before
        if new:
            name = sorted(new)[-1]
            return captures_dir / name
    return None


def wait_dosbox_window(env: dict, timeout: float = 20.0) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            out = subprocess.check_output(
                ["xdotool", "search", "--name", "DOSBox"],
                env=env,
                text=True,
                stderr=subprocess.DEVNULL,
            ).strip()
            if out:
                return out.split("\n")[0]
        except subprocess.CalledProcessError:
            pass
        time.sleep(0.3)
    raise RuntimeError("DOSBox window not found")


def _is_editor(native_rgb) -> bool:
    import numpy as np

    arr = np.array(native_rgb.convert("RGB"))
    if arr.shape[0] < 200:
        return False
    # Level editor: large blue/purple grid workspace.
    band = arr[60:340, 80:560]
    return float(band[:, :, 2].mean()) > 95.0 and float(band[:, :, 0].mean()) < 80.0


def enter_gameplay(env: dict, display: str, wid: str, grab_fn) -> bool:
    from jetpack.native_vga import is_gameplay_frame

    from PIL import Image

    def keys(key: str) -> None:
        subprocess.run(["xdotool", "windowactivate", "--sync", wid], env=env, check=False)
        subprocess.run(["xdotool", "key", "--clearmodifiers", key], env=env, check=False)

    subprocess.run(["xdotool", "windowactivate", "--sync", wid], env=env, check=False)
    subprocess.run(["xdotool", "mousemove", "--window", wid, "320", "200", "click", "1"], env=env, check=False)
    time.sleep(0.5)
    keys("s")
    time.sleep(1.2)
    keys("s")
    time.sleep(2.0)
    for key in ("Return", "space", "KP_Enter"):
        keys(key)
        time.sleep(0.35)
    time.sleep(1.5)
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
        probe = Path(tmp.name)
    for attempt in range(30):
        grab_fn(display, probe)
        im = Image.open(probe)
        if _is_editor(im):
            keys("Escape")
            time.sleep(0.5)
            keys("s")
            time.sleep(1.0)
            keys("s")
            time.sleep(1.5)
            keys("Return")
            time.sleep(1.0)
            continue
        if is_gameplay_frame(im):
            probe.unlink(missing_ok=True)
            return True
        keys("Return")
        time.sleep(0.5)
        keys("space")
        time.sleep(0.6)
    probe.replace(ROOT / "reference" / f"_slot0_enter_failed.png")
    return False


def capture_level(
    level: int,
    display: str,
    conf: Path,
    out_dir: Path,
    captures_dir: Path,
    frames: int,
    method: str,
) -> None:
    env = _env(display)
    subprocess.run(["pkill", "-f", f"Xvfb {display}"], check=False)
    time.sleep(0.2)
    xvfb = subprocess.Popen(
        ["Xvfb", display, "-screen", "0", "640x480x24", "-ac"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(0.35)
    openbox = subprocess.Popen(["openbox"], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.25)
    from jetpack.jetlev_swap import prepare_runtime_game_dir, write_slot0_level

    if RUNTIME.is_dir():
        shutil.rmtree(RUNTIME)
    prepare_runtime_game_dir(GAME_SRC, RUNTIME)
    write_slot0_level(GAME_SRC / "JETLEV.DAT", RUNTIME / "JETLEV.DAT", level)

    subprocess.run(["pkill", "-f", f"dosbox-x.*{conf.name}"], check=False)
    time.sleep(0.6)
    captures_dir.mkdir(parents=True, exist_ok=True)

    db = subprocess.Popen(
        ["dosbox-x", "-conf", str(conf), "-fastlaunch"],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        time.sleep(12.0)
        wid = wait_dosbox_window(env)
        grab_fn = grab_root if method == "root" else None
        if grab_fn is None:

            def grab_fn(d: str, dest: Path) -> None:  # noqa: F811
                before = {p.name for p in captures_dir.glob("*")}
                shot = trigger_screenshot(env, wid, captures_dir, before)
                if shot is None:
                    raise RuntimeError("DOSBox screenshot timed out")
                shutil.copy2(shot, dest)

        wid = wait_dosbox_window(env, timeout=5.0)
        if not enter_gameplay(env, display, wid, grab_root):
            raise RuntimeError(f"level {level}: failed to enter gameplay")

        for fi in range(frames):
            dest = out_dir / f"level_{level:02d}_f{fi}.png"
            if method == "dosbox":
                before = {p.name for p in captures_dir.glob("*")}
                shot = trigger_screenshot(env, wid, captures_dir, before)
                if shot is None:
                    grab_root(display, dest)
                else:
                    shutil.copy2(shot, dest)
            else:
                grab_root(display, dest)
            if fi + 1 < frames:
                time.sleep(0.45)
        print(f"level {level:02d} ok ({frames} frames, {method})")
    finally:
        db.terminate()
        try:
            db.wait(timeout=4)
        except subprocess.TimeoutExpired:
            db.kill()
        openbox.terminate()
        xvfb.terminate()


def main() -> int:
    sys.path.insert(0, str(ROOT))
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=Path, default=OUT)
    ap.add_argument("--display", default=":210")
    ap.add_argument("--levels", type=int, nargs="*", default=list(range(100)))
    ap.add_argument("--frames-per-level", type=int, default=3)
    ap.add_argument(
        "--method",
        choices=("dosbox", "root"),
        default="dosbox",
        help="dosbox=Ctrl+F5 to captures/; root=MSS top-left 640x400",
    )
    args = ap.parse_args()
    out = args.out.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)
    if not (GAME_SRC / "JETPACK.EXE").is_file():
        print(f"Missing {GAME_SRC / 'JETPACK.EXE'}", file=sys.stderr)
        return 1

    conf = ROOT / "_dosbox" / "slot0_capture.conf"
    write_conf(conf, RUNTIME, CAPTURES_DIR)

    for lv in args.levels:
        for attempt in range(3):
            try:
                capture_level(
                    lv,
                    args.display,
                    conf,
                    out,
                    CAPTURES_DIR,
                    args.frames_per_level,
                    args.method,
                )
                break
            except Exception as exc:
                print(f"level {lv:02d} attempt {attempt + 1} failed: {exc}", file=sys.stderr)
                if attempt == 2:
                    return 1
                time.sleep(2.0)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
