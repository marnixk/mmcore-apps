"""Title screen, final: real Jetpack scenery + real sprites (whole-number nearest-neighbour scaling only).
Art units: 683x384 is drawn 1:1; 1280x720 is drawn at 640x360 and scaled x2.
Scene = native 12 px grain x2 in art units. Featured hero = real player frame x4 in art units."""
import re
from common import *
from logo import make_logo
from ui import *
import real, scene_real as SR

ATTR = ["INSPIRED BY JETPACK BY ADEPT SOFTWARE (ADEPTSOFTWARE.COM).",
        "ALL IP RIGHTS REMAIN WITH ADEPT SOFTWARE.",
        "ORIGINAL GAME BY ADAM PEDERSEN"]
MENU = ['START GAME', 'LEVEL', 'HIGH SCORES', 'QUIT']
FOOT_BG, FOOT_LINE = hx('0c0a1a'), hx('3a2a5a')

def level_name(n):
    return re.sub(' +', ' ', real.level(n)['name']).strip()

def place_scene(scene, W, H):
    """scale a native scene x2 into art units, top aligned. 683: centred (6 px off each side);
    640 (1280x720): right aligned so the door column is whole (8 art px trimmed on the left)."""
    s = scale(scene, 2)
    ox = (s.width - W) if W == 640 else (s.width - W) // 2
    art = Image.new('RGBA', (W, H), (0, 0, 0, 255))
    art.alpha_composite(s.crop((ox, 0, ox + W, min(H, s.height))))
    return art, ox

def footer(art, top, W, H):
    d = ImageDraw.Draw(art)
    d.rectangle([0, top, W - 1, H - 1], fill=FOOT_BG)
    d.line([(0, top), (W - 1, top)], fill=FOOT_LINE)
    gap = (H - top - 3 * 12) // 2
    cols = [hx('c8c8dc'), hx('c8c8dc'), hx('ffd27a')]
    for i, s in enumerate(ATTR):
        text_c(art, W // 2, top + gap + i * 12 + 1, s, cols[i], shadow=hx('000000'))

def backdrop(W, H, hero_on=True):
    big = (W, H) == (640, 360)
    scene, F, _ = SR.title_scene(big)
    art, ox = place_scene(scene, W, H)
    if hero_on:
        # featured hero: real jet frame 8 (thrusting right, flame inside the cell) at x5 art units
        hero = scale(real.player(8), 5)
        hxp, hyp = (138, 128) if not big else (126, 120)
        paste(art, hero, hxp, hyp)
        backdrop.hero_rect = (hxp, hyp, hero.width, hero.height)
    footer(art, (F + 1) * 24, W, H)
    return art, F * 24

def title(W, H, sel=0, level=1, empty=False):
    art, GY = backdrop(W, H)
    logo = make_logo(cw=10, chh=11, gap=8)
    lx = (W - logo.width) // 2 + 2
    ly = 0 if H <= 360 else 4
    paste(art, logo, lx, ly)
    rb = ribbon('A PORT FOR MMCORE')
    ry = ly + logo.height - 12
    paste(art, rb, (W - rb.width) // 2, ry)
    lh = 16
    pw, ph = 248, 14 + len(MENU) * lh + 4 + 14
    px_, py_ = (W - pw) // 2, ry + rb.height + 6 + (4 if H > 360 else 0)
    paste(art, panel(pw, ph), px_, py_)
    title.panel_rect = (px_, py_, pw, ph)
    if not empty:
        for i, lab in enumerate(MENU):
            iy = py_ + 11 + i * lh
            if i == sel:
                paste(art, hilite_bar(pw - 16, 14), px_ + 8, iy - 3)
                paste(art, real.player(0), px_ + 14, iy - 3)
                col = hx('fff6c8')
            else:
                col = hx('d8d0f4')
            text(art, px_ + 32, iy, lab, col)
            if lab == 'LEVEL':
                text_r(art, px_ + pw - 18, iy, '< %02d >' % level, hx('ffd838'))
        # name of the chosen start level, from JETLEV.DAT
        nm = level_name(level - 1)
        d = ImageDraw.Draw(art)
        yl = py_ + 11 + len(MENU) * lh + 1
        d.line([(px_ + 14, yl), (px_ + pw - 15, yl)], fill=hx('5a3a8a'))
        text_c(art, W // 2, yl + 4, nm, hx('a8a0d0'))
    return art

if __name__ == '__main__':
    import os
    os.makedirs(ROOT + '/final', exist_ok=True)
    scale(title(640, 360), 2).convert('RGB').save(ROOT + '/final/title_1280x720.png')
    title(683, 384).convert('RGB').save(ROOT + '/final/title_683x384.png')
