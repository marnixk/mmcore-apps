"""Portable atlas: base tile ID + 8-neighbour tile-ID context (DOS draw rule)."""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field

import numpy as np
from PIL import Image

from .constants import ITEM_TILE_IDS, MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX
from .levels import Level
from .map_layout import context_key
from .native_vga import playfield_from_native

PLAYFIELD_W = MAP_W * TILE_PX
PLAYFIELD_H = MAP_H * TILE_PX

BACKDROP_RENDER_IDS = frozenset({0, 60, 0x3C}) | SPAWN_MARKER_TILE_IDS
GEM_TILE_IDS = frozenset({4, 5})


def _norm_tid(tid: int) -> int:
    if tid == 0:
        return 0x3C
    return tid


def render_context_key(tiles: list[int], row: int, col: int, tid: int) -> tuple:
    """Same key as JETPACK.EXE neighbour reads around (row,col) — see FORMATS.md RE."""
    return context_key(tiles, row, col, _norm_tid(tid))


def _neighbor_hamming(a: tuple, b: tuple) -> int:
    if a[0] != b[0]:
        return 999
    return sum(1 for x, y in zip(a[1:], b[1:]) if x != y)


@dataclass
class CompositeAtlas:
    base: dict[int, np.ndarray]
    context: dict[tuple, np.ndarray]  # context_key -> full RGB 24×24
    backdrop: np.ndarray
    counts: dict[int, int] = field(default_factory=dict)
    flagged_ids: list[int] = field(default_factory=list)

    def has_tile(self, tid: int) -> bool:
        if tid in BACKDROP_RENDER_IDS:
            return True
        return tid in self.base

    def lookup_context(self, tiles: list[int], row: int, col: int, tid: int) -> np.ndarray | None:
        key = render_context_key(tiles, row, col, tid)
        if key in self.context:
            return self.context[key]
        keys = [k for k in self.context if k[0] == key[0]]
        if not keys:
            return None
        best = min(keys, key=lambda k: _neighbor_hamming(key, k))
        if _neighbor_hamming(key, best) > 3:
            return None
        return self.context[best]

    def render_tile(self, tiles: list[int], row: int, col: int, tid: int) -> np.ndarray:
        if tid in BACKDROP_RENDER_IDS or tid == 0:
            return self.backdrop.copy()
        cell = self.lookup_context(tiles, row, col, tid)
        if cell is not None:
            return cell.copy()
        base = self.base.get(_norm_tid(tid))
        if base is not None:
            return base.copy()
        return self.backdrop.copy()

    def render_level(self, tile_ids: list[int]) -> Image.Image:
        out = np.zeros((PLAYFIELD_H, PLAYFIELD_W, 3), dtype=np.uint8)
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tile_ids[row * MAP_W + col]
                tile = self.render_tile(tile_ids, row, col, tid)
                y0, x0 = row * TILE_PX, col * TILE_PX
                out[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX] = tile
        return Image.fromarray(out, "RGB")


def _cell(field: np.ndarray, row: int, col: int) -> np.ndarray:
    y0, x0 = row * TILE_PX, col * TILE_PX
    return field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX]


def build_composite_atlas(
    captures: list[tuple[Level, Image.Image, np.ndarray | None]],
) -> CompositeAtlas:
    by_level: dict[int, tuple[Level, Image.Image, np.ndarray | None]] = {}
    for level, native, static in captures:
        if level.index not in by_level:
            by_level[level.index] = (level, native, static)

    buckets: dict[int, list[np.ndarray]] = defaultdict(list)
    context_buckets: dict[tuple, list[np.ndarray]] = defaultdict(list)
    backdrop_samples: list[np.ndarray] = []

    for level, native, static in by_level.values():
        field = np.array(playfield_from_native(native))
        tiles = level.tiles
        for row in range(MAP_H):
            for col in range(MAP_W):
                if static is not None and not static[row, col]:
                    continue
                tid = tiles[row * MAP_W + col]
                cell = _cell(field, row, col)
                if tid in BACKDROP_RENDER_IDS or tid == 0:
                    backdrop_samples.append(cell)
                    continue
                ntid = _norm_tid(tid)
                buckets[ntid].append(cell)
                key = render_context_key(tiles, row, col, tid)
                context_buckets[key].append(cell)

    backdrop = (
        np.median(np.stack(backdrop_samples, axis=0), axis=0).astype(np.uint8)
        if backdrop_samples
        else np.full((TILE_PX, TILE_PX, 3), (0x28, 0x20, 0x38), dtype=np.uint8)
    )

    base: dict[int, np.ndarray] = {}
    counts: dict[int, int] = {}
    flagged: list[int] = []
    for tid, samples in buckets.items():
        stack = np.stack(samples, axis=0).astype(np.float32)
        base[tid] = np.median(stack, axis=0).astype(np.uint8)
        counts[tid] = len(samples)
        if float(np.mean(np.var(stack, axis=0))) > 400.0:
            flagged.append(tid)

    context: dict[tuple, np.ndarray] = {}
    for key, samples in context_buckets.items():
        context[key] = np.median(np.stack(samples, axis=0), axis=0).astype(np.uint8)

    return CompositeAtlas(
        base=base,
        context=context,
        backdrop=backdrop,
        counts=counts,
        flagged_ids=sorted(flagged),
    )


def validation_eval_mask(level: Level, static_mask: np.ndarray) -> np.ndarray:
    """All map cells except backdrop, spawn markers, gems, and entity slots."""
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    tiles = level.tiles
    enemy_cells = {(e.x - 1, e.y - 1) for e in level.enemies}
    px, py = level.player_x - 1, level.player_y - 1
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tiles[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                continue
            if tid in GEM_TILE_IDS:
                continue
            if (col, row) in enemy_cells or (col, row) == (px, py):
                continue
            if not static_mask[row, col]:
                continue
            m[row, col] = True
    return m


def coverage_mask(tile_ids: list[int], atlas: CompositeAtlas) -> np.ndarray:
    """Cells where an exact context_key exists in the atlas."""
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                continue
            key = render_context_key(tile_ids, row, col, tid)
            if key in atlas.context:
                m[row, col] = True
    return m
