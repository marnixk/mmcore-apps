"""Playfield extraction from DOSBox native VGA frames (640×400, 24 px/tile, no resample)."""

from __future__ import annotations

from PIL import Image

from .constants import MAP_H, MAP_W, TILE_PX
from .native_vga import (
    PlayfieldGeometry,
    load_geometry,
    playfield_from_native,
    calibrate_origin,
    save_geometry,
    NATIVE_WIDTH,
    NATIVE_HEIGHT,
)


# Legacy window captures (640×480 + menu): only for old reference PNGs.
DOSBOX_CLIENT_TOP = 36
LEGACY_PLAY_CROP = (20, 48, 618, 416)


def normalize_playfield(im: Image.Image) -> Image.Image:
    """Return 624×384 playfield RGB. Native 640×400 is cropped 1:1; legacy shots are rejected."""
    rgb = im.convert("RGB")
    if rgb.size == (NATIVE_WIDTH, NATIVE_HEIGHT):
        return playfield_from_native(rgb)
    if rgb.height > 420 and rgb.width >= 600:
        raise ValueError(
            "legacy scaled DOSBox window capture is no longer supported; "
            "use native 640×400 VGA frames (scripts/dosbox_capture.py)"
        )
    raise ValueError(f"unexpected capture size {rgb.size}")


def detect_playfield_crop(im: Image.Image) -> PlayfieldGeometry:
    return load_geometry()
