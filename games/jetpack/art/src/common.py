"""Shared helpers for the Jetpack mmcore mockups. All drawing is hard-edged pixel art
(no antialiasing, no blur) in 'art units'; 1280x720 = 640x360 art x2, 683x384 = art x1."""
from PIL import Image, ImageDraw, ImageFont, ImageChops
import random, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = ROOT + '/ref'
FONT_PATH = ROOT + '/fonts/PressStart2P.ttf'

def hx(s):
    s = s.lstrip('#'); return tuple(int(s[i:i+2], 16) for i in (0, 2, 4))

BAYER4 = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]

def font(size=8):
    return ImageFont.truetype(FONT_PATH, size)

def text(img, x, y, s, col, shadow=(8, 6, 20), size=8, sh=1):
    d = ImageDraw.Draw(img); d.fontmode = '1'
    f = font(size)
    if shadow is not None:
        d.text((x + sh, y + sh), s, font=f, fill=shadow)
    d.text((x, y), s, font=f, fill=col)

def tw(s, size=8):
    return len(s) * size

def text_c(img, cx, y, s, col, shadow=(8, 6, 20), size=8, sh=1):
    text(img, cx - tw(s, size) // 2, y, s, col, shadow, size, sh)

def text_r(img, rx, y, s, col, shadow=(8, 6, 20), size=8, sh=1):
    text(img, rx - tw(s, size), y, s, col, shadow, size, sh)

def text_mask(s, size):
    f = font(size)
    m = Image.new('L', (tw(s, size) + 2, size + 2), 0)
    d = ImageDraw.Draw(m); d.fontmode = '1'
    d.text((0, 0), s, font=f, fill=255)
    return m

def dilate(mask, r):
    """square-ish (octagonal) dilation of an L mask by r pixels, hard edged"""
    out = mask.copy()
    for dx in range(-r, r + 1):
        for dy in range(-r, r + 1):
            if abs(dx) + abs(dy) > r + r // 2:  # octagon
                continue
            if dx == 0 and dy == 0: continue
            out = ImageChops.lighter(out, ImageChops.offset(mask, dx, dy))
    return out

def shift(mask, dx, dy):
    m = Image.new('L', mask.size, 0)
    m.paste(mask, (dx, dy))
    return m

def big_text(s, size, stops, outline=(14, 8, 28), shadow=(30, 12, 48), sdepth=2):
    """Big pixel text: banded vertical gradient fill, 1px outline, hard drop shadow."""
    m = text_mask(s, size)
    pad = 4
    W, H = m.width + pad * 2 + sdepth, m.height + pad * 2 + sdepth
    M = Image.new('L', (W, H), 0); M.paste(m, (pad, pad))
    out = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    o1 = dilate(M, 1)
    for i in range(1, sdepth + 1):
        out.paste(shadow + (255,), (0, 0), shift(o1, i, i))
    out.paste(outline + (255,), (0, 0), o1)
    fill = Image.new('RGBA', (W, H))
    px = fill.load()
    for y in range(H):
        t = (y - pad) / max(1, size - 1)
        c = band(stops, t)
        for x in range(W):
            px[x, y] = c + (255,)
    out.paste(fill, (0, 0), M)
    return out

def band(stops, t):
    """stops: list of (t, colour); returns the colour of the band t falls in (hard bands)"""
    t = max(0.0, min(1.0, t))
    c = stops[0][1]
    for st, col in stops:
        if t >= st: c = col
    return c

def paste(dst, src, x, y):
    dst.paste(src, (int(x), int(y)), src)

def scale(img, k):
    return img.resize((img.width * k, img.height * k), Image.NEAREST)

# ---------- reference sprites (TASVideos, 48x48 = 4x of the native 12x12) ----------
_bg = None
def ref_sprite(name, keep_bg=False, native=12):
    """Return the native-resolution sprite with the game's dithered background removed."""
    global _bg
    im = Image.open(f'{REF}/{name}.png').convert('RGB')
    k = im.width // native
    if _bg is None:
        _bg = Image.open(f'{REF}/background.png').convert('RGB')
    out = Image.new('RGBA', (im.width // k, im.height // k), (0, 0, 0, 0))
    bk = _bg.width // 12
    for y in range(out.height):
        for x in range(out.width):
            c = im.getpixel((x * k, y * k))
            b = _bg.getpixel(((x % 12) * bk, (y % 12) * bk))
            if keep_bg or c != b:
                out.putpixel((x, y), c + (255,))
    return out

def ref_tile(name):
    return ref_sprite(name, keep_bg=True)
