"""GENERATED (not original) sprites for the Jetpack mmcore port.

In the original the phaser is NOT a beam: the short blue fan in front of the player is part of the real
phaser pose frames (10-15 right, 34-39 left, 52-57 up, 58-60 down), it reaches about one tile and melts
the brick/wall tile right next to the player. The extracted data has no dissolve or partly-eaten brick
bitmap (sprites.json 'not_found': no separate phaser or digging bitmap; the sheet has one bitmap per
terrain id), so the melting brick here is GENERATED from the real light-brick tile 0x28 by removing
pixels only: removed pixels become index 0 (transparent), so drawn over the real backdrop tile (id 0)
the backdrop shows through. No new colours.

The earlier beam / hit sprites were rejected (see generated/_rejected/).

Outputs (art/generated/):
  brick_melt_{0,1,2}.png      light brick 0x28 eaten ~25 / 50 / 75 % from the LEFT (player standing left, facing right)
  brick_melt_{0,1,2}_x4.png   4x nearest neighbour
  brick_melt_{0,1,2}_on_backdrop.png  composited over the real backdrop tile 0, as it shows in play
  generated_sprites.u8        the 3 cells as raw 12x12 palette indices (144 bytes each, 0 = removed)
  generated_sprites_contact.png  (also in art/final/) real tile + real phaser poses next to the generated cells, 4x
For a player facing left, mirror the cells horizontally.
"""
import os, numpy as np
from PIL import Image, ImageDraw
import real
from common import text, hx

OUT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/generated'
os.makedirs(OUT, exist_ok=True)
BRICK = 0x28

# eaten width per row (from the left edge) for each stage, ragged like melting; plus loose holes / crumbs
EATEN = [
    [2, 3, 3, 2, 3, 4, 3, 2, 3, 3, 2, 2],
    [5, 6, 6, 5, 7, 7, 6, 5, 6, 7, 6, 5],
    [8, 9, 9, 8, 10, 10, 9, 8, 9, 10, 9, 8],
]
HOLES = [[(4, 5), (5, 9)], [(8, 4), (8, 8), (7, 1)], [(11, 5), (11, 9), (10, 1)]]   # (x, y) extra removed
CRUMBS = [[], [(1, 6), (2, 10)], [(2, 2), (4, 7), (1, 10), (6, 5)]]                   # (x, y) kept inside the eaten part

def melt(stage):
    a = real.tile_idx(BRICK).copy()
    for y, w in enumerate(EATEN[stage]):
        a[y, :w] = 0
    for (x, y) in HOLES[stage]:
        a[y, x] = 0
    src = real.tile_idx(BRICK)
    for (x, y) in CRUMBS[stage]:
        a[y, x] = src[y, x]
    return a

CELLS = {f'brick_melt_{i}': melt(i) for i in range(3)}

def sprite(name):
    return real.to_rgba(CELLS[name])

def on_backdrop(name):
    im = real.tile(0).copy(); im.alpha_composite(sprite(name)); return im

if __name__ == '__main__':
    raw = b''
    for name, a in CELLS.items():
        assert set(np.unique(a)) <= set(np.unique(real.tile_idx(BRICK))) | {0}, 'new colour!'
        im = real.to_rgba(a); im.save(f'{OUT}/{name}.png')
        im.resize((48, 48), Image.NEAREST).save(f'{OUT}/{name}_x4.png')
        on_backdrop(name).save(f'{OUT}/{name}_on_backdrop.png')
        raw += a.tobytes()
    open(f'{OUT}/generated_sprites.u8', 'wb').write(raw)
    open(f'{OUT}/README.txt', 'w').write(__doc__)

    K = 4; C = 12 * K; G = 8
    up = lambda im: im.resize((im.width * K, im.height * K), Image.NEAREST)
    out = Image.new('RGBA', (640, 420), hx('14102a') + (255,))
    text(out, 20, 12, 'JETPACK - WHAT IS STILL GENERATED (NOT ORIGINAL), 4X', hx('ffd838'))
    y = 40
    text(out, 20, y, 'REAL  PHASER POSES 10-15 (FAN IS PART OF THE FRAME)', hx('d8d0f4')); y += 14
    for i, f in enumerate(range(10, 16)):
        x = 20 + i * (C + G); out.alpha_composite(up(real.tile(0)), (x, y)); out.alpha_composite(up(real.player(f)), (x, y))
    y += C + 16
    text(out, 20, y, 'REAL  BRICK 0X28 | GENERATED  MELT 25/50/75%', hx('7fd0ff')); y += 14
    out.alpha_composite(up(real.tile(BRICK)), (20, y))
    for i, n in enumerate(CELLS):
        x = 20 + (i + 1) * (C + G) + 12
        out.alpha_composite(up(on_backdrop(n)), (x, y))
        ImageDraw.Draw(out).rectangle([x - 2, y - 2, x + C + 1, y + C + 1], outline=hx('7fd0ff'))
    y += C + 16
    text(out, 20, y, 'IN USE: REAL POSE 12 NEXT TO GENERATED MELT 50%', hx('7fd0ff')); y += 14
    for j, cell in enumerate([real.tile(0), on_backdrop('brick_melt_1'), real.tile(BRICK)]):
        out.alpha_composite(up(cell), (20 + j * C, y))
    out.alpha_composite(up(real.player(12)), (20, y))
    ImageDraw.Draw(out).rectangle([20 + C - 2, y - 2, 20 + 2 * C + 1, y + C + 1], outline=hx('7fd0ff'))
    y += C + 16
    text(out, 20, y, 'BLUE FRAME = GENERATED (PIXELS REMOVED FROM THE REAL', hx('a8a0d0')); y += 12
    text(out, 20, y, 'BRICK, NO NEW COLOURS). EVERYTHING ELSE IS ORIGINAL.', hx('a8a0d0')); y += 12
    text(out, 20, y, 'REAL JET FLAME FRAMES 4-9 / 28-33 ARE USED AS IS.', hx('a8a0d0'))
    out = out.crop((0, 0, 640, y + 22))
    out.convert('RGB').save(f'{OUT}/generated_sprites_contact.png')
    os.makedirs(os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/final', exist_ok=True)
    out.convert('RGB').save('/workspace/jetpack/art/final/generated_sprites_contact.png')
    print(out.size)
