"""Export gfx/frames from native VGA captures using composite tile atlas."""

from __future__ import annotations

import json
import pickle
from pathlib import Path

import numpy as np
from jetpack.native_vga import playfield_from_native
from PIL import Image

from .constants import ENEMY_NAMES, SPAWN_MARKER_TILE_IDS, TILE_NAMES, TILE_PX
from .levels import load_jetlev
from .map_layout import default_layout
from .native_vga import calibrate_origin, load_geometry, save_geometry
from .tile_composite import CompositeAtlas, build_composite_atlas


def _slug(name: str) -> str:
    import re

    s = re.sub(r"[^a-z0-9_]+", "_", name.lower().strip())
    return s or "unnamed"


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
        frames = [np.array(playfield_from_native(Image.open(p).convert("RGB"), geom)) for p in paths]
        if len(frames) > 1:
            med = np.median(np.stack(frames, axis=0), axis=0).astype(np.uint8)
            # Keep HUD/status from f0; only the playfield is temporally filtered.
            native = Image.open(paths[0]).convert("RGB")
            pf = Image.fromarray(med, "RGB")
            native.paste(pf, geom.origin)
        else:
            native = Image.open(paths[0]).convert("RGB")
        capture_pairs.append((levels[idx], native))

    atlas = build_composite_atlas(capture_pairs)
    ref_dir = root / "reference"
    ref_dir.mkdir(parents=True, exist_ok=True)
    with (ref_dir / "composite_atlas.pkl").open("wb") as f:
        pickle.dump(atlas, f, protocol=pickle.HIGHEST_PROTOCOL)

    out_dir = root / "gfx" / "frames"
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("*.png"):
        old.unlink()

    index: dict = {
        "source": "dosbox_native_composite",
        "render_rule": (
            "base[tile_id] + overlay[(tile_id, mask8)] for solid terrain; "
            "mask8 = N,E,S,W + diagonal solid neighbours (decor tiles use base only)."
        ),
        "map_layout": layout.to_dict(),
        "tile_size": TILE_PX,
        "native_resolution": list(geom.native_size),
        "playfield_origin": list(geom.origin),
        "flagged_tile_ids": atlas.flagged_ids,
        "tiles": [],
        "player": [],
        "enemies": [],
        "pickups": [],
    }

    exported: set[int] = set()
    for tid in sorted(atlas.base):
        if tid in SPAWN_MARKER_TILE_IDS or tid in exported:
            continue
        name = TILE_NAMES.get(tid, f"id_{tid:02x}")
        fname = f"tile_{tid:03d}_{_slug(name)}.png"
        Image.fromarray(atlas.base[tid], "RGB").convert("RGBA").save(out_dir / fname)
        index["tiles"].append(
            {"file": fname, "tile_id": tid, "name": name, "samples": atlas.counts.get(tid, 0)}
        )
        exported.add(tid)

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

    (ref_dir / "atlas_stats.json").write_text(
        json.dumps(
            {
                "map_layout": layout.to_dict(),
                "base_tile_count": len(atlas.base),
                "overlay_count": len(atlas.overlays),
                "flagged_tile_ids": atlas.flagged_ids,
                "edge_overlay_rule": (
                    "8-bit solid-neighbour mask; render = base[tile_id] + overlay[(tile_id, mask8)]."
                ),
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    (out_dir / "index.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    return index
