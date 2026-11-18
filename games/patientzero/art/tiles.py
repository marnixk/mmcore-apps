import random
from PIL import Image
from pal import P
T = 20
def new(w=T,h=T,fill=None):
    return Image.new('RGBA',(w,h),(P[fill]+(255,)) if fill else (0,0,0,0))
def px(img,x,y,k):
    if 0<=x<img.width and 0<=y<img.height: img.putpixel((x,y),P[k]+(255,))
def rect(img,x,y,w,h,k):
    for yy in range(y,y+h):
        for xx in range(x,x+w): px(img,xx,yy,k)

def asphalt(seed, base='k2', speck='k3', dark='k1', n=18):
    r = random.Random(seed); img = new(fill=base)
    for _ in range(n): px(img, r.randrange(T), r.randrange(T), speck)
    for _ in range(n//2): px(img, r.randrange(T), r.randrange(T), dark)
    return img

def crack(seed):
    img = asphalt(seed); r = random.Random(seed+99)
    x, y = r.randrange(4,8), 0
    while y < T:
        px(img,x,y,'k0'); px(img,x+1,y,'k1')
        if r.random()<0.3: px(img,x-1,y,'k1')
        y += 1; x += r.choice((-1,0,0,1)); x = max(1,min(T-3,x))
        if y==10: bx=x
    for i in range(6): px(img, bx+i, 10+i//2, 'k0')
    return img

def road_line():
    img = asphalt(7)
    r = random.Random(3)
    rect(img,2,9,16,2,'y2')
    for _ in range(8): px(img, r.randrange(2,18), r.randrange(9,11), 'y1')   # worn paint
    for _ in range(3): px(img, r.randrange(2,18), r.randrange(9,11), 'k2')
    return img

def sidewalk(seed):
    r = random.Random(seed); img = new(fill='k3')
    for _ in range(14): px(img, r.randrange(T), r.randrange(T), 'k4')
    for i in range(T): px(img,i,0,'k2'); px(img,0,i,'k2'); px(img,i,10,'k2')
    if seed%2: 
        for i in range(10): px(img,10,i,'k2')
    else:
        for i in range(10,20): px(img,10,i,'k2')
    for i in range(1,T): px(img,i,1,'k4')
    return img

def curb(top=True):
    img = sidewalk(4)
    y = 17 if top else 0
    rect(img,0,y,T,3,'k5'); rect(img,0,y+(2 if top else 0),T,1,'k1' if top else 'k6')
    for x in range(0,T,5): px(img,x,y+1,'k4')
    return img

def manhole():
    img = asphalt(11)
    for y in range(T):
        for x in range(T):
            d = (x-9.5)**2+(y-9.5)**2
            if d < 7.5**2: px(img,x,y,'k1')
            if d < 6.5**2: px(img,x,y,'k3' if (x+y)%4 else 'k1')
    for x in range(5,15): px(img,x,9,'k1')
    return img

def rubble_decal(seed):
    r = random.Random(seed); img = asphalt(seed+5)
    for _ in range(7):
        x, y = r.randrange(1,17), r.randrange(1,17)
        w, h = r.choice(((3,2),(2,2),(4,2),(2,1)))
        rect(img,x,y+1,w,h,'k1')
        rect(img,x,y,w,h,r.choice(('b3','b2','k5','b4')))
        px(img,x,y,'k6' if r.random()<.5 else 'b4')
    return img

def bricks(img, x0, y0, w, h, seed, a='b2', b='b3', mortar='b1'):
    r = random.Random(seed)
    for y in range(y0, y0+h):
        row = (y-y0)//4
        for x in range(x0, x0+w):
            off = 0 if row%2==0 else 3
            if (y-y0)%4==3 or (x+off)%6==5: px(img,x,y,mortar)
            else: px(img,x,y, a if r.random()<0.85 else b)
            if (y-y0)%4==0 and (x+off)%6!=5 and r.random()<0.5: px(img,x,y,b)

def facade(kind, seed=0):
    # ruined building front for the top border (two tile rows: 'top' and 'low')
    img = new()
    r = random.Random(seed)
    if kind=='top':        # broken roofline
        heights = [r.randrange(2,9) for _ in range(5)]
        for x in range(T):
            h0 = heights[x//4] + (1 if x%4==0 else 0)
            for y in range(h0, T): px(img,x,y,'k3')
            px(img,x,h0,'k5')
            if r.random()<0.2: px(img,x,h0-1,'k4')
        rect(img,4,11,5,6,'k0'); rect(img,12,11,5,6,'k1')  # empty windows
        px(img,13,12,'y3') if seed%3==0 else None          # lone lit window
        for x in (4,12): rect(img,x,17,5,1,'k5')
        return img
    if kind=='window':
        img = new(fill='k3')
        for _ in range(12): px(img, r.randrange(T), r.randrange(T), 'k4')
        rect(img,4,3,12,11,'k5'); rect(img,5,4,10,9,'k0')
        if seed%2: rect(img,5,4,10,9,'k1'); px(img,7,6,'y2'); px(img,8,6,'y3')
        for (x,y) in ((6,5),(9,8),(12,6),(13,10)): px(img,x,y,'k4')  # glass shards
        rect(img,4,14,12,1,'k6'); rect(img,4,15,12,1,'k1')
        return img
    if kind=='door':
        img = new(fill='k3')
        rect(img,5,2,10,18,'k5'); rect(img,6,3,8,17,'k0'); rect(img,6,3,8,1,'k1')
        px(img,12,11,'k4')
        return img
    if kind=='brick':
        img = new(); bricks(img,0,0,T,T,seed); return img
    if kind=='base':       # building base meeting the pavement
        img = new(); bricks(img,0,0,T,14,seed)
        rect(img,0,14,T,4,'k4'); rect(img,0,14,T,1,'k5'); rect(img,0,18,T,2,'k1')
        return img

def wall_top(seed=0):
    # upper half of a ruined brick wall (3/4 view): broken cap on top, bricks below
    img = new(); r = random.Random(seed)
    tops = [r.choice((4,5,5,6,7)) for _ in range(5)]
    for x in range(T):
        t = tops[x//4]
        bricks(img, x, t+3, 1, T-(t+3), seed+x, ) if False else None
    bricks(img,0,0,T,T,seed)
    for x in range(T):
        t = tops[x//4]
        for y in range(0,t): img.putpixel((x,y),(0,0,0,0))
        px(img,x,t,'b4'); px(img,x,t+1,'b4'); px(img,x,t+2,'b3')   # top cap catching the light
        if x%4==0 or tops[max(0,x//4-1)]!=t: px(img,x,t,'b3')
    return img
def wall_front(seed=0):
    img = new(); bricks(img,0,0,T,16,seed)
    rect(img,0,16,T,4,'k0')  # shadow on the ground
    rect(img,0,16,T,1,'b1')
    return img

def barrier():
    img = new(); 
    rect(img,1,4,18,12,'k5'); rect(img,1,4,18,3,'k6'); rect(img,0,15,20,2,'k4'); rect(img,0,17,20,2,'k0')
    for x in range(1,19,4): rect(img,x,9,2,3,'m2' if (x//4)%2 else 'w2')  # faded stripes
    px(img,1,4,'k4'); px(img,18,4,'k4')
    return img

def bin_():
    img = new()
    rect(img,4,5,12,12,'g1'); rect(img,4,5,12,2,'g2'); rect(img,3,3,14,3,'g2'); rect(img,3,3,14,1,'g3')
    rect(img,4,17,12,2,'k0'); rect(img,6,8,1,8,'k1'); rect(img,10,8,1,8,'k1'); rect(img,14,8,1,8,'k1')
    px(img,5,2,'c2'); px(img,8,1,'c3'); px(img,9,2,'b3')  # trash poking out
    return img

def crate():
    img = new()
    rect(img,2,3,16,15,'b3'); rect(img,2,3,16,1,'b4'); rect(img,2,17,16,1,'b1')
    for i in range(15): px(img,2+i,3+i,'b2')
    rect(img,2,3,1,15,'b2'); rect(img,17,3,1,15,'b2'); rect(img,2,10,16,1,'b2')
    rect(img,1,18,18,2,'k0')
    return img

def rubble_pile(seed=1):
    img = new(); r = random.Random(seed)
    for y in range(T):
        for x in range(T):
            d = ((x-10)/9.5)**2 + ((y-13)/7)**2
            if d < 1: px(img,x,y, 'k4' if r.random()<0.5 else 'b2')
            if d < 0.6 and r.random()<0.4: px(img,x,y,'k5')
            if d < 0.3 and r.random()<0.3: px(img,x,y,'b3')
    for x in range(2,18): px(img,x,19,'k0')
    px(img,6,7,'k6'); px(img,7,7,'k5'); rect(img,11,5,1,4,'b1'); px(img,12,4,'b1')  # rebar
    return img

def car():
    # wrecked car, 2x1 tiles (40x20), 3/4 top view, rusted
    img = new(40,20); r = random.Random(5)
    rect(img,2,4,36,13,'m1'); rect(img,2,4,36,1,'k0')
    rect(img,3,5,34,11,'b2')
    for _ in range(40): px(img,r.randrange(3,37),r.randrange(5,16),'b3')
    for _ in range(25): px(img,r.randrange(3,37),r.randrange(5,16),'b1')
    rect(img,12,3,16,12,'k1'); rect(img,13,4,14,10,'k2')   # roof
    rect(img,13,4,14,2,'k3'); px(img,16,6,'k4'); px(img,22,9,'k4')  # cracked glass
    rect(img,8,6,4,8,'k1'); rect(img,28,6,4,8,'k1')         # windscreens
    px(img,9,8,'k4'); px(img,30,10,'k4')
    rect(img,1,16,7,3,'k0'); rect(img,32,16,7,3,'k0')      # tyres
    rect(img,2,16,38,1,'k1'); rect(img,0,17,40,3,'k0') if False else None
    rect(img,2,17,36,2,'k0')
    px(img,36,8,'y2'); px(img,36,12,'m2')
    return img

def lamp():
    img = new(20,40)
    rect(img,9,6,2,32,'k4'); rect(img,9,6,1,32,'k5')
    rect(img,4,3,8,3,'k5'); rect(img,4,6,5,1,'k1'); px(img,5,6,'y3'); px(img,6,6,'y4')
    rect(img,7,37,6,2,'k3'); rect(img,6,39,8,1,'k0')
    return img

def barrel(frame):
    img = new(20,30); r = random.Random(frame)
    rect(img,5,14,10,13,'b1'); rect(img,5,14,10,1,'b3'); rect(img,5,19,10,1,'b2'); rect(img,5,24,10,1,'b2')
    rect(img,6,15,2,12,'b2'); rect(img,4,27,12,2,'k0')
    flame = [(10,12),(9,11),(11,10),(10,9),(8,12),(12,12),(9,8),(11,7)] if frame==0 else [(10,12),(11,11),(9,10),(10,8),(8,12),(12,12),(11,9),(9,7),(10,6)]
    for (x,y) in flame: px(img,x,y,'y3')
    for x in range(6,14): px(img,x,13,'y2')
    for (x,y) in [(10,11),(10,10)]: px(img,x,y,'y4')
    for (x,y) in [(7,13),(13,13),(8,11),(12,11)]: px(img,x,y,'m3')
    return img

def puddle(w, h, frame, seed=3):
    # irregular puddle from overlapping ellipses. frame 0 still, 1/2 ripple, 3 recharge glow
    img = new(w,h); r = random.Random(seed)
    blobs = [((w-1)/2, (h-1)/2, w/2-2, h/2-2.5)]
    for _ in range(2):
        bw, bh = w*r.uniform(0.25,0.35), h*r.uniform(0.25,0.4)
        blobs.append((r.uniform(bw+1, w-bw-2), r.uniform(bh+1, h-bh-2), bw, bh))
    def d(x,y): return min(((x-cx)/rx)**2+((y-cy)/ry)**2 for cx,cy,rx,ry in blobs)
    cx, cy, rx, ry = blobs[0]
    for y in range(h):
        for x in range(w):
            v = d(x,y)
            if v < 1.25: px(img,x,y,'k1')        # wet edge
            if v < 1.0: px(img,x,y,'u1')
            if v < 0.45: px(img,x,y,'u2')
    for (x,y) in [(int(cx-rx*0.45),int(cy-ry*0.35)),(int(cx-rx*0.45)+1,int(cy-ry*0.35)),(int(cx+rx*0.35),int(cy+ry*0.25))]:
        px(img,x,y,'u4')
    if frame in (1,2):
        k = 0.35 if frame==1 else 0.7
        for y in range(h):
            for x in range(w):
                q = ((x-cx)/(rx*k))**2 + ((y-cy)/(ry*k))**2
                if 0.75 < q < 1.2 and d(x,y) < 1.0: px(img,x,y,'u4' if frame==1 else 'u3')
        px(img,int(cx),int(cy),'u5' if frame==1 else 'u3')
    if frame==3:
        for y in range(h):
            for x in range(w):
                v = d(x,y)
                if 0.7 < v < 1.0: px(img,x,y,'u4')
                elif v < 0.7 and (x+2*y)%5==0: px(img,x,y,'u3')
        for (x,y) in [(int(cx)-3,0),(int(cx)+4,1),(int(cx)+1,int(cy)-1),(2,int(cy))]: px(img,x,y,'u5')
    return img
