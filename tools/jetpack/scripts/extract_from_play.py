#!/usr/bin/env python3
"""Export gfx/frames from dosbox_play_level*.png in-game captures."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.play_extract import export_play_frames


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path)
    ap.add_argument("--reference", type=Path, default=ROOT / "reference")
    ap.add_argument("-o", "--out", type=Path, default=ROOT)
    args = ap.parse_args()
    ref = args.reference.expanduser().resolve()
    captures: dict[int, Path] = {}
    for p in sorted(ref.glob("dosbox_play_level*.png")):
        stem = p.stem.replace("dosbox_play_level", "")
        try:
            captures[int(stem)] = p
        except ValueError:
            continue
    if not captures:
        print(f"No dosbox_play_level*.png in {ref}", file=sys.stderr)
        return 1
    meta = export_play_frames(args.out, args.game_dir.expanduser().resolve(), captures)
    print(
        f"Play frames: {len(meta.get('tiles', []))} tiles, source={meta.get('source')}",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
