"""Export decoded tiles and sprites as individual RGBA PNGs under gfx/frames/."""

from __future__ import annotations

import json
import re
from pathlib import Path

from PIL import Image

from .constants import ENEMY_NAMES, TILE_NAMES
from .graphics import TileAtlas
from .sprite_catalog import load_enemy_sheet_indices, load_player_frames, sheet_sprite

_CAMPAIGN_TILES = 0x78  # tile IDs 0x00–0x77


def _safe_slug(name: str) -> str:
    s = re.sub(r"[^a-z0-9_]+", "_", name.lower().strip())
    return s or "unnamed"


def _tile_filename(tile_id: int) -> str:
    label = TILE_NAMES.get(tile_id, f"id_{tile_id:02x}")
    return f"tile_{tile_id:03d}_{_safe_slug(label)}.png"


def export_frame_pngs(
    atlas: TileAtlas,
    out_dir: Path,
    sprites_index_path: Path | None = None,
) -> dict:
    """Write PNG frames and index.json; return index payload."""
    out_dir.mkdir(parents=True, exist_ok=True)
    entries: dict[str, list[dict]] = {"tiles": [], "player": [], "enemies": [], "pickups": []}

    meta: dict = {}
    if sprites_index_path and sprites_index_path.is_file():
        meta = json.loads(sprites_index_path.read_text(encoding="utf-8"))

    tile_map = {e["tile_id"]: e for e in meta.get("tile_id_map", [])}

    for tid in range(_CAMPAIGN_TILES):
        im = atlas.tile_image(tid, transparent=True)
        if im is None:
            continue
        fname = _tile_filename(tid)
        path = out_dir / fname
        im.save(path)
        loc = tile_map.get(tid, {})
        entries["tiles"].append(
            {
                "file": fname,
                "tile_id": tid,
                "name": TILE_NAMES.get(tid),
                "sheet_index": loc.get("sheet_index"),
                "jetpack_file": loc.get("file"),
                "slot_in_file": loc.get("slot_in_file"),
            }
        )

    for i, si in enumerate(load_player_frames()):
        spr = sheet_sprite(atlas, si)
        if spr is None:
            continue
        fname = f"player_fly_{i}.png"
        spr.save(out_dir / fname)
        entries["player"].append(
            {"file": fname, "frame": i, "sheet_index": si, "role": "fly"},
        )

    for enemy_id, si in sorted(load_enemy_sheet_indices().items()):
        spr = sheet_sprite(atlas, si)
        if spr is None:
            continue
        etype = ENEMY_NAMES.get(enemy_id, f"type_{enemy_id}")
        fname = f"enemy_{_safe_slug(etype)}_0.png"
        spr.save(out_dir / fname)
        entries["enemies"].append(
            {
                "file": fname,
                "enemy_id": enemy_id,
                "type": etype,
                "frame": 0,
                "sheet_index": si,
            }
        )

    # TAS-matched pickup not covered by a campaign tile export name (e.g. extra life icon).
    tas_path = out_dir.parent / "tas_reference.json"
    if tas_path.is_file():
        tas = json.loads(tas_path.read_text(encoding="utf-8"))
        for row in tas.get("references", []):
            ref = row.get("reference") or ""
            if ref == "lred.png" and "sheet_index" in row:
                si = int(row["sheet_index"])
                spr = sheet_sprite(atlas, si)
                if spr is not None:
                    fname = "pickup_extra_life_0.png"
                    spr.save(out_dir / fname)
                    entries["pickups"].append(
                        {"file": fname, "sheet_index": si, "tas_reference": ref},
                    )

    payload = {
        "tile_size": 24,
        "frame_count": sum(len(v) for v in entries.values()),
        "frames": entries,
    }
    (out_dir / "index.json").write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    return payload
