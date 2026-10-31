from PIL import Image
from pal import P
W, H = 20, 30   # character cell (1 x 1.5 tiles)

FRONT = [
"....................",
"......OOOOOOOO......",
".....OSShSSSSSO.....",
"....OSShhSSSSSSO....",
"....OShSSSSSSSsO....",
"....OSEESSSSEESO....",
"....OSEeSSSSeESO....",
"....OsSSSNNSSSsO....",
"....OsSSSSSSSSsO....",
"....OsSMMMMMMSsO....",
".....OsMTMTMMsO.....",
"......OOssssOO......",
".....OOCCOOCCOO.....",
"...OOCkCCCCCCCcOO...",
"..OSOCkCCCCCCCcOSO..",
"..OSOCCCCCCWCCcOSO..",
"..OSOCkCCCWWCCcOSO..",
"..OsOCCCCCCCCccOsO..",
"..OssOCcCCCCcCcOssO.",
"...OOOCcCOCCcOCOOO..",
]
BACK = [
"....................",
"......OOOOOOOO......",
".....OSSsSSSSSO.....",
"....OSSSSSSsSSSO....",
"....OSsSSSSSSSsO....",
"....OSSSSSsSSSSO....",
"....OsSSSSSSSSsO....",
"....OsSSSSSSSSsO....",
"....OssSSSSSSssO....",
"....OsssSSSSsssO....",
".....OsssssssssO....",
"......OOssssOO......",
".....OOCCCCCCOO.....",
"...OOcCCCCCCCCcOO...",
"..OSOcCCCCCCCCcOSO..",
"..OSOcCCCWCCCCcOSO..",
"..OSOcCCCCCCCCcOSO..",
"..OsOcCCCCCCWCcOsO..",
"..OssOcCCCCCCcOssO..",
"...OOOcCCOCCcCOOO...",
]
SIDE = [  # facing right, arms reaching forward
"....................",
".....OOOOOOO........",
"....OShSSSSSO.......",
"...OSShSSSSSSO......",
"...OSSSSSSSSSSO.....",
"...OsSSSSSSEESO.....",
"...OsSSSSSSeESO.....",
"...OsSSSSSSSSSNO....",
"...OsSSSSSSSSSO.....",
"...OssSSSSSMMMO.....",
"....OssSSSSMTO......",
".....OOssssOO.......",
".....OCCCCCCO.......",
"....OCCkCCCCOOOOOO..",
"....OCkCCCCCCCSSShO.",
"....OCCkCCCCssssssO.",
"....OCCCCCCCOOOOOO..",
"....OCCCCWCCO.......",
"....OcCCCCCcO.......",
".....OcCOCcO........",
]

PALS = {
 'jerry':  dict(O='k0',S='u4',s='u3',h='u5',E='w2',e='u1',N='m3',M='m1',T='w2',C='u2',c='u1',k='u3',W='u4',P='k3',p='k2',F='k2',D='u5'),
 'zombie': dict(O='k0',S='g3',s='g2',h='g4',E='y4',e='k0',N='g2',M='m1',T='w2',C='c2',c='c1',k='c3',W='g2',P='d2',p='d1',F='b2',D='g4'),
 'human':  dict(O='k0',S='h2',s='h1',h='h3',E='k0',e='k0',N='h1',M='r1',T='w3',C='w3',c='w2',k='w3',W='w2',P='w2',p='w1',F='w2',D='w3',R='r2',r='r1'),
 'flash':  dict(R='w3',r='u5',O='u2',S='w3',s='u5',h='w3',E='u4',e='u4',N='u5',M='u4',T='w3',C='w3',c='u5',k='w3',W='u5',P='u5',p='u4',F='u4',D='w3'),
}

def new(w=W,h=H): return Image.new('RGBA',(w,h),(0,0,0,0))
def put(img,x,y,key):
    if 0<=x<img.width and 0<=y<img.height: img.putpixel((x,y),P[key]+(255,))

def stamp(img, rows, pal, dx=0, dy=0, mirror=False):
    for y,row in enumerate(rows):
        assert len(row)==20, (len(row),row)
        for x,c in enumerate(row):
            if c=='.': continue
            xx = (19-x) if mirror else x
            put(img, xx+dx, y+dy, pal[c])

def legs_front(img, pal, frame, dy):
    # hips at row 20+dy, legs x 6..9 and 10..13; lifted leg is 2 shorter (knee up)
    top = 20+dy
    for x in range(6,14): put(img,x,top,pal['O'] if x in (6,13) else pal['P'])
    lifts = {0:(0,0),1:(2,0),2:(0,0),3:(0,2)}[frame]
    for li,(x0) in enumerate((6,10)):
        L = 6 - lifts[li]
        for y in range(top+1, top+1+L):
            for x in range(x0,x0+4):
                put(img,x,y, pal['O'] if x in (x0,x0+3) else (pal['p'] if x==x0+2 else pal['P']))
        sy = top+1+L
        for x in range(x0,x0+4): put(img,x,sy,pal['F'] if x not in (x0,) else pal['O'])
        put(img,x0+3,sy,pal['O'])
        for x in range(x0,x0+4): put(img,x,sy+1,pal['O'])
    # torn trouser hem
    if pal.get('P')=='d2' and frame in (0,2): put(img,8,top+5,pal['S'])

def legs_side(img, pal, frame, dy, mirror=False):
    top = 20+dy
    def P_(x,y,k): put(img, (19-x) if mirror else x, y, k)
    for x in range(5,12): P_(x,top,pal['O'] if x in (5,11) else pal['P'])
    # per row x offset for back leg (b) and front leg (f)
    spread = {0:0,1:2,2:0,3:-2}[frame]
    for i in range(6):
        t = (i+1)/6.0
        bx = 6 - round(spread*t)
        fx = 7 + round(spread*t)
        for (x0,shade) in ((bx,'p'),(fx,'P')):
            for x in range(x0,x0+4):
                P_(x,top+1+i, pal['O'] if x in (x0,x0+3) else pal[shade])
    for (x0,shade) in ((6-spread,'F'),(7+spread,'F')):
        y = top+7
        for x in range(x0,x0+5): P_(x,y,pal['O'] if x in (x0,x0+4) else pal['F'])
        for x in range(x0,x0+5): P_(x,y+1,pal['O'])

def arms_swing(img, pal, frame, dy, back=False):
    # front/back view: hands bob alternately by 1px
    if frame in (1,3):
        side = 2 if frame==1 else 16
        # move that hand up 1 px: clear bottom hand pixels and redraw higher
        pass

def char(kind, view, frame=0, pal=None):
    pal = PALS[pal or kind]
    img = new()
    bob = 1 if frame in (1,3) else 0
    if view=='down':
        stamp(img, FRONT, pal, dy=bob); legs_front(img, pal, frame, bob)
    elif view=='up':
        stamp(img, BACK, pal, dy=bob); legs_front(img, pal, frame, bob)
    elif view in ('right','left'):
        m = view=='left'
        legs_side(img, pal, frame, bob, mirror=m)
        stamp(img, SIDE, pal, dy=bob, mirror=m)
        # arm sway: front arm up 1px on frames 1/3 alternately
    if kind=='jerry' and view=='down':
        put(img,9,9+bob,pal['D'])   # snot drip from the red nose
        if frame in (0,1): put(img,9,10+bob,pal['D'])
    if kind=='jerry' and view in ('right','left'):
        x = 15; put(img,(19-x) if view=='left' else x, 8+bob, pal['D'])
    return img

# ---- face / head variants -------------------------------------------------
def rows_replace(rows, idx, new):
    r = list(rows); r[idx] = new; return r

ZFRONT = rows_replace(FRONT, 4, "....OShssSSssSsO....")          # heavy brows
JFRONT = rows_replace(rows_replace(FRONT, 4, "....OShSSSSSSSsO...."), 5, "....OSssSSSSssSO....")  # droopy lids
JSIDE  = rows_replace(SIDE, 5, "...OsSSSSSSssSO.....")

HUMAN_HEAD = [
"....................",
"......OOOOOOOO......",
".....ORRRRRrRRO.....",
"....ORRrRRRRRRRO....",
"....ORRSSSSSSrRO....",
"....OSSSSSSSSSSO....",
"....OSEESSSSEESO....",
"....OSSSSNNSSSSO....",
"....OhSMSSSSMShO....",
"....OSSSMMMMSSSO....",
".....OSSSSSSSSO.....",
"......OOSSSSOO......",
]
HUMAN_BODY = [
".....OOCCCCCCOO.....",
"...OOCkCCCCCCCcOO...",
"..OSOCkCCCCCCCcOSO..",
"..OSOCCCCCCCCCcOSO..",
"..OSOCkCCCCCCCcOSO..",
"..OSOCCCCCCCCccOSO..",
"..OhSOCCCCCCCcOShO..",
"...OOOCCCCCCCCOOO...",
]
HUMAN_FRONT = HUMAN_HEAD + HUMAN_BODY

def human_front(frame=0, pal='human'):
    p = PALS[pal]; img = new()
    stamp(img, HUMAN_FRONT, p); legs_front(img, p, frame, 0)
    return img

# Jerry charging: head tilted back, eyes shut, small open mouth; body stretches up
CHARGE_HEAD = {
 5: "....OSssSSSSssSO....",
 6: "....OSSSSSSSSSSO....",
 7: "....OsSSSNNSSSsO....",
 8: "....OsSSSSSSSSsO....",
 9: "....OsSSSMMSSSsO....",
 10:".....OsSSMMSSsO.....",
}
CHARGE2_HEAD = dict(CHARGE_HEAD)
CHARGE2_HEAD.update({
 3: "...OSShhSSSSSSSO....",
 4: "...OSShSSSSSSSSsO...",
 5: "...OSssSSSSSSssSO...",
 9: "....OsSSMMMMSSsO....",
 10:".....OsSMMMMSsO.....",
})
SNEEZE_HEAD = {
 4: "....OShSSSSSSSsO....",
 5: "....OSeESSSSEeSO....",   # squeezed shut  > <
 6: "....OSSeSSSSeSSO....",
 7: "....OsSSSNNSSSsO....",
 8: "....OsMMMMMMMMsO....",
 9: "....OsMTTTTTTMsO....",
 10:".....OMMMMMMMMO.....",
}
def jerry_pose(pose):
    p = PALS['jerry']; img = new()
    if pose in ('charge1','charge2'):
        heads = CHARGE_HEAD if pose=='charge1' else CHARGE2_HEAD
        rows = [heads.get(i, r) for i, r in enumerate(JFRONT)]
        if pose=='charge2':
            rows[13] = "..OOOCkCCCCCCCcOOO.."   # shoulders hunched up
        lift = -1 if pose=='charge1' else -2
        stamp(img, rows, p, dy=lift+1); legs_front(img, p, 0, 0)
        # stretch: fill the gap between shirt and hips
        for x in range(6,14):
            if img.getpixel((x,20))[3]==0: put(img,x,20,p['C'] if x not in (6,13) else p['O'])
        # air being sucked in (pale specks near the nose)
        specks = [(3,4),(16,4),(2,7),(17,7)] if pose=='charge1' else [(2,3),(17,3),(1,6),(18,6),(3,1),(16,1)]
        for (x,y) in specks: put(img,x,y,'u5')
    elif pose in ('sneeze1','sneeze2'):
        rows = [SNEEZE_HEAD.get(i, r) for i, r in enumerate(JFRONT)]
        if pose=='sneeze2':
            rows[13] = "..OOOCkCCCCCCCcOOO.."
        stamp(img, rows, p, dy=1 if pose=='sneeze1' else 2); legs_front(img, p, 0, 1 if pose=='sneeze1' else 1)
        spray = [(9,13),(10,13),(8,14),(11,14),(7,15),(12,15),(10,15)] if pose=='sneeze1' else [(9,14),(10,14),(7,15),(12,15),(6,16),(13,16),(9,16),(5,17),(14,17)]
        for (x,y) in spray: put(img,x,y,'u5' if (x+y)%2 else 'w3')
    elif pose in ('sneeze_r','sneeze_l'):
        m = pose=='sneeze_l'
        rows = list(JSIDE)
        rows[5] = "...OsSSSSSSeeSO....."
        rows[6] = "...OsSSSSSSSSSO....."
        rows[8] = "...OsSSSSSSSMMMO...."
        rows[9] = "...OssSSSSSMTTMO...."
        rows[10]= "....OssSSSSMMMO....."
        legs_side(img, p, 0, 1, mirror=m)
        stamp(img, rows, p, dy=1, mirror=m)
        for (x,y,c) in [(16,9,'w3'),(17,10,'u5'),(18,9,'u4'),(17,8,'u5'),(19,11,'u4'),(18,12,'u5'),(19,7,'u5')]:
            put(img,(19-x) if m else x, y, c)
    return img

# ---- cured human ascending, 30x30 cell ------------------------------------
WING = {  # right wing, columns from the body outwards, rows from y=8
 0: [".......ww",
     "......wWW",
     ".....wWWW",
     "....wWWWl",
     "...wWWWWl",
     "..wWWWWl.",
     ".wWWWWl..",
     "wWWWWl...",
     "WWWWl....",
     "WWll....."],
 1: [".........",
     ".........",
     ".........",
     "......www",
     "...wwWWWW",
     "wwWWWWWWl",
     "WWWWWWWl.",
     "WWWWWll..",
     "WWWll....",
     "Wll......"],
 2: [".........",
     ".........",
     ".........",
     ".........",
     ".........",
     "WWWww....",
     "WWWWWWw..",
     "WWWWWWWw.",
     "lWWWWWWl.",
     ".llWWl..."],
}
def angel(frame):
    img = Image.new('RGBA',(30,30),(0,0,0,0))
    base = human_front(0)
    # dangling legs: no walking, feet bare
    wing = WING[frame]
    cols = {'w':'w2','W':'w3','l':'w1'}
    for y,row in enumerate(wing):
        for x,c in enumerate(row):
            if c=='.': continue
            img.putpixel((21+x, 8+y), P[cols[c]]+(255,))
            img.putpixel((8-x, 8+y), P[cols[c]]+(255,))
    img.alpha_composite(base,(5,0))
    # halo
    for x in range(10,20):
        img.putpixel((x,0), P['y3']+(255,))
    img.putpixel((9,0),P['y2']+(255,)); img.putpixel((20,0),P['y2']+(255,))
    # feet bare, pointed
    for x0 in (11,15):
        for x in range(x0,x0+4):
            for y in (28,29):
                if img.getpixel((x,y))[3]: img.putpixel((x,y),P['h2']+(255,) if y==28 else P['h1']+(255,))
    return img
