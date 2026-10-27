#!/usr/bin/env python3
"""Export gfx/frames from native VGA captures (reference/native/)."""

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
    ap.add_argument(
        "--native",
        type=Path,
        default=ROOT / "reference" / "native",
        help="Directory with level_XX_fN.png captures",
    )
    ap.add_argument("-o", "--out", type=Path, default=ROOT)
    args = ap.parse_args()
    native = args.native.expanduser().resolve()
    if not any(native.glob("level_*_f*.png")):
        print(f"No level_*_f*.png in {native}", file=sys.stderr)
        return 1
    meta = export_play_frames(args.out.expanduser().resolve(), args.game_dir.expanduser().resolve(), native)
    print(
        f"Play frames: {len(meta.get('tiles', []))} tiles, source={meta.get('source')}, "
        f"flagged={len(meta.get('flagged_tile_ids', []))}",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
