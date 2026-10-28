#!/usr/bin/env python3
"""Verify each level_XX_f0.png matches JETLEV index XX (render-MSE best match)."""

from __future__ import annotations

import json
import pickle
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.atlas_median import masked_mse
from jetpack.constants import MAP_H, MAP_W
from jetpack.levels import load_jetlev
from jetpack.native_vga import load_geometry, playfield_from_native
from jetpack.tile_composite import BACKDROP_RENDER_IDS


def content_mask(tiles: list[int]) -> np.ndarray:
    m = np.ones((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tiles[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                m[row, col] = False
    return m


def main() -> int:
    pkl = ROOT / "reference" / "composite_atlas.pkl"
    if not pkl.is_file():
        print("Run extract_from_play.py first to build composite_atlas.pkl", file=sys.stderr)
        return 1
    atlas = pickle.loads(pkl.read_bytes())
    geom = load_geometry()
    levels = load_jetlev(ROOT / "_download/extracted/jetpak15/JETLEV.DAT")
    native = ROOT / "reference" / "native"

    mis = 0
    rows = []
    for label in range(100):
        cap_path = native / f"level_{label:02d}_f0.png"
        if not cap_path.is_file():
            mis += 1
            rows.append({"label": label, "ok": False, "error": "missing file"})
            continue
        cap = np.array(playfield_from_native(Image.open(cap_path).convert("RGB"), geom))
        best_i = 0
        best_mse = float("inf")
        self_mse = float("nan")
        for lv in levels:
            rend = np.array(atlas.render_level(lv.tiles))
            mse = masked_mse(cap, rend, content_mask(lv.tiles))
            if lv.index == label:
                self_mse = mse
            if mse < best_mse:
                best_mse = mse
                best_i = lv.index
        ok = best_i == label
        if not ok:
            mis += 1
        rows.append(
            {
                "file": cap_path.name,
                "label": label,
                "matched": best_i,
                "self_mse": self_mse,
                "best_mse": best_mse,
                "ok": ok,
            }
        )

    out = ROOT / "reference" / "capture_fingerprint_verify.json"
    out.write_text(
        json.dumps(
            {
                "method": "composite_render_mse_best_match",
                "mislabelled": mis,
                "total": len(rows),
                "entries": rows,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    print(f"Label verify: {len(rows) - mis}/{len(rows)} correct ({mis} mislabelled)")
    return 0 if mis == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
