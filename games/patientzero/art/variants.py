from PIL import Image
from pal import P
W, H = 20, 30
def new(): return Image.new('RGBA',(W,H),(0,0,0,0))
def put(img,x,y,key):
    if 0<=x<W and 0<=y<H: img.putpixel((x,y),P[key]+(255,))
def fix(rows):
    out=[]
    for r in rows:
        assert len(r)<=20, (len(r), r)
        out.append(r.ljust(20,'.'))
    return out
def stamp(img, rows, pal, dy=0, mirror=False, y0=0):
    for y,row in enumerate(fix(rows)):
        for x,c in enumerate(row):
            if c=='.': continue
            put(img, (19-x) if mirror else x, y0+y+dy, pal[c])

def legs_front(img, pal, frame, dy, top=20, xs=(6,10), w=4, L=6):
    t = top+dy
    for x in range(xs[0], xs[1]+w): put(img,x,t,pal['O'] if x in (xs[0],xs[1]+w-1) else pal['P'])
    lifts = {0:(0,0),1:(2,0),2:(0,0),3:(0,2)}[frame]
    for li,x0 in enumerate(xs):
        LL = L - lifts[li]
        for y in range(t+1, t+1+LL):
            for x in range(x0,x0+w):
                put(img,x,y, pal['O'] if x in (x0,x0+w-1) else (pal['p'] if x==x0+w-2 else pal['P']))
        sy = t+1+LL
        if pal.get('K'):
            for x in range(x0+1,x0+w-1): put(img,x,sy-1,pal['K'])
        for x in range(x0,x0+w): put(img,x,sy, pal['O'] if x in (x0,x0+w-1) else pal['F'])
        for x in range(x0,x0+w): put(img,x,sy+1,pal['O'])

def legs_side(img, pal, frame, dy, mirror=False, top=20, x0=6, w=4, L=6):
    t = top+dy
    def Q(x,y,k): put(img,(19-x) if mirror else x,y,k)
    for x in range(x0-1, x0+w+2): Q(x,t,pal['O'] if x in (x0-1,x0+w+1) else pal['P'])
    spread = {0:0,1:2,2:0,3:-2}[frame]
    if L < 6: spread = spread//2
    for i in range(L):
        f = (i+1)/L
        for (xb,shade) in ((x0 - round(spread*f),'p'),(x0+1 + round(spread*f),'P')):
            for x in range(xb,xb+w): Q(x,t+1+i, pal['O'] if x in (xb,xb+w-1) else pal[shade])
    for xb in (x0-spread, x0+1+spread):
        y = t+1+L
        if pal.get('K'):
            for x in range(xb+1,xb+w-1): Q(x,y-1,pal['K'])
        for x in range(xb,xb+w+1): Q(x,y,pal['O'] if x in (xb,xb+w) else pal['F'])
        for x in range(xb,xb+w+1): Q(x,y+1,pal['O'])

# --------------------------------------------------------------------------- JERRY
JERRY = {}
JERRY['A'] = dict(name='HOSPITAL GOWN', desc='escaped patient: tufty hair, pale gown with blue dots, wristband, bare blue legs',
 pal=dict(O='k0',S='u4',s='u3',h='u5',E='w2',e='u1',N='m3',M='m1',T='w2',C='w2',c='w1',k='w3',A='u3',X='w3',R='u2',r='u1',P='u4',p='u3',F='u3',D='u5'),
 front=["........O.O.........",
        "......OOROROO.......",
        ".....OSShSSSSO......",
        "....OSShhSSSSSSO....",
        "....OShSSSSSSSsO....",
        "....OSssSSSSssSO....",
        "....OSEeSSSSeESO....",
        "....OsSSSNNSSSsO....",
        "....OsSSSDSSSSsO....",
        "....OsSSMMMMSSsO....",
        ".....OsSSSSSSsO.....",
        "......OOssssOO......",
        ".....OCCCOOCCCO.....",
        "...OOCCkCCCCCCcOO...",
        "..OSOCkCAkCCACcOSO..",
        "..OSOCCCCCCCCCcOSO..",
        "..OSOCkCACCCACcOSO..",
        "..OXOCCCCCCCCCcOsO..",
        "..OssOCACCCCACcOssO.",
        "...OOOCCCCCCCCcOOO.."],
 front_post={20:".....OCcCCCCCcCO....",21:".....OOOOOOOOOOO...."},
 side=["......ORO...........",
       ".....OORROOO........",
       "....OShSSSSSO.......",
       "...OSShSSSSSSO......",
       "...OSSSSSSSSSSO.....",
       "...OsSSSSSSssSO.....",
       "...OsSSSSSSeESO.....",
       "...OsSSSSSSSSSNO....",
       "...OsSSSSSSSSSDO....",
       "...OssSSSSSSMMO.....",
       "....OssSSSSSSO......",
       ".....OOssssOO.......",
       ".....OCCCCCCO.......",
       "....OCCkCCCCCO......",
       "....OCkCCCCCOO......",
       "....OCCCCCCOSO......",
       "....OCACCCCOSO......",
       "....OCCCCCCOXO......",
       "....OCCACCCOSO......",
       "....OCCCCCCOOO......"],
 side_post={20:"....OCcCCCCCO.......",21:"....OOOOOOOOO......."},
 sneeze={5:"....OSOOSSSSOOSO....",6:"....OSSSSSSSSSSO....",8:"....OsMMMMMMMMsO....",9:"....OsMTTTTTTMsO....",10:".....OMMMMMMMMO....."},
)
JERRY['B'] = dict(name='SCARF AND BEANIE', desc='bundled-up flu victim: striped beanie with pompom, long red scarf, grey hoodie, tissue in hand',
 pal=dict(O='k0',S='u4',s='u3',h='u5',E='w2',e='u1',N='m3',M='m1',T='w2',C='k5',c='k4',k='k6',A='m2',a='w2',B='w2',X='w3',P='k3',p='k2',F='k1',D='u5'),
 front=[".........OO.........",
        "........OXXO........",
        "......OOAAAAOO......",
        ".....OAAaAAaAAO.....",
        "....OBBBBBBBBBBO....",
        "....OSSSSSSSSSSO....",
        "....OSssSSSSssSO....",
        "....OSEeSSSSeESO....",
        "....OsSSSNNSSSsO....",
        "....OsSSSMMSSSsO....",
        ".....OAAAAAAAAAO....",
        "....OAaAaAaAaAaAO...",
        "...OOCAaAOOCCCCOO...",
        "..OSOCCAaACCCCcOSO..",
        "..OSOCCaAaCCCCcOSO..",
        "..OSOCCAaACCCCcOSO..",
        "..OSOCCOOOCCCCcOSO..",
        "..OsOCCCCCCCCCcOsO..",
        "..OXXOcCCCCCCcCOssO.",
        "...OOOcCCCCCCcOOO..."],
 side=["......OXXO..........",
       ".....OOAAAOO........",
       "....OAAaAAaAO.......",
       "...OAAAaAAAaAO......",
       "...OBBBBBBBBBBO.....",
       "...OsSSSSSSssSO.....",
       "...OsSSSSSSeESO.....",
       "...OsSSSSSSSSSNO....",
       "...OsSSSSSSSSSO.....",
       "...OssSSSSSSMMO.....",
       "...OAaAaAaAaAO......",
       "..OAaOOssOOAaO......",
       "..OAaOCCCCCCO.......",
       "..OaAOCCkCCCCO......",
       "...OOOCkCCOSOCO.....",
       "....OCCCCCOSOCO.....",
       "....OCCCCCOSOCO.....",
       "....OCCCCCOXXOO.....",
       "....OcCCCCOXXO......",
       "....OOcCCCCOOO......"],
 sneeze={6:"....OSOOSSSSOOSO....",7:"....OSSSSSSSSSSO....",8:"....OsSSSNNSSSsO....",9:"....OsMMMMMMMMsO...."},
)
JERRY['C'] = dict(name='BATHROBE', desc='chunky couch patient: mustard bathrobe with red belt, wet towel tied on his forehead, pink slippers, bare shins',
 pal=dict(O='k0',S='u4',s='u3',h='u5',E='w2',e='u1',N='m3',M='m1',T='w2',C='y2',c='y1',k='y3',B='y4',A='m2',X='w3',a='w1',P='u3',p='u2',F='m3',D='u5'),
 front=["....................",
        "......OOOOOOOO......",
        ".....OXXXXXXXXO.....",
        "....OXaXXaXXaXXO...X",
        "....OShSSSSSSSsOXXX.",
        "....OSssSSSSssSO.XX.",
        "....OSEeSSSSeESO....",
        "....OsSSSNNSSSsO....",
        "....OsSSSSSSSSsO....",
        "....OsSSSMMSSSsO....",
        ".....OsSSSSSSsO.....",
        ".....OOOssssOOO.....",
        "...OOCCBBCCBBCCOO...",
        ".OOCCCCCBBBBCCCCCOO.",
        "OSOCkCCCCBBCCCCcCOSO",
        "OSOCkCCCCCCCCCCcCOSO",
        "OSOCAAAAAAAAAAAACOSO",
        "OSOCkCCCCAACCCCcCOSO",
        "OssOCkCCCCACCCCcOssO",
        ".OOOCCCCCCCCCCCCOOO."],
 front_post={20:"...OCCCCCCCCCCCCO...",21:"...OOOOOOOOOOOOOO..."},
 legs=dict(xs=(5,11), w=4, L=5), side_legs=dict(x0=6, w=5, L=5),
 side=["....................",
       ".....OOOOOOO........",
       "....OXXXXXXXO.......",
       "..XOXaXXaXXaXO......",
       ".XXOSSSSSSSSSSO.....",
       ".XXOsSSSSSSssSO.....",
       "...OsSSSSSSeESO.....",
       "...OsSSSSSSSSSNO....",
       "...OsSSSSSSSSSO.....",
       "...OssSSSSSSMMO.....",
       "....OssSSSSSSO......",
       ".....OOssssOO.......",
       "...OOCCBBCCCOO......",
       "..OCCCCCBBCCCCO.....",
       "..OCkCCCCOSSOCCO....",
       "..OCkCCCCOSSOCCO....",
       "..OAAAAAAOSSOAAO....",
       "..OCkCCCCOSSOCCO....",
       "..OCkCCCCOssOCCO....",
       "..OCCCCCCOOOOCCO...."],
 side_post={20:"...OCCCCCCCCCO......",21:"...OOOOOOOOOOO......"},
 sneeze={6:"....OSOOSSSSOOSO....",5:"....OShSSSSSSSsO....",9:"....OsMMMMMMMMsO....",10:".....OMTTTTTTMO....."},
)
JERRY['D'] = dict(name='PAJAMA KID', desc='small big-headed kid in starry yellow pajamas and a floppy blue nightcap, big eyes',
 pal=dict(O='k0',S='u4',s='u3',h='u5',E='w3',e='u1',N='m3',M='m1',T='w2',C='y4',c='y3',k='y4',A='u3',a='u3',B='w2',X='w3',P='y3',p='y2',F='u2',D='u5'),
 top=19, legs=dict(xs=(6,10), w=4, L=5), side_legs=dict(x0=6, w=4, L=5),
 capcol={'A':'y3','a':'u3','X':'w3','B':'y4'},
 front=["....................",
        "......OOOOOO.OOO....",
        ".....OAAaAAAOOXXO...",
        ".....OAaAAaAAAOOO...",
        "....OAAAAaAAAAAO....",
        "....OSSSSSSSSSSO....",
        "....OSEESSSSEESO....",
        "....OSEeSSSSeESO....",
        "....OsSSSNNSSSsO....",
        "....OsSSSDSSSSsO....",
        "....OssSSMMSSssO....",
        ".....OsssssssssO....",
        "......OOOOOOOO......",
        ".....OCCkCCCCCO.....",
        "...OSOCAkCCCACOSO...",
        "...OSOCCCCACCCOSO...",
        "...OSOCACCCCCAOSO...",
        "...OsOCCCACCCCOsO...",
        "...OOOCCCCCCCCOOO..."],
 side=["....................",
       ".OOO.OOOOOO.........",
       "OXXOOAAaAAAOO.......",
       "OOOAAaAAAaAAAO......",
       "...OAAAAaAAAAAO.....",
       "...OsSSSSSSSSSO.....",
       "...OsSSSSSSEESO.....",
       "...OsSSSSSSeESO.....",
       "...OsSSSSSSSSSNO....",
       "...OsSSSSSSSSSDO....",
       "...OssSSSSSSMMO.....",
       "....OssssssssO......",
       ".....OOOOOOOO.......",
       ".....OCCkCCCO.......",
       ".....OCAOSOCO.......",
       ".....OCCOSOAO.......",
       ".....OACOSOCO.......",
       ".....OCCOSSOO.......",
       ".....OOOOOOOO......."],
 sneeze={6:"....OSOOSSSSOOSO....",7:"....OSSSSSSSSSSO....",9:"....OsSSSSSSSSsO....",10:"....OsMMMMMMMMsO....",11:".....OMTTTTTTMO....."},
)
# --------------------------------------------------------------------------- ENEMIES
ENEMY = {}
ENEMY['A'] = dict(name='OFFICE WORKER', desc='green skin, comb-over, cracked glasses, torn beige shirt with red tie, grey trousers',
 pal=dict(O='k0',S='g3',s='g2',h='g4',E='y4',e='k0',N='g2',M='m1',T='w2',C='c3',c='c2',k='w2',A='m2',a='m1',W='g2',B='k1',R='r1',r='k1',P='k4',p='k3',F='k1'),
 front=["....................",
        "......OOOOOOOO......",
        ".....ORRRRRRrRO.....",
        "....ORrRRSSSSSSO....",
        "....OSSSSSSSSSSO....",
        "....OBBBBSSBBBBO....",
        "....OBEEBSSBeEBO....",
        "....OsBBBNNBBBsO....",
        "....OsSSSSSSSSsO....",
        "....OsSMMMMMMSsO....",
        ".....OsMTMTMMsO.....",
        "......OOssssOO......",
        ".....OOCCAACCOO.....",
        "...OOCkCCAACCCcOO...",
        "..OSOCkCCAACCCcOSO..",
        "..OSOCCCCAACCWcOSO..",
        "..OSOCkCCAACWWcOSO..",
        "..OsOCCCCaaCCCcOsO..",
        "..OssOCcCCCCcCcOssO.",
        "...OOOCcCOCCcOCOOO.."],
 side=["....................",
       ".....OOOOOOO........",
       "....ORRRRrRRO.......",
       "...ORrRRSSSSSO......",
       "...OSSSSSSSSSSO.....",
       "...OsSSSSSBBBBO.....",
       "...OsSSSSSBeEBO.....",
       "...OsSSSSSSBBSNO....",
       "...OsSSSSSSSSSO.....",
       "...OssSSSSSMMMO.....",
       "....OssSSSSMTO......",
       ".....OOssssOO.......",
       ".....OCCCAACO.......",
       "....OCCkCAACOOOOOO..",
       "....OCkCCAACCCSSShO.",
       "....OCCkCAACssssssO.",
       "....OCCCCaaCOOOOOO..",
       "....OCCCCWCCO.......",
       "....OcCCCCCcO.......",
       ".....OcCOCcO........"],
)
ENEMY['B'] = dict(name='HOODIE', desc='grey-green skin, purple hood pulled up, drawstrings and front pocket, jeans, white trainers',
 pal=dict(O='k0',S='n2',s='n1',h='n3',E='y4',e='k0',N='n1',M='m1',T='w2',C='p2',c='p1',k='p3',A='w2',B='p1',b='k1',W='n1',P='d2',p='d1',F='w2'),
 front=["....................",
        ".....OOOOOOOOOO.....",
        "....OCCCCCCCCCCO....",
        "...OCCkCCCCCCCCcO...",
        "...OCkOOOOOOOOCcO...",
        "...OCOSSSSSSSSOcO...",
        "...OCOSEESSEESOcO...",
        "...OCOSEeSSeESOcO...",
        "...OCOsSSSSSSsOcO...",
        "...OCOsSMMMMSsOcO...",
        "...OCCOsMTTMsOCcO...",
        "...OCCCOOOOOOCCcO...",
        "...OCCCCAACCCCCcO...",
        "..OOCkCCACCACCCcOO..",
        "..OCOCkCCCCCCCcOCO..",
        "..OCOCCBBBBBBCcOCO..",
        "..OCOCCBbbbbBCcOCO..",
        "..OSOCCBBBBBBCcOSO..",
        "..OssOCCCCCCCCOssO..",
        "...OOOCCCCCCCCOOO..."],
 side=["....................",
       ".....OOOOOOOO.......",
       "....OCCCCCCCCO......",
       "...OCCkCCCCCCCO.....",
       "...OCCCCCOOOOOOO....",
       "...OCCCCOSSSSSO.....",
       "...OCCCCOSSSEEO.....",
       "...OCCCCOSSSeEO.....",
       "...OCCCCOSSSSSNO....",
       "...OCCCCOsSSMMO.....",
       "...OCCCCCOsMTO......",
       "....OCCCCCOOOO......",
       ".....OCCCCCCO.......",
       "....OCCkCCCCOOOOOO..",
       "....OCkCCCCCCCCCSSO.",
       "....OCCkCCCCccccSSO.",
       "....OCBBBBCCOOOOOO..",
       "....OCBbbBCCO.......",
       "....OCBBBBCCO.......",
       ".....OCCCCCO........"],
)
ENEMY['C'] = dict(name='TRACKSUIT BRUTE', desc='wide and chunky, dark olive skin, bald with heavy brow and big jaw, red tracksuit with white stripes',
 pal=dict(O='k0',S='g2',s='g1',h='g3',E='y3',e='k0',N='g1',M='m1',T='w2',C='m2',c='m1',k='m3',A='w2',B='w3',W='g1',P='m2',p='m1',F='w2'),
 legs=dict(xs=(5,11), w=4, L=6), side_legs=dict(x0=6, w=5, L=6),
 front=["....................",
        "......OOOOOOOO......",
        ".....OSShSSSSSO.....",
        "....OSShSSSSSSSO....",
        "....OssSSSSSSssO....",
        "....OsSEeSSeESsO....",
        "....OSSSSSSSSSSO....",
        "...OSSSSSNNSSSSSO...",
        "...OSsMMMMMMMMsSO...",
        "...OSsMTTMTTTMsSO...",
        "....OssSSSSSSssO....",
        "....OOOssssssOOO....",
        "..OOCCCCAAAACCCCOO..",
        ".OCCCCCCCAACCCCCCCO.",
        "OBCOCkCCCCACCCCcOCBO",
        "OBCOCkCCCCACCCCcOCBO",
        "OBCOCCCCCCACCCCcOCBO",
        "OBCOCkCCCCACCCCcOCBO",
        "OSSOCCCCCCACCCCcOSSO",
        "OssOOOOOOOOOOOOOOssO"],
 side=["....................",
       ".....OOOOOOO........",
       "....OShSSSSSO.......",
       "...OSShSSSSSSO......",
       "...OSSSSSSSsssO.....",
       "...OsSSSSSSSeEO.....",
       "...OsSSSSSSSSSO.....",
       "...OsSSSSSSSSSNO....",
       "...OsSSSSSSSSSSO....",
       "...OssSSSSSMMMMO....",
       "...OssSSSSSMTTMO....",
       "....OOOssssOOOO.....",
       "...OOCCCCAACCOO.....",
       "..OCCCCCCAACCOOOOOO.",
       "..OCkCCCCAACCBBBSShO",
       "..OCkCCCCAACCCCCSSSO",
       "..OCCCCCCAACCccsssO.",
       "..OCkCCCCAACOOOOOO..",
       "..OCkCCCCAACCO......",
       "...OOOOOOOOOO......."],
)
ENEMY['D'] = dict(name='ZOMBIE PARAMEDIC', desc='tall and thin, pale yellow-green skin, orange jacket with reflective stripes, navy cap; fits "take him to hospital"',
 pal=dict(O='k0',S='n3',s='n2',h='w2',E='y4',e='m1',N='n2',M='m1',T='w2',C='e2',c='e1',k='y4',A='d2',a='d1',B='y4',W='m1',P='d1',p='k1',F='k0'),
 legs=dict(xs=(7,10), w=3, L=7), side_legs=dict(x0=7, w=3, L=7),
 front=["......OOOOOOOO......",
        ".....OAAaAAAAAO.....",
        ".....OAAAAAAAAO.....",
        "....OOOOOOOOOOOO....",
        ".....OSSSSSSSSO.....",
        ".....OSEESSEESO.....",
        ".....OSeESSEeSO.....",
        ".....OsSSNNSSsO.....",
        ".....OsSSSSSSsO.....",
        ".....OsMMMMMMsO.....",
        ".....OsMTMMTMsO.....",
        "......OOssssOO......",
        ".....OCCkCCkCCO.....",
        "....OCCCCCCCCCCO....",
        "...OCOCCCCCCCCOCO...",
        "...OCOBBBBBBBBOCO...",
        "...OCOCCCCCCCCOCO...",
        "...OBOCCCCWCCCOBO...",
        "...OCOCCCCCCCCOCO...",
        "...OSOCcCCCCcCOSO..."],
 front_post={20:"...OsO........OsO...",21:"...OOO........OOO..."},
 side=[".....OOOOOOO........",
       "....OAAaAAAAO.......",
       "....OAAAAAAAAOOOO...",
       "....OOOOOOOOOOOO....",
       "....OSSSSSSSSSO.....",
       "....OsSSSSSSEEO.....",
       "....OsSSSSSSeEO.....",
       "....OsSSSSSSSSNO....",
       "....OsSSSSSSSSO.....",
       "....OssSSSSMMMO.....",
       "....OssSSSSMTO......",
       ".....OOssssOO.......",
       "......OCCkCO........",
       ".....OCCCCCCOOOOOOO.",
       ".....OCCCCCCCCCCSShO",
       ".....OBBBBBBBBBBSSSO",
       ".....OCCCCCCOOOOOOO.",
       ".....OBBBBBBO.......",
       ".....OCCCWCCO.......",
       ".....OcCCCCcO......."],
)

def frame(spec, view, f=0):
    pal = dict(spec['pal'])
    img = new()
    bob = 1 if f in (1,3) else 0
    top = spec.get('top', 20)
    lg = spec.get('legs', {}); sl = spec.get('side_legs', {})
    capcol = spec.get('capcol')
    if view == 'down':
        legs_front(img, pal, f, bob, top=top, **lg)
        p2 = dict(pal)
        rows = spec['front']
        if capcol:   # nightcap uses its own colours on the top rows only
            hp = dict(pal); hp.update(capcol); stamp(img, rows[:5], hp, dy=bob); stamp(img, rows[5:], pal, dy=bob, y0=5)
        else: stamp(img, rows, pal, dy=bob)
        for y,r in spec.get('front_post',{}).items(): stamp(img,[r],pal,dy=bob,y0=y)
    elif view == 'up':
        legs_front(img, pal, f, bob, top=top, **lg)
        stamp(img, spec['back'], pal, dy=bob)
        for y,r in spec.get('back_post', spec.get('front_post',{})).items(): stamp(img,[r],pal,dy=bob,y0=y)
    elif view in ('right','left'):
        m = view == 'left'
        legs_side(img, pal, f, bob, mirror=m, top=top, **sl)
        rows = spec['side']
        if capcol:
            hp = dict(pal); hp.update(capcol); stamp(img, rows[:5], hp, dy=bob, mirror=m); stamp(img, rows[5:], pal, dy=bob, mirror=m, y0=5)
        else: stamp(img, rows, pal, dy=bob, mirror=m)
        for y,r in spec.get('side_post',{}).items(): stamp(img,[r],pal,dy=bob,y0=y,mirror=m)
    return img

def sneeze(spec):
    pal = dict(spec['pal']); img = new()
    top = spec.get('top', 20); lg = spec.get('legs', {})
    rows = [spec['sneeze'].get(i, r) for i, r in enumerate(spec['front'])]
    legs_front(img, pal, 0, 1, top=top, **lg)
    capcol = spec.get('capcol')
    if capcol:
        hp = dict(pal); hp.update(capcol); stamp(img, rows[:5], hp, dy=1); stamp(img, rows[5:], pal, dy=1, y0=5)
    else: stamp(img, rows, pal, dy=1)
    for y,r in spec.get('front_post',{}).items(): stamp(img,[r],pal,dy=1,y0=y)
    my = max(i for i in spec['sneeze']) + 2
    for (x,y) in [(9,my),(10,my),(8,my+1),(11,my+1),(7,my+2),(12,my+2),(10,my+2),(6,my+3),(13,my+3)]:
        put(img,x,y,'u5' if (x+y)%2 else 'w3')
    for (x,y,k) in [(2,3,'u5'),(17,2,'u5'),(1,6,'w3'),(18,7,'w3')]: put(img,x,y,k)
    for (x,y,k) in spec.get('sneeze_extra',[]): put(img,x,y,pal[k])
    return img

# ---------------------------------------------------------------- back views
JERRY['A']['back'] = ["........O.O.........",
        "......OOROROO.......",
        ".....OSSSSSSSO......",
        "....OSSSSSSsSSSO....",
        "....OSsSSSSSSSsO....",
        "....OSSSSSsSSSSO....",
        "....OsSSSSSSSSsO....",
        "....OsSSSSSSSSsO....",
        "....OssSSSSSSssO....",
        "....OsssSSSSsssO....",
        ".....OssssssssO.....",
        "......OOssssOO......",
        ".....OCCCOOCCCO.....",
        "...OOCCcCSSCcCCOO...",
        "..OSOCACCSSCCACOSO..",
        "..OSOCCCXSSXCCCOSO..",
        "..OSOCACCSSCCACOSO..",
        "..OSOCCCCSSCCCCOSO..",
        "..OssOCACSSCACOssO..",
        "...OOOCCCSSCCCOOO..."]
JERRY['A']['back_post'] = {20:".....OCcCCSSCCcO....",21:".....OOOOOOOOOOO...."}
ENEMY['A']['back'] = ["....................",
        "......OOOOOOOO......",
        ".....ORRRRRRRRO.....",
        "....ORRrRRRRrRRO....",
        "....ORRRRRRRRRRO....",
        "....OSRRrRRRRRSO....",
        "....OSSRRRRRRSSO....",
        "....OsSSSSSSSSsO....",
        "....OsSSSSSSSSsO....",
        "....OssSSSSSSssO....",
        ".....OssssssssO.....",
        "......OOssssOO......",
        ".....OOCCCCCCOO.....",
        "...OOcCCCCCCCCcOO...",
        "..OSOcCCCCCCCCcOSO..",
        "..OSOcCCCWCCCCcOSO..",
        "..OSOcCCCCCCCCcOSO..",
        "..OsOcCCCCCCWCcOsO..",
        "..OssOcCCCCCCcOssO..",
        "...OOOcCCOCCcCOOO..."]
ENEMY['B']['back'] = ["....................",
        ".....OOOOOOOOOO.....",
        "....OCCCCCCCCCCO....",
        "...OCCkCCCCCCCCcO...",
        "...OCCCCCCCCCCCcO...",
        "...OCCCCCCCCCCCcO...",
        "...OCCCCcCCCCCCcO...",
        "...OCCCCCcCCCCCcO...",
        "...OCCCCCCcCCCCcO...",
        "...OcCCCCCCCCCCcO...",
        "...OccCCCCCCCCccO...",
        "...OcccCCCCCCcccO...",
        "...OCCCCCCCCCCCcO...",
        "..OOCkCCCCCCCCCcOO..",
        "..OCOCkCCCCCCCcOCO..",
        "..OCOCCCCCCCCCcOCO..",
        "..OCOCCCCCCCCCcOCO..",
        "..OSOCCCCCCCCCcOSO..",
        "..OssOCCCCCCCCOssO..",
        "...OOOCCCCCCCCOOO..."]
ENEMY['C']['back'] = ["....................",
        "......OOOOOOOO......",
        ".....OSSSSSSSSO.....",
        "....OSShSSSSSSSO....",
        "....OSSSSSSSSSSO....",
        "....OSSSSSSSSSSO....",
        "....OsSSSSSSSSsO....",
        "...OSsSSSSSSSSsSO...",
        "...OSssSSSSSSssSO...",
        "...OSsssSSSSsssSO...",
        "....OssssssssssO....",
        "....OOOssssssOOO....",
        "..OOCCCCCCCCCCCCOO..",
        ".OCCCCCCCCCCCCCCCCO.",
        "OBCOCkCCCCCCCCCcOCBO",
        "OBCOCkCBBBBBBCCcOCBO",
        "OBCOCCCCCCCCCCCcOCBO",
        "OBCOCkCCCCCCCCCcOCBO",
        "OSSOCCCCCCCCCCCcOSSO",
        "OssOOOOOOOOOOOOOOssO"]
ENEMY['D']['back'] = ["......OOOOOOOO......",
        ".....OAAaAAAAAO.....",
        ".....OAAAAAAAAO.....",
        ".....OOOOOOOOOO.....",
        ".....OSSSSSSSSO.....",
        ".....OSSSsSSSSO.....",
        ".....OSSSSSSSSO.....",
        ".....OsSSSSSSsO.....",
        ".....OssSSSSssO.....",
        ".....OsssSSsssO.....",
        ".....OssssssssO.....",
        "......OOssssOO......",
        ".....OCCCCCCCCO.....",
        "....OCCCCCCCCCCO....",
        "...OCOCCCCCCCCOCO...",
        "...OCOBBBBBBBBOCO...",
        "...OCOCCCZZCCCOCO...",
        "...OBOCZZZZZZCOBO...",
        "...OCOCCCZZCCCOCO...",
        "...OSOCcCCCCcCOSO..."]
ENEMY['D']['pal']['Z'] = 'w3'
ENEMY['D']['pal']['K'] = 'w3'      # white socks above black loafers (the MJ nod)
ENEMY['D']['pal']['F'] = 'k0'
ENEMY['D']['pal']['P'] = 'd2'; ENEMY['D']['pal']['p'] = 'd1'
ENEMY['D']['desc'] = 'tall and thin, pale yellow-green skin, orange jacket with reflective stripes, navy cap, white socks and black shoes; the moonwalker'

# ---------------------------------------------------------------- C redesigned: Thriller dancer
ENEMY['C'] = dict(name='THRILLER DANCER', desc='slim, green skin, curly black hair, red jacket with black V stripes, red trousers, white socks, black loafers; moonwalks and does the Thriller shuffle',
 pal=dict(O='k0',S='g3',s='g2',h='g4',E='y4',e='k0',N='g2',M='m1',T='w2',C='m2',c='m1',k='m3',A='k1',B='k1',W='g2',R='k3',r='k5',P='m2',p='m1',F='k0',K='w3'),
 front=["......OOOOOOOO......",
        ".....ORrRRrRRrO.....",
        "....ORRRrRRRrRRO....",
        "...ORrRSSSSSSRrRO...",
        "...ORRSSSSSSSSRRO...",
        "...ORrSEESSEESrRO...",
        "...ORRSEeSSeESRRO...",
        "...OrRsSSNNSSsRrO...",
        "...ORRsSSSSSSsRRO...",
        "...OrRsSMMMMSsRrO...",
        "....OrOsMTTMsOrO....",
        "......OOssssOO......",
        "....OOCCBAABCCOO....",
        "..OOCCBCCAACCBCCOO..",
        "..OCBCCBCAACBCCBCO..",
        "..OCOCCCBAABCCCOCO..",
        "..OBOCCCCBBCCCCOBO..",
        "..OCOkCCCCCCCCcOCO..",
        "..OSOCCCCCCCCCcOSO..",
        "..OssOCCCWCCCCOssO.."],
 side=[".....OOOOOOO........",
       "....ORrRRrRRO.......",
       "...ORRRrRRRrRO......",
       "..ORrRRRSSSSSSO.....",
       "..ORRrRSSSSSSSSO....",
       "..OrRRsSSSSSEESO....",
       "..ORrRsSSSSSeESO....",
       "..OrRRsSSSSSSSSNO...",
       "...OrRsSSSSSSSSO....",
       "...ORrssSSSSSMMMO...",
       "....OrOssSSSSMTO....",
       ".....OOOssssOO......",
       ".....OCBCAACO.......",
       "....OCBCCAACOOOOOO..",
       "....OBCCCAACCBCCShO.",
       "....OCBCCAACCBCCSSO.",
       "....OCCBCAACOOOOOO..",
       "....OCCCBAACO.......",
       "....OkCCCCCcO.......",
       ".....OcCWCcO........"],
 back=["......OOOOOOOO......",
       ".....ORrRRrRRrO.....",
       "....ORRRrRRRrRRO....",
       "...ORrRRRrRRRrRRO...",
       "...ORRrRRRRrRRRrO...",
       "...ORrRRrRRRrRRRO...",
       "...ORRRrRRrRRRrRO...",
       "...OrRRRRRRRRRRrO...",
       "....OrRrRRrRRrRO....",
       ".....OrRRrRRrRO.....",
       "......OssssssO......",
       "......OOssssOO......",
       "....OOCCCCCCCCOO....",
       "..OOCCBCCCCCCBCCOO..",
       "..OCBCCBCCCCBCCBCO..",
       "..OCOCCCBCCBCCCOCO..",
       "..OBOCCCCBBCCCCOBO..",
       "..OCOkCCCCCCCCcOCO..",
       "..OSOCCCCCCCCCcOSO..",
       "..OssOCCCWCCCCOssO.."],
)
# paramedic: no MJ socks any more, plain black shoes
ENEMY['D']['pal'].pop('K', None)
ENEMY['D']['desc'] = 'tall and thin, pale yellow-green skin, orange jacket with reflective stripes, navy cap, white cross on the back, face mask he pulls up'
