"""Base per-tile-ID atlas + 4-neighbour solid edge overlays."""

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

# Tiles that participate in brick/stone adjacency borders.
SOLID_TILE_IDS = frozenset(
    range(0x18, 0x32)
) | frozenset(range(0x40, 0x49)) | frozenset({70, 71, 72, 73, 74, 75, 0x46, 0x47, 0x48, 0x72, 0x75})

BACKDROP_RENDER_IDS = frozenset({0, 60, 0x3C}) | SPAWN_MARKER_TILE_IDS


def is_solid(tid: int) -> bool:
    if tid in BACKDROP_RENDER_IDS:
        return False
    if tid in SOLID_TILE_IDS:
        return True
    if tid in ITEM_TILE_IDS or tid in (1, 2, 3, 4, 5, 6, 7, 8):
        return False
    if tid == 0:
        return False
    return tid >= 0x18


def neighbor_solid_mask(tiles: list[int], row: int, col: int) -> int:
    """4-bit N,E,S,W (1 = solid neighbour)."""
    bit = 0
    for i, (dr, dc) in enumerate(((-1, 0), (0, 1), (1, 0), (0, -1))):
        r, c = row + dr, col + dc
        if 0 <= r < MAP_H and 0 <= c < MAP_W:
            if is_solid(tiles[r * MAP_W + c]):
                bit |= 1 << i
    return bit


@dataclass
class CompositeAtlas:
    base: dict[int, np.ndarray]
    overlay: dict[tuple[int, int], np.ndarray]  # (tid, 4-bit mask) -> RGB residual
    backdrop: np.ndarray
    counts: dict[int, int] = field(default_factory=dict)
    flagged_ids: list[int] = field(default_factory=list)

    def has_tile(self, tid: int) -> bool:
        if tid in BACKDROP_RENDER_IDS:
            return True
        return tid in self.base

    def render_tile(self, tiles: list[int], row: int, col: int, tid: int) -> np.ndarray:
        if tid in BACKDROP_RENDER_IDS or tid == 0:
            return self.backdrop.copy()
        base = self.base.get(tid)
        if base is None:
            return self.backdrop.copy()
        out = base.astype(np.int16)
        mask = neighbor_solid_mask(tiles, row, col)
        key = (tid, mask)
        if key in self.overlay:
            out = out + self.overlay[key]
        elif tid in SOLID_TILE_IDS:
            # Fallback: any overlay for same tid with closest mask
            keys = [k for k in self.overlay if k[0] == tid]
            if keys:
                best = min(keys, key=lambda k: bin(k[1] ^ mask).count("1"))
                out = out + self.overlay[best]
        return np.clip(out, 0, 255).astype(np.uint8)

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
    captures: list[tuple[Level, Image.Image]],
) -> CompositeAtlas:
    from .native_vga import is_gameplay_frame

    buckets: dict[int, list[np.ndarray]] = defaultdict(list)
    overlay_buckets: dict[tuple[int, int], list[np.ndarray]] = defaultdict(list)
    backdrop_samples: list[np.ndarray] = []

    for level, native in captures:
        if not is_gameplay_frame(native):
            continue
        field = np.array(playfield_from_native(native))
        tiles = level.tiles
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tiles[row * MAP_W + col]
                cell = _cell(field, row, col)
                if tid in BACKDROP_RENDER_IDS or tid == 0:
                    backdrop_samples.append(cell)
                    continue
                buckets[tid].append(cell)

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
        med = np.median(stack, axis=0).astype(np.uint8)
        base[tid] = med
        counts[tid] = len(samples)
        if float(np.mean(np.var(stack, axis=0))) > 400.0:
            flagged.append(tid)

    # Second pass: edge residuals vs base
    for level, native in captures:
        if not is_gameplay_frame(native):
            continue
        field = np.array(playfield_from_native(native))
        tiles = level.tiles
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tiles[row * MAP_W + col]
                if tid not in base:
                    continue
                cell = _cell(field, row, col).astype(np.int16)
                b = base[tid].astype(np.int16)
                residual = cell - b
                mask = neighbor_solid_mask(tiles, row, col)
                overlay_buckets[(tid, mask)].append(residual)

    overlay: dict[tuple[int, int], np.ndarray] = {}
    for key, residuals in overlay_buckets.items():
        if len(residuals) < 2:
            continue
        overlay[key] = np.median(np.stack(residuals, axis=0), axis=0).astype(np.int16)

    return CompositeAtlas(
        base=base,
        overlay=overlay,
        backdrop=backdrop,
        counts=counts,
        flagged_ids=sorted(flagged),
    )


def coverage_mask(tile_ids: list[int], atlas: CompositeAtlas) -> np.ndarray:
    """True where a non-backdrop cell has a base tile."""
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if tid in BACKDROP_RENDER_IDS or tid == 0:
                continue
            m[row, col] = atlas.has_tile(tid)
    return m
