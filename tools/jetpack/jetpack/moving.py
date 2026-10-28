"""Sprites and palette cycle taken from the mode-13h sheet in conventional memory.

The terrain sheet (cells 0–119) is the committed ``sheet_12x12.u8``. Everything
that moves lives in the same buffer, past those cells, on the same 13-pixel
grid (12 pixels plus a separator of index 0x66). Overlay sprites are drawn by
the transparent blitter (index 0 is skipped). The door is a 24×24 opaque blit
from the right margin.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

from .vga_index import PLAY_H, PLAY_W, VGA_TILE, load_sheet

STRIDE = 320
PITCH = 13
SEPARATOR = 0x66
SPRITE_ROWS = (114, 127, 140, 153, 166, 179)
# Right-margin door frames, 24×24, row-major over these origins.
DOOR_ORIGINS = tuple((y, x) for y in (0, 25, 50, 75) for x in (265, 290))
# Spark / star indices. 247–254 are the DAC cycle; the others are the pixels
# that move across an otherwise static tile in the timed capture.
SPARK_INDICES = frozenset((7, 20, 22, 60, 86, 133, 180, 224, 231, 232)) | frozenset(range(247, 255))


def find_sheet_base(mem: np.ndarray) -> int:
    """Linear address of the sheet's top-left separator pixel.

    The load segment moves the buffer. Tile 0's first row (from the committed
    terrain sheet) plus the separator row 12 pins it.
    """
    tile0 = load_sheet()[0]
    raw = memoryview(mem).tobytes() if not isinstance(mem, bytes) else mem
    if isinstance(mem, np.ndarray):
        raw = mem.tobytes()
    row = tile0[0].tobytes()
    start = 0
    while True:
        found = raw.find(row, start)
        if found < 0:
            raise RuntimeError("terrain tile 0 is not in this memory dump")
        base = found - 1
        sep = base + 12 * STRIDE
        if base >= 0 and raw[sep : sep + 40] == bytes([SEPARATOR]) * 40:
            buf = np.frombuffer(raw, np.uint8)
            if np.array_equal(buf[base : base + VGA_TILE * STRIDE].reshape(VGA_TILE, STRIDE)[:, 1:13], tile0):
                return base
        start = found + 1


def sheet_view(mem: np.ndarray, base: int, rows: int = 210) -> np.ndarray:
    return mem[base : base + rows * STRIDE].reshape(rows, STRIDE)


def cell(buf: np.ndarray, row: int, col: int, size: int = VGA_TILE) -> np.ndarray:
    x = col * PITCH + 1
    return buf[row : row + size, x : x + size].copy()


def door_frame(buf: np.ndarray, index: int) -> np.ndarray:
    y, x = DOOR_ORIGINS[index]
    return buf[y : y + 24, x : x + 24].copy()


def playfield_any(dump: bytes | Path) -> np.ndarray:
    """192×312 crop from a JPVG dump or a JPFR sequence frame."""
    raw = dump if isinstance(dump, (bytes, bytearray)) else Path(dump).read_bytes()
    if raw[:4] == b"JPVG":
        off = 45 + 768
    elif raw[:4] == b"JPFR":
        off = 8 + 768
    else:
        raise ValueError("not a JPVG or JPFR dump")
    fb = np.frombuffer(raw[off : off + 64000], np.uint8).reshape(200, STRIDE)
    return fb[0:PLAY_H, 4 : 4 + PLAY_W].copy()


def masked_hits(sprite: np.ndarray, fb: np.ndarray, max_err: int = 0) -> list[tuple[int, int, int]]:
    """Top-left positions where opaque pixels match ``fb`` within ``max_err``.

    Index 0 is transparent. Returns ``(error, y, x)`` with the lowest error
    first. Positions are deduplicated.
    """
    mask = sprite != 0
    n = int(mask.sum())
    if n < 6:
        return []
    ys, xs = np.where(mask)
    hist = np.bincount(fb.ravel(), minlength=256)
    order = np.argsort(hist[sprite[ys, xs]], kind="stable")
    h, w = sprite.shape
    seen: set[tuple[int, int]] = set()
    found: list[tuple[int, int, int]] = []
    # A few rare anchor colors, so a spark sitting on the rarest pixel
    # does not hide the sprite.
    for k in range(min(4, len(order))):
        ay, ax = int(ys[order[k]]), int(xs[order[k]])
        color = int(sprite[ay, ax])
        cy, cx = np.where(fb == color)
        for y, x in zip((cy - ay).tolist(), (cx - ax).tolist()):
            if y < 0 or x < 0 or y + h > fb.shape[0] or x + w > fb.shape[1]:
                continue
            if (y, x) in seen:
                continue
            err = int((fb[y : y + h, x : x + w][mask] != sprite[mask]).sum())
            if err <= max_err:
                seen.add((y, x))
                found.append((err, y, x))
    found.sort()
    return found


def paint(comp: np.ndarray, sprite: np.ndarray, y: int, x: int, transparent: bool) -> None:
    h, w = sprite.shape
    y1, x1 = min(comp.shape[0], y + h), min(comp.shape[1], x + w)
    if y1 <= y or x1 <= x or y < 0 or x < 0:
        return
    src = sprite[: y1 - y, : x1 - x]
    if transparent:
        mask = src != 0
        comp[y:y1, x:x1][mask] = src[mask]
    else:
        comp[y:y1, x:x1] = src
