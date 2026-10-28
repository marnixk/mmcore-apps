"""Calibrate DAT index → RGB from native captures (LOO-safe per level)."""

from __future__ import annotations

from collections import defaultdict
from pathlib import Path

import numpy as np
from PIL import Image

from .constants import MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_PX
from .graphics import TileAtlas, load_vga_palette
from .levels import Level
from .native_vga import playfield_from_native

TilePalette = dict[int, tuple[int, int, int]]


def slot_to_rgb(px: bytes, palette: list[tuple[int, int, int]] | TilePalette) -> np.ndarray:
    arr = np.zeros((TILE_PX, TILE_PX, 3), dtype=np.uint8)
    for i, idx in enumerate(px):
        if idx == 0:
            continue
        x, y = i % TILE_PX, i // TILE_PX
        if isinstance(palette, list):
            arr[y, x] = palette[idx]
        else:
            arr[y, x] = palette.get(idx, (0, 0, 0))
    return arr


def _collect_samples(
    levels: dict[int, Level],
    native_dir: Path,
    geom,
    atlas: TileAtlas,
    exclude_level: int | None,
) -> dict[tuple[int, int], list[np.ndarray]]:
    buckets: dict[tuple[int, int], list[np.ndarray]] = defaultdict(list)
    for idx, lv in levels.items():
        if exclude_level is not None and idx == exclude_level:
            continue
        path = native_dir / f"level_{idx:02d}_f0.png"
        if not path.is_file():
            continue
        cap = np.array(playfield_from_native(Image.open(path).convert("RGB"), geom))
        tiles = lv.tiles
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = tiles[row * MAP_W + col]
                if tid in (0, 4, 5, 60, 0x3C) or tid in SPAWN_MARKER_TILE_IDS:
                    continue
                # Interior cells only — avoids edge/shadow pixels polluting index colours.
                interior = True
                for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    r, c = row + dr, col + dc
                    if not (0 <= r < MAP_H and 0 <= c < MAP_W):
                        interior = False
                        break
                    if tiles[r * MAP_W + c] != tid:
                        interior = False
                        break
                if not interior:
                    continue
                px = atlas.tile_pixels(tid)
                if not px:
                    continue
                y0, x0 = row * TILE_PX, col * TILE_PX
                cell = cap[y0 : y0 + TILE_PX, x0 : x0 + TILE_PX]
                for i, pal_idx in enumerate(px):
                    if pal_idx == 0:
                        continue
                    x, y = i % TILE_PX, i // TILE_PX
                    buckets[(tid, pal_idx)].append(cell[y, x].astype(np.float32))
    return buckets


def fit_vga_palette(
    game_dir: Path,
    native_dir: Path,
    levels: list[Level] | dict[int, Level],
    exclude_level: int | None = None,
) -> list[tuple[int, int, int]]:
    """Global list palette (index 0 transparent); used for edge slots and fallback."""
    base = load_vga_palette(game_dir / "JETPAL.DAT")
    atlas = TileAtlas.open(game_dir)
    from .native_vga import load_geometry

    geom = load_geometry()
    lvmap = levels if isinstance(levels, dict) else {lv.index: lv for lv in levels}
    buckets: dict[int, list[np.ndarray]] = defaultdict(list)
    for (tid, pidx), samples in _collect_samples(lvmap, native_dir, geom, atlas, exclude_level).items():
        buckets[pidx].extend(samples)
    out = list(base)
    for idx, samples in buckets.items():
        med = np.median(np.stack(samples, axis=0), axis=0).astype(np.uint8)
        out[idx] = (int(med[0]), int(med[1]), int(med[2]))
    out[0] = (0, 0, 0)
    return out


def fit_tile_palettes(
    game_dir: Path,
    native_dir: Path,
    levels: list[Level] | dict[int, Level],
    exclude_level: int | None = None,
) -> dict[int, TilePalette]:
    """Per map tile id: DAT index → RGB median from captures (portable with DAT blobs)."""
    atlas = TileAtlas.open(game_dir)
    from .native_vga import load_geometry

    geom = load_geometry()
    lvmap = levels if isinstance(levels, dict) else {lv.index: lv for lv in levels}
    buckets = _collect_samples(lvmap, native_dir, geom, atlas, exclude_level)
    global_pal = fit_vga_palette(game_dir, native_dir, lvmap, exclude_level)
    out: dict[int, TilePalette] = {}
    by_tid: dict[int, dict[int, list[np.ndarray]]] = defaultdict(lambda: defaultdict(list))
    for (tid, pidx), samples in buckets.items():
        by_tid[tid][pidx].extend(samples)
    for tid, idx_map in by_tid.items():
        pal: TilePalette = {0: (0, 0, 0)}
        for pidx, samples in idx_map.items():
            med = np.median(np.stack(samples, axis=0), axis=0).astype(np.uint8)
            pal[pidx] = (int(med[0]), int(med[1]), int(med[2]))
        out[tid] = pal
    out[0x3C] = out.get(0x3C) or out.get(60) or {0: (0, 0, 0)}
    return out
