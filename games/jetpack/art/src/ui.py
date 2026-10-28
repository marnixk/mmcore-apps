from common import *

PANEL_IN = hx('22104a'); PANEL_IN2 = hx('2a1658')
STEEL_L, STEEL_M, STEEL_D = hx('d4dcec'), hx('8c94b0'), hx('4a506c')
OUTL = hx('0a0818'); RED = hx('d03030')

def panel(w, h, dither=True):
    """Framed menu panel: dark outline, 3 px steel bevel with rivets, red inner line, purple inside."""
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, w - 1, h - 1], fill=OUTL)
    d.rectangle([1, 1, w - 2, h - 2], fill=STEEL_M)
    d.line([(1, 1), (w - 2, 1)], fill=STEEL_L); d.line([(1, 1), (1, h - 2)], fill=STEEL_L)
    d.line([(1, h - 2), (w - 2, h - 2)], fill=STEEL_D); d.line([(w - 2, 1), (w - 2, h - 2)], fill=STEEL_D)
    d.line([(2, 2), (w - 3, 2)], fill=hx('b0b8cc'))
    d.rectangle([4, 4, w - 5, h - 5], fill=OUTL)
    d.rectangle([5, 5, w - 6, h - 6], fill=RED)
    d.rectangle([6, 6, w - 7, h - 7], fill=PANEL_IN)
    if dither:
        px = im.load()
        for y in range(7, h - 7):
            for x in range(7, w - 7):
                if (x + y) % 2 == 0 and y < 7 + (h - 14) * 0.45 and BAYER4[y % 4][x % 4] < 8 * (1 - (y - 7) / ((h - 14) * 0.45)):
                    px[x, y] = PANEL_IN2 + (255,)
    for (x, y) in [(2, 2), (w - 4, 2), (2, h - 4), (w - 4, h - 4)]:
        d.rectangle([x, y, x + 1, y + 1], fill=hx('ffffff'))
        d.point((x + 1, y + 1), fill=STEEL_D)
    return im

def hilite_bar(w, h=14):
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, w - 1, h - 1], fill=hx('a8341a'))
    d.line([(0, 0), (w - 1, 0)], fill=hx('ffb22c'))
    d.line([(0, 1), (w - 1, 1)], fill=hx('f2701c'))
    d.line([(0, h - 1), (w - 1, h - 1)], fill=hx('5a1210'))
    # dithered fade to the right end
    px = im.load()
    for y in range(2, h - 1):
        for x in range(w - 24, w):
            if BAYER4[y % 4][x % 4] / 16 < (x - (w - 24)) / 24 * 0.8:
                px[x, y] = hx('7a2214') + (255,)
    return im

def ribbon(s, fg=hx('ffe9a0')):
    w = tw(s) + 24; h = 15
    im = Image.new('RGBA', (w + 12, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # tails
    for sx in (0, w + 2):
        d.rectangle([sx, 3, sx + 9, h - 2], fill=OUTL)
        d.rectangle([sx + 1, 4, sx + 8, h - 3], fill=hx('7a1a18'))
    d.polygon([(0, 3), (3, 8), (0, h - 2)], fill=(0, 0, 0, 0))
    d.polygon([(w + 11, 3), (w + 8, 8), (w + 11, h - 2)], fill=(0, 0, 0, 0))
    d.rectangle([6, 0, w + 5, h - 1], fill=OUTL)
    d.rectangle([7, 1, w + 4, h - 2], fill=hx('c23414'))
    d.rectangle([7, 3, w + 4, h - 4], fill=hx('3a1450'))
    text(im, 6 + 12, 4, s, fg, shadow=OUTL)
    return im
