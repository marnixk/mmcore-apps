import random, math, os
from mock_common import *
OUT='/workspace/patientzero/mockups'; os.makedirs(OUT, exist_ok=True)

def cloud(img, cx, cy, spread_x, spread_y, n_puffs, n_parts, seed):
    r = random.Random(seed)
    pts = []
    for i in range(n_puffs):
        a = r.uniform(0, 2*math.pi); d = math.sqrt(r.random())
        x = cx + math.cos(a)*spread_x*d; y = cy + math.sin(a)*spread_y*d - 4
        pts.append((y, x, r.choice(['puff16_0','puff16_1','puff16_2','puff16_3','puff24_0','puff24_1','puff32_0'] if d < .6 else ['puff16_0','puff16_1','puff16_2','puff16_3'])))
    for y,x,n in sorted(pts): 
        im = fr('fx',n); blit_im(img, im, x-im.width/2, y-im.height/2)
    for i in range(n_parts):
        a = r.uniform(0, 2*math.pi); d = r.uniform(0.6, 1.35)
        x = cx + math.cos(a)*spread_x*d; y = cy + math.sin(a)*spread_y*d
        col = r.choice(['white','pale','pale','blue','deep']); sz = r.choice([1,2,2,3,3,4,6])
        blit(img,'fx',f'part_{col}_{sz}', x-4, y-4)
    for i in range(10):
        blit(img,'fx',r.choice(['sparkle_0','sparkle_1']), cx+r.uniform(-spread_x,spread_x)-3, cy+r.uniform(-spread_y,spread_y)-3)

def ascending(img, x, feet_y, top_y, frame):
    # beam from the top of the field down to the feet, glow pool, angel at mid height
    y = top_y
    while y < feet_y-6:
        blit(img,'fx','beam', x-20, y); y += 20
    blit(img,'fx','beam_glow', x-20, feet_y-8)
    return

def gameplay():
    img = street_level(2)
    ruined_props(img)
    for (n,x,y) in (('puddle_l_0',100,250),('puddle_w_0',470,170),('puddle_l_1',560,270),('puddle_s_0',380,110),('puddle_l_2',230,95)):
        blit(img,'props' if 'puddle_s' not in n else 'tiles',n,x,y)
    # mixed crowd closing in
    for (n,x,y) in (('zombie_right_1',60,180),('zombie_down_2',150,95),('office_left_1',540,120),
                    ('hoodie_right_3',95,200),('hoodie_down_1',420,64),('zombie_left_0',600,200),
                    ('office_right_0',30,90),('zombie_up_1',470,282)):
        blit(img,'sprites',n,x,y)
    # Thriller dancer moonwalking toward Jerry: faces away (right) while sliding left
    blit(img,'sprites','dancer_moonwalk_r_1',425,250)
    for k in range(3): rect(img,449+k*7,276+k*2,5,1,'k5')
    blit(img,'sprites','dancer_thriller_0',60,280)
    # paramedic guarding a puddle, mask up because Jerry is charging nearby
    blit(img,'sprites','medic_mask_left',585,250)
    blit(img,'sprites','medic_down_1',130,232)
    # beam + ascending humans (each type keeps its look)
    bx = 400
    ascending(img, bx, 236, 60, 0)
    blit(img,'sprites','office_cure_2', bx-10, 206)
    blit(img,'angels','zombie_angel_0', bx-15, 106)
    # sneeze cloud on both sides of Jerry
    jx, jy = 300, 170
    cloud(img, jx-25, jy+16, 62, 26, 22, 70, 5); cloud(img, jx+45, jy+16, 62, 26, 22, 70, 6)
    blit(img,'sprites','hoodie_hit_1', 225, 160)
    blit(img,'sprites','zombie_cure_0', 352, 168)
    blit(img,'sprites','office_hit_1', 205, 180)
    blit(img,'sprites','jerry_sneeze2', jx, jy)
    for i in range(5): blit(img,'fx','drop_4' if i<4 else 'drop_0', jx-7+i*7, jy-14)
    return img

def moonwalk_strip():
    names = [('dancer_moonwalk_r_0','SLIDE1'),('dancer_moonwalk_r_1','SLIDE2'),('dancer_moonwalk_r_2','SLIDE3'),('dancer_moonwalk_r_3','SLIDE4'),
             ('dancer_down_0','SPIN1'),('dancer_left_0','SPIN2'),('dancer_up_0','SPIN3'),('dancer_right_0','SPIN4'),('dancer_toe_r','TOE UP'),
             ('dancer_thriller_0','SHUF1'),('dancer_thriller_1','SHUF2')]
    W = len(names)*40+4; H = 54
    img = Image.new('RGBA',(W,H),P['k1']+(255,))
    draw_text(img, 3, 2, 'THRILLER DANCER: MOONWALK, SPIN, TOE STAND, SHUFFLE', P['y3'])
    for i,(n,l) in enumerate(names):
        x = 4 + i*40
        for yy in range(12,44):
            for xx in range(x+8,x+30): img.putpixel((xx,yy),P['k2' if ((xx-x)//4+(yy-12)//4)%2 else 'k3']+(255,))
        blit(img,'sprites',n,x+9,13)
        draw_text_c(img, x+19, 46, l, P['w1'])
    big(img,4).save('/workspace/patientzero/mockups/moonwalk_strip_4x.png')

def title():
    img = build.city_bg()
    # light beam top right with angel
    for y in range(0, 300, 20): blit_im(img, big(fr('fx','beam'),2), 490, y)
    blit_im(img, big(fr('fx','beam_glow'),2), 490, 282)
    blit_im(img, big(fr('angels','angel_1'),2), 500, 160)
    blit_im(img, big(fr('sprites','cure_2'),2), 510, 230)
    # Jerry sneezing left at a group of zombies
    r = random.Random(3)
    pts=[]
    mx, my = 304, 250          # Jerry's mouth (2x sprite at 300,228)
    for i in range(30):
        t = r.random()**0.8                        # 0 at the mouth, 1 far left
        x = mx - 20 - t*230; spread = 8 + t*42
        y = my + r.uniform(-spread, spread*0.8)
        sz = 'puff16_%d' % r.randrange(4) if t < 0.25 else r.choice(['puff24_0','puff24_1','puff32_0','puff16_1'])
        pts.append((y,x,sz))
    for y,x,n in sorted(pts):
        im = big(fr('fx',n),2); blit_im(img, im, x-im.width/2, y-im.height/2)
    for i in range(90):
        t = r.random(); x = mx - t*260; spread = 4 + t*55
        y = my + r.uniform(-spread, spread)
        blit(img,'fx',f'part_{r.choice(["white","pale","blue"])}_{r.choice([1,2,3,4,6])}', x-4, y-4)
    for (n,x,y) in (('zombie_right_0',20,226),('zombie_right_2',70,232),('zombie_hit_1',120,224),('zombie_right_1',40,248)):
        blit_im(img, big(fr('sprites',n),2), x, y)
    blit_im(img, big(fr('sprites','jerry_sneeze_l'),2), 300, 228)
    lg = fr('logo','logo'); blit_im(img, lg, (640-lg.width)//2, 26)
    # menu
    items = ['START GAME','HIGH SCORES','QUIT']
    for i,t in enumerate(items):
        y = 112 + i*22
        col = P['y4'] if i==0 else P['w1']
        tmp = Image.new('RGBA',(text_width(t)+1,8),(0,0,0,0)); draw_text(tmp,0,0,t,col,shadow=P['k0'])
        tmp = big(tmp,2); blit_im(img,tmp,320-tmp.width//2,y)
        if i==0: blit_im(img, big(fr('fx','drop_4'),2), 320-tmp.width//2-22, y-2)
    return img

def pause():
    base = street_level(2); ruined_props(base)
    for (n,x,y) in (('puddle_l_3',100,250),('puddle_w_0',470,170),('puddle_l_1',560,270),('puddle_s_0',380,110),('puddle_l_2',230,95)):
        blit(base,'props' if 'puddle_s' not in n else 'tiles',n,x,y)
    for (n,x,y) in (('zombie_left_1',160,215),('zombie_left_3',200,180),('zombie_down_2',150,95),('zombie_left_1',540,120),('zombie_up_0',470,280),('zombie_right_2',30,150)):
        blit(base,'sprites',n,x,y)
    blit(base,'sprites','jerry_charge2', 110, 228)
    blit(base,'fx','meter_7', 109, 216)
    img = darken(base)
    w, h = 250, 132; x, y = (640-w)//2, (360-h)//2
    frame_box(img, x, y, w, h)
    draw_text_c(img, 320, y+12, 'PAUSED', P['y3'], shadow=P['k0'])
    rect(img, x+20, y+24, w-40, 1, 'k4')
    rows = [('ARROWS','MOVE'),('Z (HOLD)','CHARGE SNEEZE'),('V','SNEEZE'),('ESC / P','RESUME'),('Q','QUIT TO TITLE')]
    for i,(k,v) in enumerate(rows):
        yy = y+34+i*15
        draw_text(img, x+30, yy, k, P['u4'], shadow=P['k0'])
        draw_text(img, x+118, yy, v, P['w2'], shadow=P['k0'])
    draw_text_c(img, 320, y+h-16, 'STAND IN PUDDLES TO REFILL', P['k5'])
    return img, base

def gameover():
    img = build.city_bg(seed=9)
    t = __import__('logo').big_text('GAME OVER', 3, ['m3','m3','m2','m2','m2','m1','m1'], inner='m1')
    blit_im(img, t, (640-t.width)//2, 18)
    draw_text_c(img, 320, 52, 'JERRY WAS TAKEN TO HOSPITAL', P['w2'], shadow=P['k0'])
    draw_text_c(img, 320, 66, 'ZOMBIES CURED  17      SCORE  22729', P['y3'], shadow=P['k0'])
    w, h = 260, 236; x, y = (640-w)//2, 86
    frame_box(img, x, y, w, h)
    draw_text_c(img, 320, y+10, 'HIGH SCORES', P['y3'], shadow=P['k0'])
    draw_text_c(img, 320, y+22, 'NEW HIGH SCORE - TYPE YOUR NAME', P['u4'])
    table = [('ZOE',40110),('MARNIX',31266),('JERRY',26740),('MARNI',22729),('BOB',18716),('ANNA',14707),('KEES',9358),('IREAL',6685),('LD33',4011),('ZZZ',1337)]
    for i,(n,s) in enumerate(table):
        yy = y+40+i*17
        me = i==3
        if me: rect(img, x+12, yy-4, w-24, 15, 'u1')
        col = P['w3'] if me else P['w1']
        draw_text(img, x+24, yy, '%2d.' % (i+1), col)
        draw_text(img, x+52, yy, n if not me else 'MARNI', col)
        if me: rect(img, x+52+text_width('MARNI')+2, yy+6, 5, 1, 'y3')
        sx = x+w-24-text_width(str(s)); draw_text(img, sx, yy, str(s), P['y4'] if me else col)
    draw_text_c(img, 320, y+h-14, 'ENTER TO SAVE', P['k5'])
    # little scene: Jerry carried off
    blit_im(img, big(fr('sprites','jerry_idle_1'),2), 70, 250)
    blit_im(img, big(fr('sprites','zombie_left_1'),2), 110, 252)
    blit_im(img, big(fr('sprites','zombie_right_2'),2), 24, 252)
    blit_im(img, big(fr('angels','angel_2'),2), 520, 230)
    return img

if __name__ == '__main__':
    g = gameplay(); scale2(g).save(f'{OUT}/gameplay_1280x720.png')
    t = title(); scale2(t).save(f'{OUT}/title_1280x720.png')
    p, pb = pause(); scale2(p).save(f'{OUT}/pause_1280x720.png'); scale2(pb).save(f'{OUT}/gameplay_charging_1280x720.png')
    o = gameover(); scale2(o).save(f'{OUT}/gameover_highscore_1280x720.png')
    moonwalk_strip()
    scale2(g,3).save(f'{OUT}/gameplay_1920x1080.png')
    print('ok')
