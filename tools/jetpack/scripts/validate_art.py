#!/usr/bin/env python3
"""Validate the mode-13h index sheet against native captures.

The dumped DAC and the 12×12 sheet do not depend on the held-out level, so
leave-one-level-out MSE equals a full-set comparison. Exact index match is
reported for levels that have a JPVG dump.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.atlas_median import ANIMATED_TILE_IDS, masked_mse, temporal_static_mask
from jetpack.constants import MAP_H, MAP_W
from jetpack.levels import load_jetlev
from jetpack.map_layout import default_layout
from jetpack.native_vga import load_geometry, playfield_from_native
from jetpack.tile_composite import validation_eval_mask
from jetpack.vga_index import (
    SHEET_TILES,
    framebuffer_playfield,
    render_indices,
    render_level_rgb,
)

CHECK_LEVELS = (0, 5, 7, 37, 59, 99)
MAX_MSE = 12.0
MIN_COVERAGE = 0.99


def side_by_side(cap: Image.Image, rend: Image.Image, label: str) -> Image.Image:
    h = cap.height + rend.height + 24
    out = Image.new("RGB", (max(cap.width, rend.width), h), (0x10, 0x10, 0x18))
    out.paste(cap, (0, 0))
    out.paste(rend, (0, cap.height + 24))
    ImageDraw.Draw(out).text((4, cap.height + 4), label, fill=(0xE0, 0xD0, 0xA0))
    return out


def capture_f0(idx: int, ref: Path, geom) -> np.ndarray | None:
    p = ref / "native" / f"level_{idx:02d}_f0.png"
    if not p.is_file():
        return None
    return np.array(playfield_from_native(Image.open(p).convert("RGB"), geom))


def _covered(tile_ids: list[int]) -> np.ndarray:
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if 0 <= tid < SHEET_TILES:
                m[row, col] = True
    return m


def eval_level(
    idx: int,
    lv,
    ref: Path,
    geom,
    max_mse: float,
) -> dict:
    cap = capture_f0(idx, ref, geom)
    if cap is None:
        return {"skip": True}
    paths = [ref / "native" / f"level_{idx:02d}_f{n}.png" for n in range(3)]
    paths = [p for p in paths if p.is_file()]
    fields = [np.array(playfield_from_native(Image.open(p).convert("RGB"), geom)) for p in paths]
    static = temporal_static_mask(fields)
    eval_mask = validation_eval_mask(lv, static)
    # One sheet frame cannot match a conveyor/teleport phase, and the door
    # sprite is drawn on top of whatever map cell it covers.
    animated = ANIMATED_TILE_IDS | set(range(0x4A, 0x50)) | {6, 7, 8}
    door_c, door_r = lv.door_x - 1, lv.door_y - 1
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = lv.tiles[row * MAP_W + col]
            if tid in animated:
                eval_mask[row, col] = False
            if door_c <= col < door_c + 2 and door_r <= row < door_r + 2:
                eval_mask[row, col] = False
    rend = np.array(render_level_rgb(lv.tiles))
    cov = _covered(lv.tiles)
    if np.any(eval_mask):
        coverage = float(np.sum(eval_mask & cov) / np.sum(eval_mask))
    else:
        coverage = 1.0
    mse = float(masked_mse(cap, rend, eval_mask)) if np.any(eval_mask) else None
    dump = ref / "vga_dump" / f"level_{idx:02d}.bin"
    exact = None
    if dump.is_file():
        got = render_indices(lv.tiles)
        truth = framebuffer_playfield(dump)
        exact = float(np.mean(got == truth) * 100.0)
    ok = coverage >= MIN_COVERAGE and (mse is None or mse <= max_mse)
    return {
        "eval_cells": int(eval_mask.sum()),
        "dat_coverage_pct": float(coverage * 100.0),
        "anim_phase": 0,
        "mse": mse,
        "exact_index_pct": exact,
        "pass": ok,
        "render": rend,
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
    game_dir = args.game_dir.expanduser().resolve()
    levels = load_jetlev(game_dir / "JETLEV.DAT")
    geom = load_geometry()
    layout = default_layout()
    check = list(range(100)) if args.all_levels else (args.levels or list(CHECK_LEVELS))
    stats_out: dict = {
        "map_layout": layout.to_dict(),
        "validation": "vga_index_sheet_dac",
        "dac": "reference/vga_dump/dac6.bin",
        "sheet": "reference/vga_dump/sheet_12x12.u8",
        "note": (
            "DAC and sheet are fixed (not fit per level), so holding a level out does not change its MSE. "
            "MSE is static terrain only: backdrop, gems, enemies, the player cell, the door sprite, "
            "and animated tube/teleport ids are excluded. Exact index percent is the full playfield, "
            "including those overlays."
        ),
        "max_mse": args.max_mse,
        "min_dat_coverage_pct": MIN_COVERAGE * 100,
        "levels": {},
    }
    failed = False
    mses: list[tuple[int, float]] = []
    covs: list[float] = []
    side_levels = set(CHECK_LEVELS) | {59}
    for idx in check:
        lv = next((l for l in levels if l.index == idx), None)
        if lv is None:
            continue
        row = eval_level(idx, lv, ref, geom, args.max_mse)
        if row.get("skip"):
            print(f"level_{idx:02d}: skip (no capture)")
            continue
        rend = row.pop("render")
        stats_out["levels"][f"level_{idx:02d}"] = row
        if row["mse"] is not None:
            mses.append((idx, row["mse"]))
        covs.append(row["dat_coverage_pct"])
        ok = row["pass"]
        failed = failed or not ok
        mse_s = "n/a" if row["mse"] is None else f"{row['mse']:.2f}"
        exact_s = "" if row["exact_index_pct"] is None else f"  index={row['exact_index_pct']:.2f}%"
        print(
            f"level_{idx:02d}  coverage={row['dat_coverage_pct']:.1f}%  "
            f"MSE={mse_s}  eval_cells={row['eval_cells']}{exact_s}  "
            f"{'OK' if ok else 'FAIL'}"
        )
        if idx in side_levels:
            cap_arr = capture_f0(idx, ref, geom)
            assert cap_arr is not None
            cap_img = Image.fromarray(cap_arr, "RGB")
            rend_img = Image.fromarray(rend, "RGB")
            side = side_by_side(
                cap_img,
                rend_img,
                f"L{idx:02d} sheet cov={row['dat_coverage_pct']:.1f}% MSE={mse_s}",
            )
            side.save(ref / f"validate_level{idx:02d}_side_by_side.png")

    if mses:
        mses.sort(key=lambda x: x[1], reverse=True)
        stats_out["worst_10_mse"] = [{"level": i, "mse": float(m)} for i, m in mses[:10]]
        stats_out["mean_over_levels"] = {
            "dat_coverage_pct": float(np.mean(covs)),
            "mse": float(np.mean([m for _, m in mses])),
            "level_count": len(mses),
        }
        print(
            f"Mean coverage {stats_out['mean_over_levels']['dat_coverage_pct']:.1f}%  "
            f"MSE {stats_out['mean_over_levels']['mse']:.2f} ({len(mses)} levels)"
        )
        print("Worst 10:", stats_out["worst_10_mse"][:10])

    (ref / "validate_stats.json").write_text(json.dumps(stats_out, indent=2) + "\n", encoding="utf-8")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
