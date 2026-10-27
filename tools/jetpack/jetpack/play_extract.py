"""Export gfx/frames from native VGA captures using per-tile median atlas."""

from __future__ import annotations

import json
from pathlib import Path

import numpy as np
from PIL import Image

from .atlas_median import build_median_atlas, collect_samples
from .constants import ENEMY_NAMES, SPAWN_MARKER_TILE_IDS, TILE_NAMES, TILE_PX
from .levels import load_jetlev
from .native_vga import load_geometry, playfield_from_native, save_geometry, calibrate_origin


def _slug(name: str) -> str:
    import re

    s = re.sub(r"[^a-z0-9_]+", "_", name.lower().strip())
    return s or "unnamed"


def load_native_captures(native_dir: Path) -> dict[int, list[Path]]:
    """level_index -> sorted frame paths (level_XX_fN.png)."""
    out: dict[int, list[Path]] = {}
    for p in sorted(native_dir.glob("level_*_f*.png")):
        parts = p.stem.split("_")
        if len(parts) < 3:
            continue
        idx = int(parts[1])
        out.setdefault(idx, []).append(p)
    for idx in out:
        out[idx] = sorted(out[idx])
    return out


def export_play_frames(
    root: Path,
    game_dir: Path,
    native_dir: Path,
) -> dict:
    levels = {lv.index: lv for lv in load_jetlev(game_dir / "JETLEV.DAT")}
    caps = load_native_captures(native_dir)
    if not caps:
        raise FileNotFoundError(f"no native captures in {native_dir}")

    # Calibrate geometry from level 0 frame 0
    lv0 = levels[0]
    ref = Image.open(caps[0][0])
    origin = calibrate_origin(lv0.tiles, ref)
    geom = load_geometry()
    geom = type(geom)(geom.native_size, origin, geom.tile_px, geom.map_size)
    save_geometry(geom)

    capture_pairs: list[tuple] = []
    for idx, paths in sorted(caps.items()):
        if idx not in levels:
            continue
        for p in paths:
            capture_pairs.append((levels[idx], Image.open(p).convert("RGB")))

    buckets = collect_samples(capture_pairs)
    stats = build_median_atlas(buckets)
    export_stats_report(root / "reference" / "atlas_stats.json", stats)

    out_dir = root / "gfx" / "frames"
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("*.png"):
        old.unlink()

    index: dict = {
        "source": "dosbox_native_median",
        "tile_size": TILE_PX,
        "native_resolution": list(geom.native_size),
        "playfield_origin": list(geom.origin),
        "tile_counts": {str(k): v for k, v in stats.counts.items()},
        "flagged_tile_ids": stats.flagged_ids,
        "tiles": [],
        "player": [],
        "enemies": [],
        "pickups": [],
    }

    exported: set[int] = set()
    for tid in sorted(stats.tiles):
        if tid in SPAWN_MARKER_TILE_IDS or tid in exported:
            continue
        if tid == 60 and 0x3C in exported:
            continue
        name = TILE_NAMES.get(tid, f"id_{tid:02x}")
        fname = f"tile_{tid:03d}_{_slug(name)}.png"
        Image.fromarray(stats.tiles[tid], "RGB").convert("RGBA").save(out_dir / fname)
        index["tiles"].append({"file": fname, "tile_id": tid, "name": name, "samples": stats.counts.get(tid, 0)})
        exported.add(tid)

    # Player / enemies from frame diffs on level 0 sequence
    if 0 in caps and len(caps[0]) >= 2:
        from .sprites_diff import extract_sprite_frames

        spr = extract_sprite_frames(caps[0], levels[0], geom)
        if spr.player:
            spr.player.save(out_dir / "player_fly_0.png")
            index["player"].append({"file": "player_fly_0.png", "frame": 0, "role": "fly"})
        for i, (eid, im) in enumerate(spr.enemies):
            ename = ENEMY_NAMES.get(eid, f"type{eid}")
            fname = f"enemy_{ename}_{i}.png"
            im.save(out_dir / fname)
            index["enemies"].append({"file": fname, "enemy_id": eid, "name": ename})

    (out_dir / "index.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    return index


def export_stats_report(stats_path: Path, stats) -> None:
    stats_path.write_text(
        json.dumps(
            {
                "tile_count": len(stats.tiles),
                "flagged": stats.flagged_ids,
                "disagreement": {str(k): v for k, v in stats.disagreement.items()},
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
