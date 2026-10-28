from common import *

LETTERS = {
 'J': ["..####", "....##", "....##", "....##", "##..##", "##..##", ".####."],
 'E': ["######", "##....", "##....", "#####.", "##....", "##....", "######"],
 'T': ["######", "######", "..##..", "..##..", "..##..", "..##..", "..##.."],
 'P': ["#####.", "##..##", "##..##", "#####.", "##....", "##....", "##...."],
 'A': [".####.", "##..##", "##..##", "######", "##..##", "##..##", "##..##"],
 'C': [".#####", "##....", "##....", "##....", "##....", "##....", ".#####"],
 'K': ["##..##", "##.##.", "####..", "###...", "####..", "##.##.", "##..##"],
}

def letter_mask(ch, cw, ch_h):
    g = LETTERS[ch]
    rows, cols = len(g), len(g[0])
    W, H = cols * cw, rows * ch_h
    m = Image.new('L', (W, H), 0)
    d = ImageDraw.Draw(m)
    filled = lambda r, c: 0 <= r < rows and 0 <= c < cols and g[r][c] == '#'
    cut = max(2, cw // 2)
    for r in range(rows):
        for c in range(cols):
            if not filled(r, c): continue
            x0, y0 = c * cw, r * ch_h
            d.rectangle([x0, y0, x0 + cw - 1, y0 + ch_h - 1], fill=255)
    # chamfer convex corners
    for r in range(rows):
        for c in range(cols):
            if not filled(r, c): continue
            x0, y0, x1, y1 = c * cw, r * ch_h, c * cw + cw - 1, r * ch_h + ch_h - 1
            if not filled(r - 1, c) and not filled(r, c - 1):
                d.polygon([(x0, y0), (x0 + cut - 1, y0), (x0, y0 + cut - 1)], fill=0)
            if not filled(r - 1, c) and not filled(r, c + 1):
                d.polygon([(x1, y0), (x1 - cut + 1, y0), (x1, y0 + cut - 1)], fill=0)
            if not filled(r + 1, c) and not filled(r, c - 1):
                d.polygon([(x0, y1), (x0 + cut - 1, y1), (x0, y1 - cut + 1)], fill=0)
            if not filled(r + 1, c) and not filled(r, c + 1):
                d.polygon([(x1, y1), (x1 - cut + 1, y1), (x1, y1 - cut + 1)], fill=0)
    # fill concave corners (inner fillets) for smoother diagonals
    for r in range(rows):
        for c in range(cols):
            if filled(r, c): continue
            x0, y0, x1, y1 = c * cw, r * ch_h, c * cw + cw - 1, r * ch_h + ch_h - 1
            k = cut // 2
            if filled(r - 1, c) and filled(r, c - 1) and filled(r - 1, c - 1):
                d.polygon([(x0, y0), (x0 + k, y0), (x0, y0 + k)], fill=255)
            if filled(r - 1, c) and filled(r, c + 1) and filled(r - 1, c + 1):
                d.polygon([(x1, y0), (x1 - k, y0), (x1, y0 + k)], fill=255)
            if filled(r + 1, c) and filled(r, c - 1) and filled(r + 1, c - 1):
                d.polygon([(x0, y1), (x0 + k, y1), (x0, y1 - k)], fill=255)
            if filled(r + 1, c) and filled(r, c + 1) and filled(r + 1, c + 1):
                d.polygon([(x1, y1), (x1 - k, y1), (x1, y1 - k)], fill=255)
    return m

STEEL = [(0.0, hx('ffffff')), (0.07, hx('e6eefc')), (0.19, hx('c6d4f0')), (0.31, hx('a0b4dc')),
         (0.42, hx('7a90c2')), (0.49, hx('262a58')), (0.55, hx('ffe2a8')), (0.63, hx('f2b878')),
         (0.74, hx('d0845a')), (0.86, hx('9a5658')), (1.0, hx('643a5a'))]
FLAME = [hx('fff6c8'), hx('ffe060'), hx('ffb028'), hx('f06c18'), hx('b8300e')]

def make_logo(word='JETPACK', cw=11, chh=11, gap=8, glow=True, sparkle=True):
    """JETPACK logo. Marnix: no flame streak / yellow exhaust on the logo, so there is no streak option."""
    masks = [letter_mask(c, cw, chh) for c in word]
    lw = sum(m.width for m in masks) + gap * (len(masks) - 1)
    lh = masks[0].height
    pad = 18 if glow else 6
    ext = max(3, cw // 3)  # extrusion depth
    W, H = lw + pad * 2 + ext, lh + pad * 2 + ext
    M = Image.new('L', (W, H), 0)
    x = pad
    xs = []
    for m in masks:
        M.paste(m, (x, pad)); xs.append(x); x += m.width + gap
    out = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    o2 = dilate(M, 2)
    # glow: dithered, layered rings in flame colours (pure pixel art)
    if glow:
        rings = [(12, hx('5a1e5e'), 0.35), (9, hx('8a2a4e'), 0.5), (6, hx('d0501c'), 0.6), (4, hx('f08a28'), 1.0)]
        for r, col, dens in rings:
            g = dilate(M, r)
            g = shift(g, ext // 2, ext // 2)
            gp = g.load(); op = out.load()
            for yy in range(H):
                for xx in range(W):
                    if gp[xx, yy] and (BAYER4[yy % 4][xx % 4] + 0.5) / 16 < dens:
                        op[xx, yy] = col + (255,)
    # extrusion (hard 3D depth) down-right
    for i in range(ext, 0, -1):
        col = hx('1c0c34') if i > 1 else hx('3a1858')
        out.paste(col + (255,), (0, 0), shift(o2, i, i))
    out.paste(hx('0c0a1e') + (255,), (0, 0), o2)  # 2px dark outline
    # face fill (banded steel)
    face = Image.new('RGBA', (W, H)); fp = face.load()
    for yy in range(H):
        t = (yy - pad) / (lh - 1)
        c = band(STEEL, t)
        for xx in range(W):
            fp[xx, yy] = c + (255,)
    out.paste(face, (0, 0), M)
    # bevels
    mp = M.load(); op = out.load()
    for yy in range(1, H - 1):
        for xx in range(1, W - 1):
            if not mp[xx, yy]: continue
            up, lf, dn, rt = mp[xx, yy - 1], mp[xx - 1, yy], mp[xx, yy + 1], mp[xx + 1, yy]
            if not up or not lf:
                op[xx, yy] = (255, 255, 255, 255)
            elif not dn or not rt:
                op[xx, yy] = hx('3a3c62') + (255,)
            elif (not mp[xx, yy - 2] if yy >= 2 else False) or (not mp[xx - 2, yy] if xx >= 2 else False):
                # second highlight row (soft)
                r, g_, b, a = op[xx, yy]
                op[xx, yy] = (min(255, r + 40), min(255, g_ + 40), min(255, b + 40), 255)
    if sparkle:
        for (sx, sy, s) in [(xs[0] + 2 * cw + 3, pad + 3, 4), (xs[5] + 4 * cw, pad + 3, 3), (xs[3] + 5, pad + lh - 6, 3)]:
            draw_sparkle(out, sx, sy, s)
    return out

def draw_sparkle(img, x, y, s, col=(255, 255, 255), mid=(190, 220, 255)):
    d = ImageDraw.Draw(img)
    d.line([(x - s, y), (x + s, y)], fill=mid + (255,))
    d.line([(x, y - s), (x, y + s)], fill=mid + (255,))
    d.line([(x - s // 2, y), (x + s // 2, y)], fill=col + (255,))
    d.line([(x, y - s // 2), (x, y + s // 2)], fill=col + (255,))

if __name__ == '__main__':
    lg = make_logo()
    print(lg.size)
    bg = Image.new('RGBA', lg.size, hx('1a1f52') + (255,))
    bg.alpha_composite(lg)
    scale(bg, 3).save('/tmp/logo_test.png')
