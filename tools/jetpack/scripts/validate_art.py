#!/usr/bin/env python3
"""Leave-one-level-out validation: DAT tiles + edge rule (no per-level capture atlas)."""

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
from jetpack.constants import MAP_H, MAP_W
from jetpack.levels import load_jetlev
from jetpack.map_layout import default_layout
from jetpack.native_vga import load_geometry, playfield_from_native
from jetpack.tile_composite import (
    DatCompositor,
    coverage_mask,
    pick_anim_phase,
    validation_eval_mask,
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


def eval_level(
    idx: int,
    lv,
    levels,
    game_dir: Path,
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
    phase = pick_anim_phase(lv, ref / "native", geom, game_dir, exclude_level=idx)
    comp = DatCompositor.open(
        game_dir,
        ref / "native",
        levels,
        exclude_level=idx,
        anim_phase=phase,
    )
    rend = np.array(comp.render_level(lv.tiles).convert("RGB"))
    cov = coverage_mask(lv.tiles, comp)
    if np.any(eval_mask):
        coverage = float(np.sum(eval_mask & cov) / np.sum(eval_mask))
    else:
        coverage = 1.0
    mse = float(masked_mse(cap, rend, eval_mask)) if np.any(eval_mask) else None
    ok = coverage >= MIN_COVERAGE and (mse is None or mse <= max_mse)
    return {
        "eval_cells": int(eval_mask.sum()),
        "dat_coverage_pct": float(coverage * 100.0),
        "anim_phase": phase,
        "mse": mse,
        "pass": ok,
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
        "validation": "loo_dat_compositor_f0",
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
        row = eval_level(idx, lv, levels, game_dir, ref, geom, args.max_mse)
        if row.get("skip"):
            print(f"level_{idx:02d}: skip (no capture)")
            continue
        stats_out["levels"][f"level_{idx:02d}"] = row
        if row["mse"] is not None:
            mses.append((idx, row["mse"]))
        covs.append(row["dat_coverage_pct"])
        ok = row["pass"]
        failed = failed or not ok
        mse_s = "n/a" if row["mse"] is None else f"{row['mse']:.2f}"
        print(
            f"level_{idx:02d}  coverage={row['dat_coverage_pct']:.1f}%  "
            f"MSE={mse_s}  phase={row['anim_phase']}  eval_cells={row['eval_cells']}  "
            f"{'OK' if ok else 'FAIL'}"
        )
        if idx in side_levels:
            cap_arr = capture_f0(idx, ref, geom)
            assert cap_arr is not None
            phase = row["anim_phase"]
            comp = DatCompositor.open(
                game_dir, ref / "native", levels, exclude_level=idx, anim_phase=phase
            )
            cap_img = Image.fromarray(cap_arr, "RGB")
            rend_img = Image.fromarray(
                np.array(comp.render_level(lv.tiles).convert("RGB")),
                "RGB",
            )
            side = side_by_side(
                cap_img,
                rend_img,
                f"L{idx:02d} LOO DAT cov={row['dat_coverage_pct']:.1f}% MSE={mse_s}",
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
