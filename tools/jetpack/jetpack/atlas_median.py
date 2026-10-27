"""Statistical tile atlas from native playfield captures."""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field

import numpy as np
from PIL import Image

from .constants import MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX
from .levels import Level
from .native_vga import playfield_from_native

PLAYFIELD_H = MAP_H * TILE_PX
PLAYFIELD_W = MAP_W * TILE_PX


@dataclass
class TileAtlasStats:
    tiles: dict[int, np.ndarray]
    counts: dict[int, int]
    disagreement: dict[int, float]  # mean per-pixel variance across samples
    flagged_ids: list[int] = field(default_factory=list)


def _cell(field: np.ndarray, col: int, row: int) -> np.ndarray:
    x0, y0 = col * TILE_PX, row * TILE_PX
    return field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX]


def collect_samples(
    captures: list[tuple[Level, Image.Image]],
) -> dict[int, list[np.ndarray]]:
    buckets: dict[int, list[np.ndarray]] = defaultdict(list)
    from .native_vga import is_gameplay_frame

    for level, native in captures:
        if not is_gameplay_frame(native):
            continue
        field = np.array(playfield_from_native(native))
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = level.tiles[row * MAP_W + col]
                if tid in SPAWN_MARKER_TILE_IDS:
                    continue
                if tid == 0:
                    buckets[0x3C].append(_cell(field, col, row))
                    buckets[60].append(_cell(field, col, row))
                    continue
                buckets[tid].append(_cell(field, col, row))
    return buckets


def _medoid(samples: list[np.ndarray]) -> tuple[np.ndarray, float]:
    """Pick the sample closest to the per-pixel median (robust to misaligned outliers)."""
    stack = np.stack(samples, axis=0).astype(np.float32)
    if len(samples) == 1:
        return samples[0], 0.0
    med = np.median(stack, axis=0)
    dists = [float(np.mean((stack[i] - med) ** 2)) for i in range(len(samples))]
    best = int(np.argmin(dists))
    return samples[best], float(np.mean(dists))


def build_median_atlas(
    buckets: dict[int, list[np.ndarray]],
    min_count: int = 1,
    disagree_threshold: float = 800.0,
) -> TileAtlasStats:
    tiles: dict[int, np.ndarray] = {}
    counts: dict[int, int] = {}
    disagreement: dict[int, float] = {}
    flagged: list[int] = []
    for tid, samples in sorted(buckets.items()):
        if len(samples) < min_count:
            continue
        # Drop cells contaminated by sprites (high local contrast vs medoid candidate).
        filtered = [s for s in samples if float(np.std(s)) < 90.0 or tid in (0x3C, 60)]
        if not filtered:
            filtered = samples
        if tid in (0x3C, 60):
            tile = min(filtered, key=lambda s: float(np.std(s)))
            spread = float(np.std(tile))
        else:
            cand = [s for s in filtered if float(np.mean(s)) < 72.0]
            if not cand:
                cand = filtered
            tile = max(cand, key=lambda s: float(np.std(s)))
            spread = float(np.std(tile))
        tiles[tid] = tile
        counts[tid] = len(filtered)
        disagreement[tid] = spread
        if spread > disagree_threshold and tid not in (0x3C, 60):
            flagged.append(tid)
    if 0x3C in tiles:
        tiles[60] = tiles[0x3C]
    return TileAtlasStats(tiles=tiles, counts=counts, disagreement=disagreement, flagged_ids=flagged)


def render_level_from_atlas(tile_ids: list[int], stats: TileAtlasStats) -> Image.Image:
    from .constants import SPAWN_MARKER_TILE_IDS

    backdrop = stats.tiles.get(0x3C)
    if backdrop is None:
        backdrop = stats.tiles.get(60)
    h, w = PLAYFIELD_H, PLAYFIELD_W
    out = np.zeros((h, w, 3), dtype=np.uint8)
    if backdrop is not None:
        for row in range(MAP_H):
            for col in range(MAP_W):
                y0, x0 = row * TILE_PX, col * TILE_PX
                out[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX] = backdrop
    else:
        out[:] = (0x28, 0x20, 0x38)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if tid == 0 or tid in SPAWN_MARKER_TILE_IDS:
                continue
            spr = stats.tiles.get(tid)
            if spr is None and tid == 60:
                spr = stats.tiles.get(0x3C)
            if spr is None:
                continue
            y0, x0 = row * TILE_PX, col * TILE_PX
            out[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX] = spr
    return Image.fromarray(out, "RGB")


def static_cell_mask(level: Level, sprite_cells: set[tuple[int, int]]) -> np.ndarray:
    """True where rebuild should match capture (no player/enemy/sprites)."""
    mask = np.ones((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = level.tiles[row * MAP_W + col]
            if tid in SPAWN_MARKER_TILE_IDS:
                mask[row, col] = False
    for col, row in sprite_cells:
        if 0 <= col < MAP_W and 0 <= row < MAP_H:
            mask[row, col] = False
    px = level.player_x - 1
    py = level.player_y - 1
    if 0 <= px < MAP_W and 0 <= py < MAP_H:
        mask[py, px] = False
    for e in level.enemies:
        ex, ey = e.x - 1, e.y - 1
        if 0 <= ex < MAP_W and 0 <= ey < MAP_H:
            mask[ey, ex] = False
    return mask


def masked_mse(capture: np.ndarray, render: np.ndarray, mask: np.ndarray) -> float:
    m = mask.repeat(TILE_PX, axis=0).repeat(TILE_PX, axis=1)
    diff = (capture.astype(np.float32) - render.astype(np.float32)) ** 2
    if diff.ndim == 3:
        diff = np.mean(diff, axis=2)
    if not np.any(m):
        return float("nan")
    return float(np.mean(diff[m]))
