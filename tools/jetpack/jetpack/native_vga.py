"""Native 640×400 VGA framebuffer helpers (no resampling)."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .constants import MAP_H, MAP_W, TILE_PX

# Jetpack play mode surface (DOSBox-X log: surface 640×400).
NATIVE_WIDTH = 640
NATIVE_HEIGHT = 400
PLAYFIELD_W = MAP_W * TILE_PX  # 624
PLAYFIELD_H = MAP_H * TILE_PX  # 384
# Default origin: centred horizontally, playfield at top (16px HUD strip below).
DEFAULT_ORIGIN = (9, 0)

_GEOM_PATH = Path(__file__).resolve().parents[1] / "reference" / "playfield_geometry.json"


@dataclass(frozen=True)
class PlayfieldGeometry:
    native_size: tuple[int, int]
    origin: tuple[int, int]
    tile_px: int
    map_size: tuple[int, int]

    @property
    def crop_box(self) -> tuple[int, int, int, int]:
        ox, oy = self.origin
        return ox, oy, ox + PLAYFIELD_W, oy + PLAYFIELD_H

    def to_dict(self) -> dict:
        return {
            "native_resolution": list(self.native_size),
            "playfield_origin": list(self.origin),
            "tile_px": self.tile_px,
            "playfield_size": [PLAYFIELD_W, PLAYFIELD_H],
            "map_tiles": [MAP_W, MAP_H],
        }


def load_geometry() -> PlayfieldGeometry:
    if _GEOM_PATH.is_file():
        data = json.loads(_GEOM_PATH.read_text(encoding="utf-8"))
        ox, oy = data["playfield_origin"]
        nw, nh = data["native_resolution"]
        return PlayfieldGeometry((nw, nh), (ox, oy), TILE_PX, (MAP_W, MAP_H))
    return PlayfieldGeometry((NATIVE_WIDTH, NATIVE_HEIGHT), DEFAULT_ORIGIN, TILE_PX, (MAP_W, MAP_H))


def save_geometry(geom: PlayfieldGeometry) -> None:
    _GEOM_PATH.parent.mkdir(parents=True, exist_ok=True)
    _GEOM_PATH.write_text(json.dumps(geom.to_dict(), indent=2) + "\n", encoding="utf-8")


def is_native_frame(im: Image.Image) -> bool:
    return im.size == (NATIVE_WIDTH, NATIVE_HEIGHT)


def playfield_from_native(im: Image.Image, geom: PlayfieldGeometry | None = None) -> Image.Image:
    """Extract 624×384 playfield RGB without scaling."""
    g = geom or load_geometry()
    rgb = im.convert("RGB")
    if rgb.size != g.native_size:
        raise ValueError(f"expected native {g.native_size}, got {rgb.size}")
    return rgb.crop(g.crop_box)


def is_title_menu(native_rgb: Image.Image) -> bool:
    import numpy as np

    arr = np.array(native_rgb.convert("RGB"))
    if arr.shape[:2] != (NATIVE_HEIGHT, NATIVE_WIDTH):
        return False
    sky = arr[40:160, 180:460]
    return float(sky.mean()) > 85.0


def _has_status_bar(arr) -> bool:
    import numpy as np

    hud_full = arr[360:394, 8:632]
    cyan = (hud_full[:, :, 1].astype(np.float32) + hud_full[:, :, 2].astype(np.float32)) / 2.0
    return float(cyan.mean()) > 48.0 and float(cyan.max()) > 100.0


def is_gameplay_frame(native_rgb: Image.Image) -> bool:
    """In-play: dark playfield + bright HUD; not title sky or editor."""
    import numpy as np

    arr = np.array(native_rgb.convert("RGB"))
    if arr.shape[:2] != (NATIVE_HEIGHT, NATIVE_WIDTH):
        return False
    if _has_status_bar(arr):
        return True
    if is_title_menu(native_rgb):
        return False
    hud_left = arr[364:382, 24:220]
    playfield = arr[8:360, 8:632]
    if float(playfield.mean()) > 120.0:
        return False
    return float(hud_left.mean()) > 45.0


def calibrate_origin(level_tiles: list[int], native_rgb: Image.Image) -> tuple[int, int]:
    """Playfield is 624×384 centred in 640×400 (8 px side margin, HUD below)."""
    return DEFAULT_ORIGIN
