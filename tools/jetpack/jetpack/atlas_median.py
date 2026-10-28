"""Statistical tile atlas from native playfield captures (per-ID and 8-neighbour context)."""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field

import numpy as np
from PIL import Image

from .constants import ITEM_TILE_IDS, MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX
from .levels import Level
from .map_layout import context_key
from .native_vga import playfield_from_native

PLAYFIELD_H = MAP_H * TILE_PX
PLAYFIELD_W = MAP_W * TILE_PX

# Cross-level per-key mean pixel variance above this → exclude from strict validation mask.
CROSS_LEVEL_DISAGREE_MAX = 50.0

# Tile IDs that are animated or stateful in DOS captures (decimal JETLEV bytes).
ANIMATED_TILE_IDS = frozenset(
    {
        0x09,
        0x0A,
        0x0B,
        0x0C,
        0x0D,
        0x0E,
        0x46,
        0x47,
        0x48,
        0x72,
        0x75,
    }
)


@dataclass
class TileAtlasStats:
    tiles: dict[int, np.ndarray]
    counts: dict[int, int]
    disagreement: dict[int, float]
    flagged_ids: list[int] = field(default_factory=list)


@dataclass
class ContextAtlasStats:
    """Atlas keyed by (tile_id, 8 neighbours)."""

    tiles: dict[tuple, np.ndarray]
    counts: dict[tuple, int]
    disagreement: dict[tuple, float]
    flagged_keys: list[tuple] = field(default_factory=list)
    tid_fallback: dict[int, np.ndarray] = field(default_factory=dict)
    tid_disagreement: dict[int, float] = field(default_factory=dict)
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


def collect_context_samples(
    captures: list[tuple[Level, Image.Image]],
) -> dict[tuple, list[np.ndarray]]:
    buckets: dict[tuple, list[np.ndarray]] = defaultdict(list)
    from .native_vga import is_gameplay_frame

    for level, native in captures:
        if not is_gameplay_frame(native):
            continue
        field = np.array(playfield_from_native(native))
        tiles = level.tiles
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tiles[row * MAP_W + col]
                if tid in SPAWN_MARKER_TILE_IDS:
                    continue
                if tid == 0:
                    key = context_key(tiles, row, col, 0x3C)
                    buckets[key].append(_cell(field, col, row))
                    continue
                key = context_key(tiles, row, col, tid)
                buckets[key].append(_cell(field, col, row))
    return buckets


def _per_pixel_median(samples: list[np.ndarray]) -> tuple[np.ndarray, float]:
    stack = np.stack(samples, axis=0).astype(np.float32)
    med = np.median(stack, axis=0).astype(np.uint8)
    var = float(np.mean(np.var(stack, axis=0)))
    return med, var


def build_context_median_atlas(
    buckets: dict[tuple, list[np.ndarray]],
    min_count: int = 1,
    disagree_threshold: float = CROSS_LEVEL_DISAGREE_MAX,
) -> ContextAtlasStats:
    tiles: dict[tuple, np.ndarray] = {}
    counts: dict[tuple, int] = {}
    disagreement: dict[tuple, float] = {}
    flagged_keys: list[tuple] = []
    for key, samples in buckets.items():
        if len(samples) < min_count:
            continue
        tile, spread = _per_pixel_median(samples)
        tiles[key] = tile
        counts[key] = len(samples)
        disagreement[key] = spread
        if spread > disagree_threshold:
            flagged_keys.append(key)

    tid_buckets: dict[int, list[np.ndarray]] = defaultdict(list)
    for key, tile in tiles.items():
        tid_buckets[key[0]].append(tile)

    tid_fallback: dict[int, np.ndarray] = {}
    tid_disagreement: dict[int, float] = {}
    flagged_ids: list[int] = []
    for tid, samples in tid_buckets.items():
        if len(samples) == 1:
            tid_fallback[tid] = samples[0]
            tid_disagreement[tid] = 0.0
            continue
        stack = np.stack(samples, axis=0).astype(np.float32)
        tid_disagreement[tid] = float(np.mean(np.var(stack, axis=0)))
        tid_fallback[tid] = np.median(stack, axis=0).astype(np.uint8)
        if tid_disagreement[tid] > disagree_threshold:
            flagged_ids.append(tid)

    return ContextAtlasStats(
        tiles=tiles,
        counts=counts,
        disagreement=disagreement,
        flagged_keys=flagged_keys,
        tid_fallback=tid_fallback,
        tid_disagreement=tid_disagreement,
        flagged_ids=sorted(flagged_ids),
    )


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
        tile, spread = _per_pixel_median(samples)
        tiles[tid] = tile
        counts[tid] = len(samples)
        disagreement[tid] = spread
        if spread > disagree_threshold:
            flagged.append(tid)
    if 0x3C in tiles:
        tiles[60] = tiles[0x3C]
    return TileAtlasStats(tiles=tiles, counts=counts, disagreement=disagreement, flagged_ids=flagged)


def lookup_context_tile(
    stats: ContextAtlasStats,
    tiles: list[int],
    row: int,
    col: int,
    tid: int,
) -> np.ndarray | None:
    if tid == 0:
        tid = 0x3C
    key = context_key(tiles, row, col, tid)
    if key in stats.tiles:
        return stats.tiles[key]
    return stats.tid_fallback.get(tid)


def render_level_from_context_atlas(tile_ids: list[int], stats: ContextAtlasStats) -> Image.Image:
    backdrop = stats.tid_fallback.get(0x3C)
    if backdrop is None:
        backdrop = stats.tiles.get(context_key(tile_ids, 0, 0, 0x3C))
    out = np.zeros((PLAYFIELD_H, PLAYFIELD_W, 3), dtype=np.uint8)
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
            if tid in SPAWN_MARKER_TILE_IDS:
                continue
            if tid == 0:
                continue
            spr = lookup_context_tile(stats, tile_ids, row, col, tid)
            if spr is None:
                continue
            y0, x0 = row * TILE_PX, col * TILE_PX
            out[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX] = spr
    return Image.fromarray(out, "RGB")


def render_level_from_atlas(tile_ids: list[int], stats: TileAtlasStats) -> Image.Image:
    backdrop = stats.tiles.get(0x3C)
    if backdrop is None:
        backdrop = stats.tiles.get(60)
    out = np.zeros((PLAYFIELD_H, PLAYFIELD_W, 3), dtype=np.uint8)
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


def validation_mask(
    level: Level,
    ctx_stats: ContextAtlasStats,
    *,
    exclude_items: bool = True,
) -> np.ndarray:
    """Cells where context atlas should match capture (agreed keys, no entities)."""
    mask = np.ones((MAP_H, MAP_W), dtype=bool)
    tiles = level.tiles
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tiles[row * MAP_W + col]
            if tid in (0, 60, 0x3C) or tid in SPAWN_MARKER_TILE_IDS:
                mask[row, col] = False
                continue
            if exclude_items and tid in ITEM_TILE_IDS:
                mask[row, col] = False
                continue
            use_tid = 0x3C if tid == 0 else tid
            key = context_key(tiles, row, col, use_tid)
            if key not in ctx_stats.tiles:
                mask[row, col] = False
                continue
            if ctx_stats.disagreement.get(key, 999.0) > CROSS_LEVEL_DISAGREE_MAX:
                mask[row, col] = False
                continue
            if use_tid in ANIMATED_TILE_IDS:
                mask[row, col] = False
    px, py = level.player_x - 1, level.player_y - 1
    if 0 <= px < MAP_W and 0 <= py < MAP_H:
        mask[py, px] = False
    for e in level.enemies:
        ex, ey = e.x - 1, e.y - 1
        if 0 <= ex < MAP_W and 0 <= ey < MAP_H:
            mask[ey, ex] = False
    return mask


def static_cell_mask(level: Level, sprite_cells: set[tuple[int, int]]) -> np.ndarray:
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


def temporal_static_mask(frame_fields: list[np.ndarray], channel_delta: int = 14) -> np.ndarray:
    """False where any of the native frames disagree (sprites / animation)."""
    if not frame_fields:
        return np.ones((MAP_H, MAP_W), dtype=bool)
    stack = np.stack(frame_fields, axis=0).astype(np.int16)
    span = np.max(np.max(np.abs(stack - stack[0:1]), axis=0), axis=-1)
    cell = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            y0, x0 = row * TILE_PX, col * TILE_PX
            cell[row, col] = bool(np.max(span[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX]) > channel_delta)
    return ~cell


def masked_mse(capture: np.ndarray, render: np.ndarray, mask: np.ndarray) -> float:
    m = mask.repeat(TILE_PX, axis=0).repeat(TILE_PX, axis=1)
    diff = (capture.astype(np.float32) - render.astype(np.float32)) ** 2
    if diff.ndim == 3:
        diff = np.mean(diff, axis=2)
    if not np.any(m):
        return float("nan")
    return float(np.mean(diff[m]))
