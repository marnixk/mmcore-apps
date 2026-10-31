import random
from PIL import Image
from pal import P
from tiles import new, px, rect, T, puddle
# ---- hospital ----
def hosp_floor(seed):
    r = random.Random(seed); img = new(fill='k4')
    for y in range(T):
        for x in range(T):
            if (x//10 + y//10)%2: px(img,x,y,'k5')
    for i in range(T): px(img,i,0,'k3'); px(img,0,i,'k3')
    for _ in range(6): px(img,r.randrange(T),r.randrange(T),'k6')
    return img
def hosp_wall():
    img = new(fill='g1'); rect(img,0,0,T,12,'k5'); rect(img,0,12,T,2,'g2'); rect(img,0,14,T,4,'g1'); rect(img,0,18,T,2,'k0')
    rect(img,0,0,T,1,'k6'); px(img,5,6,'k4'); px(img,14,3,'k4')
    return img
def hosp_wall_top():
    img = new(fill='k3'); rect(img,0,0,T,3,'k2'); rect(img,0,17,T,3,'k5')
    return img
def ward_door(open_=True):
    img = hosp_wall()
    rect(img,3,1,14,19,'k1'); rect(img,4,2,12,18,'k0' if open_ else 'k6')
    if not open_: rect(img,10,2,1,18,'k4'); rect(img,6,6,3,3,'u2'); rect(img,11,6,3,3,'u2')
    else: rect(img,4,2,12,1,'g2')
    return img
def ward_sign():
    img = hosp_wall(); rect(img,5,3,10,6,'w3'); rect(img,9,4,2,4,'m2'); rect(img,7,5,6,2,'m2')
    return img
def bed():
    img = new(40,20)
    rect(img,1,4,38,12,'k5'); rect(img,2,5,36,10,'w2'); rect(img,2,5,8,10,'w3'); rect(img,3,6,6,6,'w2')
    rect(img,12,5,26,10,'u4'); rect(img,12,5,26,1,'u5'); rect(img,1,16,38,1,'k4'); rect(img,2,17,2,3,'k5'); rect(img,36,17,2,3,'k5')
    rect(img,1,18,38,2,'k0') if False else None
    return img
def iv_stand():
    img = new(20,30); rect(img,9,4,1,24,'k6'); rect(img,6,3,7,1,'k6'); rect(img,6,4,3,6,'u4'); rect(img,6,4,3,1,'u5')
    rect(img,5,27,9,1,'k5'); rect(img,4,28,11,2,'k0'); return img
# ---- subway ----
def platform(seed):
    r = random.Random(seed); img = new(fill='k4')
    for _ in range(14): px(img,r.randrange(T),r.randrange(T),'k5')
    for i in range(T): px(img,i,0,'k3'); px(img,10,i,'k3') if i<T else None
    return img
def platform_edge():
    img = platform(3); rect(img,0,14,T,3,'y2')
    for x in range(0,T,2): px(img,x,15,'y1')
    rect(img,0,17,T,3,'k1'); rect(img,0,19,T,1,'k0'); return img
def track(seed=0):
    img = new(fill='k1')
    for x in range(0,T,5): rect(img,x,0,3,T,'b1')
    rect(img,0,5,T,2,'k6'); rect(img,0,6,T,1,'k4'); rect(img,0,13,T,2,'k6'); rect(img,0,14,T,1,'k4')
    r = random.Random(seed)
    for _ in range(8): px(img,r.randrange(T),r.randrange(T),'k3')
    return img
def pillar():
    img = new(20,40); rect(img,4,0,12,36,'k5'); rect(img,4,0,3,36,'k6'); rect(img,13,0,3,36,'k4')
    rect(img,3,30,14,6,'k4'); rect(img,3,30,14,1,'k6'); rect(img,2,36,16,3,'k0'); rect(img,6,10,8,5,'y3'); rect(img,7,11,6,3,'k0')
    return img
def train_car(lit=True):
    img = new(120,40)
    rect(img,0,4,120,30,'k5'); rect(img,0,4,120,2,'k6'); rect(img,0,32,120,2,'k3'); rect(img,0,22,120,3,'u2')
    for wx in range(8,112,22):
        rect(img,wx,9,14,10,'k1'); 
        if lit: rect(img,wx+1,10,12,8,'y3'); rect(img,wx+1,10,12,2,'y4'); rect(img,wx+3,13,3,5,'k2')
    rect(img,0,34,120,4,'k0'); rect(img,10,34,14,4,'k1'); rect(img,96,34,14,4,'k1')
    for x in (0,119):
        for y in range(4,34): px(img,x,y,'k2')
    return img
def stairs():
    img = new(fill='k3')
    for y in range(0,T,4): rect(img,0,y,T,2,'k5'); rect(img,0,y+2,T,1,'k6')
    rect(img,0,0,2,T,'k6'); rect(img,18,0,2,T,'k6'); return img
# ---- rain ----
def rain_streak(n):
    img = new(8,8)
    pts = [(5,0),(4,2),(3,4),(2,6)] if n==0 else [(6,1),(5,3)]
    for (x,y) in pts: px(img,x,y,'u4'); px(img,x-1 if x>0 else x,y+1,'u3')
    return img
def splash():
    img = new(8,8)
    for (x,y,k) in ((1,5,'u4'),(6,5,'u4'),(2,4,'u5'),(5,4,'u5'),(3,6,'u3'),(4,6,'u3')): px(img,x,y,k)
    return img

def build_level_sheets():
    import build, json
    items = [('hosp_floor_0',hosp_floor(1)),('hosp_floor_1',hosp_floor(2)),('hosp_wall',hosp_wall()),('hosp_wall_top',hosp_wall_top()),
             ('ward_door_open',ward_door(True)),('ward_door_shut',ward_door(False)),('ward_sign',ward_sign()),
             ('platform_0',platform(1)),('platform_1',platform(2)),('platform_edge',platform_edge()),('track_0',track(0)),('track_1',track(1)),('stairs',stairs()),
             ('rain_0',rain_streak(0)),('rain_1',rain_streak(1)),('splash',splash())]
    lsheet = build.sheet('tiles_levels', 20, 20, 8, items)
    lprops = build.free('props_levels', 240, 80, [('bed',bed(),0,0),('iv',iv_stand(),40,0),('pillar',pillar(),60,0),('train',train_car(),80,0),('train_dark',train_car(False),80,40)])
    for res,k in (('base',1),('720',2),('1080',3)):
        build.scale(lsheet,k).save(f'/workspace/patientzero/gfx/{res}/tiles_levels.png')
        build.scale(lprops,k).save(f'/workspace/patientzero/gfx/{res}/props_levels.png')
    json.dump(build.INDEX, open('/workspace/patientzero/gfx/sheets.json','w'), indent=1)
    return lsheet, lprops
