"""Index-accurate playfield from the mode-13h sprite sheet and dumped DAC.

The running game (BIOS mode 13h, chain-4, linear A000) draws each map cell as a
12×12 sprite. The sheet lives in conventional memory at stride 320, 20 columns,
pitch 13 (12 pixels plus a column/row of palette index 0x66). Tile id N is
cell N. Captures at 640×400 are that playfield doubled; the crop origin is
(9, 0) because column 8 is a black gutter.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np
from PIL import Image

from .constants import MAP_H, MAP_W

VGA_TILE = 12
SHEET_COLS = 20
SHEET_TILES = 120
PLAY_W = MAP_W * VGA_TILE  # 312
PLAY_H = MAP_H * VGA_TILE  # 192
# Doubled DOSBox window: VGA (4, 0) lands at screen (8, 0).
CAPTURE_SCALE = 2

_DUMP = Path(__file__).resolve().parents[1] / "reference" / "vga_dump"
_SHEET = _DUMP / "sheet_12x12.u8"
_DAC = _DUMP / "dac6.bin"


def load_sheet(path: Path | None = None) -> np.ndarray:
    raw = (path or _SHEET).read_bytes()
    need = SHEET_TILES * VGA_TILE * VGA_TILE
    if len(raw) != need:
        raise ValueError(f"sheet is {len(raw)} bytes, expected {need}")
    return np.frombuffer(raw, np.uint8).reshape(SHEET_TILES, VGA_TILE, VGA_TILE)


def load_dac6(path: Path | None = None) -> np.ndarray:
    raw = np.frombuffer((path or _DAC).read_bytes(), np.uint8)
    if raw.shape != (768,):
        raise ValueError(f"DAC is {raw.shape}, expected (768,)")
    return raw.reshape(256, 3)


def dac_to_rgb(dac6: np.ndarray) -> np.ndarray:
    """VGA 6-bit channel to 8-bit the way the DOSBox surface does: ``(v<<2)|(v>>4)``."""
    v = dac6.astype(np.uint8)
    return ((v << 2) | (v >> 4)).astype(np.uint8)


def render_indices(tile_ids: list[int], sheet: np.ndarray | None = None) -> np.ndarray:
    """26×16 map → 192×312 palette indices. Unknown ids stay 0."""
    if sheet is None:
        sheet = load_sheet()
    out = np.zeros((PLAY_H, PLAY_W), np.uint8)
    for row in range(MAP_H):
        for col in range(MAP_W):
            tid = tile_ids[row * MAP_W + col]
            if not 0 <= tid < sheet.shape[0]:
                continue
            y, x = row * VGA_TILE, col * VGA_TILE
            out[y : y + VGA_TILE, x : x + VGA_TILE] = sheet[tid]
    return out


def indices_to_rgb(indices: np.ndarray, rgb: np.ndarray, scale: int = 1) -> np.ndarray:
    img = rgb[indices]
    if scale == 1:
        return img
    return np.repeat(np.repeat(img, scale, axis=0), scale, axis=1)


def render_level_rgb(
    tile_ids: list[int],
    sheet: np.ndarray | None = None,
    rgb: np.ndarray | None = None,
    scale: int = CAPTURE_SCALE,
) -> Image.Image:
    if rgb is None:
        rgb = dac_to_rgb(load_dac6())
    idx = render_indices(tile_ids, sheet)
    return Image.fromarray(indices_to_rgb(idx, rgb, scale), "RGB")


def framebuffer_playfield(dump: bytes | Path) -> np.ndarray:
    """192×312 index crop from a JPVG dump (origin VGA x=4, y=0)."""
    raw = dump if isinstance(dump, (bytes, bytearray)) else Path(dump).read_bytes()
    if raw[:4] != b"JPVG":
        raise ValueError("not a JPVG dump")
    fb = np.frombuffer(raw[45 + 768 : 45 + 768 + 64000], np.uint8).reshape(200, 320)
    return fb[0:PLAY_H, 4 : 4 + PLAY_W].copy()
