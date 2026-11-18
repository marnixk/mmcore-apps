import random, math, os
from mock_common import *
import ltiles as L, tiles as TT, build
OUT='/workspace/patientzero/mockups'
T=20
lsheet, lprops = L.build_level_sheets()
S['tiles_levels']=lsheet; S['props_levels']=lprops
IDX.update(build.INDEX)

def hospital():
    img = Image.new('RGBA',(640,360),P['k0']+(255,)); r = random.Random(1)
    for ty in range(18):
        for tx in range(32):
            blit(img,'tiles_levels','hosp_floor_%d'%((tx+ty)%2),tx*T,ty*T)
    # wards: top row 0-1 and bottom rows 15-17 are walls with doors; a central corridor
    for tx in range(32):
        blit(img,'tiles_levels','hosp_wall_top',tx*T,0)
        blit(img,'tiles_levels','ward_door_open' if tx%6==3 else ('ward_sign' if tx%6==2 else 'hosp_wall'),tx*T,T)
        blit(img,'tiles_levels','hosp_wall_top',tx*T,15*T)
        blit(img,'tiles_levels','ward_door_shut' if tx%8==5 else ('ward_door_open' if tx%8==1 else 'hosp_wall'),tx*T,16*T)
        blit(img,'tiles_levels','hosp_wall_top',tx*T,17*T)
    # interior divider walls with gaps (corridor splits)
    for tx in list(range(0,10))+list(range(14,22))+list(range(26,32)):
        blit(img,'tiles_levels','hosp_wall_top',tx*T,8*T); blit(img,'tiles_levels','hosp_wall',tx*T,9*T)
    for (x,y) in ((60,60),(300,60),(480,250),(140,250)): blit(img,'props_levels','bed',x,y)
    blit(img,'props_levels','iv',100,52); blit(img,'props_levels','iv',345,52)
    for (n,x,y) in (('puddle_l_0',220,120),('puddle_s_0',520,140),('puddle_l_2',380,215)):
        blit(img,'props' if 'puddle_l' not in n and 'puddle_w' not in n and 'puddle_s' in n and False else ('tiles' if 'puddle_s' in n else 'props'),n,x,y)
    for (n,x,y) in (('medic_down_1',60,36),('medic_mask_down',420,40),('office_up_2',100,300),('zombie_right_1',160,105),('office_left_0',540,90),('zombie_left_2',470,230)):
        blit(img,'sprites',n,x,y)
    blit(img,'sprites','jerry_right_1',300,215)
    return img

def rainy():
    img = street_level(7); ruined_props(img)
    # rain: big shallow puddles that grow; dim everything one step
    for (n,x,y) in (('puddle_w_0',90,240),('puddle_w_3',300,200),('puddle_l_1',480,100),('puddle_w_1',420,265),('puddle_l_0',200,110),('puddle_w_2',40,120),('puddle_l_3',560,180)):
        blit(img,'props',n,x,y)
    for (n,x,y) in (('office_right_2',60,160),('zombie_left_1',520,170),('office_down_0',250,80),('zombie_up_3',380,270),('office_left_3',590,240)):
        blit(img,'sprites',n,x,y)
    blit(img,'sprites','jerry_idle_0',320,190)
    img2 = img.copy(); r = random.Random(4)
    for _ in range(700):
        x, y = r.randrange(640), r.randrange(360)
        blit(img2,'tiles_levels','rain_%d'%r.randrange(2),x,y)
    for _ in range(60):
        blit(img2,'tiles_levels','splash',r.randrange(640),r.randrange(80,340))
    return img2

def subway():
    img = Image.new('RGBA',(640,360),P['k0']+(255,)); r = random.Random(2)
    # rows: 0 wall, 1-5 platform, 6-9 tracks (train), 10-11 platform, 12-13 tracks, 14-17 platform
    for tx in range(32):
        blit(img,'tiles','facade_brick',tx*T,0)
        for ty in range(18):
            if ty in (6,7,8,12,13): name = 'track_%d'%((tx+ty)%2)
            elif ty in (5,11): name = 'platform_edge'
            elif ty == 0: continue
            else: name = 'platform_%d'%((tx*3+ty)%2)
            blit(img,'tiles_levels',name,tx*T,ty*T)
    # flip edge for platforms below tracks
    for tx in range(32):
        e = fr('tiles_levels','platform_edge').transpose(Image.FLIP_TOP_BOTTOM)
        blit_im(img,e,tx*T,9*T); blit_im(img,e,tx*T,14*T)
    for x in (40,200,360,520): blit(img,'props_levels','pillar',x,30)
    for x in (120,440): blit(img,'props_levels','pillar',x,190)
    blit(img,'tiles_levels','stairs',300,320); blit(img,'tiles_levels','stairs',320,320)
    # train moving through rows 6-8
    for i,x in enumerate((-40,82,204)): blit(img,'props_levels','train',x,118)
    for (n,x,y) in (('puddle_l_1',250,40),('puddle_s_0',560,200),('puddle_l_2',70,300)):
        blit(img,'tiles' if 'puddle_s' in n else 'props',n,x,y)
    for (n,x,y) in (('dancer_moonwalk_l_1',80,60),('hoodie_left_1',500,50),('dancer_thriller_0',300,280),('medic_left_0',600,280),('office_down_1',430,180)):
        blit(img,'sprites',n,x,y)
    blit(img,'sprites','jerry_left_2',380,64)
    # sneeze drifting over the empty track on the right
    return img

if __name__ == '__main__':
    scale2(hospital()).save(f'{OUT}/level_hospital_1280x720.png')
    scale2(rainy()).save(f'{OUT}/level_rainy_street_1280x720.png')
    scale2(subway()).save(f'{OUT}/level_subway_1280x720.png')
    print('ok')
