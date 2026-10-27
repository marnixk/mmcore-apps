"""Build tile/sprite RGBA atlas from full-size in-game DOS captures."""

from __future__ import annotations

import json
from collections import defaultdict
from pathlib import Path

from PIL import Image

from .constants import ENEMY_NAMES, MAP_H, MAP_W, SPAWN_MARKER_TILE_IDS, TILE_NAMES, TILE_PX
from .levels import Level, load_jetlev
from .playfield import detect_playfield_crop, dosbox_client_image, normalize_playfield

_PKG = Path(__file__).resolve().parents[1]


def _isolation_score(level: Level, col: int, row: int, tid: int) -> int:
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


def extract_tiles_from_capture(
    capture_path: Path,
    level: Level,
    crop_box: tuple[int, int, int, int] | None = None,
) -> tuple[Image.Image, dict[int, Image.Image]]:
    im = Image.open(capture_path)
    if crop_box:
        from .playfield import PlayfieldCrop

        field = dosbox_client_image(im).crop(PlayfieldCrop(*crop_box).as_box()).resize(
            (MAP_W * TILE_PX, MAP_H * TILE_PX), Image.Resampling.NEAREST
        )
    else:
        field = normalize_playfield(im)
    buckets: dict[int, list[tuple[int, Image.Image]]] = defaultdict(list)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = level.tiles[row * MAP_W + col]
            cell = field.crop((col * TILE_PX, row * TILE_PX, (col + 1) * TILE_PX, (row + 1) * TILE_PX))
            rgba = cell.convert("RGBA")
            buckets[tid].append((_isolation_score(level, col, row, tid), rgba))
    tiles: dict[int, Image.Image] = {}
    for tid, items in buckets.items():
        tiles[tid] = max(items, key=lambda t: t[0])[1]
    return field, tiles


def merge_tile_dicts(base: dict[int, Image.Image], extra: dict[int, Image.Image]) -> dict[int, Image.Image]:
    out = dict(base)
    for tid, im in extra.items():
        if tid not in out or tid == 0:
            out[tid] = im
    return out


def export_play_frames(
    root: Path,
    game_dir: Path,
    captures: dict[int, Path],
    player_capture: Path | None = None,
) -> dict:
    """captures: level_index -> dosbox_play_levelXX.png"""
    out_dir = root / "gfx" / "frames"
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("*.png"):
        old.unlink()
    cells_dir = out_dir / "level00_cells"
    if cells_dir.is_dir():
        for p in cells_dir.glob("*.png"):
            p.unlink()
        cells_dir.rmdir()

    levels = {lv.index: lv for lv in load_jetlev(game_dir / "JETLEV.DAT")}
    merged: dict[int, Image.Image] = {}
    crop_ref: tuple[int, int, int, int] | None = None
    tile_buckets: dict[int, list[tuple[int, Image.Image]]] = defaultdict(list)
    for idx in sorted(captures):
        cap = captures[idx]
        if not cap.is_file():
            continue
        lv = levels[idx]
        field, tiles = extract_tiles_from_capture(cap, lv, None)
        if crop_ref is None:
            crop = detect_playfield_crop(Image.open(cap))
            crop_ref = (crop.left, crop.top, crop.right, crop.bottom)
        for row in range(MAP_H):
            for col in range(MAP_W):
                tid = lv.tiles[row * MAP_W + col]
                cell = field.crop(
                    (col * TILE_PX, row * TILE_PX, (col + 1) * TILE_PX, (row + 1) * TILE_PX)
                ).convert("RGBA")
                tile_buckets[tid].append(
                    (_isolation_score(lv, col, row, tid), -idx, cell)
                )
                if tid == 0:
                    tile_buckets[0x3C].append(
                        (_isolation_score(lv, col, row, 0), -idx - 1000, cell)
                    )
        field.save(root / "reference" / f"validate_level{idx:02d}_playfield.png")
    backdrop_ids = {0x3C, 60}
    for tid, items in tile_buckets.items():
        if tid in backdrop_ids:
            continue
        merged[tid] = max(items, key=lambda t: (t[0], t[1]))[2]
    # Backdrop: only from map cells with tile id 0 (empty), never from explicit 0x3C cells
    # (those often overlap entities and mis-register in JETLEV).
    best_back: tuple[int, int, Image.Image] | None = None
    for idx in sorted(captures):
        cap = captures[idx]
        if not cap.is_file():
            continue
        lv = levels[idx]
        field = normalize_playfield(Image.open(cap))
        for row in range(MAP_H):
            for col in range(MAP_W):
                if lv.tiles[row * MAP_W + col] != 0:
                    continue
                sc = _isolation_score(lv, col, row, 0)
                cell = field.crop(
                    (col * TILE_PX, row * TILE_PX, (col + 1) * TILE_PX, (row + 1) * TILE_PX)
                ).convert("RGBA")
                if best_back is None or sc > best_back[0]:
                    best_back = (sc, -idx, cell)
    if best_back is not None:
        merged[0x3C] = best_back[2]
        merged[60] = best_back[2]

    index: dict = {
        "source": "dosbox_play_capture",
        "tile_size": TILE_PX,
        "playfield_crop": list(crop_ref) if crop_ref else None,
        "captures": {str(k): str(v) for k, v in captures.items()},
        "tiles": [],
        "player": [],
        "enemies": [],
        "pickups": [],
    }

    for tid in sorted(merged):
        if tid in SPAWN_MARKER_TILE_IDS:
            continue
        name = TILE_NAMES.get(tid, f"id_{tid:02x}")
        fname = f"tile_{tid:03d}_{_slug(name)}.png"
        merged[tid].save(out_dir / fname)
        index["tiles"].append({"file": fname, "tile_id": tid, "name": name})

    for idx, cap in captures.items():
        if not cap.is_file():
            continue
        lv = levels[idx]
        field, _ = extract_tiles_from_capture(cap, lv, None)
        for e in lv.enemies:
            if not e.type_id:
                continue
            x0 = (e.x - 1) * TILE_PX
            y0 = (e.y - 1) * TILE_PX
            spr = field.crop((x0, y0, x0 + TILE_PX, y0 + TILE_PX)).convert("RGBA")
            ename = ENEMY_NAMES.get(e.type_id, f"type{e.type_id}")
            fname = f"enemy_{ename}_0.png"
            out_path = out_dir / fname
            if not out_path.is_file():
                spr.save(out_path)
                index["enemies"].append(
                    {"file": fname, "enemy_id": e.type_id, "name": ename, "from_level": idx}
                )

    play0 = captures.get(0)
    if play0 and play0.is_file():
        lv0 = levels[0]
        field0, _ = extract_tiles_from_capture(play0, lv0, None)
        px = (lv0.player_x - 1) * TILE_PX
        py = (lv0.player_y - 1) * TILE_PX
        player = field0.crop((px, py, px + TILE_PX, py + TILE_PX)).convert("RGBA")
        player.save(out_dir / "player_fly_0.png")
        index["player"].append({"file": "player_fly_0.png", "frame": 0, "role": "fly"})

    (out_dir / "index.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    return index


def _slug(name: str) -> str:
    import re

    s = re.sub(r"[^a-z0-9_]+", "_", name.lower().strip())
    return s or "unnamed"
