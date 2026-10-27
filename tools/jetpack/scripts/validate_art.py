#!/usr/bin/env python3
"""Compare decoded level 0 render to DOS reference minimap (MSE)."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.graphics import TileAtlas, render_level_tiles
from jetpack.levels import load_jetlev


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path)
    ap.add_argument(
        "--reference",
        type=Path,
        default=ROOT / "reference" / "level00_preview_crop2.png",
    )
    args = ap.parse_args()
    ref = Image.open(args.reference).convert("RGB")
    lv = load_jetlev(args.game_dir / "JETLEV.DAT")[0]
    atlas = TileAtlas.open(args.game_dir)
    rend = render_level_tiles(lv.tiles, atlas).convert("RGB").resize(ref.size, Image.Resampling.NEAREST)
    mse = float(np.mean((np.array(rend) - np.array(ref)) ** 2))
    print(f"level_00 minimap MSE vs {args.reference.name}: {mse:.2f}")
    return 0 if mse < 120 else 1


if __name__ == "__main__":
    raise SystemExit(main())
