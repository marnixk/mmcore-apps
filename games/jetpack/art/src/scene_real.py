"""Title / high-score scenery built from the REAL Jetpack terrain sheet, laid out like a campaign level
(same 12x12 grid, same tile ids, brick/stone end caps picked the way the campaign levels use them).
Everything is native grain; callers scale by whole numbers only (x2 into art units)."""
from common import *
import real
import gen_sprites as GS

# material -> (base id, left-end cap, right-end cap, isolated) ; caps from campaign-level statistics
MAT = {'L': (0x28, 0x30, 0x2E, 0x2F), 'D': (0x32, 0x3A, 0x38, 0x39), 'S': (0x3C, 0x44, 0x42, 0x45)}
SIMPLE = {'.': 0x00, 'H': 0x01, 'g': 0x04, 'c': 0x14, 'p': 0x15, 'v': 0x16, 'e': 0x17, 'f': 0x10, 'F': 0x0F,
          'x': 0x46, 'm': 0x48, 'G': 0x49, 'C': 0x72, 'R': 0x75, 'i': 0x5E, 'M': 0x61, 'y': 0x06, 'k': 0x1B,
          'n': 0x1C, 'w': 0x1D, 'b': 0x19}

def tiles_from_rows(rows):
    """ASCII map -> tile ids. L/D/S get the plain id inside a run and a rounded cap where the run ends
    with open air on that side (only when the cell above is open too, like the campaign data)."""
    H, W = len(rows), len(rows[0])
    g = lambda r, c: rows[r][c] if 0 <= r < H and 0 <= c < W else '#'
    ids = [[0] * W for _ in range(H)]
    for r in range(H):
        for c in range(W):
            ch = rows[r][c]
            if ch in MAT:
                base, lcap, rcap, iso = MAT[ch]
                up_open = g(r - 1, c) in '.gcpvefFHy'
                lo, ro = g(r, c - 1) in '.H', g(r, c + 1) in '.H'
                if up_open and lo and ro: t = iso
                elif up_open and lo: t = lcap
                elif up_open and ro: t = rcap
                else: t = base
                ids[r][c] = t
            else:
                ids[r][c] = SIMPLE[ch]
    return ids

def render_ids(ids, pal=None):
    H, W = len(ids), len(ids[0])
    im = Image.new('RGBA', (W * 12, H * 12))
    for r in range(H):
        for c in range(W):
            im.paste(real.tile(ids[r][c], pal=pal), (c * 12, r * 12))
    return im

def put(im, spr, c, r, dx=0, dy=0):
    im.alpha_composite(spr, (int(c * 12 + dx), int(r * 12 + dy)))

# ---------------------------------------------------------------------------------------------------
# Title map. 683x384: 29x16 cells (x2 = 696x384, shown from x=6). 1280x720: art 640x360 -> 27x15 (from x=4).
TITLE_683 = [
 ".............................",
 ".............................",
 ".............................",
 "SSSS.....................HSSS",
 ".........................H...",
 ".........................H...",
 ".........................H...",
 "LLLLH....................H...",
 "....H....................H...",
 "....H..................LLLLLL",
 "....H........................",
 "....H....DDDD.............xx.",
 "....H....DDDD.............xx.",
 "LLLLLLLLLLLLLLLSSSSLLLLLLLLLL",
 "DDDDDDDDDDDDDDDDDDDDDDDDDDDDD",
 "DDDDDDDDDDDDDDDDDDDDDDDDDDDDD",
]
TITLE_1280 = [
 "...........................",
 "...........................",
 "...........................",
 "SSSS....................HSS",
 "........................H..",
 "........................H..",
 "LLLLH...................H..",
 "....H...................H..",
 "....H................LLLLLL",
 "....H......................",
 "....H....DDDD...........xx.",
 "....H....DDDD...........xx.",
 "LLLLLLLLLLLLLLSSSSLLLLLLLLL",
 "DDDDDDDDDDDDDDDDDDDDDDDDDDD",
 "DDDDDDDDDDDDDDDDDDDDDDDDDDD",
]
for _m in (TITLE_683, TITLE_1280):
    assert len(set(map(len, _m))) == 1, 'ragged map'

def _fix_door_wall(rows):
    return list(rows)

def title_scene(big=False, items=True, zap=True):
    """Native-grain title scene. big=False -> 683x384 layout, True -> 1280x720 layout.
    Returns (image, floor_row, sprite_rects) - the hero is placed by the caller at a larger scale."""
    rows = _fix_door_wall(TITLE_1280 if big else TITLE_683)
    W = len(rows[0]); F = len(rows) - 3
    ids = tiles_from_rows(rows)
    # treasure on the ledges and the floor (real item tiles)
    def item(c, r, t):
        if items: ids[r][c] = t
    item(1, 2, 0x16); item(2, 2, 0x04)                 # vase + gem on the top-left ledge
    item(0, 6 if big else 7, 0x04 - 0) if False else None
    lr = 5 if big else 6                               # row above the left ledge
    item(1, lr, 0x04); item(3, lr, 0x14)
    Lc = W - 3 if big else W - 4                      # right-hand ladder column
    item(Lc - 2, 7 if big else 8, 0x04); item(Lc + 1, 7 if big else 8, 0x04); item(W - 1, 7 if big else 8, 0x15)
    item(W - 4, 2, 0x17) if False else None
    item(10, F - 3, 0x04); item(12, F - 3, 0x0F)       # on the dark-brick block
    item(7, F - 1, 0x04); item(16, F - 1, 0x14)
    item(W - 2, F - 3, 0x04)                            # on the crates
    im = render_ids(ids)
    # door (opaque 24x24, closed frame 0) on the top-right ledge
    im.paste(real.door(0), ((W - (2 if big else 3)) * 12, 1 * 12))
    # enemies, at their real init / walk frames
    put(im, real.enemy(0), 2, F - 1)                    # steel ball rolling on the floor
    put(im, real.enemy(13), 14, F - 1)                  # spring bobbing on the floor
    put(im, real.enemy(36), W - 7, 6 if big else 7)    # flitzer
    put(im, real.enemy(19), 2, 4 if big else 5)        # homer
    put(im, real.enemy(42), W - 2, 5 if big else 6, dy=-2)  # batbot
    put(im, real.enemy(29), 18 if big else 20, F - 3, dy=3) if False else None
    put(im, real.enemy(10), W - (3 if big else 4), 5 if big else 6)     # trackbot climbing the ladder
    if zap:
        # phaser in use, as in the original: real pose 12 (its own short blue fan) right next to a brick
        # wall; the lower brick is GENERATED half-melted (pixels removed from real tile 0x28). A trackbot
        # walks away two cells further on, separate and untouched.
        zc = W - 9
        put(im, real.player(12), zc, F - 1)
        im.paste(real.tile(0x28), ((zc + 1) * 12, (F - 2) * 12))          # real brick 0x28
        im.paste(real.tile(0x00), ((zc + 1) * 12, (F - 1) * 12))          # backdrop behind the melt
        put(im, GS.sprite('brick_melt_1'), zc + 1, F - 1)
        put(im, real.enemy(4), zc + 4, F - 1)                              # trackbot, walk right frame 4
    return im, F, ids

def hs_scene(big=False):
    """High-score backdrop: same map family, different props, no zap scene."""
    rows = _fix_door_wall(TITLE_1280 if big else TITLE_683)
    W = len(rows[0]); F = len(rows) - 3
    ids = tiles_from_rows(rows)
    lr = 5 if big else 6
    for (c, r, t) in [(1, 2, 0x15), (2, 2, 0x04), (1, lr, 0x04), (3, lr, 0x16), (W - (5 if big else 6), lr + 2, 0x14),
                      (W - (2 if big else 3), lr + 2, 0x04), (W - 1, lr + 2, 0x04), (7, F - 1, 0x04), (W - 2, F - 3, 0x17)]:
        ids[r][c] = t
    im = render_ids(ids)
    im.paste(real.door(7), ((W - (2 if big else 3)) * 12, 12))          # the door stands open on the score screen
    put(im, real.enemy(5), 3, F - 1)                     # trackbot
    put(im, real.enemy(41), 2, 4 if big else 5)          # flitzer
    put(im, real.enemy(14), W - 7, F - 1, dy=-3)         # spring
    put(im, real.enemy(1), W - 4, F - 1)                 # steel ball
    put(im, real.player(49), W - (3 if big else 4), 4)                   # player climbing the ladder
    return im, F, ids

def gameplay_field(n=0, jet=True):
    """Real campaign level n (312x192 native) frozen mid-play: door, enemies at spawn,
    player jetting (real frame 8) a little above the start cell."""
    lv = real.level(n)
    im = real.level_image(n, player_frame=None)
    px, py = int(lv['player_x']) - 1, int(lv['player_y']) - 1
    if jet:
        put(im, real.player(8), px - 1, py - 2, dy=-3)
    else:
        put(im, real.player(0), px, py)
    return im
