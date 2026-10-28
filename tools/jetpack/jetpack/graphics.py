"""Decode JETPACK*.DAT (SCLM LZ77) and JETPAL.DAT — see FORMATS.md."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .constants import MAP_H, MAP_W, TILE_PX

DAT_HEADER = 512
DAT_STRIDE = 514
DAT_BLOB_BYTES = 512
TILE_PIXELS = TILE_PX * TILE_PX  # 24×24 indexed
TILE_RLE_OUT = 576  # bytes after SCLM decompress (verified vs DOS level-0 preview)

# Slots per JETPACKn.DAT (512-byte blob + 2-byte pad each), v1.5 freeware
DAT_SLOT_COUNTS = (83, 58, 64, 74, 39, 33)


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


def sclm_lz77_decompress(src: bytes, out_size: int = TILE_RLE_OUT) -> bytes:
    """LZ77 bitstream from Adept SCLM.DAT offset 0x87 (used for JETPACK*.DAT blobs)."""
    out = bytearray()
    si = 0
    while len(out) < out_size and si < len(src):
        control = src[si]
        si += 1
        bits = control
        for _ in range(8):
            if len(out) >= out_size:
                break
            bit = bits & 1
            bits >>= 1
            # SCLM 0x87: SHR sets CF from the low bit; JB (CF=1) is a literal.
            if bit == 1:
                if si >= len(src):
                    break
                out.append(src[si])
                si += 1
            else:
                if si + 1 >= len(src):
                    break
                b0, b1 = src[si], src[si + 1]
                si += 2
                length = ((b1 >> 4) & 0x0F) + 2
                offset = (b0 | (b1 << 8)) & 0xFFF
                start = len(out) - offset
                for _ in range(length):
                    if len(out) >= out_size:
                        break
                    if 0 <= start < len(out):
                        out.append(out[start])
                        start += 1
                    else:
                        out.append(0)
    return bytes(out[:out_size])


def decode_tile_blob(blob: bytes) -> bytes:
    raw = sclm_lz77_decompress(blob[:DAT_BLOB_BYTES], TILE_RLE_OUT)
    return raw[:TILE_PIXELS]


def _slot_offset(file_index: int, slot_index: int) -> int:
    return DAT_HEADER + slot_index * DAT_STRIDE


def resolve_tile_slot(tile_id: int) -> tuple[int, int] | None:
    if tile_id < 0:
        return None
    remaining = tile_id
    for file_index, count in enumerate(DAT_SLOT_COUNTS):
        if remaining < count:
            return file_index, remaining
        remaining -= count
    return None


@dataclass
class TileAtlas:
    game_dir: Path
    palette: list[tuple[int, int, int]]
    _cache: dict[tuple[int, bool], Image.Image]

    @classmethod
    def open(cls, game_dir: Path) -> TileAtlas:
        pal_path = game_dir / "JETPAL.DAT"
        palette = load_vga_palette(pal_path) if pal_path.is_file() else [(0, 0, 0)] * 256
        return cls(game_dir=game_dir, palette=palette, _cache={})  # type: ignore[arg-type]

    def _read_blob(self, file_index: int, slot_index: int) -> bytes:
        path = self.game_dir / f"JETPACK{file_index}.DAT"
        data = path.read_bytes()
        off = _slot_offset(file_index, slot_index)
        return data[off : off + DAT_BLOB_BYTES]

    def tile_pixels(self, tile_id: int) -> bytes | None:
        loc = resolve_tile_slot(tile_id)
        if loc is None:
            return None
        fi, si = loc
        return decode_tile_blob(self._read_blob(fi, si))

    def tile_image(self, tile_id: int, transparent: bool = True) -> Image.Image | None:
        key = (tile_id, transparent)
        if key in self._cache:
            return self._cache[key]
        px = self.tile_pixels(tile_id)
        if px is None:
            return None
        flat = [c for rgb in self.palette for c in rgb][:768]
        im = Image.new("P", (TILE_PX, TILE_PX))
        im.putpalette(flat)
        im.putdata(list(px))
        if transparent:
            rgba = Image.new("RGBA", (TILE_PX, TILE_PX))
            for i, idx in enumerate(px):
                x, y = i % TILE_PX, i // TILE_PX
                if idx == 0:
                    rgba.putpixel((x, y), (0, 0, 0, 0))
                else:
                    r, g, b = self.palette[idx]
                    rgba.putpixel((x, y), (r, g, b, 255))
            im = rgba
        self._cache[key] = im
        return im


def export_sprite_sheet(
    game_dir: Path,
    out_png: Path,
    out_json: Path,
    vga_palette: list[tuple[int, int, int]] | None = None,
) -> dict:
    palette = vga_palette or load_vga_palette(game_dir / "JETPAL.DAT")
    flat = [c for rgb in palette for c in rgb][:768]
    meta: list[dict] = []
    sheet_tiles: list[Image.Image] = []
    sheet_index = 0
    for file_index, count in enumerate(DAT_SLOT_COUNTS):
        dat = game_dir / f"JETPACK{file_index}.DAT"
        if not dat.is_file():
            continue
        data = dat.read_bytes()
        for slot in range(count):
            off = _slot_offset(file_index, slot)
            blob = data[off : off + DAT_BLOB_BYTES]
            px = decode_tile_blob(blob)
            tile = Image.new("P", (TILE_PX, TILE_PX))
            tile.putpalette(flat)
            tile.putdata(list(px))
            sheet_tiles.append(tile)
            meta.append(
                {
                    "sheet_index": sheet_index,
                    "file": f"JETPACK{file_index}.DAT",
                    "slot_in_file": slot,
                    "width": TILE_PX,
                    "height": TILE_PX,
                }
            )
            sheet_index += 1

    if not sheet_tiles:
        raise RuntimeError("no sprites decoded")

    cols = 16
    rows = (len(sheet_tiles) + cols - 1) // cols
    sheet = Image.new("P", (cols * TILE_PX, rows * TILE_PX))
    sheet.putpalette(flat)
    for i, tile in enumerate(sheet_tiles):
        x = (i % cols) * TILE_PX
        y = (i // cols) * TILE_PX
        sheet.paste(tile, (x, y))

    out_png.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out_png)

    tile_map = []
    for tid in range(120):
        loc = resolve_tile_slot(tid)
        if loc is None:
            continue
        fi, si = loc
        global_slot = sum(DAT_SLOT_COUNTS[:fi]) + si
        tile_map.append(
            {
                "tile_id": tid,
                "sheet_index": global_slot,
                "file": f"JETPACK{fi}.DAT",
                "slot_in_file": si,
            }
        )

    payload = {
        "tile_size": TILE_PX,
        "slot_count": len(sheet_tiles),
        "tile_id_map": tile_map,
        "slots": meta,
    }
    out_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    return payload


def render_level_tiles(
    tiles: list[int],
    atlas: TileAtlas,
    background_tile_id: int = 0x3C,
) -> Image.Image:
    """Render 26×16 map with VGA art; index 0 in tiles is transparent."""
    w, h = MAP_W * TILE_PX, MAP_H * TILE_PX
    base = atlas.tile_image(background_tile_id, transparent=False)
    if base is None:
        frame = Image.new("RGBA", (w, h), (0x28, 0x20, 0x38, 255))
    else:
        frame = Image.new("RGBA", (w, h))
        if base.mode != "RGBA":
            base = base.convert("RGBA")
        for y in range(MAP_H):
            for x in range(MAP_W):
                frame.paste(base, (x * TILE_PX, y * TILE_PX))

    for y in range(MAP_H):
        for x in range(MAP_W):
            tid = tiles[y * MAP_W + x]
            if tid == 0:
                continue
            spr = atlas.tile_image(tid, transparent=True)
            if spr is None:
                continue
            if spr.mode != "RGBA":
                spr = spr.convert("RGBA")
            frame.paste(spr, (x * TILE_PX, y * TILE_PX), spr)

    return frame
