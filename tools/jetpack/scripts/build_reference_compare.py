#!/usr/bin/env python3
"""Build tools/jetpack/reference/compare.png (TASVideos vs decoded VGA art)."""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.graphics import TileAtlas, export_sprite_sheet, load_vga_palette

TAS_DIR = ROOT / "reference" / "tasvideos"
OUT_PNG = ROOT / "reference" / "compare.png"
OUT_JSON = ROOT / "gfx" / "tas_reference.json"

# Campaign tile IDs for static map objects (see jetpack/constants.py).
TILE_ID_FOR_REF = {
    "ladder.png": 0x01,
    "emerald_gem.png": 0x04,
    "tank_of_fuel.png": 0x10,
    "stunner.png": 0x13,
    "door.png": None,  # 2×2 door; match sheet only
}


def _bg_color(im: Image.Image) -> tuple[int, int, int]:
    corners = [
        im.getpixel((0, 0)),
        im.getpixel((im.width - 1, 0)),
        im.getpixel((0, im.height - 1)),
        im.getpixel((im.width - 1, im.height - 1)),
    ]
    return Counter(corners).most_common(1)[0][0][:3]


def _mask(cell: Image.Image, bg: tuple[int, int, int]) -> tuple[list[int], int]:
    m: list[int] = []
    opaque = 0
    for y in range(24):
        for x in range(24):
            on = cell.getpixel((x, y))[:3] != bg
            m.append(1 if on else 0)
            opaque += on
    return m, opaque


def _mask_mse(a: list[int], b: list[int]) -> float:
    return sum((x - y) ** 2 for x, y in zip(a, b)) / len(a)


def _sheet_crop(sheet: Image.Image, sheet_index: int) -> Image.Image:
    x = (sheet_index % 16) * 24
    y = (sheet_index // 16) * 24
    return sheet.crop((x, y, x + 24, y + 24))


def _best_sheet_slot(
    sheet: Image.Image, ref_cell: Image.Image, bg: tuple[int, int, int], slot_count: int
) -> tuple[int, float]:
    mref, op = _mask(ref_cell, bg)
    if op < 12:
        return -1, 1.0
    best_si = 0
    best = 1.0
    for si in range(slot_count):
        crop = _sheet_crop(sheet, si)
        mc, op2 = _mask(crop, (0, 0, 0))
        if op2 < 12:
            continue
        s = _mask_mse(mref, mc)
        if s < best:
            best = s
            best_si = si
    return best_si, best


def _pick_ref_cell(ref: Image.Image, bg: tuple[int, int, int]) -> tuple[Image.Image, tuple[int, int]]:
    best: tuple[float, tuple[int, int], Image.Image] | None = None
    for gy in range(ref.height // 24):
        for gx in range(ref.width // 24):
            cell = ref.crop((gx * 24, gy * 24, gx * 24 + 24, gy * 24 + 24))
            _, op = _mask(cell, bg)
            if op < 12:
                continue
            score = op  # prefer fuller frames
            if best is None or score > best[0]:
                best = (score, (gx, gy), cell)
    if best is None:
        cell = ref.crop((0, 0, 24, 24))
        return cell, (0, 0)
    return best[2], best[1]


def _try_font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for name in ("DejaVuSans.ttf", "LiberationSans-Regular.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path, help="jetpak15 directory (JETPACK*.DAT, JETPAL.DAT)")
    args = ap.parse_args()
    game_dir = args.game_dir.expanduser().resolve()

    sheet_path = ROOT / "gfx" / "sprites_vga.png"
    index_path = ROOT / "gfx" / "sprites_index.json"
    if not sheet_path.is_file():
        export_sprite_sheet(
            game_dir,
            sheet_path,
            index_path,
            load_vga_palette(game_dir / "JETPAL.DAT"),
        )

    atlas = TileAtlas.open(game_dir)
    sheet = Image.open(sheet_path).convert("RGBA")
    meta = json.loads(index_path.read_text(encoding="utf-8"))
    slot_count = int(meta.get("slot_count", 351))
    tile_to_sheet = {e["tile_id"]: e["sheet_index"] for e in meta.get("tile_id_map", [])}

    entries: list[dict] = []
    rows: list[tuple[str, Image.Image, Image.Image, str]] = []

    refs = sorted(TAS_DIR.glob("*.png"))
    if not refs:
        print(f"No PNGs in {TAS_DIR}", file=sys.stderr)
        return 1

    for ref_path in refs:
        name = ref_path.name
        ref = Image.open(ref_path).convert("RGBA")
        bg = _bg_color(ref)
        cell, cell_xy = _pick_ref_cell(ref, bg)
        tid = TILE_ID_FOR_REF.get(name)
        label_bits: list[str] = [name.replace(".png", "")]

        if tid is not None and tid in tile_to_sheet:
            si = tile_to_sheet[tid]
            decoded = atlas.tile_image(tid, transparent=True)
            label_bits.append(f"tile 0x{tid:02X} → slot {si}")
            entries.append(
                {
                    "reference": name,
                    "tas_cell": list(cell_xy),
                    "tile_id": tid,
                    "sheet_index": si,
                    "match_mask_mse": None,
                }
            )
        else:
            si, mse = _best_sheet_slot(sheet, cell, bg, slot_count)
            decoded = _sheet_crop(sheet, si)
            label_bits.append(f"best slot {si} (mask MSE {mse:.3f})")
            entries.append(
                {
                    "reference": name,
                    "tas_cell": list(cell_xy),
                    "tile_id": tid,
                    "sheet_index": si,
                    "match_mask_mse": round(mse, 4),
                }
            )

        rows.append((" / ".join(label_bits), cell.convert("RGB"), decoded.convert("RGB"), name))

    # Player animation frames from guy.png (all non-empty cells).
    guy_path = TAS_DIR / "guy.png"
    if guy_path.is_file():
        ref = Image.open(guy_path).convert("RGBA")
        bg = _bg_color(ref)
        frames: list[int] = []
        for gy in range(ref.height // 24):
            for gx in range(ref.width // 24):
                cell = ref.crop((gx * 24, gy * 24, gx * 24 + 24, gy * 24 + 24))
                si, mse = _best_sheet_slot(sheet, cell, bg, slot_count)
                if si >= 0:
                    frames.append(si)
        entries.append({"reference": "guy.png", "player_animation_sheet_indices": frames})

    OUT_JSON.write_text(json.dumps({"references": entries}, indent=2) + "\n", encoding="utf-8")

    row_h = 28
    img_h = 24
    pad = 8
    label_w = 220
    col_w = 24 * 3 + pad  # upscale decoded for visibility
    w = label_w + col_w * 2 + pad * 3
    h = pad + len(rows) * (row_h + img_h + pad) + 24
    out = Image.new("RGB", (w, h), (0x18, 0x18, 0x22))
    draw = ImageDraw.Draw(out)
    font = _try_font(11)
    draw.text((pad, pad), "TASVideos reference (left) vs decoded VGA (right, 3×)", fill=(0xe0, 0xd8, 0xc0), font=font)
    y = pad + 20
    for label, left, right, _ in rows:
        draw.text((pad, y), label, fill=(0xb0, 0xa8, 0xc0), font=font)
        y += row_h
        left3 = left.resize((24 * 3, 24 * 3), Image.Resampling.NEAREST)
        right3 = right.resize((24 * 3, 24 * 3), Image.Resampling.NEAREST)
        out.paste(left3, (label_w, y))
        out.paste(right3, (label_w + col_w + pad, y))
        y += img_h * 3 + pad

    OUT_PNG.parent.mkdir(parents=True, exist_ok=True)
    out.save(OUT_PNG)
    print(f"Wrote {OUT_PNG} and {OUT_JSON}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
