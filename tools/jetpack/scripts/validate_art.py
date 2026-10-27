#!/usr/bin/env python3
"""Compare decoded level 0 render to DOS reference minimap."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.dos_art import preview_subimage
from jetpack.frame_atlas import FrameAtlas
from jetpack.levels import load_jetlev

MAX_MSE = 12.0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path)
    ap.add_argument(
        "--reference",
        type=Path,
        default=ROOT / "reference" / "level00_play2.png",
    )
    args = ap.parse_args()
    ref_full = Image.open(args.reference).convert("RGB")
    ref = preview_subimage(ref_full).resize((26 * 24, 16 * 24), Image.Resampling.NEAREST)
    lv = load_jetlev(args.game_dir / "JETLEV.DAT")[0]
    atlas = FrameAtlas.load(ROOT / "gfx" / "frames")
    rend = atlas.render_level(lv.tiles, use_level0_cells=True).convert("RGB")
    mse = float(np.mean((np.array(rend) - np.array(ref)) ** 2))
    print(f"level_00 playfield MSE vs DOS minimap (scaled): {mse:.2f} (max {MAX_MSE})")
    out = ROOT / "reference" / "validate_level00_render.png"
    rend.save(out)
    return 0 if mse < MAX_MSE else 1


if __name__ == "__main__":
    raise SystemExit(main())
