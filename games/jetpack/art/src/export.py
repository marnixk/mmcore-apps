"""Write the run-time image assets for the game build, per resolution (house style: name_WxH.png),
from the REAL extracted Jetpack art. Regenerate everything:
    cd art/src && python3 gen_sprites.py && python3 title.py && python3 screens.py && python3 export.py
"""
import numpy as np
from common import *
from logo import make_logo
from ui import *
import real, scene_real as SR, gen_sprites as GS
import title as T
import screens as S

A = ROOT + '/assets'
os.makedirs(A, exist_ok=True)
RES = [('1280x720', 640, 360, 2), ('683x384', 683, 384, 1)]

def save(im, name, k=1):
    (scale(im, k) if k > 1 else im).save(f'{A}/{name}')

for tag, W, H, k in RES:
    # full screens with empty menus/tables (text + highlight drawn at run time)
    save(T.title(W, H, empty=True).convert('RGB'), f'title_{tag}.png', k)
    save(S.highscores(W, H, empty=True).convert('RGB'), f'scores_{tag}.png', k)
    save(S.pause_panel(empty=True), f'pause_{tag}.png', k)
    # parts
    save(make_logo(cw=10, chh=11, gap=8), f'logo_{tag}.png', k)
    save(make_logo(cw=6, chh=6, gap=5, sparkle=False), f'logo_small_{tag}.png', k)
    save(ribbon('A PORT FOR MMCORE'), f'ribbon_port_{tag}.png', k)
    save(panel(248, 14 + len(T.MENU) * 16 + 4 + 14), f'menu_panel_{tag}.png', k)
    save(hilite_bar(232, 14), f'menu_hilite_{tag}.png', k)
    save(big_text('HIGH SCORES', 16, S.GOLD), f'text_high_scores_{tag}.png', k)
    save(big_text('PAUSED', 16, S.GOLD), f'text_paused_{tag}.png', k)
    # title backdrop without logo / panel (scene + hero + footer), for an animated title
    bd, _ = T.backdrop(W, H)
    save(bd.convert('RGB'), f'title_backdrop_{tag}.png', k)
    # undimmed + dimmed frozen level for the pause screen (screen resolution)
    Wp, Hp = (1280, 720) if k == 2 else (683, 384)
    g = S.gameplay(Wp, Hp)
    g.convert('RGB').save(f'{A}/gameplay_sample_{tag}.png')
    S.dim_colours(g).convert('RGB').save(f'{A}/gameplay_dim_{tag}.png')

# ---- native-grain real sheets for the game (12x12 cells, index 0 / alpha = transparent)
def grid(cells, cols):
    rows = (len(cells) + cols - 1) // cols
    im = Image.new('RGBA', (cols * 12, rows * 12), (0, 0, 0, 0))
    for i, c in enumerate(cells):
        if c is not None: im.alpha_composite(c, ((i % cols) * 12, (i // cols) * 12))
    return im
save(grid([real.tile(t) for t in range(120)], 20), 'terrain_real_native12.png')
save(grid([real.player(f) if real.has_player(f) else None for f in range(96)], 24), 'player_real_native12.png')
save(grid([real.enemy(f) if f in real.enemy_frames() else None for f in range(48)], 24), 'enemies_real_native12.png')
dr = Image.new('RGBA', (8 * 24, 24)); [dr.paste(real.door(i), (i * 24, 0)) for i in range(8)]
save(dr, 'door_real_native24.png')
save(grid([GS.sprite(n) for n in GS.CELLS], 3), 'generated_brick_melt_native12.png')   # GENERATED
save(S.dim_colours(grid([real.tile(t) for t in range(120)], 20)), 'terrain_real_native12_dim.png')

# small font sheet (Press Start 2P 8px, ASCII 32..95), 9x9 cells incl. 1px shadow
chars = ''.join(chr(c) for c in range(32, 96))
styles = [('menu', 'd8d0f4'), ('select', 'fff6c8'), ('value', 'ffd838'), ('dim', 'a8a0d0'), ('head', 'a898d8'), ('legal', 'c8c8dc')]
fs = Image.new('RGBA', (32 * 9, 2 * 9 * len(styles)), (0, 0, 0, 0))
for si, (_, col) in enumerate(styles):
    for i, ch in enumerate(chars):
        text(fs, (i % 32) * 9, si * 18 + (i // 32) * 9, ch, hx(col), shadow=(8, 6, 20))
save(fs, 'font_small_8px.png')
open(f'{A}/font_small_8px.txt', 'w').write('Press Start 2P 8px (OFL). 9x9 cells (glyph + 1px shadow), chars ASCII 32..95, 32 per row,\n2 rows per style, styles top to bottom: ' + ', '.join(s for s, _ in styles) + '\nAt 1280x720 blit x2.\n')
print('done')
