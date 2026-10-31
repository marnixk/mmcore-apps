import random, math
from PIL import Image
from pal import P
def new(w,h): return Image.new('RGBA',(w,h),(0,0,0,0))
def px(img,x,y,k):
    if 0<=x<img.width and 0<=y<img.height: img.putpixel((x,y),P[k]+(255,))

def disc(size, cols, hl=True):
    # cols: (outer, inner, highlight)
    img = new(size,size); c=(size-1)/2; R=size/2
    for y in range(size):
        for x in range(size):
            d = math.hypot(x-c,y-c)
            if d <= R-0.25: px(img,x,y,cols[0])
            if d <= R-1.5 and size>=5: px(img,x,y,cols[1])
    if hl and size>=4: px(img,int(c-R/2.5),int(c-R/2.5),cols[2])
    if hl and size>=8: px(img,int(c-R/2.5)+1,int(c-R/2.5),cols[2]); px(img,int(c-R/2.5),int(c-R/2.5)+1,cols[2])
    return img

def particles():
    # 8x8 cells: sizes 1,2,3,4,6,8 in three colours (white, light blue, blue)
    out = []
    sets = [('w3','w3','w3'),('u5','u4','w3'),('u4','u3','u5'),('u3','u2','u4')]
    for cols in sets:
        row = []
        for s in (1,2,3,4,6,8):
            img = new(8,8)
            if s==1: px(img,3,3,cols[0])
            elif s==2: [px(img,x,y,cols[0]) for x in (3,4) for y in (3,4)]
            elif s==3: [px(img,x,y,cols[0]) for (x,y) in ((3,2),(2,3),(3,3),(4,3),(3,4))]
            else:
                d = disc(s, (cols[1],cols[0],'w3') if s>=6 else (cols[0],cols[0],'w3'))
                img.alpha_composite(d,((8-s)//2,(8-s)//2))
            row.append(img)
        out.append(row)
    return out

def puff(seed, size=16):
    # bumpy cumulus puff; each bump is lit from the top-left and shaded bottom-right
    r = random.Random(seed); img = new(size,size)
    S = size
    blobs = [(S*0.5, S*0.66, S*0.44, S*0.26)]
    n = 3 if S < 20 else 4
    for i in range(n):
        cx = S*(0.27 + 0.46*i/(n-1)) + r.uniform(-0.8,0.8)
        rr = S*r.uniform(0.2,0.27) * (1.15 if i==1 else 1)
        cy = S*0.5 - r.uniform(S*0.02, S*0.14)
        blobs.append((cx, cy, rr, rr))
    def inside(x,y): return min(((x-cx)/rx)**2+((y-cy)/ry)**2 for cx,cy,rx,ry in blobs) <= 1.0
    for y in range(S):
        for x in range(S):
            X, Y = x+0.5, y+0.5
            if not inside(X,Y): continue
            if not inside(X, Y+1) or not inside(X+1, Y+1): k = 'u3'
            elif not inside(X+1.6, Y+1.6): k = 'u4'
            elif not inside(X-1.2, Y-1.2): k = 'w3'
            else: k = 'u5'
            px(img,x,y,k)
    return img

def sparkle(frame, col='w3', col2='u5'):
    img = new(7,7)
    if frame==0: px(img,3,3,col); [px(img,3+dx,3+dy,col2) for dx,dy in ((1,0),(-1,0),(0,1),(0,-1))]
    else:
        px(img,3,3,col)
        for i in (1,2): [px(img,3+dx*i,3+dy*i,col if i==1 else col2) for dx,dy in ((1,0),(-1,0),(0,1),(0,-1))]
        for dx,dy in ((1,1),(-1,1),(1,-1),(-1,-1)): px(img,3+dx,3+dy,col2)
    return img

def beam_segment(w=40, h=20, seed=0):
    # tileable vertical light beam, ordered dither with transparency (no blending needed)
    img = new(w,h); c = (w-1)/2
    bayer = [[0,8,2,10],[12,4,14,6],[3,11,1,9],[15,7,13,5]]
    for y in range(h):
        for x in range(w):
            t = 1 - abs(x-c)/(w/2)
            b = bayer[y%4][x%4]
            k = None
            if t > 0.8: k = 'y4' if b < 9 else ('y3' if b < 12 else None)
            elif t > 0.55: k = 'y3' if b < 6 else ('y2' if b < 8 else None)
            elif t > 0.3: k = 'y2' if b < 4 else None
            elif t > 0.08: k = 'y1' if b < 1 else None
            if k: px(img,x,y,k)
    return img

def beam_glow(w=40, h=12):
    img = new(w,h); c=(w-1)/2; cy=(h-1)/2
    bayer = [[0,8,2,10],[12,4,14,6],[3,11,1,9],[15,7,13,5]]
    for y in range(h):
        for x in range(w):
            d = ((x-c)/(w/2))**2 + ((y-cy)/(h/2))**2
            if d < 1:
                v = (1-d)*16; b = bayer[y%4][x%4]
                k = 'y4' if v>12 and b<(v-12)*4 else ('y3' if v>6 and b < (v-6)*2 else ('y2' if b < v*0.9 else None))
                if k: px(img,x,y,k)
    return img

def meter(level, over=False):
    # in-world charge bar: 22x6, level 0..10
    img = new(22,6)
    for x in range(22):
        for y in range(6):
            px(img,x,y,'k0')
    for x in range(1,21):
        for y in range(1,5): px(img,x,y,'k2')
    n = level*2
    for x in range(1,1+n):
        for y in range(1,5):
            k = ('m3' if y<3 else 'm2') if over else (('u5' if y==1 else 'u4') if x>14 else ('u4' if y==1 else 'u3'))
            px(img,x,y,k)
    return img

def drop(fill):
    # sneeze ammo pip 7x9, fill 0..4 (4 = full); partial fill shows puddle recharge progress
    img = new(7,9)
    shape = ["...O...","..OXO..","..OXO..",".OXXXO.","OXXXXXO","OXXXXXO","OXXXXXO",".OXXXO.","..OOO.."]
    for y,row in enumerate(shape):
        for x,c in enumerate(row):
            if c=='O': px(img,x,y,'k0')
            elif c=='X':
                level = 8 - y          # rows from bottom
                filled = level <= fill*2 - 1 if fill<4 else True
                px(img,x,y,('u4' if (x<3 and y<6) else 'u3') if filled else 'k2')
    if fill==4: px(img,2,4,'w3')
    return img

def heart(full):
    img = new(9,8)
    shape = [".OO...OO.","OXXO.OXXO","OXXXOXXXO","OXXXXXXXO",".OXXXXXO.","..OXXXO..","...OXO...","....O...."]
    for y,row in enumerate(shape):
        for x,c in enumerate(row):
            if c=='O': px(img,x,y,'k0')
            elif c=='X': px(img,x,y,('m3' if (x<4 and y<3) else 'm2') if full else 'k3')
    if full: px(img,2,1,'w3')
    return img

def snot_drop():
    img = new(5,7)
    for (x,y,k) in [(2,0,'u5'),(2,1,'u5'),(1,2,'u4'),(2,2,'u5'),(3,2,'u4'),(1,3,'u3'),(2,3,'u4'),(3,3,'u4'),(1,4,'u3'),(2,4,'u3'),(3,4,'u4'),(2,5,'u2')]:
        px(img,x,y,k)
    return img
