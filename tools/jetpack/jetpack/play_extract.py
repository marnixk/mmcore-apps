"""Export gfx/frames from native VGA captures using context median atlas."""

from __future__ import annotations

import json
import pickle
from pathlib import Path

import numpy as np
from PIL import Image

from .atlas_median import (
    build_context_median_atlas,
    collect_context_samples,
    render_level_from_context_atlas,
)
from .constants import ENEMY_NAMES, SPAWN_MARKER_TILE_IDS, TILE_NAMES, TILE_PX
from .levels import load_jetlev
from .map_layout import default_layout
from .native_vga import calibrate_origin, load_geometry, playfield_from_native, save_geometry


def _slug(name: str) -> str:
    import re

    s = re.sub(r"[^a-z0-9_]+", "_", name.lower().strip())
    return s or "unnamed"


def _key_slug(key: tuple) -> str:
    return "_".join(str(x) for x in key)


def load_native_captures(native_dir: Path) -> dict[int, list[Path]]:
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
    layout = default_layout()
    levels = {lv.index: lv for lv in load_jetlev(game_dir / "JETLEV.DAT")}
    caps = load_native_captures(native_dir)
    if not caps:
        raise FileNotFoundError(f"no native captures in {native_dir}")

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

    buckets = collect_context_samples(capture_pairs)
    stats = build_context_median_atlas(buckets)
    ref_dir = root / "reference"
    ref_dir.mkdir(parents=True, exist_ok=True)
    with (ref_dir / "context_atlas.pkl").open("wb") as f:
        pickle.dump(stats, f, protocol=pickle.HIGHEST_PROTOCOL)
    export_stats_report(ref_dir / "atlas_stats.json", stats, layout)

    out_dir = root / "gfx" / "frames"
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("*.png"):
        old.unlink()
    variants_dir = out_dir / "context_variants"
    if variants_dir.is_dir():
        for old in variants_dir.glob("*.png"):
            old.unlink()
    else:
        variants_dir.mkdir(parents=True, exist_ok=True)

    index: dict = {
        "source": "dosbox_native_context_median",
        "map_layout": layout.to_dict(),
        "tile_size": TILE_PX,
        "native_resolution": list(geom.native_size),
        "playfield_origin": list(geom.origin),
        "context_keys": len(stats.tiles),
        "flagged_tile_ids": stats.flagged_ids,
        "flagged_context_keys": len(stats.flagged_keys),
        "tiles": [],
        "context_variants": [],
        "player": [],
        "enemies": [],
        "pickups": [],
    }

    exported: set[int] = set()
    for tid in sorted(stats.tid_fallback):
        if tid in SPAWN_MARKER_TILE_IDS or tid in exported:
            continue
        if tid == 60 and 0x3C in exported:
            continue
        name = TILE_NAMES.get(tid, f"id_{tid:02x}")
        fname = f"tile_{tid:03d}_{_slug(name)}.png"
        Image.fromarray(stats.tid_fallback[tid], "RGB").convert("RGBA").save(out_dir / fname)
        index["tiles"].append(
            {
                "file": fname,
                "tile_id": tid,
                "name": name,
                "cross_level_disagreement": stats.tid_disagreement.get(tid, 0.0),
            }
        )
        exported.add(tid)

    for key in sorted(stats.tiles, key=lambda k: (k[0], k[1:])):
        if stats.disagreement.get(key, 0.0) > 50.0:
            continue
        tid = key[0]
        fname = f"ctx_{tid:03d}_{_key_slug(key)}.png"
        if len(fname) > 200:
            fname = f"ctx_{tid:03d}_{hash(key) & 0xFFFFFFFF:08x}.png"
        Image.fromarray(stats.tiles[key], "RGB").convert("RGBA").save(variants_dir / fname)
        index["context_variants"].append(
            {
                "file": f"context_variants/{fname}",
                "tile_id": tid,
                "key": list(key),
                "samples": stats.counts.get(key, 0),
                "disagreement": stats.disagreement.get(key, 0.0),
            }
        )

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


def export_stats_report(stats_path: Path, stats, layout) -> None:
    neighbour_rule = (
        "Brick and structure tiles are chosen from JETLEV tile ID plus 8-neighbour "
        "context (N,E,S,W,NE,NW,SE,SW); atlas stores one 24×24 RGB tile per context key."
    )
    stats_path.write_text(
        json.dumps(
            {
                "map_layout": layout.to_dict(),
                "context_key_count": len(stats.tiles),
                "flagged_tile_ids": stats.flagged_ids,
                "flagged_context_key_count": len(stats.flagged_keys),
                "tid_disagreement": {str(k): v for k, v in stats.tid_disagreement.items()},
                "neighbour_variant_rule": neighbour_rule,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
