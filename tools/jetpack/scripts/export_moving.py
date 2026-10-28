#!/usr/bin/env python3
"""Export moving graphics from a conventional-memory dump.

Reproducible capture (not committed):

    python3 scripts/dump_vga_level.py --level 99 --display :240 --out /tmp/lv99.bin
    python3 scripts/dump_vga_level.py --level 7 --display :241 --sequence --out /tmp/seq7.bin

``--sequence`` asks the TSR (``vga_dump.asm``) to write ``F00.BIN``..``F19.BIN``
every 3 IRQ0 ticks, then ``MEM.BIN``. This script reads that memory image and
the single-frame ``JPVG`` dumps. It does not read copyrighted binaries out of
``games/``.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from jetpack.levels import load_jetlev  # noqa: E402
from jetpack.moving import (  # noqa: E402
    DOOR_ORIGINS,
    PITCH,
    SEPARATOR,
    SPARK_INDICES,
    SPRITE_ROWS,
    STRIDE,
    cell,
    door_frame,
    find_sheet_base,
    paint,
    playfield_any,
    sheet_view,
)
from jetpack.vga_index import VGA_TILE  # noqa: E402
from jetpack.vga_index import dac_to_rgb, load_dac6, load_sheet, render_indices  # noqa: E402

# Sheet row, columns, enemy type id (None for the player), and which columns
# were the bitmap actually on a dumped framebuffer. Columns not listed under
# observed are stored in the sheet and did not appear in the captures.
GROUPS = [
    {
        "name": "player_facing_a",
        "row": 114,
        "cols": list(range(18)),
        "enemy_type_id": None,
        "observed_cols": [],
        "note": (
            "Suited figure. Every frame contains at least one of indices "
            "247, 224 or 133 (flame / spark). Not drawn on any dumped frame."
        ),
    },
    {
        "name": "player_facing_b",
        "row": 127,
        "cols": list(range(18)),
        "enemy_type_id": None,
        "observed_cols": [],
        "note": (
            "Second facing. Thirteen of the eighteen frames are the horizontal "
            "mirror of the same column of player_facing_a. Not observed in play."
        ),
    },
    {
        "name": "player_more",
        "row": 140,
        "cols": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 17, 18, 19, 20, 21, 22],
        "enemy_type_id": None,
        "observed_cols": [],
        "note": (
            "Further suited frames. Columns 10–12 use helmet indices 84–89 and "
            "150–152 and do not use index 180. Not observed in play."
        ),
    },
    {
        "name": "player_breakup",
        "row": 153,
        "cols": list(range(24)),
        "enemy_type_id": None,
        "observed_cols": [],
        "note": (
            "Opaque count falls from a full figure to a few pixels, then index-224 "
            "bursts. The nearest thing in the sheet to a death sequence. "
            "Not observed in play. No digging or phaser bitmap is in these rows."
        ),
    },
    {
        "name": "steel_ball",
        "row": 166,
        "cols": [0, 1, 2, 3],
        "enemy_type_id": 2,
        "observed_cols": [0],
        "note": "Four highlight positions. Only column 0 was on screen, for the whole timed capture.",
    },
    {
        "name": "trackbot",
        "row": 166,
        "cols": [4, 5, 6, 7, 8, 9, 10, 11],
        "enemy_type_id": 1,
        "observed_cols": [4],
        "note": "Eight stored frames. Only column 4 was on screen.",
    },
    {
        "name": "spring",
        "row": 166,
        "cols": [12, 13, 14, 15],
        "enemy_type_id": 3,
        "observed_cols": [13],
        "note": "Column 14 is the horizontal mirror of column 13. Only column 13 was on screen.",
    },
    {
        "name": "spikes",
        "row": 166,
        "cols": [16, 17, 18],
        "enemy_type_id": 5,
        "observed_cols": [16],
        "note": "Three shapes, not rotations of one bitmap. Only column 16 was on screen.",
    },
    {
        "name": "homer",
        "row": 166,
        "cols": [19, 20],
        "enemy_type_id": 7,
        "observed_cols": [19],
        "note": (
            "Small spark. Column 19 sits on the level-55 homer. It uses DAC-cycled "
            "indices 247 and 254, so it blinks with the palette cycle. Column 20 was not on screen."
        ),
    },
    {
        "name": "missile_up",
        "row": 166,
        "cols": [21, 22, 23],
        "enemy_type_id": 4,
        "observed_cols": [21],
        "note": (
            "Column 21 has exhaust index 39 on the bottom edge and is every missile "
            "in the level 5, 7 and 99 dumps. Columns 22 and 23 are other exhaust frames "
            "and were not on screen."
        ),
    },
    {
        "name": "missile_other",
        "row": 179,
        "cols": [0, 1, 2, 3, 4, 5, 6, 7, 8],
        "enemy_type_id": 4,
        "observed_cols": [],
        "note": (
            "Column 3 is the vertical flip of missile_up column 21 (exhaust on top). "
            "Column 0 has exhaust index 39 on the left; column 6 is its horizontal mirror. "
            "None of these were the bitmap in the dumps."
        ),
    },
    {
        "name": "flitzer",
        "row": 179,
        "cols": list(range(9, 18)),
        "enemy_type_id": 6,
        "observed_cols": [9],
        "note": "Nine frames, one silhouette. Only column 9 was on screen, and it did not advance.",
    },
    {
        "name": "batbot",
        "row": 179,
        "cols": [18, 19, 20, 21],
        "enemy_type_id": 8,
        "observed_cols": [18],
        "note": "Column 22 duplicates column 20 and column 23 duplicates column 19, so those two are not separate frames. Only column 18 was on screen.",
    },
]

# Map tiles whose bitmap did not change, exported as a one-frame strip.
STATIC_TILES = [
    ("tube_4a", 0x4A),
    ("tube_4b", 0x4B),
    ("tube_4c", 0x4C),
    ("tube_4d", 0x4D),
    ("tube_4e", 0x4E),
    ("tube_4f", 0x4F),
    ("teleport_yellow", 6),
    ("teleport_pink", 7),
    ("teleport_green", 8),
    ("conveyor_left", 0x72),
    ("conveyor_right", 0x75),
    ("gem", 4),
    ("fuel_double", 0x0F),
    ("fuel_single", 0x10),
]


def rgba_of(sprite: np.ndarray, rgb: np.ndarray, transparent: bool) -> Image.Image:
    img = np.zeros(sprite.shape + (4,), np.uint8)
    img[..., :3] = rgb[sprite]
    img[..., 3] = 255
    if transparent:
        img[sprite == 0, 3] = 0
    return Image.fromarray(img, "RGBA")


def strip(frames: list[np.ndarray], rgb: np.ndarray, transparent: bool) -> Image.Image:
    if not frames:
        raise ValueError("empty strip")
    h = max(f.shape[0] for f in frames)
    w = sum(f.shape[1] for f in frames)
    acc = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    x = 0
    for frame in frames:
        acc.paste(rgba_of(frame, rgb, transparent), (x, 0))
        x += frame.shape[1]
    return acc


def palette_steps(sequence: Path) -> dict:
    frames = sorted(sequence.glob("F??.BIN"))
    if len(frames) < 4:
        raise RuntimeError(f"need four JPFR frames in {sequence}")
    steps = []
    ticks = []
    for path in frames[:4]:
        raw = path.read_bytes()
        ticks.append(int.from_bytes(raw[4:6], "little"))
        dac = np.frombuffer(raw[8 : 8 + 768], np.uint8).reshape(256, 3)
        steps.append(dac[247:255].tolist())
    deltas = [ticks[i + 1] - ticks[i] for i in range(3)]
    if deltas != [3, 3, 3]:
        raise RuntimeError(f"palette ticks are {ticks}, expected a step every 3 IRQ0 ticks")
    # The next three frames must be the same four colors, rotated.
    for i, path in enumerate(frames[4:8]):
        dac = np.frombuffer(path.read_bytes()[8 : 8 + 768], np.uint8).reshape(256, 3)
        if dac[247:255].tolist() != steps[i]:
            raise RuntimeError(f"{path.name} did not repeat palette step {i}")
    return {
        "indices": list(range(247, 255)),
        "groups": [[247, 248, 249, 250], [251, 252, 253, 254]],
        "direction": "rotate right inside each group of four",
        "ticks_per_step": 3,
        "period_steps": 4,
        "period_ticks": 12,
        "irq0_hz": 18.2065,
        "source_frames": [p.name for p in frames[:4]],
        "source_ticks": ticks,
        "steps_dac6": steps,
    }


def tube_changes(sequence: Path, level: int) -> dict:
    """Index changes of tube cells across a JPFR sequence. Zero means the bitmap is static."""
    from jetpack.levels import load_jetlev as _load

    frames = sorted(sequence.glob("F??.BIN"))
    levels = _load(ROOT / "_download" / "extracted" / "jetpak15" / "JETLEV.DAT")
    ids = np.array(levels[level].tiles).reshape(16, 26)
    base = playfield_any(frames[0])
    out = {}
    for tid in list(range(0x4A, 0x50)) + [6, 7, 8, 0x72, 0x75]:
        pos = list(zip(*np.where(ids == tid)))
        if not pos:
            out[f"0x{tid:02X}"] = {"present": False}
            continue
        r, c = pos[0]
        changed = 0
        for path in frames[1:]:
            fb = playfield_any(path)
            changed += int(
                (fb[r * 12 : (r + 1) * 12, c * 12 : (c + 1) * 12] != base[r * 12 : (r + 1) * 12, c * 12 : (c + 1) * 12]).sum()
            )
        out[f"0x{tid:02X}"] = {"present": True, "at": [int(r), int(c)], "changed_pixels": changed, "frames": len(frames)}
    return out


def _anchors(sprite: np.ndarray, fb: np.ndarray) -> list[tuple[int, int]]:
    """Candidate top-lefts from colors that occur more than once in the sprite.

    A one-pixel color is often the spark that is not on this frame, so it is
    not used as the anchor.
    """
    mask = sprite != 0
    vals, counts = np.unique(sprite[mask], return_counts=True)
    hist = np.bincount(fb.ravel(), minlength=256)
    ranked = []
    for value, count in zip(vals.tolist(), counts.tolist()):
        if count < 2:
            continue
        ranked.append((hist[value] / count, -count, int(value)))
    ranked.sort()
    found: list[tuple[int, int]] = []
    seen: set[tuple[int, int]] = set()
    h, w = sprite.shape
    for _score, _count, color in ranked[:4]:
        ys, xs = np.where(sprite == color)
        ay, ax = int(ys[0]), int(xs[0])
        cy, cx = np.where(fb == color)
        for y, x in zip((cy - ay).tolist(), (cx - ax).tolist()):
            if y < 0 or x < 0 or y + h > fb.shape[0] or x + w > fb.shape[1]:
                continue
            if (y, x) in seen:
                continue
            seen.add((y, x))
            found.append((y, x))
            if len(found) > 500:
                return found
    return found


def composite_level(buf: np.ndarray, tiles: list[int], door: tuple[int, int], fb: np.ndarray) -> tuple[np.ndarray, dict]:
    comp = render_indices(tiles)
    placed = []
    dx, dy = door
    if dx and dy:
        origin = ((dy - 1) * VGA_TILE, (dx - 1) * VGA_TILE)
        closed = door_frame(buf, 0)
        before = int((comp == fb).sum())
        paint(comp, closed, origin[0], origin[1], transparent=False)
        after = int((comp == fb).sum())
        err = None
        if 0 <= origin[0] and 0 <= origin[1] and origin[0] + 24 <= comp.shape[0] and origin[1] + 24 <= comp.shape[1]:
            err = int((fb[origin[0] : origin[0] + 24, origin[1] : origin[1] + 24] != closed).sum())
        placed.append({"name": "door", "frame": 0, "at": list(origin), "error": err, "gained": after - before})

    candidates = []
    for group in GROUPS:
        if group["enemy_type_id"] is None:
            continue
        for col in group["cols"]:
            sprite = cell(buf, group["row"], col)
            mask = sprite != 0
            opaque = int(mask.sum())
            if opaque < 6:
                continue
            for y, x in _anchors(sprite, fb):
                win_fb = fb[y : y + 12, x : x + 12]
                win_c = comp[y : y + 12, x : x + 12]
                agree = mask & (win_fb == sprite)
                gain = int((agree & (win_c != win_fb)).sum())
                loss = int((mask & (win_fb != sprite) & (win_c == win_fb)).sum())
                uncovered = mask & (win_fb != sprite)
                clipped = int((uncovered & (win_fb == 0)).sum())
                real_miss = int(uncovered.sum()) - clipped
                # A spark on the sprite is a few non-zero misses. A black corner
                # can cover half of a sprite with index 0; that is clipped, not a
                # different frame. Another frame of the same enemy fails this.
                if loss == 0 and gain >= max(6, opaque // 2) and real_miss <= 6:
                    candidates.append((gain, group["name"], col, sprite, y, x, real_miss, clipped))
    candidates.sort(reverse=True)
    taken = np.zeros(fb.shape, np.uint8)
    for gain, name, col, sprite, y, x, real_miss, clipped in candidates:
        mask = sprite != 0
        if int((taken[y : y + 12, x : x + 12][mask]).sum()) > 4:
            continue
        paint(comp, sprite, y, x, transparent=True)
        taken[y : y + 12, x : x + 12][mask] = 1
        placed.append(
            {
                "name": name,
                "col": col,
                "at": [y, x],
                "gained": gain,
                "spark_or_phase_pixels": real_miss,
                "clipped_by_zero": clipped,
            }
        )
    diff = fb != comp
    residual = int(diff.sum())
    spark = int(np.isin(fb[diff], list(SPARK_INDICES)).sum()) if residual else 0
    black = int(((fb == 0) & diff).sum()) if residual else 0
    return comp, {
        "placed": placed,
        "pixels": int(fb.size),
        "mismatch": residual,
        "exact_index_pct": round(100.0 * (1.0 - residual / fb.size), 4),
        "residual_spark_indices": spark,
        "residual_framebuffer_zero": black,
        "residual_other": residual - spark,
    }


def side_by_side(fb: np.ndarray, comp: np.ndarray, rgb: np.ndarray) -> Image.Image:
    a = Image.fromarray(rgb[fb], "RGB").resize((fb.shape[1] * 2, fb.shape[0] * 2), Image.NEAREST)
    b = Image.fromarray(rgb[comp], "RGB").resize((comp.shape[1] * 2, comp.shape[0] * 2), Image.NEAREST)
    gap = 8
    out = Image.new("RGB", (a.width + gap + b.width, a.height), (0, 0, 0))
    out.paste(a, (0, 0))
    out.paste(b, (a.width + gap, 0))
    return out


def contact_sheet(images: list[tuple[str, Image.Image]]) -> Image.Image:
    pad = 8
    label_h = 14
    scale = 3
    width = 0
    height = pad
    scaled = []
    for name, im in images:
        big = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
        scaled.append((name, big))
        width = max(width, big.width + pad * 2)
        height += label_h + big.height + pad
    sheet = Image.new("RGB", (width, height), (24, 24, 28))
    draw = ImageDraw.Draw(sheet)
    y = pad
    for name, big in scaled:
        draw.text((pad, y), name, fill=(230, 230, 230))
        y += label_h
        bg = Image.new("RGB", big.size, (48, 48, 52))
        bg.paste(big, mask=big.split()[-1])
        sheet.paste(bg, (pad, y))
        y += big.height + pad
    return sheet


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mem", type=Path, required=True)
    ap.add_argument("--dumps", required=True, help="level=path pairs, comma separated")
    ap.add_argument("--sequence", type=Path, required=True, help="directory of F00.BIN.. JPFR frames")
    ap.add_argument("--sequence-level", type=int, default=7)
    ap.add_argument("--anim-dir", type=Path, default=ROOT / "gfx" / "anim")
    ap.add_argument("--sprites-json", type=Path, default=ROOT / "gfx" / "sprites.json")
    ap.add_argument("--stats", type=Path, default=ROOT / "reference" / "sprite_match.json")
    ap.add_argument("--artifacts", type=Path, default=None)
    args = ap.parse_args()

    mem = np.frombuffer(args.mem.read_bytes(), np.uint8)
    base = find_sheet_base(mem)
    buf = sheet_view(mem, base)
    rgb = dac_to_rgb(load_dac6())
    terrain = load_sheet()
    args.anim_dir.mkdir(parents=True, exist_ok=True)

    catalog = []
    index_blobs = []
    atlas_at = 0
    contact_rows = []
    mirror_pairs = []
    for col in range(18):
        a = cell(buf, 114, col)
        b = cell(buf, 127, col)
        if np.array_equal(b, a[:, ::-1]):
            mirror_pairs.append(col)

    for group in GROUPS:
        frames = []
        meta_frames = []
        for col in group["cols"]:
            sprite = cell(buf, group["row"], col)
            if int((sprite != 0).sum()) < 6 and group["name"].startswith("player_breakup") is False:
                if int((sprite != 0).sum()) == 0:
                    continue
            if int((sprite != 0).sum()) == 0:
                continue
            frames.append(sprite)
            raw = sprite.tobytes()
            index_blobs.append(raw)
            meta_frames.append(
                {
                    "sheet_row": group["row"],
                    "sheet_col": col,
                    "w": 12,
                    "h": 12,
                    "atlas_offset": atlas_at,
                    "opaque": int((sprite != 0).sum()),
                    "observed": col in group["observed_cols"],
                }
            )
            atlas_at += len(raw)
        if not frames:
            continue
        image = strip(frames, rgb, transparent=True)
        image.save(args.anim_dir / f"{group['name']}.png")
        contact_rows.append((group["name"], image))
        catalog.append(
            {
                "name": group["name"],
                "file": f"gfx/anim/{group['name']}.png",
                "enemy_type_id": group["enemy_type_id"],
                "size": [12, 12],
                "transparency_index": 0,
                "frames": meta_frames,
                "note": group["note"],
            }
        )

    doors = [door_frame(buf, i) for i in range(len(DOOR_ORIGINS))]
    door_img = strip(doors, rgb, transparent=False)
    door_img.save(args.anim_dir / "door.png")
    contact_rows.append(("door (closed, then opening, then open)", door_img))
    door_frames = []
    for i, (sprite, (y, x)) in enumerate(zip(doors, DOOR_ORIGINS)):
        raw = sprite.tobytes()
        index_blobs.append(raw)
        door_frames.append(
            {
                "index": i,
                "sheet_y": y,
                "sheet_x": x,
                "atlas_offset": atlas_at,
                "observed": i == 0,
            }
        )
        atlas_at += len(raw)
    catalog.append(
        {
            "name": "door",
            "file": "gfx/anim/door.png",
            "size": [24, 24],
            "transparency_index": None,
            "blit": "opaque",
            "frames": door_frames,
            "note": (
                "Eight unique 24×24 frames in the right margin. Frame 0 is the closed door "
                "and is the only one on the dumped levels. Opening and open were not on screen, "
                "so the ticks per frame were not measured."
            ),
        }
    )

    for name, tid in STATIC_TILES:
        sprite = terrain[tid]
        image = strip([sprite], rgb, transparent=False)
        image.save(args.anim_dir / f"{name}.png")
        contact_rows.append((name, image))
        raw = sprite.tobytes()
        index_blobs.append(raw)
        catalog.append(
            {
                "name": name,
                "file": f"gfx/anim/{name}.png",
                "tile_id": tid,
                "size": [12, 12],
                "transparency_index": None,
                "frames": [{"sheet_cell": tid, "atlas_offset": atlas_at, "observed": True}],
                "note": "Single bitmap in the terrain sheet. No second phase was found in the buffer.",
            }
        )
        atlas_at += len(raw)

    # Palette swatch: 4 steps × 8 indices.
    cycle = palette_steps(args.sequence)
    swatch = np.zeros((4 * 14, 8 * 14), np.uint8)
    dac = np.array(cycle["steps_dac6"], np.uint8)
    expanded = dac_to_rgb(dac.reshape(-1, 3)).reshape(4, 8, 3)
    sw = np.zeros((4 * 14, 8 * 14, 3), np.uint8)
    for s in range(4):
        for i in range(8):
            sw[s * 14 + 1 : s * 14 + 13, i * 14 + 1 : i * 14 + 13] = expanded[s, i]
    Image.fromarray(sw, "RGB").save(args.anim_dir / "palette_cycle.png")
    contact_rows.append(("palette 247-254, four steps", Image.fromarray(sw, "RGB").convert("RGBA")))

    atlas = b"".join(index_blobs)
    (args.anim_dir / "frames.u8").write_bytes(atlas)

    dumps = {}
    for part in args.dumps.split(","):
        level_s, path_s = part.split("=", 1)
        dumps[int(level_s)] = Path(path_s)
    levels = load_jetlev(ROOT / "_download" / "extracted" / "jetpak15" / "JETLEV.DAT")
    stats = {}
    if args.artifacts:
        args.artifacts.mkdir(parents=True, exist_ok=True)
    for level, path in sorted(dumps.items()):
        fb = playfield_any(path)
        level_obj = levels[level]
        comp, info = composite_level(buf, level_obj.tiles, (level_obj.door_x, level_obj.door_y), fb)
        # The running framebuffer paints the four corners index 0, about one
        # tile and sometimes the cell beside it, on top of whatever the map says.
        corner = np.zeros(fb.shape, dtype=bool)
        for row, col in ((0, 0), (0, 1), (0, 24), (0, 25), (15, 0), (15, 1), (15, 24), (15, 25)):
            block = fb[row * 12 : (row + 1) * 12, col * 12 : (col + 1) * 12]
            if float((block == 0).mean()) > 0.4:
                corner[row * 12 : (row + 1) * 12, col * 12 : (col + 1) * 12] = True
        outside = fb != comp
        outside[corner] = False
        kept = int((~corner).sum())
        info["corner_zero_pixels"] = int(corner.sum())
        info["exact_index_pct_outside_corners"] = round(100.0 * (1.0 - int(outside.sum()) / kept), 4)
        info["enemies"] = [
            {"type_id": e.type_id, "name": e.type_name, "x": e.x, "y": e.y} for e in level_obj.enemies
        ]
        stats[str(level)] = info
        print(
            f"level {level:3d}  exact {info['exact_index_pct']:8.4f}%  "
            f"outside corners {info['exact_index_pct_outside_corners']:8.4f}%  "
            f"mismatch {info['mismatch']:4d}  placed {len(info['placed'])}"
        )
        if args.artifacts:
            side_by_side(fb, comp, rgb).save(args.artifacts / f"side_level_{level:02d}.png")

    document = {
        "sheet": {
            "stride": STRIDE,
            "pitch": PITCH,
            "separator_index": SEPARATOR,
            "cell": VGA_TILE,
            "sprite_rows": list(SPRITE_ROWS),
            "columns": "content at col*13+1, width 12, columns 0..23",
            "base_linear": base,
            "base_note": (
                "Linear address in this dump. It moves with the EXE load segment. "
                "find_sheet_base() relocates it by matching terrain tile 0 and separator row 12."
            ),
            "transparency_index": 0,
            "door_blit": "opaque 24x24",
            "rows_not_exported": "y>=192 is the HIGH SCORE / menu / title bitmap and is not game art",
            "index_atlas": "gfx/anim/frames.u8 concatenates every frame in catalog order. atlas_offset is the byte start; 12x12 frames are 144 bytes and door frames are 576.",
        },
        "palette_cycle": cycle,
        "timed_tile_changes": tube_changes(args.sequence, args.sequence_level),
        "player_mirror_columns": mirror_pairs,
        "not_found": [
            "No dumped frame shows the player. Rows 114, 127, 140 and 153 are the suited figure and a breakup sequence, but none of those bitmaps occur in levels 0, 5, 7, 37, 55, 59, 99 or in 20 frames of levels 7 and 99.",
            "Walking, climbing, jetpack thrust, digging and death were therefore not timed. Every frame of rows 114 and 127 already contains a flame index, so those states are not separate sheets.",
            "No phaser or digging bitmap was found in the sprite rows.",
            "Tubes 0x4A–0x4F are one static vertical gradient each (indices 145–150). The timed level-7 capture changed 0 tube pixels. The gradient row occurs only in the terrain band, not in a second sheet.",
            "Teleports 6/7/8, conveyors 0x72/0x75 and toggles 0x09–0x0E have one bitmap. They were not in the timed capture, and no second phase was found in the buffer.",
            "The door's seven opening frames are stored and were not on screen.",
            "Enemy frames other than the observed column stayed unused for the whole ~3.3 s capture, so ticks per enemy frame were not measured.",
            "Gem tile 4 does not have a second bitmap. A few pixels on top of it change; they are spark indices (180, 22, 60, 86, 150), not a stored gem phase.",
            "Fuel is map tiles 0x0F and 0x10, not an overlay sprite.",
        ],
        "sprites": catalog,
    }
    args.sprites_json.write_text(json.dumps(document, indent=2) + "\n")
    args.stats.parent.mkdir(parents=True, exist_ok=True)
    # Stats without the long placed list duplicated: keep placed, it's the evidence.
    args.stats.write_text(json.dumps({"sheet_base": base, "levels": stats}, indent=2) + "\n")
    if args.artifacts:
        contact_sheet(contact_rows).save(args.artifacts / "sprite_contact.png")
        print(f"contact {args.artifacts / 'sprite_contact.png'}")
    print(f"wrote {args.sprites_json} base {hex(base)} mirror cols {mirror_pairs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
