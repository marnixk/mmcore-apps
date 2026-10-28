"""Real Jetpack (DOS v1.5) graphics, from marnixk/mmcore-apps PR #4 (task/jetpack-vga-index-e23d @ 30c19ad):
tools/jetpack/reference/vga_dump/sheet_12x12.u8 (120 terrain cells, tile id N = cell N),
tools/jetpack/gfx/anim/frames.u8 + gfx/sprites.json (player / enemy frames, door), dac6.bin (level-0 DAC).
Copies live in art/real/. Colours: 6-bit DAC -> (v<<2)|(v>>4). Index 0 transparent for overlay sprites."""
import json, os
import numpy as np
from PIL import Image

R = os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + '/real'
_dac = np.frombuffer(open(f'{R}/dac6.bin', 'rb').read(), np.uint8).reshape(256, 3)
_meta = json.load(open(f'{R}/gfx/sprites.json'))
_atlas = np.frombuffer(open(f'{R}/gfx/anim/frames.u8', 'rb').read(), np.uint8)
_sheet = np.frombuffer(open(f'{R}/sheet_12x12.u8', 'rb').read(), np.uint8).reshape(120, 12, 12)

def palette(step=0):
    """RGB palette; step = sparkle cycle step (0..3) for indices 247-254."""
    d = _dac.copy()
    steps = _meta['palette_cycle']['steps_dac6']
    for i, idx in enumerate(range(247, 255)):
        d[idx] = steps[step % len(steps)][i]
    return ((d << 2) | (d >> 4)).astype(np.uint8)

PAL = palette(0)

def to_rgba(idx, transparent=True, pal=None):
    pal = PAL if pal is None else pal
    rgb = pal[idx]
    a = np.where((idx == 0) & transparent, 0, 255).astype(np.uint8)
    return Image.fromarray(np.dstack([rgb, a]), 'RGBA')

# frame lookups ------------------------------------------------------------
_player = {}; _enemy = {}; _door = []
for s in _meta['sprites']:
    for f in s['frames']:
        if 'sheet_row' in f:
            band = _meta['sheet']['sprite_rows'].index(f['sheet_row'])
            raw = _atlas[f['atlas_offset']:f['atlas_offset'] + 144].reshape(12, 12)
            if band < 4: _player[band * 24 + f['sheet_col']] = raw
            else: _enemy[(band - 4) * 24 + f['sheet_col']] = raw
        elif 'sheet_x' in f:
            _door.append(_atlas[f['atlas_offset']:f['atlas_offset'] + 576].reshape(24, 24))

def player_idx(f): return _player[f]
def enemy_idx(f): return _enemy[f]
def player(f, pal=None): return to_rgba(_player[f], pal=pal)
def enemy(f, pal=None): return to_rgba(_enemy[f], pal=pal)
def door(f=0, pal=None): return to_rgba(_door[f], transparent=False, pal=pal)
def tile_idx(t): return _sheet[t]
def tile(t, transparent=False, pal=None): return to_rgba(_sheet[t], transparent=transparent, pal=pal)
def has_player(f): return f in _player
def player_frames(): return sorted(_player)
def enemy_frames(): return sorted(_enemy)

BACKDROP = 0x3C  # playfield backdrop tile id (map tile 0 'blank' draws as the backdrop too)

def level(n):
    return json.load(open(f'{R}/levels/level_{n:02d}.json'))

def level_image(n, pal=None, sprites=True, enemy_frame=None, player_frame=0):
    """Full 312x192 native playfield of campaign level n: terrain, door, enemies at spawn, player at start."""
    lv = level(n)
    pal = PAL if pal is None else pal
    t = lv['tiles']
    idx = np.zeros((192, 312), np.uint8)
    for r in range(16):
        for c in range(26):
            idx[r*12:(r+1)*12, c*12:(c+1)*12] = _sheet[t[r * 26 + c]]
    im = to_rgba(idx, transparent=False, pal=pal)
    if sprites:
        dx, dy = int(lv['door_x']) - 1, int(lv['door_y']) - 1
        im.paste(door(0, pal), (dx * 12, dy * 12))
        init = {1: 4, 2: 0, 3: 13, 4: 21, 5: 16, 6: 33, 7: 19, 8: 42}
        for e in lv['enemies']:
            f = init[e['type_id']] if enemy_frame is None else enemy_frame
            im.alpha_composite(enemy(f, pal), ((e['x'] - 1) * 12, (e['y'] - 1) * 12))
        if player_frame is not None:
            im.alpha_composite(player(player_frame, pal), ((int(lv['player_x']) - 1) * 12, (int(lv['player_y']) - 1) * 12))
    return im

def item(t, pal=None):
    """A map item tile (gem, coin, vase...) cut out of its baked-in backdrop: pixels equal to the
    backdrop tile (id 0) at the same position become transparent. For icons drawn off the playfield."""
    a = _sheet[t]; b = _sheet[0]
    im = to_rgba(a, transparent=False, pal=pal)
    m = np.where(a == b, 0, 255).astype(np.uint8)
    im.putalpha(Image.fromarray(m, 'L'))
    return im
