"""Detect and normalize in-game playfield from DOSBox screenshots."""

from __future__ import annotations

from dataclasses import dataclass

from PIL import Image

from .constants import MAP_H, MAP_W, TILE_PX

# Default crop inside DOSBox client area (after stripping ~36px menu bar).
DOSBOX_CLIENT_TOP = 36
# Calibrated on dosbox_play_level00.png (client coords, 23 px/tile → 598×368).
DEFAULT_PLAY_CROP = (20, 48, 618, 416)
SCREEN_TILE_PX = 23
SCREEN_PLAY_SIZE = (MAP_W * SCREEN_TILE_PX, MAP_H * SCREEN_TILE_PX)


@dataclass(frozen=True)
class PlayfieldCrop:
    left: int
    top: int
    right: int
    bottom: int

    def as_box(self) -> tuple[int, int, int, int]:
        return self.left, self.top, self.right, self.bottom


def is_playfield_bg(rgb: tuple[int, int, int]) -> bool:
    r, g, b = rgb
    return r < 80 and g < 80 and 45 < b < 110


def dosbox_client_image(im: Image.Image) -> Image.Image:
    rgb = im.convert("RGB")
    if rgb.height > 400:
        return rgb.crop((0, DOSBOX_CLIENT_TOP, rgb.width, rgb.height))
    return rgb


def detect_playfield_crop(im: Image.Image) -> PlayfieldCrop:
    """Playfield crop in DOSBox client coordinates (calibrated on dosbox_play_level00)."""
    l, t, r, b = DEFAULT_PLAY_CROP
    return PlayfieldCrop(l, t, r, b)


def normalize_playfield(im: Image.Image, crop: PlayfieldCrop | None = None) -> Image.Image:
    """Crop DOS capture and resample to canonical 26×16 × 24 px."""
    rgb = dosbox_client_image(im)
    box = (crop or detect_playfield_crop(im)).as_box()
    field = rgb.crop(box)
    if field.size != SCREEN_PLAY_SIZE:
        field = field.resize(SCREEN_PLAY_SIZE, Image.Resampling.NEAREST)
    return field.resize((MAP_W * TILE_PX, MAP_H * TILE_PX), Image.Resampling.NEAREST)
