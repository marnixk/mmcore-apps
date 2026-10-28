#!/usr/bin/env python3
"""Build tools/jetpack/reference/compare.png (TASVideos vs DOS-captured art)."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.frame_atlas import FrameAtlas

TAS_DIR = ROOT / "reference" / "tasvideos"
OUT_PNG = ROOT / "reference" / "compare.png"
OUT_JSON = ROOT / "gfx" / "tas_reference.json"

# Prefer these tile IDs when a TAS sheet maps to a campaign object.
TILE_ID_FOR_REF = {
    "ladder.png": 0x01,
    "emerald_gem.png": 0x04,
    "tank_of_fuel.png": 0x10,
    "stunner.png": 0x13,
    "lred.png": 0x3C,
}



def _bg_color(im: Image.Image) -> tuple[int, int, int]:
    from collections import Counter

    corners = [
        im.getpixel((0, 0)),
        im.getpixel((im.width - 1, 0)),
        im.getpixel((0, im.height - 1)),
        im.getpixel((im.width - 1, im.height - 1)),
    ]
    return Counter(corners).most_common(1)[0][0][:3]


def _mask(cell: Image.Image, bg: tuple[int, int, int]) -> list[int]:
    m: list[int] = []
    for y in range(24):
        for x in range(24):
            m.append(1 if cell.getpixel((x, y))[:3] != bg else 0)
    return m


def _mask_mse(a: list[int], b: list[int]) -> float:
    return sum((x - y) ** 2 for x, y in zip(a, b)) / len(a)


def _rgb_mse(ref: Image.Image, cand: Image.Image, ref_bg: tuple[int, int, int]) -> float:
    """MSE on TAS foreground pixels (grayscale matte) vs DOS RGB."""
    ref = ref.convert("RGB")
    cand = cand.convert("RGB")
    err = 0.0
    n = 0
    for y in range(24):
        for x in range(24):
            rp = ref.getpixel((x, y))
            if rp == ref_bg:
                continue
            cp = cand.getpixel((x, y))
            err += sum((a - b) ** 2 for a, b in zip(rp, cp))
            n += 1
    if n < 8:
        return 1.0
    return err / (n * 3)


def _pick_ref_cell(ref: Image.Image, bg: tuple[int, int, int]) -> tuple[Image.Image, tuple[int, int]]:
    best: tuple[float, tuple[int, int], Image.Image] | None = None
    for gy in range(ref.height // 24):
        for gx in range(ref.width // 24):
            cell = ref.crop((gx * 24, gy * 24, gx * 24 + 24, gy * 24 + 24))
            opaque = sum(_mask(cell, bg))
            if opaque < 12:
                continue
            if best is None or opaque > best[0]:
                best = (opaque, (gx, gy), cell)
    if best is None:
        return ref.crop((0, 0, 24, 24)), (0, 0)
    return best[2], best[1]


def _atlas_candidates(atlas: FrameAtlas) -> list[tuple[str, Image.Image]]:
    out: list[tuple[str, Image.Image]] = []
    for tid, im in sorted(atlas.tiles.items()):
        if tid == 0:
            continue
        out.append((f"tile 0x{tid:02X}", im))
    for i, im in enumerate(atlas.player_frames):
        out.append((f"player_fly_{i}", im))
    return out


def _best_match(
    ref_cell: Image.Image,
    bg: tuple[int, int, int],
    candidates: list[tuple[str, Image.Image]],
    prefer_label: str | None = None,
) -> tuple[Image.Image, str, float]:
    if prefer_label:
        for label, im in candidates:
            if label == prefer_label:
                return im, label, _rgb_mse(ref_cell, im, bg)
    best_im = candidates[0][1]
    best_label = candidates[0][0]
    best = 1e9
    for label, im in candidates:
        mse = _rgb_mse(ref_cell, im, bg)
        if mse < best:
            best = mse
            best_im = im
            best_label = label
    return best_im, best_label, float(best)


def _try_font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for name in ("DejaVuSans.ttf", "LiberationSans-Regular.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("game_dir", type=Path, help="jetpak15 directory (unused; kept for CLI compat)")
    args = ap.parse_args()
    _ = args.game_dir

    atlas = FrameAtlas.load(ROOT / "gfx" / "frames")
    candidates = _atlas_candidates(atlas)
    if not candidates:
        print("No frame candidates in gfx/frames", file=sys.stderr)
        return 1

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
        prefer = f"tile 0x{tid:02X}" if tid is not None and atlas.tile_image(tid) else None
        if name == "guy.png" and atlas.player_frames:
            decoded = atlas.player_frames[0]
            label = "player_fly_0"
            mse = _rgb_mse(cell, decoded, bg)
        else:
            decoded, label, mse = _best_match(cell, bg, candidates, prefer_label=prefer)
        label_bits = [name.replace(".png", ""), f"{label} (mask MSE {mse:.3f})"]
        entries.append(
            {
                "reference": name,
                "tas_cell": list(cell_xy),
                "tile_id": tid,
                "dos_match": label,
                "match_mask_mse": round(mse, 4),
                "atlas_source": atlas.source,
            }
        )
        rows.append((" / ".join(label_bits), cell.convert("RGB"), decoded.convert("RGB"), name))

    OUT_JSON.write_text(json.dumps({"references": entries}, indent=2) + "\n", encoding="utf-8")

    row_h = 28
    img_h = 24
    pad = 8
    label_w = 280
    col_w = 24 * 3 + pad
    w = label_w + col_w * 2 + pad * 3
    h = pad + len(rows) * (row_h + img_h + pad) + 24
    out = Image.new("RGB", (w, h), (0x18, 0x18, 0x22))
    draw = ImageDraw.Draw(out)
    font = _try_font(11)
    draw.text(
        (pad, pad),
        f"TASVideos (left) vs DOS capture art (right, 3×) — source: {atlas.source}",
        fill=(0xe0, 0xd8, 0xc0),
        font=font,
    )
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
