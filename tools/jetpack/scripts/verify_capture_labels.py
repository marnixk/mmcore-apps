#!/usr/bin/env python3
"""Match each native f0 capture to the best JETLEV level (structural + tile-ID decode)."""

from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.constants import MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX
from jetpack.levels import load_jetlev
from jetpack.native_vga import load_geometry, playfield_from_native

STRUCTURAL_IDS = frozenset({0, 60, 0x3C}) | SPAWN_MARKER_TILE_IDS


def level_structural_mask(tiles: list[int]) -> np.ndarray:
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tiles[row * MAP_W + col]
            if tid not in STRUCTURAL_IDS:
                m[row, col] = True
    return m


def capture_structural_mask(field: np.ndarray, bg_thresh: float = 18.0) -> np.ndarray:
    """True where cell is not backdrop-like."""
    samples = [
        field[0:24, 0:24],
        field[0:24, 24:48],
        field[360:384, 0:24],
    ]
    bg = np.median(np.concatenate([s.reshape(-1, 3) for s in samples], axis=0), axis=0)
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            y0, x0 = row * TILE_PX, col * TILE_PX
            cell = field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX].astype(np.float32)
            if float(np.mean(np.abs(cell - bg))) > bg_thresh:
                m[row, col] = True
    return m


def mask_iou(a: np.ndarray, b: np.ndarray) -> float:
    inter = float(np.sum(a & b))
    union = float(np.sum(a | b))
    return inter / union if union > 0 else 0.0


def build_prototypes(levels: dict[int, list[int]], fields: dict[int, np.ndarray]) -> dict[int, np.ndarray]:
    """Per tile-ID median patch from captures assumed correctly at first pass."""
    buckets: dict[int, list[np.ndarray]] = {}
    for idx, field in fields.items():
        tiles = levels[idx]
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tiles[row * MAP_W + col]
                if tid in STRUCTURAL_IDS:
                    continue
                y0, x0 = row * TILE_PX, col * TILE_PX
                buckets.setdefault(tid, []).append(field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX])
    proto: dict[int, np.ndarray] = {}
    for tid, patches in buckets.items():
        proto[tid] = np.median(np.stack(patches, axis=0), axis=0).astype(np.uint8)
    return proto


def decode_field(field: np.ndarray, prototypes: dict[int, np.ndarray]) -> list[int]:
    ids = sorted(prototypes.keys())
    stack = np.stack([prototypes[i].astype(np.float32) for i in ids], axis=0)
    grid = [0] * (MAP_W * MAP_H)
    for row in range(MAP_H):
        for col in range(MAP_W):
            y0, x0 = row * TILE_PX, col * TILE_PX
            cell = field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX].astype(np.float32)
            dists = np.mean((stack - cell) ** 2, axis=(1, 2, 3))
            grid[row * MAP_W + col] = ids[int(np.argmin(dists))]
    return grid


def grid_match_score(grid: list[int], tiles: list[int]) -> float:
    same = 0
    total = 0
    for i, (a, b) in enumerate(zip(grid, tiles)):
        if tiles[i] in STRUCTURAL_IDS and a in STRUCTURAL_IDS:
            same += 1
            total += 1
        elif tiles[i] not in STRUCTURAL_IDS and a not in STRUCTURAL_IDS:
            if a == b:
                same += 1
            total += 1
    return same / total if total else 0.0


def best_level_for_field(
    field: np.ndarray,
    all_levels: list,
    prototypes: dict[int, np.ndarray] | None,
) -> tuple[int, float, float]:
    cap_mask = capture_structural_mask(field)
    best_i = 0
    best_score = -1.0
    best_iou = 0.0
    for lv in all_levels:
        iou = mask_iou(cap_mask, level_structural_mask(lv.tiles))
        if iou > best_iou:
            best_iou = iou
            best_i = lv.index
            best_score = iou
    if prototypes:
        grid = decode_field(field, prototypes)
        refined = 0
        best_r = best_i
        best_rs = -1.0
        for lv in all_levels:
            s = grid_match_score(grid, lv.tiles)
            if s > best_rs:
                best_rs = s
                best_r = lv.index
        if best_rs > 0.35:
            return best_r, best_rs, best_iou
    return best_i, best_score, best_iou


def main() -> int:
    native = ROOT / "reference" / "native"
    geom = load_geometry()
    all_levels = load_jetlev(ROOT / "_download/extracted/jetpak15/JETLEV.DAT")
    by_idx = {lv.index: lv.tiles for lv in all_levels}

    fields: dict[int, np.ndarray] = {}
    paths: dict[int, Path] = {}
    for p in sorted(native.glob("level_*_f0.png")):
        idx = int(p.stem.split("_")[1])
        fields[idx] = np.array(playfield_from_native(Image.open(p).convert("RGB"), geom))
        paths[idx] = p

    prototypes = build_prototypes(by_idx, fields)
    results = []
    mis = 0
    for label in sorted(fields):
        field = fields[label]
        best, score, iou = best_level_for_field(field, all_levels, prototypes)
        ok = best == label
        if not ok:
            mis += 1
        results.append(
            {
                "file": paths[label].name,
                "label": label,
                "matched_level": best,
                "match_score": score,
                "structural_iou": iou,
                "mislabelled": not ok,
            }
        )

    out = ROOT / "reference" / "capture_label_audit.json"
    out.write_text(
        json.dumps(
            {"mislabelled_count": mis, "total": len(results), "entries": results},
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    print(f"Mislabelled: {mis} / {len(results)}")
    for r in results:
        if r["mislabelled"]:
            print(f"  {r['file']}: label {r['label']} -> best {r['matched_level']} (score={r['match_score']:.3f})")
    print(f"Wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
