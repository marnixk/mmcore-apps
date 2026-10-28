"""Coarse level fingerprints for capture ↔ JETLEV matching."""

from __future__ import annotations

import numpy as np

from .constants import MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX

BACKDROP = frozenset({0, 60, 0x3C}) | SPAWN_MARKER_TILE_IDS


def jetlev_fingerprint(tiles: list[int]) -> np.ndarray:
    """Histogram + door/player coords (normalized)."""
    hist = np.zeros(128, dtype=np.float32)
    for tid in tiles:
        if tid < 128:
            hist[tid] += 1.0
    hist /= max(1.0, hist.sum())
    return hist


def capture_fingerprint(field: np.ndarray) -> np.ndarray:
    """Approximate tile-id histogram from RGB cells."""
    hist = np.zeros(128, dtype=np.float32)
    bg = np.median(field[0:48, 0:48].reshape(-1, 3), axis=0)
    for row in range(MAP_H):
        for col in range(MAP_W):
            y0, x0 = row * TILE_PX, col * TILE_PX
            cell = field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX].astype(np.float32)
            mean = cell.mean(axis=(0, 1))
            r, g, b = mean
            dist = float(np.linalg.norm(mean - bg))
            if dist < 20:
                tid = 0
            elif g > 130 and g > r + 25:
                tid = 4
            elif g > 120 and abs(r - b) < 35:
                tid = 1
            elif r > 100 and r > g:
                tid = 40
            elif 70 < r < 150 and 70 < g < 150:
                tid = 70
            else:
                tid = 60
            hist[tid] += 1.0
    hist /= max(1.0, hist.sum())
    return hist


def fingerprint_distance(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.sum((a - b) ** 2))
