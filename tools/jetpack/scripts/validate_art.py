#!/usr/bin/env python3
"""Validate composite atlas: coverage + MSE on all non-backdrop, non-sprite cells."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.atlas_median import masked_mse, temporal_static_mask
from jetpack.constants import MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX
from jetpack.frame_atlas import FrameAtlas
from jetpack.levels import load_jetlev
from jetpack.map_layout import default_layout
from jetpack.native_vga import load_geometry, playfield_from_native
from jetpack.tile_composite import BACKDROP_RENDER_IDS, coverage_mask

CHECK_LEVELS = (0, 5, 37, 99)
MAX_MSE = 2.0
MIN_COVERAGE = 0.99


def content_mask(tile_ids: list[int]) -> np.ndarray:
    m = np.ones((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                m[row, col] = False
    return m


def render_coverage(tile_ids: list[int], render: np.ndarray, backdrop: np.ndarray) -> np.ndarray:
    """True where rebuild drew a non-backdrop tile (not equal to flat backdrop)."""
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    bd = backdrop.astype(np.float32)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                continue
            y0, x0 = row * TILE_PX, col * TILE_PX
            cell = render[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX].astype(np.float32)
            if float(np.mean(np.abs(cell - bd))) > 6.0:
                m[row, col] = True
    return m


def side_by_side(cap: Image.Image, rend: Image.Image, label: str) -> Image.Image:
    h = cap.height + rend.height + 24
    out = Image.new("RGB", (max(cap.width, rend.width), h), (0x10, 0x10, 0x18))
    out.paste(cap, (0, 0))
    out.paste(rend, (0, cap.height + 24))
    ImageDraw.Draw(out).text((4, cap.height + 4), label, fill=(0xE0, 0xD0, 0xA0))
    return out


def eval_level(
    idx: int,
    lv,
    atlas: FrameAtlas,
    ref: Path,
    geom,
) -> dict:
    paths = [ref / "native" / f"level_{idx:02d}_f{n}.png" for n in range(3)]
    paths = [p for p in paths if p.is_file()]
    if not paths:
        return {"skip": True}
    fields = [np.array(playfield_from_native(Image.open(p).convert("RGB"), geom)) for p in paths]
    cap = np.median(np.stack(fields, axis=0), axis=0).astype(np.uint8) if len(fields) > 1 else fields[0]
    rend = np.array(atlas.render_level(lv.tiles, level_index=lv.index).convert("RGB"))
    content = content_mask(lv.tiles)
    sprites = ~temporal_static_mask(fields)
    eval_mask = content & ~sprites
    cov_atlas = coverage_mask(lv.tiles, atlas.composite)
    # Coverage: every non-backdrop map cell must have a base tile to draw.
    content_eval = content & ~sprites
    coverage = float(np.sum(content_eval & cov_atlas) / np.sum(content_eval)) if np.any(content_eval) else 0.0
    mse = masked_mse(cap, rend, eval_mask)
    return {
        "content_cells": int(content.sum()),
        "eval_cells": int(eval_mask.sum()),
        "coverage_pct": coverage * 100.0,
        "atlas_has_base_pct": coverage * 100.0,
        "mse": mse,
        "pass": coverage >= MIN_COVERAGE and mse <= MAX_MSE,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path)
    ap.add_argument("--reference", type=Path, default=ROOT / "reference")
    ap.add_argument("--levels", type=int, nargs="*", default=None)
    ap.add_argument("--max-mse", type=float, default=MAX_MSE)
    ap.add_argument("--all-levels", action="store_true")
    args = ap.parse_args()
    ref = args.reference.expanduser().resolve()
    levels = {lv.index: lv for lv in load_jetlev(args.game_dir / "JETLEV.DAT")}
    atlas = FrameAtlas.load(ROOT / "gfx" / "frames")
    if atlas.composite is None:
        print("Missing composite_atlas.pkl — run extract_from_play.py", file=sys.stderr)
        return 1
    geom = load_geometry()
    layout = default_layout()
    check = list(range(100)) if args.all_levels else (args.levels or list(CHECK_LEVELS))
    stats_out: dict = {
        "map_layout": layout.to_dict(),
        "max_mse": args.max_mse,
        "min_coverage_pct": MIN_COVERAGE * 100,
        "levels": {},
    }
    failed = False
    mses: list[float] = []
    covs: list[float] = []
    for idx in check:
        if idx not in levels:
            continue
        row = eval_level(idx, levels[idx], atlas, ref, geom)
        if row.get("skip"):
            print(f"level_{idx:02d}: skip (no capture)")
            continue
        stats_out["levels"][f"level_{idx:02d}"] = row
        mses.append(row["mse"])
        covs.append(row["coverage_pct"])
        ok = row["pass"]
        failed = failed or not ok
        print(
            f"level_{idx:02d}  coverage={row['coverage_pct']:.1f}%  "
            f"MSE={row['mse']:.2f}  eval_cells={row['eval_cells']}  "
            f"{'OK' if ok else 'FAIL'}"
        )
        if idx in CHECK_LEVELS or not args.all_levels:
            paths = [ref / "native" / f"level_{idx:02d}_f{n}.png" for n in range(3)]
            paths = [p for p in paths if p.is_file()]
            fields = [
                np.array(playfield_from_native(Image.open(p).convert("RGB"), geom)) for p in paths
            ]
            cap_arr = (
                np.median(np.stack(fields, axis=0), axis=0).astype(np.uint8)
                if len(fields) > 1
                else fields[0]
            )
            cap_img = Image.fromarray(cap_arr, "RGB")
            rend_img = Image.fromarray(
                np.array(
                    atlas.render_level(levels[idx].tiles, level_index=idx).convert("RGB"),
                ),
                "RGB",
            )
            side = side_by_side(
                cap_img,
                rend_img,
                f"L{idx:02d} top=native bottom=rebuild cov={row['coverage_pct']:.1f}% MSE={row['mse']:.1f}",
            )
            side.save(ref / f"validate_level{idx:02d}_side_by_side.png")

    if mses:
        stats_out["mean_over_checked"] = {
            "coverage_pct": float(np.mean(covs)),
            "mse": float(np.mean(mses)),
            "level_count": len(mses),
        }
        print(
            f"Mean over {len(mses)} levels: coverage={np.mean(covs):.1f}%  MSE={np.mean(mses):.2f}"
        )
    (ref / "validate_stats.json").write_text(json.dumps(stats_out, indent=2) + "\n", encoding="utf-8")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
