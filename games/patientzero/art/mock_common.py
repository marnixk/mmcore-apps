import random, math
from PIL import Image
from pal import P
import build
from font import draw_text, draw_text_c, text_width
S = build.main()
IDX = build.INDEX
def fr(sheet, name):
    x,y,w,h = IDX[sheet]['frames'][name]
    return S[sheet].crop((x,y,x+w,y+h))
def blit(img, sheet, name, x, y):
    im = fr(sheet,name); img.alpha_composite(im,(int(x),int(y))); return im
def blit_im(img, im, x, y): img.alpha_composite(im,(int(x),int(y)))
def scale2(img, k=2): return img.resize((img.width*k,img.height*k), Image.NEAREST)
def big(im, k): return im.resize((im.width*k, im.height*k), Image.NEAREST)
def rect(img,x,y,w,h,k):
    for yy in range(max(0,y),min(img.height,y+h)):
        for xx in range(max(0,x),min(img.width,x+w)): img.putpixel((xx,yy),P[k]+(255,))
def frame_box(img,x,y,w,h, bg='k1', border='k4', hl='k5'):
    rect(img,x-1,y-1,w+2,h+2,'k0'); rect(img,x,y,w,h,border); rect(img,x+1,y+1,w-2,h-2,hl); rect(img,x+2,y+2,w-4,h-4,'k0'); rect(img,x+3,y+3,w-6,h-6,bg)
def darken(img, keep=None):
    # pixel-art dim: map every colour one step darker in its ramp + checker black
    ramps = [['k0','k1','k2','k3','k4','k5','k6'],['b1','b2','b3','b4'],['u1','u2','u3','u4','u5'],['g1','g2','g3','g4'],
             ['y1','y2','y3','y4'],['w1','w2','w3'],['h1','h2','h3'],['r1','r2'],['c1','c2','c3'],['d1','d2'],['m1','m2','m3']]
    down = {}
    for r in ramps:
        for i,k in enumerate(r): down[P[k]] = P[r[max(0,i-2)]] if r[0]!='k0' else P[r[max(0,i-2)]]
    for k in ('w1','w2','w3'): down[P[k]] = P['k4']
    for k in ('y1','y2','y3','y4'): down[P[k]] = P['b2']
    out = img.copy(); px = out.load()
    for y in range(out.height):
        for x in range(out.width):
            c = px[x,y][:3]
            n = down.get(c, P['k0'])
            if (x+y)%2==0: n = down.get(n, P['k0']) if n in down else n
            px[x,y] = n+(255,)
    return out

def street_level(seed=2, puddles=None):
    """32x18 tile ruined street. returns image and list of solid tiles"""
    r = random.Random(seed)
    img = Image.new('RGBA',(640,360),P['k0']+(255,))
    T = 20
    roofs = ['facade_roof_0','facade_roof_1','facade_roof_2']
    for tx in range(32):
        blit(img,'tiles',roofs[(tx*7+3)%3] if tx%5 else 'facade_roof_1',tx*T,0)
        blit(img,'tiles',['facade_window_0','facade_window_1','facade_brick','facade_window_0','facade_door'][(tx*3)%5] if tx%9 else 'facade_door',tx*T,T)
        blit(img,'tiles','facade_base',tx*T,2*T)
        blit(img,'tiles','sidewalk_%d'%(tx%2),tx*T,3*T)
        blit(img,'tiles','curb_top',tx*T,4*T) if False else None
    for ty in range(4,16):
        for tx in range(32):
            n = r.random()
            name = 'asphalt_%d'%r.randrange(4)
            if n < 0.025: name = 'asphalt_crack'
            elif n < 0.04: name = 'asphalt_crack_1'
            elif n < 0.08: name = 'rubble_decal_%d'%r.randrange(2)
            if ty == 4: name = 'curb_top'
            if ty == 10 and tx%3 != 2: name = 'road_line'
            blit(img,'tiles',name,tx*T,ty*T)
    blit(img,'tiles','manhole',9*T,12*T)
    for tx in range(32):
        blit(img,'tiles','curb_bottom',tx*T,15*T)
        blit(img,'tiles','sidewalk_%d'%((tx+1)%2),tx*T,16*T)
        blit(img,'tiles','wall_top' if tx%11 not in (4,5) else 'sidewalk_0',tx*T,16*T) if False else None
        blit(img,'tiles','wall_front' if tx%13 not in (6,7) else 'sidewalk_1',tx*T,17*T)
    return img

def ruined_props(img):
    T=20
    blit(img,'props','car',5*T,6*T)
    blit(img,'tiles','barrier',23*T,12*T); blit(img,'tiles','barrier',24*T,12*T)
    blit(img,'tiles','crate',27*T,6*T); blit(img,'tiles','crate',28*T,6*T)
    blit(img,'tiles','rubble_pile_0',14*T,13*T); blit(img,'tiles','rubble_pile_1',2*T,13*T)
    blit(img,'tiles','bin',12*T,3*T); blit(img,'tiles','bin',25*T,16*T)
    blit(img,'props','lamp',3*T,2*T); blit(img,'props','lamp',19*T,2*T)
    blit(img,'props','barrel_0',30*T,13*T-10)
    # ruined wall segment
    for tx in (17,18):
        blit(img,'tiles','wall_top',tx*T,6*T); blit(img,'tiles','wall_front',tx*T,7*T)
