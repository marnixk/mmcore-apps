from PIL import Image
from pal import P
from font import G
def big_text(s, scale, grad, sp=None, outline='k0', inner='b1', shine=None):
    sp = scale if sp is None else sp
    gw, gh = 6*scale, 7*scale
    W = len(s)*(gw+sp)-sp + 4; H = gh + 5
    mask = [[False]*W for _ in range(H)]
    for i,ch in enumerate(s):
        g = [''.join('1' if (row+'0')[i]=='1' or ('0'+row)[i]=='1' else '0' for i in range(6)) for row in G[ch]]  # bold
        for yy,row in enumerate(g):
            for xx,c in enumerate(row):
                if c=='1':
                    for a in range(scale):
                        for b in range(scale):
                            mask[2+yy*scale+a][2+i*(gw+sp)+xx*scale+b] = True
    img = Image.new('RGBA',(W,H),(0,0,0,0))
    def on(x,y): return 0<=x<W and 0<=y<H and mask[y][x]
    # drop shadow (2px down) and outlines
    for y in range(H):
        for x in range(W):
            if on(x,y-2) or on(x-1,y-2) or on(x+1,y-2): img.putpixel((x,y),P['k0']+(255,))
    for y in range(H):
        for x in range(W):
            if not on(x,y) and any(on(x+dx,y+dy) for dx in (-1,0,1) for dy in (-1,0,1)):
                img.putpixel((x,y),P[inner]+(255,))
            if not on(x,y) and not any(on(x+dx,y+dy) for dx in (-1,0,1) for dy in (-1,0,1)) and any(on(x+dx,y+dy) for dx in (-2,-1,0,1,2) for dy in (-2,-1,0,1,2)):
                img.putpixel((x,y),P[outline]+(255,))
    for y in range(H):
        for x in range(W):
            if on(x,y):
                row = y-2
                k = grad[min(len(grad)-1, row*len(grad)//gh)]
                if shine and row == shine: k = 'w3'
                # bevel: top edge of each stroke lighter
                if not on(x,y-1): k = 'y4' if k!='w3' else k
                img.putpixel((x,y),P[k]+(255,))
    return img

def logo():
    l1 = big_text("PATIENT ZERO:", 3, ['y4','y3','y3','y3','y2','y2','y1'], shine=None)
    l2 = big_text("BLESS YOU", 5, ['y4','y3','y3','y3','y2','y2','y2','y1'], shine=None)
    W = max(l1.width, l2.width); H = l1.height + l2.height + 2
    img = Image.new('RGBA',(W,H),(0,0,0,0))
    img.alpha_composite(l1, ((W-l1.width)//2, 0))
    img.alpha_composite(l2, ((W-l2.width)//2, l1.height+2))
    return img
