"""High-score and pause screens, final: real Jetpack scenery and sprites (whole-number scaling only)."""
from common import *
from logo import make_logo
from ui import *
import real, scene_real as SR
from title import place_scene, footer, ATTR

# no difficulty any more: # / NAME / LEVEL / SCORE
SCORES = [('ZIPPY ZOE', 42, 98750), ('CAPTAIN FUEL', 37, 81200), ('GEM GOBLIN', 30, 64415),
          ('ROCKET ROSA', 26, 52980), ('JETSET JIM', 21, 40125), ('PHASE PHIL', 17, 31060),
          ('BRICK BUSTER', 12, 22340), ('HOVER HANNA', 9, 15775), ('NOZZLE NED', 5, 8410), ('TEST PILOT', 2, 2650)]
GOLD = [(0.0, hx('fff6c0')), (0.2, hx('ffe060')), (0.45, hx('ffc030')), (0.6, hx('f08a28')), (0.8, hx('d05a1c'))]
RANK_ITEM = [0x16, 0x15, 0x14]   # real treasure tiles as rank icons: golden vase, pile of gold, gold coin

def highscores(W, H, new_row=3, empty=False):
    big = (W, H) == (640, 360)
    scene, F, _ = SR.hs_scene(big)
    art, ox = place_scene(scene, W, H)
    footer(art, (F + 1) * 24, W, H)
    logo = make_logo(cw=6, chh=6, gap=5, sparkle=False)
    paste(art, logo, (W - logo.width) // 2 + 1, -6)
    hs = big_text('HIGH SCORES', 16, GOLD)
    hy = logo.height - 12
    paste(art, hs, (W - hs.width) // 2 + 2, hy)
    pw = 360; lh = 15
    ph = 30 + len(SCORES) * lh + 8
    px_, py_ = (W - pw) // 2, hy + hs.height + 2
    paste(art, panel(pw, ph), px_, py_)
    highscores.panel_rect = (px_, py_, pw, ph)
    cx = {'#': px_ + 18, 'NAME': px_ + 62, 'LEVEL': px_ + 228, 'SCORE': px_ + pw - 20}
    hy2 = py_ + 13
    head = hx('a898d8')
    text(art, cx['#'], hy2, '#', head); text(art, cx['NAME'], hy2, 'NAME', head)
    text(art, cx['LEVEL'], hy2, 'LEVEL', head); text_r(art, cx['SCORE'], hy2, 'SCORE', head)
    d = ImageDraw.Draw(art)
    d.line([(px_ + 14, hy2 + 12), (px_ + pw - 15, hy2 + 12)], fill=hx('5a3a8a'))
    d.line([(px_ + 14, hy2 + 13), (px_ + pw - 15, hy2 + 13)], fill=hx('120a26'))
    medal = [hx('ffd838'), hx('d8e0f0'), hx('e89a5a')]
    for i, (nm, lv, sc) in enumerate(SCORES if not empty else []):
        y = py_ + 30 + i * lh
        if i == new_row:
            paste(art, hilite_bar(pw - 16, 13), px_ + 8, y - 3)
            paste(art, real.player(0), cx['NAME'] - 16, y - 3)       # the new entry gets the player
        col = hx('fff6c8') if i == new_row else (medal[i] if i < 3 else hx('d8d0f4'))
        text_r(art, cx['#'] + 16, y, str(i + 1), col)
        if i < 3:
            paste(art, real.item(RANK_ITEM[i]), cx['NAME'] - 16, y - 3)
        text(art, cx['NAME'], y, nm + ('_' if i == new_row else ''), col)
        text_r(art, cx['LEVEL'] + 40, y, '%02d' % lv, col if i == new_row else hx('a8a0d0'))
        text_r(art, cx['SCORE'], y, str(sc), hx('ffd838') if i != new_row else col)
    return art

# ---------------- pause: dimmed real level ----------------
def gameplay(W, H, n=0):
    """1280x720: playfield x3 (936x576), 683x384: x2 (624x384); gutters = dark real stone tile."""
    k = 3 if H >= 576 else 2
    lv = scale(SR.gameplay_field(n), k)
    out = Image.new('RGBA', (W, H), (0, 0, 0, 255))
    st = scale(real.tile(0x3C), k)
    st = Image.eval(st.convert('RGB'), lambda v: int(v * 0.35)).convert('RGBA')
    ox, oy = (W - lv.width) // 2, (H - lv.height) // 2
    for y in range(oy % st.height - st.height, H, st.height):
        for x in range(ox % st.width - st.width, W, st.width):
            out.paste(st, (x, y))
    d = ImageDraw.Draw(out)
    d.rectangle([ox - 3, oy - 3, ox + lv.width + 2, oy + lv.height + 2], fill=hx('0a0818'))
    out.alpha_composite(lv, (ox, oy))
    return out

def dim_colours(img):
    """Darkened, blue-shifted copy. In mmcore: draw the frozen field from a pre-dimmed sheet."""
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            px[x, y] = (r * 38 // 100 + 4, g * 38 // 100 + 3, b * 42 // 100 + 12, a)
    return img

PAUSE_ITEMS = ['RESUME', 'QUIT TO TITLE']
# keys from tools/jetpack/GAMEPLAY.md "Controls" + PORT_PLAN.md "Input" (PR #4 @ 30c19ad)
# Alt and Ctrl are not distinct held keys in mmcore KEYDOWN (they return no
# code). Insert and Delete arrive on INKEY$ but are not in the held-key list
# either. Jet is Space / 0, phase is Enter / Z: both are readable with KEYDOWN.
CONTROLS = [
    ('walk',  'ARROWS',     'WALK / CLIMB'),
    ('jet',   'SPACE / 0',  'JETPACK'),
    ('phase', 'ENTER / Z',  'PHASER'),
    ('use',   'DOWN',       'TELEPORT / USE'),
    ('pause', 'P / ESC',    'PAUSE / RESUME'),
    ('quit',  'Q',          'QUIT (IN PAUSE)'),
]

def ctrl_icon(kind):
    """12x12 real sprite icon per control (beam cell generated)."""
    if kind == 'walk': return real.player(1)
    if kind == 'jet': return real.player(8)
    if kind == 'phase': return real.player(12)   # real pose with its fan, no beam
    if kind == 'use': return real.item(0x09)   # real switch tile (DOWN works teleports and switches)
    return None

def pause_panel(sel=0, empty=False):
    pw = 312; lh = 16; cl = 14
    ph = 40 + len(PAUSE_ITEMS) * lh + 10 + len(CONTROLS) * cl + 10
    im = panel(pw, ph)
    t = big_text('PAUSED', 16, GOLD)
    paste(im, t, (pw - t.width) // 2 + 2, 8)
    y0 = 38
    for i, s in enumerate(PAUSE_ITEMS if not empty else []):
        y = y0 + i * lh
        if i == sel:
            paste(im, hilite_bar(pw - 16, 14), 8, y - 3)
            paste(im, real.player(0), 14, y - 3)
        text(im, 32, y, s, hx('fff6c8') if i == sel else hx('d8d0f4'))
    yl = y0 + len(PAUSE_ITEMS) * lh + 2
    d = ImageDraw.Draw(im)
    d.line([(14, yl), (pw - 15, yl)], fill=hx('5a3a8a')); d.line([(14, yl + 1), (pw - 15, yl + 1)], fill=hx('120a26'))
    for i, (kind, k, v) in enumerate(CONTROLS):
        y = yl + 9 + i * cl
        ic = ctrl_icon(kind)
        if ic is not None:
            paste(im, ic, 14, y - 3)
        text(im, 44, y, k, hx('ffd838'))
        text(im, 140, y, v, hx('c8c0e8'))
    return im

def pause_screen(W=1280, H=720):
    g = dim_colours(gameplay(W, H))
    p = scale(pause_panel(), 2 if W >= 1280 else 1)
    paste(g, p, (W - p.width) // 2, (H - p.height) // 2)
    return g

if __name__ == '__main__':
    F = ROOT + '/final'
    scale(highscores(640, 360), 2).convert('RGB').save(F + '/highscores_1280x720.png')
    highscores(683, 384).convert('RGB').save(F + '/highscores_683x384.png')
    pause_screen().convert('RGB').save(F + '/pause_1280x720.png')
    pause_screen(683, 384).convert('RGB').save(F + '/pause_683x384.png')
    gameplay(1280, 720).convert('RGB').save(ROOT + '/_preview/gameplay_real_1280x720.png')
