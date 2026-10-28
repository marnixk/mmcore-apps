"""Base per-tile-ID atlas + neighbour-context full-tile variants."""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field

import numpy as np
from PIL import Image

from .constants import ITEM_TILE_IDS, MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX
from .levels import Level
from .native_vga import playfield_from_native

PLAYFIELD_W = MAP_W * TILE_PX
PLAYFIELD_H = MAP_H * TILE_PX

BACKDROP_RENDER_IDS = frozenset({0, 60, 0x3C}) | SPAWN_MARKER_TILE_IDS
NON_SOLID = BACKDROP_RENDER_IDS | frozenset(ITEM_TILE_IDS.keys()) | frozenset({1, 2, 3, 4, 5, 6, 7, 8})


def is_solid(tid: int) -> bool:
    return tid not in NON_SOLID and tid != 0


def neighbor_solid_mask8(tiles: list[int], row: int, col: int) -> int:
    """8-bit N,E,S,W,NE,NW,SE,SW solid neighbours."""
    offs = [(-1, 0), (0, 1), (1, 0), (0, -1), (-1, -1), (-1, 1), (1, -1), (1, 1)]
    mask = 0
    for i, (dr, dc) in enumerate(offs):
        r, c = row + dr, col + dc
        if 0 <= r < MAP_H and 0 <= c < MAP_W and is_solid(tiles[r * MAP_W + c]):
            mask |= 1 << i
    return mask


@dataclass
class CompositeAtlas:
    base: dict[int, np.ndarray]
    variants: dict[tuple[int, int], np.ndarray]  # (tid, 8-bit mask) -> RGB
    backdrop: np.ndarray
    counts: dict[int, int] = field(default_factory=dict)
    flagged_ids: list[int] = field(default_factory=list)
    # Observed cells from each level's capture (for exact rebuild validation).
    level_cells: dict[int, dict[tuple[int, int, int], np.ndarray]] = field(default_factory=dict)

    def has_tile(self, tid: int) -> bool:
        if tid in BACKDROP_RENDER_IDS:
            return True
        return tid in self.base

    def render_tile(
        self,
        tiles: list[int],
        row: int,
        col: int,
        tid: int,
        level_index: int | None = None,
    ) -> np.ndarray:
        if tid in BACKDROP_RENDER_IDS or tid == 0:
            return self.backdrop.copy()
        if level_index is not None:
            obs = self.level_cells.get(level_index, {}).get((row, col, tid))
            if obs is not None:
                return obs.copy()
        mask = neighbor_solid_mask8(tiles, row, col)
        key = (tid, mask)
        if key in self.variants:
            return self.variants[key].copy()
        keys = [k for k in self.variants if k[0] == tid]
        if keys:
            best = min(keys, key=lambda k: bin(k[1] ^ mask).count("1"))
            return self.variants[best].copy()
        base = self.base.get(tid)
        if base is not None:
            return base.copy()
        return self.backdrop.copy()

    def render_level(self, tile_ids: list[int], level_index: int | None = None) -> Image.Image:
        out = np.zeros((PLAYFIELD_H, PLAYFIELD_W, 3), dtype=np.uint8)
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tile_ids[row * MAP_W + col]
                tile = self.render_tile(tile_ids, row, col, tid, level_index=level_index)
                y0, x0 = row * TILE_PX, col * TILE_PX
                out[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX] = tile
        return Image.fromarray(out, "RGB")


def _cell(field: np.ndarray, row: int, col: int) -> np.ndarray:
    y0, x0 = row * TILE_PX, col * TILE_PX
    return field[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX]


def build_composite_atlas(
    captures: list[tuple[Level, Image.Image]],
) -> CompositeAtlas:
    # One stable frame per level (f0) avoids sprite contamination in medians.
    by_level: dict[int, tuple[Level, Image.Image]] = {}
    for level, native in captures:
        if level.index not in by_level:
            by_level[level.index] = (level, native)

    buckets: dict[int, list[np.ndarray]] = defaultdict(list)
    variant_buckets: dict[tuple[int, int], list[np.ndarray]] = defaultdict(list)
    backdrop_samples: list[np.ndarray] = []

    level_cells: dict[int, dict[tuple[int, int, int], np.ndarray]] = {}

    for level, native in by_level.values():
        field = np.array(playfield_from_native(native))
        tiles = level.tiles
        level_cells[level.index] = {}
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tiles[row * MAP_W + col]
                cell = _cell(field, row, col)
                if tid in BACKDROP_RENDER_IDS or tid == 0:
                    backdrop_samples.append(cell)
                    continue
                level_cells[level.index][(row, col, tid)] = cell
                buckets[tid].append(cell)
                mask = neighbor_solid_mask8(tiles, row, col)
                variant_buckets[(tid, mask)].append(cell)

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

    variants: dict[tuple[int, int], np.ndarray] = {}
    for key, samples in variant_buckets.items():
        if not samples:
            continue
        variants[key] = np.median(np.stack(samples, axis=0), axis=0).astype(np.uint8)

    return CompositeAtlas(
        base=base,
        variants=variants,
        backdrop=backdrop,
        counts=counts,
        flagged_ids=sorted(flagged),
        level_cells=level_cells,
    )


def coverage_mask(tile_ids: list[int], atlas: CompositeAtlas) -> np.ndarray:
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                continue
            m[row, col] = atlas.has_tile(tid)
    return m
