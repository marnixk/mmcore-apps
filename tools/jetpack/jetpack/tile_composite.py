"""Portable atlas: per-tile base RGB + cardinal solid-neighbour edge overlays."""

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

# Per (tile_id, mask4) bucket: above this mean pixel variance, edge rule is not portable.
OVERLAY_DISAGREE_MAX = 50.0

BACKDROP_RENDER_IDS = frozenset({0, 60, 0x3C}) | SPAWN_MARKER_TILE_IDS

# Ladders, teleports, toggles, pickups — drawn on backdrop; no edge compositing.
DECOR_TILE_IDS = (
    frozenset(ITEM_TILE_IDS.keys())
    | frozenset(range(0x01, 0x0F))
    | frozenset({0x05})  # gem_collected
)

# Animated / stateful — excluded from strict validation; render base tile only.
ANIMATED_TERRAIN_IDS = frozenset(
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
        0x72,  # conveyor_left
        0x75,  # conveyor_right
    }
)


def is_solid_terrain(tid: int) -> bool:
    if tid in BACKDROP_RENDER_IDS or tid == 0:
        return False
    if tid in DECOR_TILE_IDS or tid in ANIMATED_TERRAIN_IDS:
        return False
    return True


def is_solid_for_mask(tid: int) -> bool:
    """Neighbour counts as solid for edge/shadow rules."""
    if tid in BACKDROP_RENDER_IDS or tid == 0:
        return False
    if tid in DECOR_TILE_IDS:
        return False
    return True


def neighbor_solid_mask4(tiles: list[int], row: int, col: int) -> int:
    """4-bit N,E,S,W solid neighbours (cardinal-only; used in docs)."""
    offs = [(-1, 0), (0, 1), (1, 0), (0, -1)]
    mask = 0
    for i, (dr, dc) in enumerate(offs):
        r, c = row + dr, col + dc
        if 0 <= r < MAP_H and 0 <= c < MAP_W and is_solid_for_mask(tiles[r * MAP_W + c]):
            mask |= 1 << i
    return mask


def neighbor_solid_mask8(tiles: list[int], row: int, col: int) -> int:
    """8-bit N,E,S,W + diagonal solid neighbours for edge/shadow overlays."""
    offs = [(-1, 0), (0, 1), (1, 0), (0, -1), (-1, -1), (-1, 1), (1, -1), (1, 1)]
    mask = 0
    for i, (dr, dc) in enumerate(offs):
        r, c = row + dr, col + dc
        if 0 <= r < MAP_H and 0 <= c < MAP_W and is_solid_for_mask(tiles[r * MAP_W + c]):
            mask |= 1 << i
    return mask


def _hamming(a: int, b: int) -> int:
    return (a ^ b).bit_count()


@dataclass
class CompositeAtlas:
    base: dict[int, np.ndarray]
    overlays: dict[tuple[int, int], np.ndarray]  # (tid, mask8) -> int16 residual RGB
    backdrop: np.ndarray
    overlay_disagreement: dict[tuple[int, int], float] = field(default_factory=dict)
    counts: dict[int, int] = field(default_factory=dict)
    flagged_ids: list[int] = field(default_factory=list)
    mask_bits: int = 8

    def has_tile(self, tid: int) -> bool:
        if tid in BACKDROP_RENDER_IDS:
            return True
        return tid in self.base

    def overlay_for(self, tid: int, mask8: int) -> np.ndarray | None:
        key = (tid, mask8)
        if key in self.overlays:
            return self.overlays[key]
        keys = [k for k in self.overlays if k[0] == tid]
        if not keys:
            return None
        best = min(keys, key=lambda k: _hamming(k[1], mask8))
        if _hamming(best[1], mask8) > 2:
            return None
        return self.overlays[best]

    def has_exact_overlay(self, tid: int, mask8: int) -> bool:
        return (tid, mask8) in self.overlays

    def render_tile(self, tiles: list[int], row: int, col: int, tid: int) -> np.ndarray:
        if tid in BACKDROP_RENDER_IDS or tid == 0:
            return self.backdrop.copy()
        base = self.base.get(tid)
        if base is None:
            return self.backdrop.copy()
        if tid in DECOR_TILE_IDS or tid in ANIMATED_TERRAIN_IDS or not is_solid_terrain(tid):
            return base.copy()
        mask8 = neighbor_solid_mask8(tiles, row, col)
        key = (tid, mask8)
        disagree = self.overlay_disagreement.get(key, 999.0)
        if disagree > OVERLAY_DISAGREE_MAX:
            return base.copy()
        ov = self.overlays.get(key)
        if ov is None:
            ov = self.overlay_for(tid, mask4)
        if ov is None:
            return base.copy()
        out = base.astype(np.int16) + ov
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
    by_level: dict[int, tuple[Level, Image.Image]] = {}
    for level, native in captures:
        if level.index not in by_level:
            by_level[level.index] = (level, native)

    buckets: dict[int, list[np.ndarray]] = defaultdict(list)
    overlay_buckets: dict[tuple[int, int], list[np.ndarray]] = defaultdict(list)
    backdrop_samples: list[np.ndarray] = []

    for level, native in by_level.values():
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
                if is_solid_terrain(tid):
                    mask8 = neighbor_solid_mask8(tiles, row, col)
                    overlay_buckets[(tid, mask8)].append(cell)

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

    overlays: dict[tuple[int, int], np.ndarray] = {}
    overlay_disagreement: dict[tuple[int, int], float] = {}
    for key, samples in overlay_buckets.items():
        tid, _mask = key
        b = base.get(tid)
        if b is None:
            continue
        stack = np.stack(samples, axis=0).astype(np.float32)
        med = np.median(stack, axis=0)
        overlays[key] = (med - b.astype(np.float32)).astype(np.int16)
        overlay_disagreement[key] = float(np.mean(np.var(stack, axis=0)))

    return CompositeAtlas(
        base=base,
        overlays=overlays,
        overlay_disagreement=overlay_disagreement,
        backdrop=backdrop,
        counts=counts,
        flagged_ids=sorted(flagged),
    )


def structural_eval_mask(
    tile_ids: list[int],
    static_mask: np.ndarray,
    atlas: CompositeAtlas | None = None,
) -> np.ndarray:
    """Static solid terrain where (tile_id, mask4) overlay rule is agreed across captures."""
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if not is_solid_terrain(tid):
                continue
            if not static_mask[row, col]:
                continue
            if atlas is not None:
                mask8 = neighbor_solid_mask8(tile_ids, row, col)
                key = (tid, mask8)
                if key not in atlas.overlays:
                    continue
                if atlas.overlay_disagreement.get(key, 999.0) > OVERLAY_DISAGREE_MAX:
                    continue
            m[row, col] = True
    return m


def coverage_mask(tile_ids: list[int], atlas: CompositeAtlas) -> np.ndarray:
    """Solid terrain cells with a base tile and an overlay rule for their mask4."""
    m = np.zeros((MAP_H, MAP_W), dtype=bool)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if not is_solid_terrain(tid):
                continue
            if tid not in atlas.base:
                continue
            mask8 = neighbor_solid_mask8(tile_ids, row, col)
            key = (tid, mask8)
            if key in atlas.overlays and atlas.overlay_disagreement.get(key, 999.0) <= OVERLAY_DISAGREE_MAX:
                m[row, col] = True
    return m
