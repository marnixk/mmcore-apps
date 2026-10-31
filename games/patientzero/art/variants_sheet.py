import os, random
from PIL import Image
from pal import P
import variants as V, chars
from font import draw_text, draw_text_c, text_width
from mock_common import street_level, ruined_props, blit, blit_im, fr, scale2, rect
OUT = '/workspace/patientzero/variants'; os.makedirs(OUT, exist_ok=True)

def frames_j(spec):
    if spec is None:
        return [chars.char('jerry','down',0), chars.char('jerry','down',1), chars.char('jerry','right',1), chars.char('jerry','left',3), chars.jerry_pose('sneeze1')]
    return [V.frame(spec,'down',0), V.frame(spec,'down',1), V.frame(spec,'right',1), V.frame(spec,'left',3), V.sneeze(spec)]
def frames_e(spec):
    if spec is None:
        return [chars.char('zombie','down',0), chars.char('zombie','down',1), chars.char('zombie','right',1), chars.char('zombie','left',3)]
    return [V.frame(spec,'down',0), V.frame(spec,'down',1), V.frame(spec,'right',1), V.frame(spec,'left',3)]

def comparison():
    W, H = 268, 252
    img = Image.new('RGBA',(W,H),P['k1']+(255,))
    draw_text(img, 4, 3, 'CHARACTER VARIANTS - 20X30 SPRITES SHOWN 4X', P['w2'])
    cols = [(4,'PLAYER: JERRY', V.JERRY, frames_j, ['IDLE','WALK','SIDE','SIDE','SNEEZE']),
            (140,'ENEMY ZOMBIES', V.ENEMY, frames_e, ['IDLE','WALK','SIDE','SIDE'])]
    for x0, title, group, fn, caps in cols:
        draw_text(img, x0, 15, title, P['y3'])
        for ci,c in enumerate(caps):
            tmp = Image.new('RGBA',(text_width(c),7),(0,0,0,0)); draw_text(tmp,0,0,c,P['k5'])
            tmp = tmp.resize((tmp.width//2+1, 4), Image.NEAREST)  # tiny caption
        rows = [('CURRENT', None)] + [(k+'  '+group[k]['name'], group[k]) for k in 'ABCD']
        for ri,(label, spec) in enumerate(rows):
            y = 26 + ri*45
            draw_text(img, x0, y, label, P['u4'] if ri else P['k5'])
            for fi, im in enumerate(fn(spec)):
                fx, fy = x0 + fi*25, y+9
                for yy in range(fy, fy+32):
                    for xx in range(fx, fx+22):
                        img.putpixel((xx,yy), P['k2' if ((xx-fx)//4+(yy-fy)//4)%2 else 'k3']+(255,))
                img.alpha_composite(im, (fx+1, fy+1))
    img = img.resize((W*4,H*4), Image.NEAREST)
    img.save(f'{OUT}/character_variants_4x.png'); return img

def pairs_mockup():
    img = street_level(3); ruined_props(img)
    for (n,x,y) in (('puddle_l_0',100,250),('puddle_w_0',470,170),('puddle_l_1',560,270),('puddle_l_2',230,95)):
        blit(img,'props',n,x,y)
    zones = {'A':(150,110),'B':(470,110),'C':(150,255),'D':(470,255)}
    r = random.Random(1)
    for k,(cx,cy) in zones.items():
        j = V.JERRY[k]; e = V.ENEMY[k]
        jim = V.sneeze(j) if k in 'AD' else V.frame(j,'right' if k=='B' else 'down',1)
        blit_im(img, jim, cx-10, cy-15)
        if k in 'AD':
            # small cloud around a sneeze
            for i in range(10):
                blit(img,'fx',r.choice(['puff16_0','puff16_1','puff16_2']), cx-34+r.randrange(0,56)-8*(i%2), cy+2+r.randrange(-6,8))
            for i in range(25):
                blit(img,'fx',f'part_{r.choice(["white","pale","blue"])}_{r.choice([1,2,3,4])}', cx-50+r.randrange(100), cy-8+r.randrange(28))
            blit_im(img, jim, cx-10, cy-15)
        for (dx,dy,v,f) in ((-75,-20,'right',1),(70,-5,'left',3),(-40,35,'right',3),(55,40,'left',1),(30,-38,'down',2)):
            blit_im(img, V.frame(e,v,f), cx+dx-10, cy+dy-15)
        # tag
        t = k + ': ' + j['name'].split()[0] + ' + ' + e['name'].split()[-1]
        w = text_width(t)+6
        rect(img, cx-w//2, cy-70, w, 11, 'k0'); draw_text(img, cx-w//2+3, cy-68, t, P['y3'])
    out = scale2(img); out.save(f'{OUT}/variants_ingame_pairs_1280x720.png'); return out

def mixed_mockup(jk='B'):
    img = street_level(5); ruined_props(img)
    for (n,x,y) in (('puddle_l_3',90,240),('puddle_w_0',470,170),('puddle_l_1',560,270),('puddle_l_2',230,95)):
        blit(img,'props',n,x,y)
    r = random.Random(7)
    keys = 'ABCD'
    spots = [(60,160),(110,210),(200,90),(260,250),(420,80),(520,120),(590,200),(470,280),(360,300),(40,90),(560,60),(150,300)]
    for i,(x,y) in enumerate(spots):
        e = V.ENEMY[keys[i%4]]
        v = 'right' if x < 320 else 'left'
        if y < 110 and 300 < x < 500: v = 'down'
        blit_im(img, V.frame(e, v, r.randrange(4)), x, y)
    j = V.JERRY[jk]
    blit_im(img, V.frame(j,'down',0), 310, 175)
    for i in range(5): blit(img,'fx','drop_4' if i<3 else 'drop_0', 303+i*7, 161)
    t = 'MIXED CROWD: ENEMIES A-D TOGETHER, JERRY ' + jk
    w = text_width(t)+6; rect(img, 320-w//2, 342, w, 11, 'k0'); draw_text(img, 320-w//2+3, 344, t, P['y3'])
    out = scale2(img); out.save(f'{OUT}/variants_mixed_crowd_1280x720.png'); return out

if __name__ == '__main__':
    comparison(); pairs_mockup(); mixed_mockup()
    # per-variant sprite strips (all directions x 4 frames) for later use
    for grp, nm in ((V.JERRY,'jerry'),(V.ENEMY,'zombie')):
        for k,spec in grp.items():
            strip = Image.new('RGBA',(20*13, 30),(0,0,0,0))
            ims = [V.frame(spec,'down',f) for f in range(4)] + [V.frame(spec,'right',f) for f in range(4)] + [V.frame(spec,'left',f) for f in range(4)]
            if nm=='jerry': ims.append(V.sneeze(spec))
            for i,im in enumerate(ims): strip.alpha_composite(im,(i*20,0))
            strip.save(f'{OUT}/{nm}_{k}_frames_base.png')
    print('ok')
