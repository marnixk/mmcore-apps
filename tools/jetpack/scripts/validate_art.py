#!/usr/bin/env python3
"""Compare rebuilt playfields to in-game DOS captures (not minimap)."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.frame_atlas import FrameAtlas
from jetpack.levels import load_jetlev
from jetpack.playfield import normalize_playfield
from jetpack.constants import MAP_H, MAP_W, TILE_PX

MAX_MSE = 22.0
DEFAULT_LEVELS = (0, 10, 20, 50)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path)
    ap.add_argument(
        "--reference",
        type=Path,
        default=ROOT / "reference",
    )
    ap.add_argument("--levels", type=int, nargs="*", default=list(DEFAULT_LEVELS))
    args = ap.parse_args()
    ref = args.reference.expanduser().resolve()
    levels = {lv.index: lv for lv in load_jetlev(args.game_dir / "JETLEV.DAT")}
    atlas = FrameAtlas.load(ROOT / "gfx" / "frames")
    failed = False
    for idx in args.levels:
        cap = ref / f"dosbox_play_level{idx:02d}.png"
        if not cap.is_file():
            print(f"level_{idx:02d}: skip (no {cap.name})")
            continue
        lv = levels[idx]
        ref_field = normalize_playfield(Image.open(cap))
        rend = atlas.render_level(lv.tiles).convert("RGB")
        r = np.array(rend)
        g = np.array(ref_field)
        mask = np.ones((MAP_H, MAP_W), dtype=bool)
        mask[lv.player_y - 1, lv.player_x - 1] = False
        for e in lv.enemies:
            if e.type_id:
                mask[e.y - 1, e.x - 1] = False
        errs: list[float] = []
        for row in range(MAP_H):
            for col in range(MAP_W):
                if not mask[row, col]:
                    continue
                y0, x0 = row * TILE_PX, col * TILE_PX
                tile_r = r[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX]
                tile_g = g[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX]
                errs.append(float(np.mean((tile_r - tile_g) ** 2)))
        mse = float(np.mean(errs)) if errs else 999.0
        ok = mse < MAX_MSE
        failed = failed or not ok
        print(f"level_{idx:02d} playfield MSE vs DOS play capture: {mse:.2f} (max {MAX_MSE})")
        out = ref / f"validate_level{idx:02d}_render.png"
        rend.save(out)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
