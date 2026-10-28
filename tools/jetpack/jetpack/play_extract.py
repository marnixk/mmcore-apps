"""Export gfx/frames from DAT decode + DAC calibration (no context RGB atlas)."""

from __future__ import annotations

import json
from pathlib import Path

from PIL import Image

from .constants import ENEMY_NAMES, SPAWN_MARKER_TILE_IDS, TILE_NAMES, TILE_PX
from .graphics import TileAtlas
from .levels import load_jetlev
from .map_layout import default_layout
from .native_vga import calibrate_origin, load_geometry, save_geometry
from .palette_fit import fit_tile_palettes, fit_vga_palette, slot_to_rgb
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
    levels = load_jetlev(game_dir / "JETLEV.DAT")
    caps = load_native_captures(native_dir)
    if not caps:
        raise FileNotFoundError(f"no native captures in {native_dir}")

    lv0 = next(lv for lv in levels if lv.index == 0)
    ref = Image.open(caps[0][0])
    origin = calibrate_origin(lv0.tiles, ref)
    geom = load_geometry()
    geom = type(geom)(geom.native_size, origin, geom.tile_px, geom.map_size)
    save_geometry(geom)

    atlas = TileAtlas.open(game_dir)
    global_pal = fit_vga_palette(game_dir, native_dir, levels)
    tile_pals = fit_tile_palettes(game_dir, native_dir, levels)

    ref_dir = root / "reference"
    ref_dir.mkdir(parents=True, exist_ok=True)

    out_dir = root / "gfx" / "frames"
    out_dir.mkdir(parents=True, exist_ok=True)
    for old in out_dir.glob("*.png"):
        old.unlink()

    index: dict = {
        "source": "dat_sclm_decode",
        "render_rule": (
            "JETPACKn.DAT base (AH=8) + edge strip slot 120+tile_id (AH=9, index 0 transparent); "
            "DAC from interior-cell capture medians; see FORMATS.md 0x4730/0x48AD."
        ),
        "map_layout": layout.to_dict(),
        "tile_size": TILE_PX,
        "native_resolution": list(geom.native_size),
        "playfield_origin": list(geom.origin),
        "tiles": [],
        "player": [],
        "enemies": [],
        "pickups": [],
    }

    exported: set[int] = set()
    for tid in range(120):
        if tid in SPAWN_MARKER_TILE_IDS or tid in exported:
            continue
        px = atlas.tile_pixels(tid)
        if not px:
            continue
        pal = tile_pals.get(tid) or global_pal
        rgb = slot_to_rgb(px, pal)
        name = TILE_NAMES.get(tid, f"id_{tid:02x}")
        fname = f"tile_{tid:03d}_{_slug(name)}.png"
        Image.fromarray(rgb, "RGB").convert("RGBA").save(out_dir / fname)
        index["tiles"].append({"file": fname, "tile_id": tid, "name": name, "samples": 0})
        exported.add(tid)

    if 0 in caps and len(caps[0]) >= 2:
        from .sprites_diff import extract_sprite_frames

        spr = extract_sprite_frames(caps[0], lv0, geom)
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
                "base_tile_count": len(exported),
                "context_variant_count": 0,
                "renderer": "dat_compositor",
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    (out_dir / "index.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    return index
