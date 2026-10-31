' enemies.inc - fixed pool of TYPE records. One SELECT CASE per enemy per tick.

type TZed
  x as integer
  y as integer
  vx as integer
  vy as integer
  hx as integer
  hy as integer
  kind as integer
  st as integer
  tim as integer
  frm as integer
  face as integer
  a as integer
  b as integer
  alive as integer
end type

dim zed(EN_LAST) as TZed
dim en.ox(EN_LAST), en.oy(EN_LAST), en.ow(EN_LAST), en.oh(EN_LAST)
dim ek.base(4), ek.score(4)
dim en_i, en_j, en_k, en_id
dim en_dx, en_dy, en_ax, en_ay, en_mag, en_spd
dim en_d2, en_zx, en_zy, en_px, en_py, en_tx, en_ty
dim en_nx, en_ny
dim en_base, en_sid, en_st, en_fr, en_dash, en_moon, en_n

sub en.init()
  ek.base(KZ_ZOMBIE) = S_ZOMBIE
  ek.base(KZ_OFFICE) = S_OFFICE
  ek.base(KZ_HOODIE) = S_HOODIE
  ek.base(KZ_DANCER) = S_DANCER
  ek.base(KZ_MEDIC) = S_MEDIC
  ek.score(KZ_ZOMBIE) = 1337
  ek.score(KZ_OFFICE) = 1337
  ek.score(KZ_HOODIE) = 2000
  ek.score(KZ_DANCER) = 2500
  ek.score(KZ_MEDIC) = 2000
end sub

sub en.clear()
  for en_i = 0 to EN_LAST
    zed(en_i).alive = 0
    en.ow(en_i) = 0
  next en_i
  en_n = 0
end sub

sub en.head(slot)
  if zed(slot).kind = KZ_OFFICE then
    if rndn(2) = 0 then
      zed(slot).hx = 0
      zed(slot).hy = 1
      if rndn(2) = 0 then zed(slot).hy = -1
    else
      zed(slot).hy = 0
      zed(slot).hx = 1
      if rndn(2) = 0 then zed(slot).hx = -1
    end if
  else
    zed(slot).hx = rndn(3) - 1
    zed(slot).hy = rndn(3) - 1
    if zed(slot).hx = 0 and zed(slot).hy = 0 then zed(slot).hx = 1
  end if
end sub

sub en.fill(slot, kd, sx, sy)
  zed(slot).x = sx * 256
  zed(slot).y = sy * 256
  zed(slot).kind = kd
  zed(slot).st = ST_WALK
  zed(slot).tim = 0
  zed(slot).frm = rndn(4)
  zed(slot).face = rndn(4)
  zed(slot).a = 0
  zed(slot).b = 0
  zed(slot).vx = 0
  zed(slot).vy = 0
  zed(slot).alive = 1
  en.ow(slot) = 0
  en.head slot
end sub

function en.free()
  en.free = -1
  for en_k = 0 to EN_LAST
    if zed(en_k).alive = 0 then
      en.free = en_k
      exit function
    end if
  next en_k
end function

function en.pickOpen()
  en.pickOpen = -1
  en_ax = 0
  for en_j = 0 to door.n - 1
    if door.open(en_j) = 1 then en_ax = en_ax + 1
  next en_j
  if en_ax < 1 then
    exit function
  end if
  en_ay = rndn(en_ax)
  en_ax = 0
  for en_j = 0 to door.n - 1
    if door.open(en_j) = 1 then
      if en_ax = en_ay then
        en.pickOpen = en_j
        exit function
      end if
      en_ax = en_ax + 1
    end if
  next en_j
end function

function en.pickCool()
  en.pickCool = -1
  en_ax = 0
  for en_j = 0 to door.n - 1
    if door.open(en_j) = 1 and door.cool(en_j) = 0 then en_ax = en_ax + 1
  next en_j
  if en_ax < 1 then
    exit function
  end if
  en_ay = rndn(en_ax)
  en_ax = 0
  for en_j = 0 to door.n - 1
    if door.open(en_j) = 1 and door.cool(en_j) = 0 then
      if en_ax = en_ay then
        en.pickCool = en_j
        exit function
      end if
      en_ax = en_ax + 1
    end if
  next en_j
end function

sub en.atDoor(slot, di)
  en_k = door.sy(di) * TILE
  if door.sy(di) > 8 then en_k = en_k - 20
  en.fill slot, door.kind(di), door.tx(di) * TILE, en_k
end sub

sub en.trySpawn()
  en_id = en.free()
  if en_id < 0 then
    exit sub
  end if
  en_st = lv.kind()
  if lv.themeNow = 3 then
    en.fill en_id, en_st, (15 + rndn(2)) * TILE, 15 * TILE
    exit sub
  end if
  for en_j = 0 to 11
    en_ax = rndn(MAP_W) * TILE
    en_ay = rndn(MAP_H) * TILE
    if lv.blocked(en_ax, en_ay) = 0 then
      en_dx = en_ax + 10 - (pl.x \ 256)
      en_dy = en_ay + 15 - (pl.y \ 256)
      if en_dx * en_dx + en_dy * en_dy >= R112 then
        en.fill en_id, en_st, en_ax, en_ay
        exit sub
      end if
    end if
  next en_j
end sub

sub en.spawnDoorNow()
  en_id = en.free()
  if en_id < 0 then
    exit sub
  end if
  en_j = en.pickOpen()
  if en_j < 0 then
    exit sub
  end if
  door.kind(en_j) = lv.kind()
  en.atDoor en_id, en_j
end sub

sub en.arm()
  en_j = en.pickCool()
  if en_j < 0 then
    exit sub
  end if
  door.cool(en_j) = 28
  door.kind(en_j) = lv.kind()
end sub

sub en.request()
  if lv.themeNow = 2 then
    en.arm
    exit sub
  end if
  en.trySpawn
end sub

sub en.initial()
  for en_i = 1 to lv.startN
    if lv.themeNow = 2 then
      en.spawnDoorNow
    else
      en.trySpawn
    end if
  next en_i
end sub

sub door.tick()
  if lv.themeNow <> 2 then
    exit sub
  end if
  for en_j = 0 to door.n - 1
    if door.cool(en_j) > 0 then
      door.cool(en_j) = door.cool(en_j) - 1
      if door.cool(en_j) = 0 then
        en_id = en.free()
        if en_id >= 0 then en.atDoor en_id, en_j
      end if
    end if
  next en_j
end sub

sub en.dist()
  en_px = (pl.x \ 256) + 10
  en_py = (pl.y \ 256) + 15
  en_zx = (zed(en_id).x \ 256) + 10
  en_zy = (zed(en_id).y \ 256) + 15
  en_dx = en_zx - en_px
  en_dy = en_zy - en_py
  en_d2 = en_dx * en_dx + en_dy * en_dy
end sub

sub en.aim()
  en_ax = en_tx - (zed(en_id).x \ 256) - 10
  en_ay = en_ty - (zed(en_id).y \ 256) - 15
  if en_ax < 0 then
    en_dx = 0 - en_ax
  else
    en_dx = en_ax
  end if
  if en_ay < 0 then
    en_dy = 0 - en_ay
  else
    en_dy = en_ay
  end if
  if en_dx > en_dy then
    en_mag = en_dx + (en_dy \ 2)
  else
    en_mag = en_dy + (en_dx \ 2)
  end if
  if en_mag < 1 then
    zed(en_id).vx = 0
    zed(en_id).vy = 0
    exit sub
  end if
  zed(en_id).vx = (en_spd * en_ax) \ en_mag
  zed(en_id).vy = (en_spd * en_ay) \ en_mag
end sub

sub en.wander(spd)
  zed(en_id).st = ST_WALK
  if zed(en_id).hx = 0 and zed(en_id).hy = 0 then en.head en_id
  zed(en_id).vx = zed(en_id).hx * spd
  zed(en_id).vy = zed(en_id).hy * spd
end sub

sub en.turn()
  if rndn(2) = 0 then
    en_k = zed(en_id).hx
    zed(en_id).hx = zed(en_id).hy
    zed(en_id).hy = 0 - en_k
  else
    en_k = zed(en_id).hx
    zed(en_id).hx = 0 - zed(en_id).hy
    zed(en_id).hy = en_k
  end if
  if zed(en_id).hx = 0 and zed(en_id).hy = 0 then zed(en_id).hx = 1
  zed(en_id).vx = zed(en_id).hx * SP_O07
  zed(en_id).vy = zed(en_id).hy * SP_O07
end sub

sub en.zombie()
  if pl.charging = 1 and pl.flem > 30 and en_d2 < R169 then
    zed(en_id).st = ST_FLEE
    en_tx = en_zx + en_dx
    en_ty = en_zy + en_dy
    en_spd = SP_F15
    en.aim
    exit sub
  end if
  if en_d2 < R169 and pl.shield = 0 then
    zed(en_id).st = ST_CHASE
    en_tx = en_px
    en_ty = en_py
    en_spd = SP_C08
    en.aim
    exit sub
  end if
  en.wander SP_W05
end sub

sub en.office()
  if pl.charging = 1 and pl.flem > 15 and en_d2 < R197 then
    zed(en_id).st = ST_FLEE
    if en_dx < 0 then
      en_ax = 0 - en_dx
    else
      en_ax = en_dx
    end if
    if en_dy < 0 then
      en_ay = 0 - en_dy
    else
      en_ay = en_dy
    end if
    if en_ax >= en_ay then
      zed(en_id).vy = 0
      zed(en_id).hy = 0
      if en_dx > 0 then
        zed(en_id).vx = SP_O18
        zed(en_id).hx = 1
      else
        zed(en_id).vx = 0 - SP_O18
        zed(en_id).hx = -1
      end if
    else
      zed(en_id).vx = 0
      zed(en_id).hx = 0
      if en_dy > 0 then
        zed(en_id).vy = SP_O18
        zed(en_id).hy = 1
      else
        zed(en_id).vy = 0 - SP_O18
        zed(en_id).hy = -1
      end if
    end if
    exit sub
  end if
  en_ax = ((pl.x \ 256) + 10) \ TILE
  en_ay = ((pl.y \ 256) + 29) \ TILE
  en_k = ((zed(en_id).x \ 256) + 10) \ TILE
  en_base = ((zed(en_id).y \ 256) + 29) \ TILE
  if en_d2 < R225 then
    if en_k = en_ax or en_base = en_ay then
      zed(en_id).st = ST_CHASE
      if en_base = en_ay then
        zed(en_id).vy = 0
        zed(en_id).hy = 0
        if en_dx > 0 then
          zed(en_id).vx = 0 - SP_O10
          zed(en_id).hx = -1
        else
          zed(en_id).vx = SP_O10
          zed(en_id).hx = 1
        end if
      else
        zed(en_id).vx = 0
        zed(en_id).hx = 0
        if en_dy > 0 then
          zed(en_id).vy = 0 - SP_O10
          zed(en_id).hy = -1
        else
          zed(en_id).vy = SP_O10
          zed(en_id).hy = 1
        end if
      end if
      exit sub
    end if
  end if
  en.wander SP_O07
end sub

sub en.hoodie()
  if pl.charging = 1 and pl.flem > 30 then
    if zed(en_id).st = ST_DASH then
      zed(en_id).st = ST_REST
      zed(en_id).tim = 30
    end if
    if zed(en_id).st = ST_REST then
      zed(en_id).tim = zed(en_id).tim - 1
      if zed(en_id).tim <= 0 then zed(en_id).st = ST_WALK
    end if
    zed(en_id).vx = 0
    zed(en_id).vy = 0
    exit sub
  end if
  if zed(en_id).st = ST_DASH then
    zed(en_id).tim = zed(en_id).tim - 1
    en_tx = en_px
    en_ty = en_py
    en_spd = SP_H25
    en.aim
    if zed(en_id).tim <= 0 then
      zed(en_id).st = ST_REST
      zed(en_id).tim = 60
      zed(en_id).vx = 0
      zed(en_id).vy = 0
    end if
    exit sub
  end if
  if zed(en_id).st = ST_REST then
    zed(en_id).vx = 0
    zed(en_id).vy = 0
    zed(en_id).tim = zed(en_id).tim - 1
    if zed(en_id).tim <= 0 then zed(en_id).st = ST_WALK
    exit sub
  end if
  if en_d2 < R197 then
    if en_dash < 2 and en_d2 < R141 then
      if (pl.lx * en_dx + pl.ly * en_dy) < 0 then
        zed(en_id).st = ST_DASH
        zed(en_id).tim = 30
        en_dash = en_dash + 1
        en_tx = en_px
        en_ty = en_py
        en_spd = SP_H25
        en.aim
        exit sub
      end if
    end if
    if (en_dx * (0 - pl.ly) - en_dy * (0 - pl.lx)) >= 0 then
      en_tx = en_zx - (en_dy \ 3)
      en_ty = en_zy + (en_dx \ 3)
    else
      en_tx = en_zx + (en_dy \ 3)
      en_ty = en_zy - (en_dx \ 3)
    end if
    if en_d2 > R141 then
      en_tx = (en_tx * 2 + en_px) \ 3
      en_ty = (en_ty * 2 + en_py) \ 3
    end if
    if en_d2 < R84 then
      en_tx = en_zx + en_dx
      en_ty = en_zy + en_dy
    end if
    en_spd = SP_H09
    en.aim
    exit sub
  end if
  en.wander SP_H04
end sub

sub en.moon()
  en_tx = en_px
  en_ty = en_py
  en_spd = SP_D12
  en.aim
  en_nx = (zed(en_id).x \ 256) + ((zed(en_id).vx * 8) \ 256)
  en_ny = (zed(en_id).y \ 256) + ((zed(en_id).vy * 8) \ 256)
  zed(en_id).tim = zed(en_id).tim - 1
  if lv.blocked(en_nx, en_ny) = 1 or zed(en_id).tim <= 0 then
    zed(en_id).st = ST_SPIN
    zed(en_id).tim = 16
    zed(en_id).vx = 0
    zed(en_id).vy = 0
  end if
end sub

sub en.dancer()
  if zed(en_id).st = ST_MOON then
    en.moon
    exit sub
  end if
  if zed(en_id).st = ST_SPIN then
    zed(en_id).vx = 0
    zed(en_id).vy = 0
    zed(en_id).face = (16 - zed(en_id).tim) \ 4
    if zed(en_id).face > 3 then zed(en_id).face = 3
    if zed(en_id).face < 0 then zed(en_id).face = 0
    zed(en_id).tim = zed(en_id).tim - 1
    if zed(en_id).tim <= 0 then
      zed(en_id).st = ST_TOE
      zed(en_id).tim = 30
    end if
    exit sub
  end if
  if zed(en_id).st = ST_TOE then
    zed(en_id).vx = 0
    zed(en_id).vy = 0
    zed(en_id).tim = zed(en_id).tim - 1
    if zed(en_id).tim <= 0 then
      if en_d2 < R225 then
        zed(en_id).st = ST_THRILL
        zed(en_id).tim = 90 + rndn(61)
        zed(en_id).a = 0
        zed(en_id).b = 1
      else
        zed(en_id).st = ST_WALK
      end if
    end if
    exit sub
  end if
  if zed(en_id).st = ST_THRILL then
    zed(en_id).tim = zed(en_id).tim - 1
    zed(en_id).a = zed(en_id).a + 1
    if zed(en_id).a >= 12 then
      zed(en_id).a = 0
      zed(en_id).b = 0 - zed(en_id).b
      if zed(en_id).b = 0 then zed(en_id).b = 1
    end if
    zed(en_id).vx = zed(en_id).b * 256
    zed(en_id).vy = 0
    if zed(en_id).tim <= 0 then
      if en_moon < 1 then
        zed(en_id).st = ST_MOON
        zed(en_id).tim = 120
        en_moon = en_moon + 1
      else
        zed(en_id).st = ST_SPIN
        zed(en_id).tim = 16
        zed(en_id).vx = 0
        zed(en_id).vy = 0
      end if
    end if
    exit sub
  end if
  if en_d2 < R225 then
    zed(en_id).st = ST_THRILL
    zed(en_id).tim = 90 + rndn(61)
    zed(en_id).a = 0
    zed(en_id).b = 1
    zed(en_id).vx = 0
    zed(en_id).vy = 0
  else
    en.wander SP_W05
  end if
end sub

sub en.medic()
  if zed(en_id).b > 0 and zed(en_id).st <> ST_MASK then
    zed(en_id).b = zed(en_id).b - 1
  end if
  if zed(en_id).st = ST_MASK then
    zed(en_id).vx = 0
    zed(en_id).vy = 0
    zed(en_id).tim = zed(en_id).tim - 1
    if zed(en_id).tim <= 0 then
      zed(en_id).st = ST_PATROL
      zed(en_id).b = 180
    end if
    exit sub
  end if
  if pl.charging = 1 and pl.flem > 30 and en_d2 < R169 and zed(en_id).b = 0 then
    zed(en_id).st = ST_MASK
    zed(en_id).tim = 90
    zed(en_id).vx = 0
    zed(en_id).vy = 0
    exit sub
  end if
  if pl.refilled = 1 then
    zed(en_id).st = ST_GOTO
    if pd.n > 0 then zed(en_id).a = pd.nearest()
  end if
  if en_d2 < R113 and pl.shield = 0 then
    zed(en_id).st = ST_CHASE
    en_tx = en_px
    en_ty = en_py
    en_spd = SP_M09
    en.aim
    exit sub
  end if
  if pd.n < 1 then
    en.wander SP_M06
    exit sub
  end if
  if zed(en_id).a < 0 or zed(en_id).a >= pd.n then zed(en_id).a = pd.nearest()
  en_k = zed(en_id).a
  if pool(en_k).alive = 0 then
    en_k = pd.nearest()
    zed(en_id).a = en_k
  end if
  en_tx = pool(en_k).x + (pool(en_k).w \ 2)
  en_ty = pool(en_k).y + (pool(en_k).h \ 2)
  en_ax = en_tx - en_zx
  en_ay = en_ty - en_zy
  en_mag = en_ax * en_ax + en_ay * en_ay
  if en_mag > 1600 then
    zed(en_id).st = ST_GOTO
    en_spd = SP_M06
    en.aim
    exit sub
  end if
  zed(en_id).st = ST_PATROL
  if zed(en_id).tim <= 0 then
    zed(en_id).tim = 24 + rndn(30)
    zed(en_id).hx = rndn(3) - 1
    zed(en_id).hy = rndn(3) - 1
  end if
  zed(en_id).tim = zed(en_id).tim - 1
  zed(en_id).vx = zed(en_id).hx * SP_M06
  zed(en_id).vy = zed(en_id).hy * SP_M06
end sub

sub en.integrate()
  en_nx = zed(en_id).x + zed(en_id).vx
  if lv.blocked(en_nx \ 256, zed(en_id).y \ 256) = 1 then
    if zed(en_id).kind = KZ_OFFICE and zed(en_id).st = ST_WALK then
      en.turn
    else
      zed(en_id).vx = 0 - zed(en_id).vx
      zed(en_id).hx = 0 - zed(en_id).hx
    end if
  else
    zed(en_id).x = en_nx
  end if
  en_ny = zed(en_id).y + zed(en_id).vy
  if lv.blocked(zed(en_id).x \ 256, en_ny \ 256) = 1 then
    if zed(en_id).kind = KZ_OFFICE and zed(en_id).st = ST_WALK then
      en.turn
    else
      zed(en_id).vy = 0 - zed(en_id).vy
      zed(en_id).hy = 0 - zed(en_id).hy
    end if
  else
    zed(en_id).y = en_ny
  end if
end sub

sub en.face()
  if zed(en_id).vx = 0 and zed(en_id).vy = 0 then
    exit sub
  end if
  if zed(en_id).vx < 0 then
    en_ax = 0 - zed(en_id).vx
  else
    en_ax = zed(en_id).vx
  end if
  if zed(en_id).vy < 0 then
    en_ay = 0 - zed(en_id).vy
  else
    en_ay = zed(en_id).vy
  end if
  if en_ax > en_ay then
    if zed(en_id).vx > 0 then
      zed(en_id).face = 2
    else
      zed(en_id).face = 3
    end if
  else
    if zed(en_id).vy > 0 then
      zed(en_id).face = 0
    else
      zed(en_id).face = 1
    end if
  end if
end sub

sub en.anim()
  if zed(en_id).vx = 0 and zed(en_id).vy = 0 then
    exit sub
  end if
  if gm.frame = (gm.frame \ 8) * 8 then
    zed(en_id).frm = (zed(en_id).frm + 1) mod 4
  end if
end sub

sub en.cureTick()
  zed(en_id).tim = zed(en_id).tim + 1
  zed(en_id).vx = 0
  zed(en_id).vy = 0
  if zed(en_id).tim > 18 then
    zed(en_id).st = ST_ANGEL
    zed(en_id).tim = 0
    zed(en_id).frm = 0
  end if
end sub

sub en.angelTick()
  zed(en_id).y = zed(en_id).y - 180
  zed(en_id).tim = zed(en_id).tim + 1
  if gm.frame = (gm.frame \ 4) * 4 then
    zed(en_id).frm = (zed(en_id).frm + 1) mod 4
  end if
  if zed(en_id).tim > 70 or zed(en_id).y < 0 then zed(en_id).alive = 0
end sub

sub en.hitSneeze()
  if pl.sneeze <= 0 then
    exit sub
  end if
  if zed(en_id).st = ST_MASK or zed(en_id).st = ST_CURE or zed(en_id).st = ST_ANGEL then
    exit sub
  end if
  en_zx = (zed(en_id).x \ 256) + 10
  en_zy = (zed(en_id).y \ 256) + 15
  en_px = pl.x \ 256
  en_py = pl.y \ 256
  if en_zx < en_px - pl.reach or en_zx > en_px + 20 + pl.reach then
    exit sub
  end if
  if en_zy < en_py - 28 or en_zy > en_py + 45 then
    exit sub
  end if
  zed(en_id).st = ST_CURE
  zed(en_id).tim = 0
  zed(en_id).vx = 0
  zed(en_id).vy = 0
  gm.score = gm.score + ek.score(zed(en_id).kind)
  gm.lcured = gm.lcured + 1
  gm.cured = gm.cured + 1
  pt.spark en_zx, en_zy
  if lv.themeNow = 2 and gm.lcured >= gm.opened * 4 + 4 then
    lv.openNext
    gm.opened = gm.opened + 1
  end if
  if gm.lcured >= lv.cure then gm.won = 1
end sub

sub en.touch()
  if pl.shield > 0 then
    exit sub
  end if
  if zed(en_id).st = ST_CURE or zed(en_id).st = ST_ANGEL then
    exit sub
  end if
  en_px = (pl.x \ 256) + 10
  en_py = (pl.y \ 256) + 29
  en_zx = zed(en_id).x \ 256
  en_zy = zed(en_id).y \ 256
  if en_px < en_zx + 2 or en_px > en_zx + 18 then
    exit sub
  end if
  if en_py < en_zy + 18 or en_py > en_zy + 30 then
    exit sub
  end if
  if zed(en_id).kind = KZ_MEDIC then
    gm.hurt 1
  else
    gm.hurt 0
  end if
end sub

sub en.one()
  if zed(en_id).st = ST_CURE then
    en.cureTick
    exit sub
  end if
  if zed(en_id).st = ST_ANGEL then
    en.angelTick
    exit sub
  end if
  en.dist
  select case zed(en_id).kind
    case KZ_ZOMBIE
      en.zombie
    case KZ_OFFICE
      en.office
    case KZ_HOODIE
      en.hoodie
    case KZ_DANCER
      en.dancer
    case else
      en.medic
  end select
  en.integrate
  if tr.on = 1 then
    if trHits((zed(en_id).x \ 256) + 10, (zed(en_id).y \ 256) + 29) = 1 then
      zed(en_id).alive = 0
      exit sub
    end if
  end if
  en.face
  en.anim
  en.hitSneeze
  if zed(en_id).alive = 1 and zed(en_id).st <> ST_CURE then en.touch
end sub

sub en.tick()
  en_dash = 0
  en_moon = 0
  en_n = 0
  for en_i = 0 to EN_LAST
    if zed(en_i).alive = 1 then
      if zed(en_i).st = ST_DASH then en_dash = en_dash + 1
      if zed(en_i).st = ST_MOON then en_moon = en_moon + 1
      if zed(en_i).st <> ST_CURE and zed(en_i).st <> ST_ANGEL then en_n = en_n + 1
    end if
  next en_i
  for en_i = 0 to EN_LAST
    if zed(en_i).alive = 1 then
      en_id = en_i
      en.one
    end if
  next en_i
  pl.refilled = 0
end sub

sub en.picture()
  en_base = ek.base(zed(en_id).kind)
  en_fr = zed(en_id).frm mod 4
  en_st = zed(en_id).st
  if en_st = ST_ANGEL then
    en_sid = F_ANG + zed(en_id).kind * 4 + en_fr
    exit sub
  end if
  if en_st = ST_CURE then
    if zed(en_id).tim < 6 then
      en_sid = en_base + 16 + (zed(en_id).tim \ 3)
    else
      en_sid = en_base + 18 + ((zed(en_id).tim \ 6) mod 3)
    end if
    exit sub
  end if
  if en_st = ST_MASK then
    en_sid = S_MMASK_D
    if zed(en_id).face = 2 then en_sid = S_MMASK_R
    if zed(en_id).face = 3 then en_sid = S_MMASK_L
    exit sub
  end if
  if zed(en_id).kind = KZ_DANCER then
    if en_st = ST_THRILL then
      en_sid = S_DTHRILL + (en_fr mod 2)
      exit sub
    end if
    if en_st = ST_MOON then
      if zed(en_id).vx > 0 then
        en_sid = S_DMOON_L + en_fr
      else
        en_sid = S_DMOON_R + en_fr
      end if
      exit sub
    end if
    if en_st = ST_TOE then
      en_sid = S_DTOE_R
      if zed(en_id).face = 3 then en_sid = S_DTOE_L
      exit sub
    end if
    if en_st = ST_SPIN then
      en_sid = en_base + zed(en_id).face * 4
      exit sub
    end if
  end if
  en_sid = en_base + zed(en_id).face * 4 + en_fr
end sub
