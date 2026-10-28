"""Runtime sprite sheet indices (from TASVideos reference matching)."""

from __future__ import annotations

import json
from pathlib import Path

_PKG_ROOT = Path(__file__).resolve().parents[1]
_TAS_REF = _PKG_ROOT / "gfx" / "tas_reference.json"

# Default player animation slots (guy.png cells) when JSON not generated yet.
_DEFAULT_PLAYER_FRAMES = (182, 309, 182, 293)

_REF_TO_ENEMY = {
    "trackbot.png": 1,
    "ball.png": 2,
    "missile.png": 4,
}


def load_player_frames() -> tuple[int, ...]:
    if not _TAS_REF.is_file():
        return _DEFAULT_PLAYER_FRAMES
    data = json.loads(_TAS_REF.read_text(encoding="utf-8"))
    for entry in data.get("references", []):
        if entry.get("reference") == "guy.png":
            frames = entry.get("player_animation_sheet_indices")
            if frames:
                return tuple(int(x) for x in frames)
    return _DEFAULT_PLAYER_FRAMES


def load_enemy_sheet_indices() -> dict[int, int]:
    out: dict[int, int] = {}
    if not _TAS_REF.is_file():
        return {1: 239, 2: 131, 4: 198}
    data = json.loads(_TAS_REF.read_text(encoding="utf-8"))
    for entry in data.get("references", []):
        ref = entry.get("reference")
        if ref in _REF_TO_ENEMY and "sheet_index" in entry:
            si = int(entry["sheet_index"])
            if 0 <= si < 351:
                out[_REF_TO_ENEMY[ref]] = si
    return out


def sheet_sprite(atlas, sheet_index: int | None):
    """Return RGBA 24×24 from exported sheet index."""
    from .graphics import TileAtlas  # noqa: F401

    if sheet_index is None or sheet_index < 0:
        return None
    if not isinstance(atlas, TileAtlas):
        raise TypeError("atlas must be TileAtlas")
    path = _PKG_ROOT / "gfx" / "sprites_vga.png"
    if not path.is_file():
        return None
    from PIL import Image

    sheet = Image.open(path).convert("RGBA")
    x = (sheet_index % 16) * 24
    y = (sheet_index // 16) * 24
    return sheet.crop((x, y, x + 24, y + 24))
