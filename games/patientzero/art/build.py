import json, os, random, math
from PIL import Image
from pal import P
import chars, tiles, fx, logo
from font import draw_text, draw_text_c

ROOT = '/workspace/patientzero'
INDEX = {}

def sheet(name, cell_w, cell_h, cols, items):
    """items: list of (label, image). Fixed grid, transparent background."""
    rows = (len(items)+cols-1)//cols
    img = Image.new('RGBA',(cols*cell_w, rows*cell_h),(0,0,0,0))
    idx = {}
    for i,(label,im) in enumerate(items):
        x, y = (i%cols)*cell_w, (i//cols)*cell_h
        img.alpha_composite(im,(x,y)); idx[label] = [x,y,im.width,im.height]
    INDEX[name] = {'cell':[cell_w,cell_h],'frames':idx}
    return img

def free(name, W, H, placements):
    img = Image.new('RGBA',(W,H),(0,0,0,0)); idx={}
    for label,im,x,y in placements:
        img.alpha_composite(im,(x,y)); idx[label]=[x,y,im.width,im.height]
    INDEX[name] = {'cell':None,'frames':idx}
    return img

ALIASES = {'cure_0':'zombie_cure_0','cure_1':'zombie_cure_1','cure_2':'zombie_cure_2'}
def build_sprites():
    import roster
    fr = roster.all_frames()
    rows = roster.sheet_rows()
    cols = 16
    img = Image.new('RGBA',(cols*20, len(rows)*30),(0,0,0,0)); idx = {}
    for r,row in enumerate(rows):
        assert len(row) <= cols, row
        for c,n in enumerate(row):
            img.alpha_composite(fr[n],(c*20,r*30)); idx[n] = [c*20,r*30,20,30]
    for a,t in ALIASES.items(): idx[a] = idx[t]
    INDEX['sprites'] = {'cell':[20,30],'frames':idx,'aliases':ALIASES}
    return img

def build_angels():
    import roster
    an = roster.all_angels()
    img = Image.new('RGBA',(4*30, len(roster.TYPES)*32),(0,0,0,0)); idx = {}
    for r,t in enumerate(roster.TYPES):
        for i in range(4):
            n = f'{t}_angel_{i}'; img.alpha_composite(an[n],(i*30,r*32)); idx[n] = [i*30,r*32,30,32]
    for i in range(4): idx[f'angel_{i}'] = idx[f'zombie_angel_{i}']
    INDEX['angels'] = {'cell':[30,32],'frames':idx}
    return img

def build_tiles():
    t = tiles
    it = [('asphalt_0',t.asphalt(1)),('asphalt_1',t.asphalt(2)),('asphalt_2',t.asphalt(3)),('asphalt_3',t.asphalt(4)),
          ('asphalt_crack',t.crack(5)),('road_line',t.road_line()),('manhole',t.manhole()),('rubble_decal_0',t.rubble_decal(3)),
          ('rubble_decal_1',t.rubble_decal(8)),('asphalt_crack_1',t.crack(12)),
          ('sidewalk_0',t.sidewalk(1)),('sidewalk_1',t.sidewalk(2)),('curb_top',t.curb(True)),('curb_bottom',t.curb(False)),
          ('facade_roof_0',t.facade('top',0)),('facade_roof_1',t.facade('top',1)),('facade_roof_2',t.facade('top',2)),
          ('facade_window_0',t.facade('window',0)),('facade_window_1',t.facade('window',1)),('facade_door',t.facade('door')),
          ('facade_brick',t.facade('brick',2)),('facade_base',t.facade('base',3)),('wall_top',t.wall_top(1)),('wall_front',t.wall_front(2)),
          ('barrier',t.barrier()),('bin',t.bin_()),('crate',t.crate()),('rubble_pile_0',t.rubble_pile(1)),('rubble_pile_1',t.rubble_pile(6)),
          ('puddle_s_0',t.puddle(20,20,0,4)),('puddle_s_1',t.puddle(20,20,1,4)),('puddle_s_2',t.puddle(20,20,2,4)),('puddle_s_3',t.puddle(20,20,3,4))]
    return sheet('tiles', 20, 20, 10, it)

def build_props():
    t = tiles; pl = []
    pl.append(('car', t.car(), 0, 0))
    pl.append(('lamp', t.lamp(), 40, 0))
    pl.append(('barrel_0', t.barrel(0), 60, 0)); pl.append(('barrel_1', t.barrel(1), 80, 0))
    for f in range(4): pl.append((f'puddle_l_{f}', t.puddle(40,20,f,3), 100+ (f%2)*40, (f//2)*20))
    for f in range(4): pl.append((f'puddle_w_{f}', t.puddle(60,20,f,9), 180, f*20))
    return free('props', 240, 80, pl)

def build_fx():
    pl = []
    for r,row in enumerate(fx.particles()):
        for c,im in enumerate(row):
            pl.append((f'part_{["white","pale","blue","deep"][r]}_{[1,2,3,4,6,8][c]}', im, c*8, r*8))
    for i in range(4): pl.append((f'puff16_{i}', fx.puff(i,16), 48+i*16, 0))
    pl.append(('puff24_0', fx.puff(7,24), 48, 16)); pl.append(('puff24_1', fx.puff(11,24), 72, 16))
    pl.append(('puff32_0', fx.puff(8,32), 96, 16))
    pl.append(('sparkle_0', fx.sparkle(0), 0, 32)); pl.append(('sparkle_1', fx.sparkle(1), 8, 32))
    pl.append(('sparkle_gold_0', fx.sparkle(0,'y4','y3'), 16, 32)); pl.append(('sparkle_gold_1', fx.sparkle(1,'y4','y3'), 24, 32))
    pl.append(('snot_drop', fx.snot_drop(), 32, 32))
    pl.append(('beam', fx.beam_segment(), 128, 0)); pl.append(('beam_glow', fx.beam_glow(), 128, 20))
    for i in range(11): pl.append((f'meter_{i}', fx.meter(i), 168 + (i%4)*22, (i//4)*6))
    pl.append(('meter_over', fx.meter(10,True), 168+3*22, 12))
    for i in range(5): pl.append((f'drop_{i}', fx.drop(i), 168 + i*7, 18))
    pl.append(('heart_full', fx.heart(1), 208, 18)); pl.append(('heart_empty', fx.heart(0), 217, 18))
    return free('fx', 256, 48, pl)

def dither_band(img, y0, y1, a, b):
    bayer = [[0,8,2,10],[12,4,14,6],[3,11,1,9],[15,7,13,5]]
    for y in range(y0,y1):
        t = (y-y0)/max(1,(y1-y0))
        for x in range(img.width):
            img.putpixel((x,y), P[b if bayer[y%4][x%4] < t*16 else a]+(255,))

def city_bg(W=640, H=360, seed=4, ground=True):
    r = random.Random(seed)
    img = Image.new('RGBA',(W,H),P['k0']+(255,))
    dither_band(img, 0, 140, 'k0', 'k1'); dither_band(img, 140, 260, 'k1', 'k2')
    for y in range(260,H):
        for x in range(W): img.putpixel((x,y),P['k2']+(255,))
    for _ in range(140):
        x, y = r.randrange(W), r.randrange(170)
        img.putpixel((x,y), P[r.choice(('k4','k5','w1'))]+(255,))
    # moon
    for y in range(40):
        for x in range(40):
            d = math.hypot(x-19.5,y-19.5)
            if d < 16: img.putpixel((60+x,26+y), P['w2' if d<15 else 'w1']+(255,))
    for (cx,cy,rr) in ((74,38,3),(83,50,2),(70,52,2)):
        for y in range(-rr,rr+1):
            for x in range(-rr,rr+1):
                if x*x+y*y<=rr*rr: img.putpixel((cx+x,cy+y),P['w1']+(255,))
    # far skyline
    x = 0
    while x < W:
        w = r.randrange(18,46); h = r.randrange(60,150)
        top = 270-h
        for yy in range(top, 272):
            for xx in range(x, min(W,x+w)): img.putpixel((xx,yy),P['k1']+(255,))
        # broken top
        for xx in range(x, min(W,x+w)):
            dd = r.randrange(0,6) if r.random()<0.4 else 0
            for yy in range(top, top+dd): img.putpixel((xx,yy),P['k1' if yy<140 else 'k1']+(255,))
        x += w + r.randrange(0,6)
    # near ruins
    x = -10
    while x < W:
        w = r.randrange(40,90); h = r.randrange(70,170); top = 285-h
        prof = [top + (r.randrange(0,25) if (i//6)%3==0 else r.randrange(0,6)) for i in range(w)]
        for i in range(w):
            xx = x+i
            if not 0<=xx<W: continue
            for yy in range(prof[i], 290): img.putpixel((xx,yy),P['k2']+(255,))
            img.putpixel((xx,prof[i]),P['k3']+(255,))
        # windows
        for wy in range(top+14, 270, 14):
            for wx in range(x+6, x+w-8, 11):
                if 0<=wx<W-6 and wy > max(prof[max(0,min(w-1,wx-x))], prof[max(0,min(w-1,wx-x+5))])+3:
                    k = 'k0' if r.random()<0.8 else ('y2' if r.random()<0.5 else 'k1')
                    for a in range(6):
                        for b in range(8): img.putpixel((wx+a,wy+b),P[k]+(255,))
        # rebar sticking out
        for _ in range(3):
            i = r.randrange(w); xx = x+i
            if 0<=xx<W:
                for d in range(r.randrange(4,10)): img.putpixel((xx, prof[i]-d), P['b2']+(255,))
        x += w + r.randrange(10,40)
    if ground:
        for y in range(288, H):
            for x in range(W): img.putpixel((x,y),P['k2' if (x*7+y*13)%23 else 'k3']+(255,))
        for _ in range(90):
            x, y = r.randrange(W), r.randrange(292,H-3)
            w = r.randrange(2,7)
            for a in range(w):
                img.putpixel((min(W-1,x+a),y),P[r.choice(('k4','k3','b2'))]+(255,))
                img.putpixel((min(W-1,x+a),y+1),P['k1']+(255,))
        for x in range(W): img.putpixel((x,288),P['k3']+(255,))
    return img

def scale(img, k): return img.resize((img.width*k, img.height*k), Image.NEAREST)

def main():
    sheets = {
        'sprites': build_sprites(), 'angels': build_angels(), 'tiles': build_tiles(),
        'props': build_props(), 'fx': build_fx(), 'logo': None, 'title_bg': None}
    lg = logo.logo(); sheets['logo'] = lg; INDEX['logo'] = {'cell':None,'frames':{'logo':[0,0,lg.width,lg.height]}}
    bg = city_bg(); sheets['title_bg'] = bg; INDEX['title_bg'] = {'cell':None,'frames':{'title_bg':[0,0,640,360]}}
    for res,k in (('base',1),('720',2),('1080',3)):
        d = f'{ROOT}/gfx/{res}'; os.makedirs(d, exist_ok=True)
        for n,im in sheets.items(): scale(im,k).save(f'{d}/{n}.png')
    json.dump(INDEX, open(f'{ROOT}/gfx/sheets.json','w'), indent=1)
    return sheets

if __name__ == '__main__':
    s = main()
    # palette check
    allowed = set(P.values())
    for n,im in s.items():
        cols = {c[1][:3] for c in im.getcolors(1<<20) if c[1][3]==255}
        alphas = {c[1][3] for c in im.getcolors(1<<20)}
        bad = cols - allowed
        print(n, im.size, 'colours', len(cols), 'bad', len(bad), 'alphas', sorted(alphas))
