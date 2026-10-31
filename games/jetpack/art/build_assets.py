#!/usr/bin/env python3
"""Build games/jetpack gfx, level data and layout constants.

Whole-number nearest-neighbour only. The logo is the approved art (no flame
streak). In-game melt frames are the original dissolve strip from a running
VGA dump (art/real/dissolve_12.u8), not the generated brick_melt cells.
Those generated cells are only the half-eaten brick on the title scene.

Run from anywhere:
    python3 games/jetpack/art/build_assets.py
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ART = Path(__file__).resolve().parent
ROOT = ART.parent
sys.path.insert(0, str(ART / "src"))
import real  # noqa: E402
import screens as S  # noqa: E402
import title as T  # noqa: E402

GFX = ROOT / "gfx"
DATA = ROOT / "data"
INC = ROOT / "includes"

# screen, mode, tile scale of the 12px cell, font/UI scale, title source
# title: ("art", W, H, k) draws title(W,H) then nearest x k
#        ("place", ox, oy) centres the 683 title on a filled screen
LAYOUTS = [
    ("683x384", 20, 683, 384, 2, 1, ("art", 683, 384, 1)),
    ("1366x768", 19, 1366, 768, 4, 2, ("art", 683, 384, 2)),
    ("1280x720", 11, 1280, 720, 3, 2, ("art", 640, 360, 2)),
    ("1920x1080", 16, 1920, 1080, 5, 3, ("art", 640, 360, 3)),
    ("800x600", 1, 800, 600, 2, 1, ("place", 683, 384)),
]

# direction bit: 1 right, 2 left, 4 up, 8 down. Reject lists are (id-0x50) mod 20.
REJECT = {
    1: {1, 5, 8, 9, 0xA, 0xC, 0xD, 0x10, 0x13},          # right
    2: {1, 3, 6, 7, 0xA, 0xB, 0xC, 0x10, 0x13},          # left
    4: {0, 4, 7, 8, 0xB, 0xC, 0xD, 0x10, 0x13},          # up
    8: {0, 2, 6, 9, 0xA, 0xB, 0xD, 0x10, 0x13},          # down
}

# kind codes consumed by tiles.inc / play.inc
K_PASS, K_SOLID, K_LADDER, K_ESC = 0, 1, 2, 3
K_GEM, K_FUEL1, K_FUEL2, K_CHARGE, K_DRAIN = 4, 5, 6, 7, 8
K_COIN, K_PILE, K_VASE, K_LIFE, K_STUN = 9, 10, 11, 12, 13
K_TELY, K_TELP, K_TELG = 14, 15, 16
K_SWB, K_SWR, K_SWC = 17, 18, 19
K_CONVL, K_CONVR, K_TUBE = 20, 21, 22


def phase_class(tid: int) -> int:
    """0 light, 1 dark, 2 crate, 3 purple, -1 never."""
    if tid == 0x46:
        return 2
    if tid == 0x47:
        return 3
    # 0x1E dissolved in the scripted capture; 0x18-0x27 are the same brick row.
    band = (0x18 <= tid <= 0x3B) or (0x50 <= tid <= 0x77)
    if not band:
        return -1
    dark = (
        (0x32 <= tid <= 0x3B)
        or (0x64 <= tid <= 0x71)
        or tid in (0x5F, 0x62, 0x73, 0x76)
        or tid >= 0x76
    )
    return 1 if dark else 0


def phase_mask(tid: int) -> int:
    cls = phase_class(tid)
    if cls < 0:
        return 0
    if 0x50 <= tid <= 0x77:
        di = (tid - 0x50) % 20
        mask = 0
        for bit, bad in REJECT.items():
            if di not in bad:
                mask |= bit
        return mask
    return 15


def kind_of(tid: int) -> int:
    table = {
        0x00: K_PASS, 0x05: K_PASS,
        0x01: K_LADDER, 0x02: K_ESC, 0x03: K_ESC,
        0x04: K_GEM,
        0x06: K_TELY, 0x07: K_TELP, 0x08: K_TELG,
        0x09: K_SWB, 0x0A: K_SWB, 0x0B: K_SWR, 0x0C: K_SWR, 0x0D: K_SWC, 0x0E: K_SWC,
        0x0F: K_FUEL2, 0x10: K_FUEL1, 0x11: K_CHARGE, 0x12: K_DRAIN, 0x13: K_STUN,
        0x14: K_COIN, 0x15: K_PILE, 0x16: K_VASE, 0x17: K_LIFE,
        0x4A: K_TUBE, 0x4B: K_TUBE, 0x4C: K_TUBE, 0x4D: K_TUBE, 0x4E: K_TUBE, 0x4F: K_TUBE,
        0x72: K_CONVL, 0x73: K_CONVL, 0x74: K_CONVL,
        0x75: K_CONVR, 0x76: K_CONVR, 0x77: K_CONVR,
    }
    return table.get(tid, K_SOLID)


def flatten(im: Image.Image, transparent: bool) -> Image.Image:
    im = im.convert("RGBA")
    bg = (0, 0, 0, 255) if transparent else (0, 0, 0, 255)
    out = Image.new("RGBA", im.size, bg)
    out.alpha_composite(im)
    rgb = out.convert("RGB")
    if transparent:
        px = rgb.load()
        src = im.load()
        for y in range(im.height):
            for x in range(im.width):
                r, g, b, a = src[x, y]
                if a < 128:
                    px[x, y] = (0, 0, 0)
                elif (r, g, b) == (0, 0, 0):
                    px[x, y] = (1, 0, 0)
    return rgb


def nn(im: Image.Image, k: int) -> Image.Image:
    if k == 1:
        return im
    return im.resize((im.width * k, im.height * k), Image.NEAREST)


def paste(dst: Image.Image, im: Image.Image, x: int, y: int) -> None:
    dst.paste(im, (x, y))


def build_sheet(k: int, screen: tuple[int, int]) -> tuple[Image.Image, dict]:
    """Pack terrain, player, enemies, door and the original dissolve strip."""
    W, H = screen
    sheet = Image.new("RGB", (W, H), (0, 0, 0))
    y = 0
    terrain = flatten(nn(real_grid([real.tile(t, transparent=False) for t in range(120)], 20), k), False)
    paste(sheet, terrain, 0, y)
    # Conveyor arrow phases. The DOS game rotates palette 251-254 every
    # 3 IRQ ticks (four captured steps). Bake each belt id 114-119 at
    # sheet column 20+phase, row (id-114), beside the terrain block.
    phases = Image.new("RGBA", (4 * 12, 6 * 12), (0, 0, 0, 0))
    for phase in range(4):
        pal = real.palette(phase)
        for t in range(6):
            phases.paste(real.tile(114 + t, transparent=False, pal=pal), (phase * 12, t * 12))
    paste(sheet, flatten(nn(phases, k), False), 20 * 12 * k, y)
    info = {"syTile": y, "tw": terrain.width, "th": terrain.height}
    y += terrain.height
    player = flatten(nn(real_grid([real.player(f) if real.has_player(f) else None for f in range(96)], 24), k), True)
    paste(sheet, player, 0, y)
    info["syPlayer"] = y
    y += player.height
    enemy = flatten(nn(real_grid([real.enemy(f) if f in real.enemy_frames() else None for f in range(48)], 24), k), True)
    paste(sheet, enemy, 0, y)
    info["syEnemy"] = y
    y += enemy.height
    door = Image.new("RGBA", (8 * 24, 24))
    for i in range(8):
        door.paste(real.door(i), (i * 24, 0))
    door = flatten(nn(door, k), False)
    paste(sheet, door, 0, y)
    info["syDoor"] = y
    y += door.height
    diss = np.frombuffer((ART / "real" / "dissolve_12.u8").read_bytes(), np.uint8).reshape(20, 12, 12)
    cells = []
    backdrop = real.tile(0, transparent=False)
    for i in range(20):
        over = real.to_rgba(diss[i], transparent=True)
        base = backdrop.copy()
        base.alpha_composite(over)
        cells.append(base)
    melt = flatten(nn(real_grid(cells, 20), k), False)
    paste(sheet, melt, 0, y)
    info["syMelt"] = y
    info["sheetH"] = y + melt.height
    if info["sheetH"] > H or max(terrain.width, player.width, enemy.width, door.width, melt.width) > W:
        raise SystemExit(f"sheet {info['sheetH']} does not fit {W}x{H}")
    return sheet, info


def real_grid(cells, cols: int) -> Image.Image:
    rows = (len(cells) + cols - 1) // cols
    im = Image.new("RGBA", (cols * 12, rows * 12), (0, 0, 0, 0))
    for i, c in enumerate(cells):
        if c is not None:
            im.paste(c, ((i % cols) * 12, (i // cols) * 12))
    return im


def dim_terrain(k: int) -> Image.Image:
    g = real_grid([real.tile(t, transparent=False) for t in range(120)], 20)
    g = S.dim_colours(g)
    return flatten(nn(g, k), False)


def ui_page(W: int, H: int, fsc: int) -> tuple[Image.Image, dict]:
    page = Image.new("RGB", (W, H), (0, 0, 0))
    font = flatten(nn(Image.open(ART / "assets" / "font_small_8px.png").convert("RGBA"), fsc), True)
    paste(page, font, 0, 0)
    p0 = flatten(nn(S.pause_panel(sel=0, empty=False), fsc), False)
    p1 = flatten(nn(S.pause_panel(sel=1, empty=False), fsc), False)
    y = font.height + 4
    paste(page, p0, 0, y)
    paste(page, p1, p0.width + 8, y)
    icon = flatten(nn(real.player(0), fsc), True)
    ix = 0
    iy = y + p0.height + 4
    if iy + icon.height <= H:
        paste(page, icon, ix, iy)
    else:
        ix, iy = p0.width + 8 + p1.width + 4, y
        paste(page, icon, ix, iy)
    return page, {
        "syFont": 0, "fontW": font.width, "fontH": font.height,
        "panelW": p0.width, "panelH": p0.height,
        "p0x": 0, "p0y": y, "p1x": p0.width + 8, "p1y": y,
        "iconX": ix, "iconY": iy, "icon": icon.width,
    }


def fill_screen(src: Image.Image, W: int, H: int) -> Image.Image:
    """Centre a 683x384 (or other) picture on a screen filled with its corner colour."""
    out = Image.new("RGB", (W, H), src.getpixel((0, 0)))
    # extend with the picture's own edge pixels, then centre
    ox = (W - src.width) // 2
    oy = (H - src.height) // 2
    out.paste(src, (ox, oy))
    return out


def title_image(spec, W: int, H: int) -> Image.Image:
    kind = spec[0]
    if kind == "art":
        _, aw, ah, k = spec
        im = T.title(aw, ah, empty=True).convert("RGB")
        im = nn(im, k)
        if im.size != (W, H):
            raise SystemExit(f"title {im.size} != {W}x{H}")
        return im
    im = T.title(683, 384, empty=True).convert("RGB")
    return fill_screen(im, W, H)


def scores_image(spec, W: int, H: int) -> Image.Image:
    kind = spec[0]
    if kind == "art":
        _, aw, ah, k = spec
        im = S.highscores(aw, ah, empty=True).convert("RGB")
        return nn(im, k)
    im = S.highscores(683, 384, empty=True).convert("RGB")
    return fill_screen(im, W, H)


def menu_geom(spec, W: int, H: int) -> dict:
    """Menu coordinates in screen pixels, matching title()."""
    kind = spec[0]
    if kind == "art":
        _, aw, ah, k = spec
        T.title(aw, ah, empty=True)
        px, py, pw, ph = T.title.panel_rect
        lh = 16 * k
        item0 = (py + 11) * k
        name_y = (py + 11 + 4 * 16 + 1 + 4) * k
        return {
            "mx": px * k, "my": py * k, "mw": pw * k, "mh": ph * k,
            "mlh": lh, "item0": item0, "nameY": name_y,
            "textX": (px + 32) * k, "valR": (px + pw - 18) * k,
            "barX": (px + 8) * k, "barW": (pw - 16) * k, "barH": 14 * k,
        }
    # 683 art placed on the larger screen
    T.title(683, 384, empty=True)
    px, py, pw, ph = T.title.panel_rect
    ox = (W - 683) // 2
    oy = (H - 384) // 2
    return {
        "mx": px + ox, "my": py + oy, "mw": pw, "mh": ph,
        "mlh": 16, "item0": py + 11 + oy, "nameY": py + 11 + 4 * 16 + 5 + oy,
        "textX": px + 32 + ox, "valR": px + pw - 18 + ox,
        "barX": px + 8 + ox, "barW": pw - 16, "barH": 14,
    }


def scores_geom(spec, W: int, H: int) -> dict:
    kind = spec[0]
    if kind == "art":
        _, aw, ah, k = spec
        S.highscores(aw, ah, empty=True)
        px, py, pw, ph = S.highscores.panel_rect
        return {
            "sx": px * k, "sy": py * k, "sw": pw * k, "sh": ph * k,
            "srow": 15 * k, "s0": (py + 30) * k,
            "snum": (px + 18) * k, "sname": (px + 62) * k,
            "slv": (px + 228) * k, "ssc": (px + pw - 20) * k,
        }
    S.highscores(683, 384, empty=True)
    px, py, pw, ph = S.highscores.panel_rect
    ox = (W - 683) // 2
    oy = (H - 384) // 2
    return {
        "sx": px + ox, "sy": py + oy, "sw": pw, "sh": ph,
        "srow": 15, "s0": py + 30 + oy,
        "snum": px + 18 + ox, "sname": px + 62 + ox,
        "slv": px + 228 + ox, "ssc": px + pw - 20 + ox,
    }


def write_levels() -> None:
    DATA.mkdir(parents=True, exist_ok=True)
    lines = ["100"]
    for i in range(100):
        d = json.loads((ART / "real" / "levels" / f"level_{i:02d}.json").read_text())
        name = " ".join(d["name"].split())
        gems = sum(1 for t in d["tiles"] if t == 4)
        lines.append(name if name else "-")
        lines.append(f"{d['door_x']} {d['door_y']} {d['player_x']} {d['player_y']} {gems}")
        ens = d["enemies"]
        lines.append(str(len(ens)))
        if ens:
            lines.append(" ".join(f"{e['type_id']},{e['x']},{e['y']}" for e in ens))
        else:
            lines.append("-")
        hx = "".join(f"{t & 255:02x}" for t in d["tiles"])
        for row in range(8):
            lines.append(hx[row * 104:(row + 1) * 104])
    (DATA / "levels.dat").write_text("\n".join(lines) + "\n")


def write_tiles() -> None:
    kinds = [kind_of(t) for t in range(120)]
    masks = [phase_mask(t) for t in range(120)]
    clss = [phase_class(t) for t in range(120)]
    def rows(vals):
        out = []
        for i in range(0, 120, 20):
            out.append("data " + ",".join(str(v) for v in vals[i:i + 20]))
        return "\n".join(out)
    text = f"""' tiles.inc - generated by art/build_assets.py
' kind: 0 pass 1 solid 2 ladder 3 escalator 4 gem 5 fuel1 6 fuel2
' 7 charger 8 drain 9 coin 10 pile 11 vase 12 life 13 stunner
' 14 tel yellow 15 tel pink 16 tel green 17 sw blue 18 sw red 19 sw cyan
' 20 conveyor left 21 conveyor right 22 tube
' phase mask bits: 1 right 2 left 4 up 8 down (0 = cannot phase)
' phase class: 0 light (26) 1 dark (56) 2 crate (5, gone) 3 purple (5) -1 none
dim tf.k(119), tf.m(119), tf.c(119)

sub tf.init()
  local i
  restore tfKind
  for i = 0 to 119
    read tf.k(i)
  next i
  restore tfMask
  for i = 0 to 119
    read tf.m(i)
  next i
  restore tfClass
  for i = 0 to 119
    read tf.c(i)
  next i
end sub

tfKind:
{rows(kinds)}
tfMask:
{rows(masks)}
tfClass:
{rows(clss)}
"""
    (INC / "tiles.inc").write_text(text)


def write_layouts(rows_src: list[str]) -> None:
    (INC / "layouts.inc").write_text("\n".join(rows_src) + "\n")


def main() -> None:
    GFX.mkdir(parents=True, exist_ok=True)
    INC.mkdir(parents=True, exist_ok=True)
    # assets/ font comes from export; make sure it is there
    if not (ART / "assets" / "font_small_8px.png").exists():
        raise SystemExit("run art/src/export.py first")
    lay_lines = [
        "' layouts.inc - generated by art/build_assets.py. Do not edit.",
        "const LAY_N = 5",
        "dim lay.modes(LAY_N - 1) = (20, 19, 11, 16, 1)",
        "dim lay.ws(LAY_N - 1) = (683, 1366, 1280, 1920, 800)",
        "dim lay.hs(LAY_N - 1) = (384, 768, 720, 1080, 600)",
        "",
        "sub lay.set(i)",
        "  select case i",
    ]
    for idx, (tag, mode, W, H, k, fsc, spec) in enumerate(LAYOUTS):
        tile = k * 12
        sheet, sh = build_sheet(k, (W, H))
        sheet.save(GFX / f"sheet_{tag}.png")
        dim_terrain(k).save(GFX / f"dim_{tag}.png")
        ui, u = ui_page(W, H, fsc)
        ui.save(GFX / f"ui_{tag}.png")
        title_image(spec, W, H).save(GFX / f"title_{tag}.png")
        scores_image(spec, W, H).save(GFX / f"scores_{tag}.png")
        g = menu_geom(spec, W, H)
        s = scores_geom(spec, W, H)
        ox = (W - 26 * tile) // 2
        oy = (H - 16 * tile) // 2
        lay_lines.append(f"    case {idx}  ' {tag}")
        lay_lines.append(
            f"      lay.mode = {mode} : lay.W = {W} : lay.H = {H} : lay.sc = 1 : lay.res$ = \"{tag}\""
        )
        lay_lines.append(f"      lay.tile = {tile} : lay.ox = {ox} : lay.oy = {oy} : lay.fsc = {fsc}")
        lay_lines.append(
            f"      lay.syTile = {sh['syTile']} : lay.syPlayer = {sh['syPlayer']} : lay.syEnemy = {sh['syEnemy']}"
        )
        lay_lines.append(f"      lay.syDoor = {sh['syDoor']} : lay.syMelt = {sh['syMelt']}")
        lay_lines.append(
            f"      lay.mx = {g['mx']} : lay.my = {g['my']} : lay.mw = {g['mw']} : lay.mh = {g['mh']}"
        )
        lay_lines.append(
            f"      lay.mlh = {g['mlh']} : lay.item0 = {g['item0']} : lay.nameY = {g['nameY']}"
        )
        lay_lines.append(
            f"      lay.textX = {g['textX']} : lay.valR = {g['valR']} : lay.barX = {g['barX']} : lay.barW = {g['barW']} : lay.barH = {g['barH']}"
        )
        lay_lines.append(
            f"      lay.sx = {s['sx']} : lay.sy = {s['sy']} : lay.sw = {s['sw']} : lay.sh = {s['sh']}"
        )
        lay_lines.append(
            f"      lay.srow = {s['srow']} : lay.s0 = {s['s0']} : lay.snum = {s['snum']} : lay.sname = {s['sname']}"
        )
        lay_lines.append(f"      lay.slv = {s['slv']} : lay.ssc = {s['ssc']}")
        lay_lines.append(
            f"      lay.fontY = {u['syFont']} : lay.p0x = {u['p0x']} : lay.p0y = {u['p0y']} : lay.p1x = {u['p1x']} : lay.p1y = {u['p1y']}"
        )
        lay_lines.append(
            f"      lay.pw = {u['panelW']} : lay.ph = {u['panelH']} : lay.iconX = {u['iconX']} : lay.iconY = {u['iconY']} : lay.iconS = {u['icon']}"
        )
        print(tag, "sheetH", sh["sheetH"], "field", 26 * tile, 16 * tile, "ox", ox, "oy", oy)
    lay_lines += ["  end select", "end sub", ""]
    write_layouts(lay_lines)
    write_tiles()
    write_levels()
    print("wrote gfx, layouts, tiles, levels")


if __name__ == "__main__":
    main()
