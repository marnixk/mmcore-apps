"""All in-game characters: Jerry (variant A) and the five enemy types, with every frame."""
from PIL import Image
from pal import P
import variants as V, chars

TYPES = ['zombie','office','hoodie','dancer','medic']           # introduction order is in levels.md
SPEC  = {'office':V.ENEMY['A'], 'hoodie':V.ENEMY['B'], 'dancer':V.ENEMY['C'], 'medic':V.ENEMY['D']}

def lum(c): return 0.3*c[0]+0.59*c[1]+0.11*c[2]
def flash(img):
    out = img.copy(); px = out.load()
    for y in range(out.height):
        for x in range(out.width):
            c = px[x,y]
            if c[3]==0: continue
            if c[:3]==P['k0']: px[x,y] = P['u2']+(255,)
            elif lum(c) > 95: px[x,y] = P['w3']+(255,)
            elif lum(c) > 45: px[x,y] = P['u5']+(255,)
            else: px[x,y] = P['u4']+(255,)
    return out
def sparkle(img):
    out = img.copy()
    for (x,y,k) in ((1,3,'y4'),(18,6,'y4'),(2,16,'y4'),(17,20,'y4'),(1,2,'y3'),(1,4,'y3'),(0,3,'y3'),(2,3,'y3')):
        chars.put(out,x,y,k)
    return out

def human_spec(spec):
    """same outfit, human skin, calm face, wounds gone"""
    h = dict(spec); pal = dict(spec['pal'])
    pal.update(S='h2', s='h1', h='h3', E='h2', e='r1', N='h1', M='h1', T='h2', W=pal['C'], D='h2')
    h['pal'] = pal; return h

def angel_from(front):
    img = Image.new('RGBA',(30,32),(0,0,0,0))
    cols = {'w':'w2','W':'w3','l':'w1'}
    wing = chars.WING
    def wings(f):
        for y,row in enumerate(wing[f]):
            for x,c in enumerate(row):
                if c=='.': continue
                img.putpixel((21+x, 9+y), P[cols[c]]+(255,)); img.putpixel((8-x, 9+y), P[cols[c]]+(255,))
    return img, wings
def angels(front):
    out = []
    for f in (0,1,2,1):
        img, wings = angel_from(front); wings(f)
        img.alpha_composite(front,(5,2))
        for x in range(10,20): img.putpixel((x,0), P['y3']+(255,))
        img.putpixel((9,0),P['y2']+(255,)); img.putpixel((20,0),P['y2']+(255,))
        out.append(img)
    return out

# ---------------------------------------------------------------- Jerry A poses
JA = V.JERRY['A']
def jerry_charge(level):
    pal = JA['pal']; img = V.new()
    rows = list(JA['front'])
    rows[5] = "....OSssSSSSssSO...."
    rows[6] = "....OSOOSSSSOOSO...."      # eyes squeezed
    rows[8] = "....OsSSSSSSSSsO...."
    rows[9] = "....OsSSSMMSSSsO...." if level==1 else "....OsSSMMMMSSsO...."
    if level==2:
        rows[3] = "...OSShhSSSSSSSO...."
        rows[13] = "..OOOCCkCCCCCCcOOO.."
    lift = -1 if level==1 else -2
    V.legs_front(img, pal, 0, 0)
    V.stamp(img, rows, pal, dy=lift)
    for y,r in JA['front_post'].items(): V.stamp(img,[r],pal,dy=lift,y0=y)
    specks = [(3,4),(16,4),(2,7),(17,7)] if level==1 else [(2,3),(17,3),(1,6),(18,6),(3,1),(16,1)]
    for (x,y) in specks: V.put(img,x,y,'u5')
    return img
def jerry_sneeze(level):
    img = V.sneeze(JA)
    if level==2:   # squash: shift whole sprite down 1, wider spray
        img2 = V.new(); img2.alpha_composite(img,(0,1)); img = img2
        for (x,y) in ((4,14),(15,14),(3,15),(16,15)): V.put(img,x,y,'u5')
    return img
def jerry_sneeze_side(left):
    pal = JA['pal']; img = V.new()
    rows = list(JA['side'])
    rows[5] = "...OsSSSSSSssSO....."
    rows[6] = "...OsSSSSSSOOSO....."
    rows[8] = "...OsSSSSSSSSMMO...."
    rows[9] = "...OssSSSSSMTTMO...."
    rows[10]= "....OssSSSSMMMO....."
    V.legs_side(img, pal, 0, 1, mirror=left)
    V.stamp(img, rows, pal, dy=1, mirror=left)
    for y,r in JA['side_post'].items(): V.stamp(img,[r],pal,dy=1,y0=y,mirror=left)
    for (x,y,c) in [(16,9,'w3'),(17,10,'u5'),(18,9,'u4'),(17,8,'u5'),(19,11,'u4'),(18,12,'u5'),(19,7,'u5'),(16,11,'w3')]:
        V.put(img,(19-x) if left else x, y, c)
    return img
def jerry_hurt():
    spec = dict(JA); pal = dict(JA['pal']); pal.update(S='m3',s='m2',h='w3',C='m3',c='m2',k='w3',P='m3',p='m2',F='m2',R='m1',r='m1',A='m1')
    spec['pal'] = pal; return V.frame(spec,'down',0)

# ---------------------------------------------------------------- moonwalk + Thriller shuffle (dancer, C)
MD = V.ENEMY['C']
MW_TORSO = {13:"....OCBCCAACOOO.....",
            14:"....OBCCCAACCCCO....",
            15:"....OCBCCAACCSSO....",
            16:"....OCCBCAACOOO....."}
MW_SWING = {14:"....OBCCCAACCCO.....",15:"....OCBCCAACCSO.....",16:"....OCCBCAACOO......"}
def mw_leg(img, pal, xs, foot, shade, mirror):
    """leg as a filled 2px strip with an automatic 1px outline (no stair-step gaps)"""
    pts = {}
    for i,x in enumerate(xs):
        y = 21+i
        for dx in (0,1): pts[(x+dx,y)] = pal[shade]
    x = xs[-1]
    pts[(x,27)] = pal['K']; pts[(x+1,27)] = pal['K']
    if foot=='flat':
        for dx in range(0,4): pts[(x+dx,28)] = pal['F']
    else:      # tiptoe: heel lifted, toe on the ground in front
        pts[(x+1,28)] = pal['F']; pts[(x+2,28)] = pal['F']; pts[(x+2,27)] = pal['F']
    # outline first, then fill
    for (px_,py_) in list(pts):
        for dx in (-1,0,1):
            for dy in (-1,0,1):
                q = (px_+dx, py_+dy)
                if q not in pts and 21 <= q[1] <= 29:
                    V.put(img,(19-q[0]) if mirror else q[0],q[1],pal['O'])
    for (px_,py_),k in pts.items():
        V.put(img,(19-px_) if mirror else px_,py_,k)
MW_LEGS = [  # (rear leg xs, foot), (front leg xs, foot) for rows 21..27, facing right
    (([6,6,6,6,6,6,6],'flat'), ([8,9,10,11,10,9,9],'toe')),
    (([6,6,5,5,4,4,3],'flat'), ([8,8,9,10,9,8,8],'toe')),
    (([7,8,9,10,9,8,8],'toe'), ([7,7,7,7,7,7,7],'flat')),
    (([7,8,8,9,8,8,7],'toe'),  ([7,7,6,6,5,5,4],'flat')),
]
def moonwalk(i, left=False):
    pal = MD['pal']; img = V.new()
    rows = [MW_TORSO.get(k, r) for k, r in enumerate(MD['side'])]
    for x in range(5,13): V.put(img,(19-x) if left else x,20,pal['O'] if x in (5,12) else pal['P'])
    (b,bf),(f,ff) = MW_LEGS[i]
    mw_leg(img,pal,b,bf,'p',left); mw_leg(img,pal,f,ff,'P',left)
    if i in (1,3):      # arm swings a little
        for k,r in MW_SWING.items(): rows[k] = r
    V.stamp(img, rows, pal, mirror=left)
    return img
def toe_stand(left=False):
    """on the toes, one arm straight up"""
    pal = MD['pal']; img = V.new()
    rows = [MW_TORSO.get(k, r) for k, r in enumerate(MD['side'])]
    rows[13] = "....OCBCCAACO......."
    rows[14] = "....OBCCCAACO......."
    rows[15] = "....OCBCCAACO......."
    rows[16] = "....OCCBCAACO......."
    for x in range(5,13): V.put(img,(19-x) if left else x,20,pal['O'] if x in (5,12) else pal['P'])
    mw_leg(img,pal,[6,6,6,6,6,6,6],'toe','p',left); mw_leg(img,pal,[8,8,8,8,8,8,8],'toe','P',left)
    V.stamp(img, rows, pal, dy=-1, mirror=left)
    add_part(img, {(12,11):'C',(12,10):'C',(13,9):'C',(13,8):'C',(13,7):'C',(14,6):'S',(13,6):'S',(14,5):'S'}, pal, left)
    return img

def add_part(img, pts, pal, mirror=False):
    """pixels with an automatic outline, drawn over the sprite"""
    P_ = {((19-x) if mirror else x, y): k for (x,y),k in pts.items()}
    for (x,y) in P_:
        for dx in (-1,0,1):
            for dy in (-1,0,1):
                q = (x+dx,y+dy)
                if q not in P_ and 0<=q[0]<20 and 0<=q[1]<30 and img.getpixel(q)[3]==0:
                    V.put(img,q[0],q[1],pal['O'])
    for (x,y),k in P_.items(): V.put(img,x,y,pal[k])

def thriller(i):
    """front-view Thriller shuffle: claws up, lean and step to one side"""
    pal = MD['pal']; img = V.new()
    rows = list(MD['front'])
    for r in range(13,20):    # take the hanging arms off
        rows[r] = rows[r][:2].replace('O','.') + '..' + rows[r][4:16] + '..' + rows[r][18:]
        rows[r] = '....' + rows[r][4:16] + '....'
    rows[13] = "....OCBCCAACCBCO...."
    V.legs_front(img, pal, 1 if i==0 else 3, 0)
    V.stamp(img, rows, pal)
    # both arms bent up, hands as claws above the shoulders
    left  = {(3,13):'C',(3,12):'C',(2,11):'C',(2,10):'B',(1,9):'S',(2,9):'S',(1,8):'S',(3,8):'S',(2,8):'S'}
    right = {(16,13):'C',(16,12):'C',(17,11):'C',(17,10):'B',(18,9):'S',(17,9):'S',(18,8):'S',(16,8):'S',(17,8):'S'}
    add_part(img, left, pal); add_part(img, right, pal)
    out = V.new(); out.alpha_composite(img, (-1 if i==0 else 1, 0) if False else (0,0))
    if i==0: out = img.transform(img.size, Image.AFFINE, (1,0,1,0,1,0))   # lean: shift 1px left
    else:    out = img.transform(img.size, Image.AFFINE, (1,0,-1,0,1,0))
    return out

# ---------------------------------------------------------------- paramedic face mask (D)
def medic_mask(view):
    spec = dict(V.ENEMY['D']); pal = dict(spec['pal']); pal['Y'] = 'w2'; pal['y'] = 'w1'; spec['pal'] = pal
    if view == 'down':
        rows = list(spec['front'])
        rows[7] = ".....OYYYNNYYYO....."
        rows[8] = ".....OYYYYYYYYO....."
        rows[9] = ".....OYyYYYYyYO....."
        rows[10]= ".....OsYYYYYYsO....."
        spec['front'] = rows
    else:
        rows = list(spec['side'])
        rows[7] = "....OsSSSYYYYYNO...."
        rows[8] = "....OsSSSYYYYYO....."
        rows[9] = "....OssSYYYYYYO....."
        rows[10]= "....OssSSYYYYO......"
        spec['side'] = rows
    return V.frame(spec, view, 0)

# ---------------------------------------------------------------- frames
def enemy_walk(t, view, f):
    return chars.char('zombie', view, f) if t=='zombie' else V.frame(SPEC[t], view, f)
def enemy_human_front(t):
    return chars.human_front(0) if t=='zombie' else V.frame(human_spec(SPEC[t]),'down',0)

def all_frames():
    fr = {}
    for v in ('down','up','right','left'):
        for f in range(4): fr[f'jerry_{v}_{f}'] = V.frame(JA, v, f)
    idle1 = V.frame(JA,'down',0); V.put(idle1,9,9,'u5'); V.put(idle1,9,10,'u4')
    fr.update(jerry_idle_0=V.frame(JA,'down',0), jerry_idle_1=idle1,
              jerry_charge1=jerry_charge(1), jerry_charge2=jerry_charge(2),
              jerry_sneeze1=jerry_sneeze(1), jerry_sneeze2=jerry_sneeze(2),
              jerry_sneeze_r=jerry_sneeze_side(False), jerry_sneeze_l=jerry_sneeze_side(True),
              jerry_hurt=jerry_hurt())
    for t in TYPES:
        for v in ('down','up','right','left'):
            for f in range(4): fr[f'{t}_{v}_{f}'] = enemy_walk(t, v, f)
        base = enemy_walk(t,'down',0); hum = enemy_human_front(t)
        fr[f'{t}_hit_0'] = base; fr[f'{t}_hit_1'] = flash(base)
        fr[f'{t}_cure_0'] = flash(hum); fr[f'{t}_cure_1'] = hum; fr[f'{t}_cure_2'] = sparkle(hum)
    for i in range(4):
        fr[f'dancer_moonwalk_r_{i}'] = moonwalk(i, False); fr[f'dancer_moonwalk_l_{i}'] = moonwalk(i, True)
    fr['dancer_toe_r'] = toe_stand(False); fr['dancer_toe_l'] = toe_stand(True)
    fr['dancer_thriller_0'] = thriller(0); fr['dancer_thriller_1'] = thriller(1)
    for v in ('down','right','left'): fr[f'medic_mask_{v}'] = medic_mask(v)
    return fr

def all_angels():
    out = {}
    for t in TYPES:
        hum = enemy_human_front(t)
        for i,im in enumerate(angels(hum)): out[f'{t}_angel_{i}'] = im
    return out

SHEET_ORDER = (
    [f'jerry_{v}_{f}' for v in ('down','up','right','left') for f in range(4)],
    ['jerry_idle_0','jerry_idle_1','jerry_charge1','jerry_charge2','jerry_sneeze1','jerry_sneeze2','jerry_sneeze_r','jerry_sneeze_l','jerry_hurt'],
)
def sheet_rows():
    rows = list(SHEET_ORDER)
    for t in TYPES:
        rows.append([f'{t}_{v}_{f}' for v in ('down','up','right','left') for f in range(4)])
        extra = [f'{t}_hit_0',f'{t}_hit_1',f'{t}_cure_0',f'{t}_cure_1',f'{t}_cure_2']
        if t=='dancer': extra += ['dancer_thriller_0','dancer_thriller_1','dancer_toe_r','dancer_toe_l']
        if t=='medic':  extra += ['medic_mask_down','medic_mask_right','medic_mask_left']
        rows.append(extra)
        if t=='dancer': rows.append([f'dancer_moonwalk_r_{i}' for i in range(4)] + [f'dancer_moonwalk_l_{i}' for i in range(4)])
    return rows
