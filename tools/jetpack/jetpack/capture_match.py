"""Match native playfield captures to JETLEV level indices."""

from __future__ import annotations

import numpy as np

from .constants import ITEM_TILE_IDS, MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX

BACKDROP_IDS = frozenset({0, 60, 0x3C}) | SPAWN_MARKER_TILE_IDS
LADDER_IDS = frozenset({1})
GEM_IDS = frozenset({4, 5})
GREY_BLOCK_IDS = frozenset({0x46, 0x47, 0x48, 70, 71, 72, 73, 74, 75, 0x72, 0x75})
BRICK_LIKE = frozenset(range(0x18, 0x32)) | frozenset(range(0x40, 0x46)) | frozenset({40, 41, 42, 43, 97, 94, 114})


def jetlev_category(tid: int) -> int:
    if tid in BACKDROP_IDS:
        return 0
    if tid in LADDER_IDS:
        return 1
    if tid in GEM_IDS:
        return 2
    if tid in ITEM_TILE_IDS:
        return 3
    if tid in GREY_BLOCK_IDS:
        return 4
    if tid in BRICK_LIKE or (0x20 <= tid <= 0x7F and tid not in (0x3C,)):
        return 5
    return 6


def jetlev_category_grid(tiles: list[int]) -> np.ndarray:
    g = np.zeros((MAP_H, MAP_W), dtype=np.uint8)
    for row in range(MAP_H):
        for col in range(MAP_W):
            g[row, col] = jetlev_category(tiles[row * MAP_W + col])
    return g


def capture_category_grid(field: np.ndarray) -> np.ndarray:
    """Heuristic 7-class grid from RGB playfield (624×384)."""
    g = np.zeros((MAP_H, MAP_W), dtype=np.uint8)
    bg = np.median(field[0:48, 0:48].reshape(-1, 3), axis=0)
    for row in range(MAP_H):
        for col in range(MAP_W):
            y0, x0 = row * TILE_PX, col * TILE_PX
            cell = field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX].astype(np.float32)
            mean = cell.mean(axis=(0, 1))
            dist_bg = float(np.linalg.norm(mean - bg))
            r, gr, b = mean
            if dist_bg < 22.0:
                g[row, col] = 0
            elif gr > 150 and r > 120 and b < 100:
                g[row, col] = 2
            elif gr > 130 and abs(r - b) < 40 and gr > r:
                g[row, col] = 1
            elif r > 90 and r > gr and r > b:
                g[row, col] = 5
            elif 80 < r < 140 and 80 < gr < 140 and 80 < b < 140:
                g[row, col] = 4
            elif gr > 100 and b > 100:
                g[row, col] = 3
            else:
                g[row, col] = 6
    return g


def grid_match_score(cap: np.ndarray, jet: np.ndarray) -> float:
    """Fraction of cells with same category (ignore backdrop-backdrop)."""
    mask = (cap != 0) | (jet != 0)
    if not np.any(mask):
        return 0.0
    return float(np.mean(cap[mask] == jet[mask]))


def best_level_for_capture(field: np.ndarray, all_tiles: dict[int, list[int]]) -> tuple[int, float]:
    cap = capture_category_grid(field)
    best_i = 0
    best_s = -1.0
    for idx, tiles in all_tiles.items():
        jet = jetlev_category_grid(tiles)
        s = grid_match_score(cap, jet)
        if s > best_s:
            best_s = s
            best_i = idx
    return best_i, best_s
