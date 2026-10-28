#!/usr/bin/env python3
"""Validate context-median atlas rebuild vs native captures (static agreed cells)."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.atlas_median import masked_mse, temporal_static_mask, validation_mask
from jetpack.frame_atlas import FrameAtlas
from jetpack.levels import load_jetlev
from jetpack.map_layout import default_layout
from jetpack.native_vga import load_geometry, playfield_from_native

MAX_MSE = 1.0
CHECK_LEVELS = (0, 5, 37, 99)


def _native_frames(ref: Path, idx: int) -> list[Path]:
    paths = [ref / "native" / f"level_{idx:02d}_f{n}.png" for n in range(3)]
    return [p for p in paths if p.is_file()]


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
    if atlas.context_stats is None:
        print("ERROR: missing reference/context_atlas.pkl — run extract_from_play.py", file=sys.stderr)
        return 1
    geom = load_geometry()
    layout = default_layout()
    stats_out: dict[str, dict] = {
        "map_layout": layout.to_dict(),
        "max_mse": args.max_mse,
        "levels": {},
    }
    failed = False
    for idx in args.levels:
        cap_paths = _native_frames(ref, idx)
        if not cap_paths:
            print(f"level_{idx:02d}: skip (no capture)")
            continue
        lv = levels[idx]
        fields = [np.array(playfield_from_native(Image.open(p).convert("RGB"), geom)) for p in cap_paths]
        ref_field = fields[0]
        rend = np.array(atlas.render_level(lv.tiles).convert("RGB"))
        mask = validation_mask(lv, atlas.context_stats) & temporal_static_mask(fields)
        mse = masked_mse(ref_field, rend, mask)
        ok = mse <= args.max_mse
        failed = failed or not ok
        stats_out["levels"][f"level_{idx:02d}"] = {
            "static_mse": mse,
            "pass": ok,
            "cells": int(mask.sum()),
        }
        print(
            f"level_{idx:02d} static MSE: {mse:.4f} (max {args.max_mse}) "
            f"{'OK' if ok else 'FAIL'} [{mask.sum()} cells]"
        )
        cap_img = Image.fromarray(ref_field, "RGB")
        rend_img = Image.fromarray(rend, "RGB")
        side = side_by_side(
            cap_img,
            rend_img,
            f"level {idx:02d}  top=native  bottom=context rebuild  static_MSE={mse:.4f}",
        )
        side.save(ref / f"validate_level{idx:02d}_side_by_side.png")
        rend_img.save(ref / f"validate_level{idx:02d}_render.png")
    (ref / "validate_stats.json").write_text(json.dumps(stats_out, indent=2) + "\n", encoding="utf-8")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
