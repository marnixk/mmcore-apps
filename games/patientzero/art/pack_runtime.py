#!/usr/bin/env python3
"""Pack Patient Zero sheets so they fit an mmcore page, and emit includes/atlas.inc.

Sprites are repacked into a 16-column grid (the approved sheet is one row too
tall for MODE 11 / MODE 16). Other sheets stay intact and are placed on one
atlas. 720 and 1080 copies are flattened onto black so BLIT's colour key works.
"""
from __future__ import annotations

import json
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.abspath(os.path.join(HERE, ".."))
GFX = os.path.join(GAME, "gfx")
INC = os.path.join(GAME, "includes", "atlas.inc")

sys.path.insert(0, HERE)
import logo  # noqa: E402

FACING = ("down", "up", "right", "left")
KINDS = ("zombie", "office", "hoodie", "dancer", "medic")

# Font rows. Screen pixels are these colours scaled nearest-neighbour.
SMALL_COLORS = (
    (212, 222, 236),
    (255, 210, 70),
    (120, 180, 244),
    (150, 162, 184),
    (232, 80, 80),
    (84, 94, 118),
)
BIG_COLORS = (
    (255, 210, 70),
    (212, 222, 236),
    (150, 162, 184),
)


def flatten(im: Image.Image) -> Image.Image:
    bg = Image.new("RGBA", im.size, (0, 0, 0, 255))
    bg.alpha_composite(im.convert("RGBA"))
    return bg.convert("RGB")


def load_base(name: str) -> Image.Image:
    return Image.open(os.path.join(GFX, "base", name)).convert("RGBA")


def sheet_frames() -> dict:
    with open(os.path.join(GFX, "sheets.json"), encoding="utf-8") as fh:
        return json.load(fh)


def sprite_list() -> tuple[list[str], dict[str, int]]:
    names: list[str] = []

    def add(name: str) -> int:
        names.append(name)
        return len(names) - 1

    for face in FACING:
        for frame in range(4):
            add(f"jerry_{face}_{frame}")
    ids = {
        "S_JERRY": 0,
        "S_JERRY_IDLE": add("jerry_idle_0"),
    }
    add("jerry_idle_1")
    ids["S_JERRY_CHARGE"] = add("jerry_charge1")
    add("jerry_charge2")
    ids["S_JERRY_SNEEZE"] = add("jerry_sneeze1")
    add("jerry_sneeze2")
    ids["S_JERRY_SNEEZE_R"] = add("jerry_sneeze_r")
    ids["S_JERRY_SNEEZE_L"] = add("jerry_sneeze_l")
    ids["S_JERRY_HURT"] = add("jerry_hurt")
    for kind in KINDS:
        ids["S_" + kind.upper()] = len(names)
        for face in FACING:
            for frame in range(4):
                add(f"{kind}_{face}_{frame}")
        add(f"{kind}_hit_0")
        add(f"{kind}_hit_1")
        add(f"{kind}_cure_0")
        add(f"{kind}_cure_1")
        add(f"{kind}_cure_2")
    ids["S_DTHRILL"] = add("dancer_thriller_0")
    add("dancer_thriller_1")
    ids["S_DTOE_R"] = add("dancer_toe_r")
    ids["S_DTOE_L"] = add("dancer_toe_l")
    ids["S_DMOON_R"] = add("dancer_moonwalk_r_0")
    add("dancer_moonwalk_r_1")
    add("dancer_moonwalk_r_2")
    add("dancer_moonwalk_r_3")
    ids["S_DMOON_L"] = add("dancer_moonwalk_l_0")
    add("dancer_moonwalk_l_1")
    add("dancer_moonwalk_l_2")
    add("dancer_moonwalk_l_3")
    ids["S_MMASK_D"] = add("medic_mask_down")
    ids["S_MMASK_R"] = add("medic_mask_right")
    ids["S_MMASK_L"] = add("medic_mask_left")
    return names, ids


def pack_sprites(frames: dict, names: list[str]) -> Image.Image:
    src = load_base("sprites.png")
    cols = 16
    rows = (len(names) + cols - 1) // cols
    img = Image.new("RGBA", (cols * 20, rows * 30), (0, 0, 0, 0))
    table = frames["sprites"]["frames"]
    for i, name in enumerate(names):
        x, y, w, h = table[name]
        cell = src.crop((x, y, x + w, y + h))
        img.alpha_composite(cell, ((i % cols) * 20, (i // cols) * 30))
    return img


def compose_title() -> Image.Image:
    frames = sheet_frames()
    sheets = {
        name: load_base(name + ".png")
        for name in ("sprites", "angels", "fx", "logo", "title_bg")
    }

    def fr(sheet: str, name: str) -> Image.Image:
        x, y, w, h = frames[sheet]["frames"][name]
        return sheets[sheet].crop((x, y, x + w, y + h))

    def blit(img: Image.Image, sheet: str, name: str, x: int, y: int) -> None:
        im = fr(sheet, name)
        img.alpha_composite(im, (int(x), int(y)))

    def big(im: Image.Image, k: int = 2) -> Image.Image:
        return im.resize((im.width * k, im.height * k), Image.Resampling.NEAREST)

    img = sheets["title_bg"].copy()
    for y in range(0, 300, 20):
        beam = big(fr("fx", "beam"), 2)
        img.alpha_composite(beam, (490, y))
    img.alpha_composite(big(fr("fx", "beam_glow"), 2), (490, 282))
    img.alpha_composite(big(fr("angels", "angel_1"), 2), (500, 160))
    img.alpha_composite(big(fr("sprites", "cure_2"), 2), (510, 230))
    import random

    rng = random.Random(3)
    mx, my = 304, 250
    puffs = []
    for _ in range(30):
        t = rng.random() ** 0.8
        x = mx - 20 - t * 230
        spread = 8 + t * 42
        y = my + rng.uniform(-spread, spread * 0.8)
        name = f"puff16_{rng.randrange(4)}" if t < 0.25 else rng.choice(
            ["puff24_0", "puff24_1", "puff32_0", "puff16_1"]
        )
        puffs.append((y, x, name))
    for y, x, name in sorted(puffs):
        im = big(fr("fx", name), 2)
        img.alpha_composite(im, (int(x - im.width / 2), int(y - im.height / 2)))
    for _ in range(70):
        t = rng.random()
        x = mx - t * 260
        spread = 4 + t * 55
        y = my + rng.uniform(-spread, spread)
        col = rng.choice(["white", "pale", "blue"])
        sz = rng.choice([1, 2, 3, 4, 6])
        blit(img, "fx", f"part_{col}_{sz}", int(x - 4), int(y - 4))
    for name, x, y in (
        ("zombie_right_0", 20, 226),
        ("zombie_right_2", 70, 232),
        ("zombie_hit_1", 120, 224),
        ("zombie_right_1", 40, 248),
    ):
        img.alpha_composite(big(fr("sprites", name), 2), (x, y))
    img.alpha_composite(big(fr("sprites", "jerry_sneeze_l"), 2), (300, 228))
    lg = fr("logo", "logo")
    img.alpha_composite(lg, ((640 - lg.width) // 2, 26))
    return img


def glyph_image(ch: str, color: tuple[int, int, int], scale: int) -> Image.Image:
    from font import G

    rows = G.get(ch, G["?"])
    w, h = 6 * scale, 8 * scale
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px = im.load()
    for yy, row in enumerate(rows):
        for xx, bit in enumerate(row):
            if bit == "1":
                for dy in range(scale):
                    for dx in range(scale):
                        px[xx * scale + dx, yy * scale + dy] = color + (255,)
    return im


def build_font(scale: int, over: Image.Image, clear: Image.Image) -> tuple[Image.Image, dict]:
    chars = [chr(c) for c in range(32, 96)]
    small_w, small_h = 6 * scale, 8 * scale
    big_w, big_h = 12 * scale, 16 * scale
    big_x = 100 * scale
    width = max(big_x + 16 * big_w, 16 * small_w)
    head_y = 210 * scale
    over_s = over.resize((over.width * scale, over.height * scale), Image.Resampling.NEAREST)
    clear_s = clear.resize((clear.width * scale, clear.height * scale), Image.Resampling.NEAREST)
    height = max(head_y + over_s.height + 8 * scale + clear_s.height, 6 * 4 * small_h)
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    for ci, color in enumerate(SMALL_COLORS):
        for i, ch in enumerate(chars):
            g = glyph_image(ch, color, scale)
            x = (i % 16) * small_w
            y = (ci * 4 + i // 16) * small_h
            img.alpha_composite(g, (x, y))
    for ci, color in enumerate(BIG_COLORS):
        for i, ch in enumerate(chars):
            g = glyph_image(ch, color, scale * 2)
            x = big_x + (i % 16) * big_w
            y = (ci * 4 + i // 16) * big_h
            img.alpha_composite(g, (x, y))
    img.alpha_composite(over_s, (0, head_y))
    img.alpha_composite(clear_s, (0, head_y + over_s.height + 4 * scale))
    meta = {
        "FNT_BIG_X": 100,
        "HD_OVER_Y": 210,
        "HD_OVER_W": over.width,
        "HD_OVER_H": over.height,
        "HD_CLEAR_Y": 210 + over.height + 4,
        "HD_CLEAR_W": clear.width,
        "HD_CLEAR_H": clear.height,
    }
    return img, meta


def atlas_layout(frames: dict) -> dict[str, tuple[int, int]]:
    """Base-pixel origin of each source sheet on the atlas page."""
    return {
        "angels": (0, 0),
        "fx": (128, 0),
        "logo": (128, 56),
        "tiles": (0, 168),
        "tiles_levels": (220, 168),
        "props": (0, 260),
        "props_levels": (260, 260),
    }


def other_frames(frames: dict) -> list[tuple[str, str, str]]:
    """(const, sheet, frame) in an order the game relies on (contiguous runs)."""
    items: list[tuple[str, str, str]] = []

    def add(const: str, sheet: str, frame: str) -> None:
        items.append((const, sheet, frame))

    for i in range(4):
        add(f"F_ASPHALT_{i}", "tiles", f"asphalt_{i}")
    add("F_ASPHALT_CRACK", "tiles", "asphalt_crack")
    add("F_ASPHALT_CRACK_1", "tiles", "asphalt_crack_1")
    add("F_ROAD", "tiles", "road_line")
    add("F_MANHOLE", "tiles", "manhole")
    add("F_DECAL_0", "tiles", "rubble_decal_0")
    add("F_DECAL_1", "tiles", "rubble_decal_1")
    add("F_WALK_0", "tiles", "sidewalk_0")
    add("F_WALK_1", "tiles", "sidewalk_1")
    add("F_CURB_TOP", "tiles", "curb_top")
    add("F_CURB_BOT", "tiles", "curb_bottom")
    for i in range(3):
        add(f"F_ROOF_{i}", "tiles", f"facade_roof_{i}")
    add("F_WIN_0", "tiles", "facade_window_0")
    add("F_WIN_1", "tiles", "facade_window_1")
    add("F_DOOR", "tiles", "facade_door")
    add("F_BRICK", "tiles", "facade_brick")
    add("F_FBASE", "tiles", "facade_base")
    add("F_WALL_TOP", "tiles", "wall_top")
    add("F_WALL", "tiles", "wall_front")
    add("F_BARRIER", "tiles", "barrier")
    add("F_BIN", "tiles", "bin")
    add("F_CRATE", "tiles", "crate")
    add("F_RUBBLE_0", "tiles", "rubble_pile_0")
    add("F_RUBBLE_1", "tiles", "rubble_pile_1")
    for i in range(4):
        add(f"F_PUDDLE_S_{i}", "tiles", f"puddle_s_{i}")
    for i in range(4):
        add(f"F_PUDDLE_L_{i}", "props", f"puddle_l_{i}")
    for i in range(4):
        add(f"F_PUDDLE_W_{i}", "props", f"puddle_w_{i}")
    add("F_CAR", "props", "car")
    add("F_LAMP", "props", "lamp")
    add("F_BARREL_0", "props", "barrel_0")
    add("F_BARREL_1", "props", "barrel_1")
    add("F_HFLOOR_0", "tiles_levels", "hosp_floor_0")
    add("F_HFLOOR_1", "tiles_levels", "hosp_floor_1")
    add("F_HWALL", "tiles_levels", "hosp_wall")
    add("F_HWALL_TOP", "tiles_levels", "hosp_wall_top")
    add("F_DOOR_OPEN", "tiles_levels", "ward_door_open")
    add("F_DOOR_SHUT", "tiles_levels", "ward_door_shut")
    add("F_WARD_SIGN", "tiles_levels", "ward_sign")
    add("F_PLAT_0", "tiles_levels", "platform_0")
    add("F_PLAT_1", "tiles_levels", "platform_1")
    add("F_EDGE", "tiles_levels", "platform_edge")
    add("F_TRACK_0", "tiles_levels", "track_0")
    add("F_TRACK_1", "tiles_levels", "track_1")
    add("F_STAIRS", "tiles_levels", "stairs")
    add("F_RAIN_0", "tiles_levels", "rain_0")
    add("F_RAIN_1", "tiles_levels", "rain_1")
    add("F_SPLASH", "tiles_levels", "splash")
    add("F_BED", "props_levels", "bed")
    add("F_IV", "props_levels", "iv")
    add("F_PILLAR", "props_levels", "pillar")
    add("F_TRAIN", "props_levels", "train")
    add("F_TRAIN_DARK", "props_levels", "train_dark")
    for kind in KINDS:
        for i in range(4):
            const = "F_ANG" if kind == "zombie" and i == 0 else ""
            add(const, "angels", f"{kind}_angel_{i}")
    # fx used at runtime, contiguous where the game adds an index
    for col in ("white", "pale", "blue", "deep"):
        for sz in (2, 4, 6):
            const = "F_PART" if col == "white" and sz == 2 else ""
            add(const, "fx", f"part_{col}_{sz}")
    for i in range(4):
        add("F_PUFF16" if i == 0 else "", "fx", f"puff16_{i}")
    add("F_PUFF24", "fx", "puff24_0")
    add("", "fx", "puff24_1")
    add("F_PUFF32", "fx", "puff32_0")
    add("F_SPARK", "fx", "sparkle_0")
    add("", "fx", "sparkle_1")
    add("F_SPARK_G", "fx", "sparkle_gold_0")
    add("", "fx", "sparkle_gold_1")
    add("F_BEAM", "fx", "beam")
    add("F_BEAM_GLOW", "fx", "beam_glow")
    for i in range(11):
        add("F_METER" if i == 0 else "", "fx", f"meter_{i}")
    add("F_METER_OVER", "fx", "meter_over")
    for i in range(5):
        add("F_DROP" if i == 0 else "", "fx", f"drop_{i}")
    add("F_HEART", "fx", "heart_full")
    add("F_HEART_EMPTY", "fx", "heart_empty")
    return items


def write_inc(sprite_ids: dict[str, int], s_n: int, items, origins, meta) -> None:
    lines = [
        "' atlas.inc - generated by art/pack_runtime.py. Do not edit.",
        "' Sprite ids 0..S_N-1 live on PG_SPR in a 16-wide grid of 20x30 cells.",
        "' Every later id is on PG_ATLAS. Coordinates are base pixels (multiply by scr.sc).",
        f"const S_N = {s_n}",
    ]
    for name in (
        "S_JERRY",
        "S_JERRY_IDLE",
        "S_JERRY_CHARGE",
        "S_JERRY_SNEEZE",
        "S_JERRY_SNEEZE_R",
        "S_JERRY_SNEEZE_L",
        "S_JERRY_HURT",
        "S_ZOMBIE",
        "S_OFFICE",
        "S_HOODIE",
        "S_DANCER",
        "S_MEDIC",
        "S_DTHRILL",
        "S_DTOE_R",
        "S_DTOE_L",
        "S_DMOON_R",
        "S_DMOON_L",
        "S_MMASK_D",
        "S_MMASK_R",
        "S_MMASK_L",
    ):
        lines.append(f"const {name} = {sprite_ids[name]}")
    f_n = s_n + len(items)
    lines.append(f"const F_N = {f_n}")
    named = []
    for i, (const, _sheet, _frame) in enumerate(items):
        if const:
            named.append((const, s_n + i))
    # contiguous bases the game adds to
    lines.append(f"const F_ANG = {dict(named)['F_ANG']}")
    lines.append(f"const F_PART = {dict(named)['F_PART']}")
    lines.append(f"const F_PUFF16 = {dict(named)['F_PUFF16']}")
    lines.append(f"const F_PUFF24 = {dict(named)['F_PUFF24']}")
    lines.append(f"const F_PUFF32 = {dict(named)['F_PUFF32']}")
    lines.append(f"const F_SPARK = {dict(named)['F_SPARK']}")
    lines.append(f"const F_SPARK_G = {dict(named)['F_SPARK_G']}")
    lines.append(f"const F_BEAM = {dict(named)['F_BEAM']}")
    lines.append(f"const F_BEAM_GLOW = {dict(named)['F_BEAM_GLOW']}")
    lines.append(f"const F_METER = {dict(named)['F_METER']}")
    lines.append(f"const F_METER_OVER = {dict(named)['F_METER_OVER']}")
    lines.append(f"const F_DROP = {dict(named)['F_DROP']}")
    lines.append(f"const F_HEART = {dict(named)['F_HEART']}")
    lines.append(f"const F_HEART_EMPTY = {dict(named)['F_HEART_EMPTY']}")
    for const, val in named:
        if const.startswith("F_ANG") or const in {
            "F_PART",
            "F_PUFF16",
            "F_PUFF24",
            "F_PUFF32",
            "F_SPARK",
            "F_SPARK_G",
            "F_BEAM",
            "F_BEAM_GLOW",
            "F_METER",
            "F_METER_OVER",
            "F_DROP",
            "F_HEART",
            "F_HEART_EMPTY",
        }:
            continue
        lines.append(f"const {const} = {val}")
    for key, val in meta.items():
        lines.append(f"const {key} = {val}")
    lines.append("dim fr.x(319), fr.y(319), fr.w(319), fr.h(319), fr.pg(319)")
    lines.append("dim fr_i")
    lines.append("")
    lines.append("sub fr.init()")
    lines.append("  for fr_i = 0 to S_N - 1")
    lines.append("    fr.x(fr_i) = (fr_i mod 16) * 20")
    lines.append("    fr.y(fr_i) = (fr_i \\ 16) * 30")
    lines.append("    fr.w(fr_i) = 20")
    lines.append("    fr.h(fr_i) = 30")
    lines.append("    fr.pg(fr_i) = PG_SPR")
    lines.append("  next fr_i")
    lines.append("  restore frData")
    lines.append("  for fr_i = S_N to F_N - 1")
    lines.append("    read fr.pg(fr_i), fr.x(fr_i), fr.y(fr_i), fr.w(fr_i), fr.h(fr_i)")
    lines.append("  next fr_i")
    lines.append("end sub")
    lines.append("")
    lines.append("frData:")
    for const, sheet, frame in items:
        ox, oy = origins[sheet]
        x, y, w, h = sheet_frames()[sheet]["frames"][frame]
        # page 3 = PG_ATLAS
        lines.append(f"data 3, {ox + x}, {oy + y}, {w}, {h}")
    lines.append("")
    os.makedirs(os.path.dirname(INC), exist_ok=True)
    with open(INC, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")


def flatten_tree() -> None:
    for res in ("720", "1080"):
        folder = os.path.join(GFX, res)
        for name in os.listdir(folder):
            if not name.endswith(".png"):
                continue
            path = os.path.join(folder, name)
            im = Image.open(path).convert("RGBA")
            flatten(im).save(path)


def main() -> None:
    frames = sheet_frames()
    names, ids = sprite_list()
    packed = pack_sprites(frames, names)
    title = compose_title()
    over = logo.big_text("GAME OVER", 3, ["m3", "m3", "m2", "m2", "m2", "m1", "m1"], inner="m1")
    clear = logo.big_text("LEVEL CLEAR", 3, ["y4", "y3", "y3", "y2", "y2", "y1", "y1"], inner="y1")
    font_meta = None
    for res, scale in (("720", 2), ("1080", 3)):
        folder = os.path.join(GFX, res)
        os.makedirs(folder, exist_ok=True)
        flatten(packed.resize((packed.width * scale, packed.height * scale), Image.Resampling.NEAREST)).save(
            os.path.join(folder, "pack_spr.png")
        )
        flatten(title.resize((title.width * scale, title.height * scale), Image.Resampling.NEAREST)).save(
            os.path.join(folder, "title_scene.png")
        )
        font, font_meta = build_font(scale, over, clear)
        if font.height > (720 if scale == 2 else 1080) or font.width > (1280 if scale == 2 else 1920):
            raise SystemExit(f"font {res} is {font.size}, larger than the page")
        flatten(font).save(os.path.join(folder, "font.png"))
    assert font_meta is not None
    origins = atlas_layout(frames)
    # fit check in base pixels against a 640x360 page
    base_size = {
        "angels": load_base("angels.png").size,
        "fx": load_base("fx.png").size,
        "logo": load_base("logo.png").size,
        "tiles": load_base("tiles.png").size,
        "tiles_levels": load_base("tiles_levels.png").size,
        "props": load_base("props.png").size,
        "props_levels": load_base("props_levels.png").size,
    }
    rects = []
    for name, (x, y) in origins.items():
        w, h = base_size[name]
        rects.append((name, x, y, x + w, y + h))
        if x < 0 or y < 0 or x + w > 640 or y + h > 360:
            raise SystemExit(f"atlas {name} {w}x{h} at {x},{y} does not fit")
    for i, a in enumerate(rects):
        for b in rects[i + 1 :]:
            if a[1] < b[3] and b[1] < a[3] and a[2] < b[4] and b[2] < a[4]:
                raise SystemExit(f"atlas overlap {a[0]} and {b[0]}")
    items = other_frames(frames)
    write_inc(ids, len(names), items, origins, font_meta)
    flatten_tree()
    print(f"sprites {len(names)}  frames {len(names) + len(items)}  font {font_meta}")


if __name__ == "__main__":
    main()
