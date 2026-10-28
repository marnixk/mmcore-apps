"""Extract moving sprites by differencing native frames against static rebuild."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image

from .atlas_median import build_median_atlas, collect_samples, render_level_from_atlas
from .levels import Level
from .native_vga import PlayfieldGeometry, playfield_from_native


@dataclass
class ExtractedSprites:
    player: Image.Image | None
    enemies: list[tuple[int, Image.Image]]


def extract_sprite_frames(
    frame_paths: list[Path],
    level: Level,
    geom: PlayfieldGeometry,
) -> ExtractedSprites:
    frames = [Image.open(p).convert("RGB") for p in frame_paths]
    buckets = collect_samples([(level, frames[0])])
    stats = build_median_atlas(buckets)
    static = np.array(render_level_from_atlas(level.tiles, stats))

    player_box = None
    enemies: list[tuple[int, Image.Image]] = []
    px = (level.player_x - 1) * 24
    py = (level.player_y - 1) * 24

    for fr in frames[1:]:
        cap = np.array(playfield_from_native(fr, geom))
        diff = np.abs(cap.astype(np.int16) - static.astype(np.int16))
        mask = np.max(diff, axis=2) > 18
        if not np.any(mask):
            continue
        # player region
        region = mask[py : py + 24, px : px + 24]
        if np.mean(region) > 0.15:
            rgba = _rgba_from_diff(cap, static, px, py, 24, 24)
            player_box = rgba
        for e in level.enemies:
            ex, ey = (e.x - 1) * 24, (e.y - 1) * 24
            if np.mean(mask[ey : ey + 24, ex : ex + 24]) > 0.1:
                rgba = _rgba_from_diff(cap, static, ex, ey, 24, 24)
                enemies.append((e.type_id, rgba))
    return ExtractedSprites(player=player_box, enemies=enemies)


def _rgba_from_diff(cap: np.ndarray, static: np.ndarray, x: int, y: int, w: int, h: int) -> Image.Image:
    cell = cap[y : y + h, x : x + w]
    ref = static[y : y + h, x : x + w]
    diff = np.max(np.abs(cell.astype(np.int16) - ref.astype(np.int16)), axis=2)
    alpha = (diff > 12).astype(np.uint8) * 255
    rgba = np.dstack([cell, alpha])
    return Image.fromarray(rgba, "RGBA")
