"""Decode JETPACK*.DAT sprite blobs and JETPAL.DAT (partial — see FORMATS.md)."""

from __future__ import annotations

import json
import struct
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

TILE_BYTES = 24 * 24


def load_vga_palette(path: Path) -> list[tuple[int, int, int]]:
    raw = path.read_bytes()
    pal: list[tuple[int, int, int]] = [(0, 0, 0)] * 256
    i = 0
    while i + 6 <= len(raw):
        if raw[i] == 0x55:
            i += 1
            continue
        idx, t1, t2 = raw[i], raw[i + 1], raw[i + 2]
        if t1 == 1 and t2 == 0:
            r, g, b = raw[i + 3], raw[i + 4], raw[i + 5]
            pal[idx] = ((r & 63) * 4, (g & 63) * 4, (b & 63) * 4)
            i += 6
        else:
            i += 1
    return pal


def parse_offset_table(data: bytes) -> list[int]:
    offs: list[int] = []
    i = 0
    while i + 2 <= len(data):
        v = struct.unpack_from("<H", data, i)[0]
        if v >= len(data) or (offs and v < offs[-1]):
            break
        offs.append(v)
        i += 2
    return offs


def decompress_rle_zero(blob: bytes, out_size: int = TILE_BYTES) -> bytes:
    """Best-effort RLE used in JETPACK*.DAT (0x00 count value runs)."""
    out = bytearray()
    p = 0
    while p < len(blob) and len(out) < out_size:
        b = blob[p]
        p += 1
        if b == 0:
            if p >= len(blob):
                break
            cnt = blob[p]
            p += 1
            if p >= len(blob):
                break
            val = blob[p]
            p += 1
            out.extend([val] * cnt)
        else:
            out.append(b)
    return bytes(out)


@dataclass
class SpriteIndex:
    file: str
    index: int
    global_id: int


def extract_sprites_from_dat(path: Path, file_label: str, start_gid: int) -> tuple[list[bytes], list[SpriteIndex], int]:
    data = path.read_bytes()
    offs = parse_offset_table(data)
    sprites: list[bytes] = []
    index: list[SpriteIndex] = []
    gid = start_gid
    for j in range(len(offs) - 1):
        size = offs[j + 1] - offs[j]
        if size < 10:
            continue
        dec = decompress_rle_zero(data[offs[j] : offs[j + 1]])
        if len(dec) < TILE_BYTES:
            continue
        sprites.append(dec[:TILE_BYTES])
        index.append(SpriteIndex(file=file_label, index=len(sprites) - 1, global_id=gid))
        gid += 1
    return sprites, index, gid


def export_sprite_sheet(
    game_dir: Path,
    out_png: Path,
    out_json: Path,
    vga_palette: list[tuple[int, int, int]],
) -> dict:
    all_sprites: list[bytes] = []
    meta: list[dict] = []
    gid = 0
    for n in range(6):
        dat = game_dir / f"JETPACK{n}.DAT"
        if not dat.is_file():
            continue
        sprites, idx, gid = extract_sprites_from_dat(dat, f"JETPACK{n}.DAT", gid)
        base = len(all_sprites)
        all_sprites.extend(sprites)
        for ent in idx:
            meta.append(
                {
                    "global_id": ent.global_id,
                    "file": ent.file,
                    "index_in_file": ent.index,
                    "sheet_index": base + ent.index,
                    "width": 24,
                    "height": 24,
                }
            )

    if not all_sprites:
        raise RuntimeError("no sprites decoded")

    cols = 16
    rows = (len(all_sprites) + cols - 1) // cols
    sheet = Image.new("P", (cols * 24, rows * 24))
    pal_flat = [c for rgb in vga_palette for c in rgb][:768]
    sheet.putpalette(pal_flat)
    for i, spr in enumerate(all_sprites):
        tile = Image.new("P", (24, 24))
        tile.putpalette(pal_flat)
        tile.putdata(list(spr))
        x = (i % cols) * 24
        y = (i // cols) * 24
        sheet.paste(tile, (x, y))

    out_png.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out_png)
    payload = {"tile_size": 24, "count": len(all_sprites), "sprites": meta}
    out_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    return payload
