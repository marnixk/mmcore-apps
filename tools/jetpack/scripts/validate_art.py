#!/usr/bin/env python3
"""Validate median atlas rebuild vs native captures (structural cells)."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.constants import ITEM_TILE_IDS, MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX
from jetpack.frame_atlas import FrameAtlas
from jetpack.levels import load_jetlev
from jetpack.native_vga import load_geometry, playfield_from_native

MAX_MSE = 120.0
CHECK_LEVELS = (0, 5, 37, 99)


def _native_frame(ref: Path, idx: int) -> Path | None:
    native = ref / "native" / f"level_{idx:02d}_f0.png"
    return native if native.is_file() else None


def structural_mask(level) -> np.ndarray:
    mask = np.ones((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = level.tiles[row * MAP_W + col]
            if tid in (0, 60, 0x3C) or tid in SPAWN_MARKER_TILE_IDS or tid in ITEM_TILE_IDS:
                mask[row, col] = False
    mask[level.player_y - 1, level.player_x - 1] = False
    for e in level.enemies:
        mask[e.y - 1, e.x - 1] = False
    return mask


def masked_mse(cap: np.ndarray, rend: np.ndarray, mask: np.ndarray) -> float:
    from jetpack.atlas_median import masked_mse as _mse

    return _mse(cap, rend, mask)


def side_by_side(cap: Image.Image, rend: Image.Image, label: str) -> Image.Image:
    h = cap.height + rend.height + 24
    out = Image.new("RGB", (max(cap.width, rend.width), h), (0x10, 0x10, 0x18))
    out.paste(cap, (0, 0))
    out.paste(rend, (0, cap.height + 24))
    ImageDraw.Draw(out).text((4, cap.height + 4), label, fill=(0xE0, 0xD0, 0xA0))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path)
    ap.add_argument("--reference", type=Path, default=ROOT / "reference")
    ap.add_argument("--levels", type=int, nargs="*", default=list(CHECK_LEVELS))
    ap.add_argument("--max-mse", type=float, default=MAX_MSE)
    args = ap.parse_args()
    ref = args.reference.expanduser().resolve()
    levels = {lv.index: lv for lv in load_jetlev(args.game_dir / "JETLEV.DAT")}
    atlas = FrameAtlas.load(ROOT / "gfx" / "frames")
    geom = load_geometry()
    stats_out: dict[str, dict] = {}
    failed = False
    for idx in args.levels:
        cap_path = _native_frame(ref, idx)
        if cap_path is None:
            print(f"level_{idx:02d}: skip (no capture)")
            continue
        lv = levels[idx]
        im = Image.open(cap_path).convert("RGB")
        ref_field = np.array(playfield_from_native(im, geom))
        rend = np.array(atlas.render_level(lv.tiles).convert("RGB"))
        mask = structural_mask(lv)
        mse = masked_mse(ref_field, rend, mask)
        ok = mse <= args.max_mse
        failed = failed or not ok
        stats_out[f"level_{idx:02d}"] = {"structural_mse": mse, "pass": ok, "cells": int(mask.sum())}
        print(
            f"level_{idx:02d} structural MSE: {mse:.2f} (max {args.max_mse}) "
            f"{'OK' if ok else 'FAIL'} [{mask.sum()} cells]"
        )
        cap_img = Image.fromarray(ref_field, "RGB")
        rend_img = Image.fromarray(rend, "RGB")
        side = side_by_side(
            cap_img,
            rend_img,
            f"level {idx:02d}  top=native  bottom=rebuild  structural_MSE={mse:.1f}",
        )
        side.save(ref / f"validate_level{idx:02d}_side_by_side.png")
        rend_img.save(ref / f"validate_level{idx:02d}_render.png")
    (ref / "validate_stats.json").write_text(json.dumps(stats_out, indent=2) + "\n", encoding="utf-8")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
