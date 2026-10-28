#!/usr/bin/env python3
"""Search JETLEV map layout; write reference/map_layout.json and layout_search.json."""

from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.atlas_median import (
    build_context_median_atlas,
    collect_context_samples,
    masked_mse,
    render_level_from_context_atlas,
    validation_mask,
)
from jetpack.constants import SPAWN_MARKER_TILE_IDS
from jetpack.levels import parse_level_blob
from jetpack.constants import LEVEL_HEADER_SKIP, LEVEL_SIZE
from jetpack.map_layout import (
    MapLayout,
    Order,
    iter_layout_candidates,
    load_level_record,
    save_default_layout,
    within_id_variance_score,
)
from jetpack.native_vga import load_geometry, playfield_from_native

SKIP = SPAWN_MARKER_TILE_IDS | frozenset({0})
CHECK_LEVELS = (0, 5, 37, 99)
LOCKED = MapLayout("off00|row_major", 0, Order.ROW_MAJOR)


def _levels_for_layout(layout: MapLayout) -> dict[int, object]:
    data = (ROOT / "_download/extracted/jetpak15/JETLEV.DAT").read_bytes()
    body = data[LEVEL_HEADER_SKIP:]
    out = {}
    for i in range(len(body) // LEVEL_SIZE):
        blob = body[i * LEVEL_SIZE : (i + 1) * LEVEL_SIZE]
        lv = parse_level_blob(blob, i)
        lv.tiles = layout.read_grid(blob)
        out[i] = lv
    return out


def _context_confirm(layout: MapLayout) -> dict[str, dict]:
    levels = _levels_for_layout(layout)
    native = ROOT / "reference" / "native"
    geom = load_geometry()
    pairs = []
    for p in sorted(native.glob("level_*_f0.png")):
        idx = int(p.stem.split("_")[1])
        if idx in levels:
            pairs.append((levels[idx], Image.open(p).convert("RGB")))
    stats = build_context_median_atlas(collect_context_samples(pairs))
    out: dict[str, dict] = {}
    for li in CHECK_LEVELS:
        lv = levels[li]
        cap = np.array(playfield_from_native(Image.open(native / f"level_{li:02d}_f0.png"), geom))
        rend = np.array(render_level_from_context_atlas(lv.tiles, stats))
        mask = validation_mask(lv, stats)
        out[f"level_{li:02d}"] = {
            "static_mse": masked_mse(cap, rend, mask),
            "agreed_cells": int(mask.sum()),
        }
    return out


def main() -> int:
    dat = (ROOT / "_download/extracted/jetpak15/JETLEV.DAT").read_bytes()
    scores_l0: list[dict] = []
    for layout in iter_layout_candidates(32):
        blob = load_level_record(dat, 0)
        field = np.array(
            playfield_from_native(Image.open(ROOT / "reference/native/level_00_f0.png").convert("RGB"))
        )
        var, n_ids, n_samples = within_id_variance_score(layout, blob, field, SKIP)
        if var == float("inf"):
            continue
        scores_l0.append(
            {"name": layout.name, "mean_within_id_var": var, "tile_ids": n_ids, "samples": n_samples}
        )
    scores_l0.sort(key=lambda x: x["mean_within_id_var"])
    top5 = scores_l0[:5]

    within_confirm: dict[str, dict] = {}
    for li in CHECK_LEVELS:
        blob = load_level_record(dat, li)
        field = np.array(
            playfield_from_native(
                Image.open(ROOT / f"reference/native/level_{li:02d}_f0.png").convert("RGB")
            )
        )
        var, n_ids, n_samples = within_id_variance_score(LOCKED, blob, field, SKIP)
        within_confirm[f"level_{li:02d}"] = {
            "mean_within_id_var": var,
            "tile_ids": n_ids,
            "samples": n_samples,
        }

    context_confirm = _context_confirm(LOCKED)
    save_default_layout(LOCKED)

    out = ROOT / "reference" / "layout_search.json"
    out.write_text(
        json.dumps(
            {
                "level_00_top5_within_id_variance": top5,
                "locked_layout": LOCKED.to_dict(),
                "lock_rationale": (
                    "416-byte map at record offset 0 (door at 0x1A0=416). "
                    "Top within-ID score off26|row_major shifts the grid and misaligns "
                    "the level record; off00|row_major matches structure and yields "
                    "~0 static MSE with 8-neighbour context atlas."
                ),
                "locked_within_id_variance_on_levels": within_confirm,
                "locked_context_static_mse_on_levels": context_confirm,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    print("Level 0 — top 5 layouts by mean within-ID pixel variance (skip tile 0):")
    for row in top5:
        print(
            f"  {row['name']:40s}  var={row['mean_within_id_var']:.4f}  "
            f"ids={row['tile_ids']}  samples={row['samples']}"
        )
    print(f"\nLocked layout: {LOCKED.name} (byte offset {LOCKED.byte_offset}, {LOCKED.order.value})")
    print("Within-ID variance on check levels (locked layout):")
    for k, v in within_confirm.items():
        print(f"  {k}: var={v['mean_within_id_var']:.2f}")
    print("Context-atlas static MSE on check levels (locked layout):")
    for k, v in context_confirm.items():
        print(f"  {k}: mse={v['static_mse']:.4f}  cells={v['agreed_cells']}")
    print(f"Wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
