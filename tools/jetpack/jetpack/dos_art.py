"""Build tile/sprite RGBA frames from DOSBox reference captures (ground truth)."""

from __future__ import annotations

import json
from collections import defaultdict
from pathlib import Path

from PIL import Image

from .constants import ENEMY_NAMES, MAP_H, MAP_W, TILE_PX
from .levels import Level

# Minimap inside reference/level00_play2.png (exact match to level00_preview_crop2.png).
PREVIEW_ORIGIN = (175, 95)
PREVIEW_SIZE = (290, 270)


def preview_subimage(full: Image.Image) -> Image.Image:
    x, y = PREVIEW_ORIGIN
    w, h = PREVIEW_SIZE
    return full.crop((x, y, x + w, y + h))


def _cell_box(col: int, row: int) -> tuple[int, int, int, int]:
    tw = PREVIEW_SIZE[0] / MAP_W
    th = PREVIEW_SIZE[1] / MAP_H
    x0 = PREVIEW_ORIGIN[0] + col * tw
    y0 = PREVIEW_ORIGIN[1] + row * th
    return int(x0), int(y0), int(round(x0 + tw)), int(round(y0 + th))


def extract_cell(full: Image.Image, col: int, row: int) -> Image.Image:
    box = _cell_box(col, row)
    cell = full.crop(box)
    return cell.resize((TILE_PX, TILE_PX), Image.Resampling.NEAREST).convert("RGBA")


def _isolation_score(level: Level, col: int, row: int, tid: int) -> int:
    """Prefer minimap cells with no foreign tile IDs in the 3×3 neighborhood."""
    if level.tiles[row * MAP_W + col] != tid:
        return -10_000
    score = 0
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if dx == 0 and dy == 0:
                continue
            c, r = col + dx, row + dy
            if 0 <= c < MAP_W and 0 <= r < MAP_H:
                t = level.tiles[r * MAP_W + c]
                if t == tid:
                    score += 4
                elif t == 0:
                    score += 1
                else:
                    return -10_000
    return score


def build_tiles_from_level_preview(
    preview_path: Path,
    level: Level,
) -> dict[int, Image.Image]:
    """One RGBA 24×24 tile per ID seen on this level minimap."""
    full = Image.open(preview_path).convert("RGB")
    buckets: dict[int, list[tuple[int, Image.Image]]] = defaultdict(list)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = level.tiles[row * MAP_W + col]
            cell = extract_cell(full, col, row)
            buckets[tid].append((_isolation_score(level, col, row, tid), cell))

    out: dict[int, Image.Image] = {}
    for tid, cells in buckets.items():
        out[tid] = max(cells, key=lambda t: t[0])[1]
    return out


def crop_title_player(title_path: Path) -> list[Image.Image]:
    """Rough jetpack-guy frames from title screen (single pose)."""
    im = Image.open(title_path).convert("RGBA")
    w, h = im.size
    # Player cluster below sun, above logo (calibrated on 640×480 dosbox_title.png).
    boxes = [
        (w // 2 - 18, 52, w // 2 + 18, 100),
    ]
    frames: list[Image.Image] = []
    for x0, y0, x1, y1 in boxes:
        spr = im.crop((x0, y0, x1, y1))
        frames.append(spr.resize((TILE_PX, TILE_PX), Image.Resampling.NEAREST))
    return frames


def export_dos_frames(
    root: Path,
    game_dir: Path,
    preview_path: Path,
    title_path: Path | None,
    level_index: int = 0,
) -> dict:
    from .levels import load_jetlev

    out_dir = root / "gfx" / "frames"
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("*.png"):
        old.unlink()

    level = load_jetlev(game_dir / "JETLEV.DAT")[level_index]
    tiles = build_tiles_from_level_preview(preview_path, level)

    index: dict = {
        "source": "dosbox_capture",
        "preview_image": str(preview_path),
        "tile_size": TILE_PX,
        "tiles": [],
        "player": [],
        "enemies": [],
        "pickups": [],
    }

    from .constants import TILE_NAMES

    cells_dir = out_dir / "level00_cells"
    cells_dir.mkdir(parents=True, exist_ok=True)
    full = Image.open(preview_path).convert("RGB")
    for row in range(MAP_H):
        for col in range(MAP_W):
            cell = extract_cell(full, col, row)
            cell.save(cells_dir / f"cell_{col:02d}_{row:02d}.png")

    for tid in sorted(tiles):
        if tid == 0:
            continue
        name = TILE_NAMES.get(tid, f"id_{tid:02x}")
        fname = f"tile_{tid:03d}_{_slug(name)}.png"
        tiles[tid].save(out_dir / fname)
        index["tiles"].append({"file": fname, "tile_id": tid, "name": name})
    if 0 in tiles:
        tiles[0].save(out_dir / "tile_000_backdrop.png")
        index["tiles"].append({"file": "tile_000_backdrop.png", "tile_id": 0, "name": "backdrop"})

    if title_path and title_path.is_file():
        for i, spr in enumerate(crop_title_player(title_path)):
            fname = f"player_fly_{i}.png"
            spr.save(out_dir / fname)
            index["player"].append({"file": fname, "frame": i, "role": "fly"})

    # Emeralds on title for compare / UI reuse.
    if title_path and title_path.is_file():
        im = Image.open(title_path).convert("RGBA")
        w, h = im.size

        def _gem_box(left_half: bool) -> tuple[int, int, int, int]:
            xs: list[int] = []
            ys: list[int] = []
            x_range = range(0, w // 2) if left_half else range(w // 2, w)
            for y in range(h):
                for x in x_range:
                    r, g, b, _a = im.getpixel((x, y))
                    if g > 120 and r < 90 and b < 90 and g > r + 20:
                        xs.append(x)
                        ys.append(y)
            if not xs:
                return (40, 110, 86, 156) if left_half else (w - 86, 110, w - 40, 156)
            pad = 4
            return (
                max(0, min(xs) - pad),
                max(0, min(ys) - pad),
                min(w, max(xs) + pad),
                min(h, max(ys) + pad),
            )

        for left, fname in [(True, "pickup_emerald_left.png"), (False, "pickup_emerald_right.png")]:
            box = _gem_box(left)
            gem = im.crop(box).resize((TILE_PX, TILE_PX), Image.Resampling.NEAREST)
            gem.save(out_dir / fname)
            index["pickups"].append({"file": fname, "tas_reference": "emerald_gem.png"})

    (out_dir / "index.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    return index


def _slug(name: str) -> str:
    import re

    s = re.sub(r"[^a-z0-9_]+", "_", name.lower().strip())
    return s or "unnamed"
