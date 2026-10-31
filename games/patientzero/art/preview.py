from PIL import Image
from pal import P
from font import draw_text
import build
import ltiles as L
ROOT='/workspace/patientzero'
s = build.main()
lsheet, lprops = L.build_level_sheets()
s['tiles_levels']=lsheet; s['props_levels']=lprops
W,H = 680, 620
img = Image.new('RGBA',(W,H),P['k1']+(255,))
def checker(x,y,w,h):
    for yy in range(y,y+h):
        for xx in range(x,x+w):
            img.putpixel((xx,yy),P['k2' if ((xx-x)//4+(yy-y)//4)%2 else 'k3']+(255,))
def place(name, x, y, label):
    im = s[name]; draw_text(img, x, y, label, P['y3'])
    checker(x, y+9, im.width, im.height); img.alpha_composite(im,(x,y+9))
    return y+9+im.height+8
draw_text(img, 4, 4, 'PATIENT ZERO - ART (BASE PIXELS AT 4X, 2X OF THE 1280X720 SHEETS)', P['w2'])
x0, y = 58, 16
ys = place('sprites', x0, y, 'SPRITES.PNG  20X30 CELLS')
labels = ['JERRY','JERRY','ZOMBIE','  CURE','OFFICE','  CURE','HOODIE','  CURE','DANCER',' CURE+','MOONWALK','MEDIC',' CURE+']
for i,t in enumerate(labels): draw_text(img, 2, y+9+i*30+11, t[:9], P['w1'])
place('angels', x0, ys, 'ANGELS.PNG  30X32, ONE ROW PER TYPE')
x1, y = 392, 16
y = place('tiles', x1, y, 'TILES.PNG  20X20')
y = place('props', x1, y, 'PROPS.PNG')
y = place('fx', x1, y, 'FX.PNG')
draw_text(img, x1, y, 'PALETTE (%d COLOURS)' % len(P), P['y3'])
for i,(k,v) in enumerate(P.items()):
    xx, yy = x1 + (i%24)*11, y+10+(i//24)*11
    for a in range(9):
        for b in range(9): img.putpixel((xx+a,yy+b), v+(255,))
y += 38
y = place('tiles_levels', x1, y, 'TILES_LEVELS.PNG')
y = place('props_levels', x1, y, 'PROPS_LEVELS.PNG')
place('logo', x0+130, 400, 'LOGO.PNG') if False else None
img = img.resize((W*4,H*4), Image.NEAREST)
img.save(f'{ROOT}/preview/spritesheet_preview_2x.png'); print(img.size, y)
