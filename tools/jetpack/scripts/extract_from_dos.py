#!/usr/bin/env python3
"""Build gfx/frames from DOSBox reference PNGs (see jetpack/dos_art.py)."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.dos_art import export_dos_frames


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path)
    ap.add_argument(
        "--preview",
        type=Path,
        default=ROOT / "reference" / "level00_play2.png",
    )
    ap.add_argument(
        "--title",
        type=Path,
        default=ROOT / "reference" / "dosbox_title.png",
    )
    args = ap.parse_args()
    info = export_dos_frames(
        ROOT,
        args.game_dir.expanduser().resolve(),
        args.preview.expanduser().resolve(),
        args.title.expanduser().resolve() if args.title else None,
    )
    print(f"DOS frames: {len(info['tiles'])} tiles, source={info['source']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
