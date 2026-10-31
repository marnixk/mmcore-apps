' levels.inc - 32x18 maps, puddles, doors, trains.
' Themes with art: street, rain, hospital, subway. The other four stay data-only.

const LV_N = 4
const PR_LAST = 31
const DOOR_LAST = 15

type TPool
  x as integer
  y as integer
  w as integer
  h as integer
  sz as integer
  frm as integer
  alive as integer
  ox as integer
  oy as integer
  ow as integer
  oh as integer
end type

dim pool(PD_LAST) as TPool
dim pd.n
dim pd_i, pd_x, pd_y

dim map.s(31, 17), map.t(31, 17), map.f(31, 17)
dim pr.id(PR_LAST), pr.x(PR_LAST), pr.y(PR_LAST), pr.n
dim door.tx(DOOR_LAST), door.ty(DOOR_LAST), door.sy(DOOR_LAST)
dim door.open(DOOR_LAST), door.cool(DOOR_LAST), door.n
dim door.ox(DOOR_LAST), door.oy(DOOR_LAST), door.ow(DOOR_LAST), door.oh(DOOR_LAST)
dim door.kind(DOOR_LAST)

dim tr.x(1), tr.y(1), tr.vx(1), tr.st(1), tr.t(1)
dim tr.ox(1), tr.oy(1), tr.ow(1), tr.oh(1)
dim tr.wx(1), tr.wy(1), tr.ww(1), tr.wh(1)
dim tr.on, tr_i, tr_tx, tr_px, tr_py
dim pd_best, pd_d2, pd_dx, pd_dy

dim string lv.name$(3), lv.fresh$(3)
dim lv.cure0(3), lv.start0(3), lv.spawn0(3), lv.max0(3)
dim lv.theme(3), lv.newk(3)
dim lv.mix(3, 4)
dim lv.m(4)
dim lv.cure, lv.startN, lv.spawnEvery, lv.maxA, lv.themeNow, lv.seed
dim lv.rain, lv.rainT, lv.raining, lv.px, lv.py
dim lv_i, lv_x, lv_y, lv_id, lv_h, lv_n
dim string lv.b1$, lv.b2$

sub lv.init()
  lv.name$(0) = "STREET"
  lv.fresh$(0) = "ZOMBIE"
  lv.cure0(0) = 20
  lv.start0(0) = 5
  lv.spawn0(0) = 140
  lv.max0(0) = 12
  lv.theme(0) = 0
  lv.newk(0) = KZ_ZOMBIE
  lv.mix(0, 0) = 100
  lv.mix(0, 1) = 0
  lv.mix(0, 2) = 0
  lv.mix(0, 3) = 0
  lv.mix(0, 4) = 0

  lv.name$(1) = "RAINY STREET"
  lv.fresh$(1) = "OFFICE WORKER"
  lv.cure0(1) = 25
  lv.start0(1) = 6
  lv.spawn0(1) = 110
  lv.max0(1) = 14
  lv.theme(1) = 1
  lv.newk(1) = KZ_OFFICE
  lv.mix(1, 0) = 70
  lv.mix(1, 1) = 30
  lv.mix(1, 2) = 0
  lv.mix(1, 3) = 0
  lv.mix(1, 4) = 0

  lv.name$(2) = "HOSPITAL"
  lv.fresh$(2) = "PARAMEDIC"
  lv.cure0(2) = 25
  lv.start0(2) = 4
  lv.spawn0(2) = 100
  lv.max0(2) = 14
  lv.theme(2) = 2
  lv.newk(2) = KZ_MEDIC
  lv.mix(2, 0) = 50
  lv.mix(2, 1) = 25
  lv.mix(2, 2) = 0
  lv.mix(2, 3) = 0
  lv.mix(2, 4) = 25

  lv.name$(3) = "SUBWAY"
  lv.fresh$(3) = "THRILLER DANCER"
  lv.cure0(3) = 30
  lv.start0(3) = 5
  lv.spawn0(3) = 90
  lv.max0(3) = 16
  lv.theme(3) = 3
  lv.newk(3) = KZ_DANCER
  lv.mix(3, 0) = 30
  lv.mix(3, 1) = 20
  lv.mix(3, 2) = 20
  lv.mix(3, 3) = 15
  lv.mix(3, 4) = 15
end sub

sub lv.apply(idx)
  local n
  n = idx
  if n < 0 then n = 0
  if n > LV_N - 1 then n = 0
  lv.themeNow = lv.theme(n)
  lv.cure = lv.cure0(n) + gm.round * 8
  lv.startN = lv.start0(n)
  lv.maxA = lv.max0(n) + gm.round
  if lv.maxA > EN_LAST then lv.maxA = EN_LAST
  lv.spawnEvery = lv.spawn0(n) * 4 \ (4 + gm.round)
  if lv.spawnEvery < 40 then lv.spawnEvery = 40
  for lv_i = 0 to 4
    lv.m(lv_i) = lv.mix(n, lv_i)
  next lv_i
  lv.b1$ = lv.name$(n)
  if gm.round > 0 then lv.b1$ = lv.b1$ + "  " + num$(gm.round + 1)
  lv.b2$ = "NEW  " + lv.fresh$(n)
  lv.seed = 2 + n * 3 + gm.round
  lv.rain = 0
  if lv.themeNow = 1 then lv.rain = 1
  lv.rainT = 0
  lv.raining = 0
end sub

function lv.kind()
  lv_h = rndn(100)
  lv_x = 0
  lv.kind = 0
  for lv_i = 0 to 4
    lv_x = lv_x + lv.m(lv_i)
    if lv_h < lv_x then
      lv.kind = lv_i
      exit function
    end if
  next lv_i
end function

function lv.hash(x, y)
  lv.hash = (x * 17 + y * 31 + lv.seed * 13) mod 97
end function

sub lv.cell(tx, ty, id, solid)
  map.t(tx, ty) = id
  map.s(tx, ty) = solid
  map.f(tx, ty) = 0
end sub

sub lv.clearMap()
  for lv_y = 0 to MAP_H - 1
    for lv_x = 0 to MAP_W - 1
      map.s(lv_x, lv_y) = 0
      map.t(lv_x, lv_y) = F_ASPHALT_0
      map.f(lv_x, lv_y) = 0
    next lv_x
  next lv_y
  pr.n = 0
  pd.n = 0
  door.n = 0
  tr.on = 0
  tr.ow(0) = 0
  tr.ow(1) = 0
  tr.ww(0) = 0
  tr.ww(1) = 0
  for pd_i = 0 to PD_LAST
    pool(pd_i).alive = 0
    pool(pd_i).ow = 0
  next pd_i
end sub

sub pd.apply(i)
  if pool(i).sz < 1 then
    pool(i).sz = 0
    pool(i).w = 20
    pool(i).h = 20
  end if
  if pool(i).sz = 1 then
    pool(i).w = 40
    pool(i).h = 20
  end if
  if pool(i).sz > 1 then
    pool(i).sz = 2
    pool(i).w = 60
    pool(i).h = 20
  end if
end sub

sub lv.puddle(x, y, sz)
  if pd.n > PD_LAST then
    exit sub
  end if
  pool(pd.n).x = x
  pool(pd.n).y = y
  pool(pd.n).sz = sz
  pool(pd.n).frm = 0
  pool(pd.n).alive = 1
  pool(pd.n).ox = 0
  pool(pd.n).oy = 0
  pool(pd.n).ow = 0
  pool(pd.n).oh = 0
  pd.apply pd.n
  pd.n = pd.n + 1
end sub

sub lv.prop(id, x, y)
  local x1, y1, x2, y2, tx, ty
  if pr.n > PR_LAST then
    exit sub
  end if
  pr.id(pr.n) = id
  pr.x(pr.n) = x
  pr.y(pr.n) = y
  pr.n = pr.n + 1
  x1 = x \ TILE
  y1 = y \ TILE
  x2 = (x + fr.w(id) - 1) \ TILE
  y2 = (y + fr.h(id) - 1) \ TILE
  for ty = y1 to y2
    for tx = x1 to x2
      if tx >= 0 and ty >= 0 and tx < MAP_W and ty < MAP_H then map.s(tx, ty) = 1
    next tx
  next ty
end sub

sub lv.door(tx, ty, sy, open)
  if door.n > DOOR_LAST then
    exit sub
  end if
  door.tx(door.n) = tx
  door.ty(door.n) = ty
  door.sy(door.n) = sy
  door.open(door.n) = open
  door.cool(door.n) = 0
  door.ow(door.n) = 0
  door.oh(door.n) = 0
  door.kind(door.n) = 0
  door.n = door.n + 1
end sub

function lv.solidAt(px, py)
  lv.solidAt = 1
  if px < 0 or py < 0 then
    exit function
  end if
  if px >= BASE_W or py >= BASE_H then
    exit function
  end if
  lv.solidAt = map.s(px \ TILE, py \ TILE)
end function

function lv.blocked(px, py)
  lv.blocked = 0
  if lv.solidAt(px + 6, py + 29) = 1 then lv.blocked = 1
  if lv.solidAt(px + 14, py + 29) = 1 then lv.blocked = 1
end function

sub lv.street()
  local h, id
  lv.clearMap()
  for lv_x = 0 to 31
    lv.cell lv_x, 0, F_ROOF_0 + ((lv_x * 7 + 3) mod 3), 1
    if lv_x mod 9 = 0 then
      lv.cell lv_x, 1, F_DOOR, 1
    else
      h = (lv_x * 3) mod 5
      id = F_WIN_0
      if h = 1 then id = F_WIN_1
      if h = 2 then id = F_BRICK
      if h = 4 then id = F_DOOR
      lv.cell lv_x, 1, id, 1
    end if
    lv.cell lv_x, 2, F_FBASE, 1
    lv.cell lv_x, 3, F_WALK_0 + (lv_x mod 2), 0
    lv.cell lv_x, 4, F_CURB_TOP, 0
    for lv_y = 5 to 14
      h = lv.hash(lv_x, lv_y)
      id = F_ASPHALT_0 + (h mod 4)
      if h < 5 then id = F_ASPHALT_CRACK
      if h < 3 then id = F_ASPHALT_CRACK_1
      if h > 90 then id = F_DECAL_0 + (h mod 2)
      lv.cell lv_x, lv_y, id, 0
    next lv_y
    if lv_x mod 3 <> 2 then lv.cell lv_x, 10, F_ROAD, 0
    lv.cell lv_x, 15, F_CURB_BOT, 0
    lv.cell lv_x, 16, F_WALK_0 + ((lv_x + 1) mod 2), 0
    lv.cell lv_x, 17, F_WALL, 1
  next lv_x
  lv.cell 9, 12, F_MANHOLE, 0
  lv.cell 17, 6, F_WALL_TOP, 1
  lv.cell 18, 6, F_WALL_TOP, 1
  lv.cell 17, 7, F_WALL, 1
  lv.cell 18, 7, F_WALL, 1
  lv.cell 23, 12, F_BARRIER, 1
  lv.cell 24, 12, F_BARRIER, 1
  lv.cell 27, 6, F_CRATE, 1
  lv.cell 28, 6, F_CRATE, 1
  lv.cell 14, 13, F_RUBBLE_0, 1
  lv.cell 2, 13, F_RUBBLE_1, 1
  lv.cell 12, 3, F_BIN, 1
  lv.cell 25, 16, F_BIN, 1
  lv.prop F_CAR, 100, 120
  lv.prop F_LAMP, 60, 40
  lv.prop F_LAMP, 380, 40
  lv.prop F_BARREL_0, 600, 250
  if lv.themeNow = 1 then
    lv.puddle 90, 240, 2
    lv.puddle 300, 200, 2
    lv.puddle 480, 100, 1
    lv.puddle 420, 265, 2
    lv.puddle 200, 110, 1
    lv.puddle 40, 120, 2
    lv.puddle 560, 180, 1
    lv.raining = 1
  else
    lv.puddle 100, 250, 1
    lv.puddle 470, 170, 2
    lv.puddle 560, 270, 1
    lv.puddle 380, 110, 0
    lv.puddle 230, 95, 1
  end if
  lv.px = 300
  lv.py = 170
end sub

sub lv.hospital()
  local id, solid
  lv.clearMap()
  for lv_y = 0 to 17
    for lv_x = 0 to 31
      lv.cell lv_x, lv_y, F_HFLOOR_0 + ((lv_x + lv_y) mod 2), 0
    next lv_x
  next lv_y
  for lv_x = 0 to 31
    lv.cell lv_x, 0, F_HWALL_TOP, 1
    if lv_x mod 6 = 3 then
      lv.cell lv_x, 1, F_DOOR_OPEN, 0
      lv.door lv_x, 1, 2, 1
    elseif lv_x mod 6 = 2 then
      lv.cell lv_x, 1, F_WARD_SIGN, 1
    else
      lv.cell lv_x, 1, F_HWALL, 1
    end if
    lv.cell lv_x, 15, F_HWALL_TOP, 1
    if lv_x mod 8 = 5 then
      lv.cell lv_x, 16, F_DOOR_SHUT, 1
      lv.door lv_x, 16, 14, 0
    elseif lv_x mod 8 = 1 then
      lv.cell lv_x, 16, F_DOOR_OPEN, 0
      lv.door lv_x, 16, 14, 1
    else
      lv.cell lv_x, 16, F_HWALL, 1
    end if
    lv.cell lv_x, 17, F_HWALL_TOP, 1
  next lv_x
  for lv_x = 0 to 31
    solid = 0
    if lv_x < 10 or (lv_x >= 14 and lv_x < 22) or lv_x >= 26 then solid = 1
    if solid = 1 then
      lv.cell lv_x, 8, F_HWALL_TOP, 1
      lv.cell lv_x, 9, F_HWALL, 1
    end if
  next lv_x
  lv.prop F_BED, 60, 60
  lv.prop F_BED, 300, 60
  lv.prop F_BED, 480, 250
  lv.prop F_BED, 140, 250
  lv.prop F_IV, 100, 52
  lv.prop F_IV, 345, 52
  lv.puddle 220, 120, 1
  lv.puddle 520, 140, 0
  lv.puddle 380, 215, 1
  lv.px = 300
  lv.py = 200
end sub

sub lv.subway()
  lv.clearMap()
  for lv_x = 0 to 31
    lv.cell lv_x, 0, F_BRICK, 1
    for lv_y = 1 to 17
      if lv_y = 6 or lv_y = 7 or lv_y = 8 or lv_y = 12 or lv_y = 13 then
        lv.cell lv_x, lv_y, F_TRACK_0 + ((lv_x + lv_y) mod 2), 0
      elseif lv_y = 5 or lv_y = 11 then
        lv.cell lv_x, lv_y, F_EDGE, 0
      else
        lv.cell lv_x, lv_y, F_PLAT_0 + ((lv_x * 3 + lv_y) mod 2), 0
      end if
    next lv_y
    map.t(lv_x, 9) = F_EDGE
    map.f(lv_x, 9) = 2
    map.s(lv_x, 9) = 0
    map.t(lv_x, 14) = F_EDGE
    map.f(lv_x, 14) = 2
    map.s(lv_x, 14) = 0
  next lv_x
  lv.cell 15, 16, F_STAIRS, 0
  lv.cell 16, 16, F_STAIRS, 0
  lv.prop F_PILLAR, 40, 30
  lv.prop F_PILLAR, 200, 30
  lv.prop F_PILLAR, 360, 30
  lv.prop F_PILLAR, 520, 30
  lv.prop F_PILLAR, 120, 190
  lv.prop F_PILLAR, 440, 190
  lv.puddle 250, 40, 1
  lv.puddle 560, 200, 0
  lv.puddle 70, 300, 1
  tr.on = 1
  tr.y(0) = 120
  tr.y(1) = 240
  tr.x(0) = -160 * 256
  tr.x(1) = 680 * 256
  tr.vx(0) = 1280
  tr.vx(1) = -1280
  tr.st(0) = 0
  tr.st(1) = 0
  tr.t(0) = 40
  tr.t(1) = 90
  tr.ow(0) = 0
  tr.ow(1) = 0
  tr.ww(0) = 0
  tr.ww(1) = 0
  lv.px = 380
  lv.py = 50
end sub

sub lv.paintCell(tx, ty)
  b_ori = 4 + map.f(tx, ty)
  b_w = fr.w(map.t(tx, ty)) * scr.sc
  b_h = fr.h(map.t(tx, ty)) * scr.sc
  b_dx = tx * TILE * scr.sc
  b_dy = ty * TILE * scr.sc
  b_sx = fr.x(map.t(tx, ty)) * scr.sc
  b_sy = fr.y(map.t(tx, ty)) * scr.sc
  blit b_sx, b_sy, b_dx, b_dy, b_w, b_h, fr.pg(map.t(tx, ty)), b_ori
end sub

sub lv.paint()
  page write PG_FIELD
  cls c.bg
  for lv_y = 0 to MAP_H - 1
    for lv_x = 0 to MAP_W - 1
      lv.paintCell lv_x, lv_y
    next lv_x
  next lv_y
  for lv_i = 0 to pr.n - 1
    spr.at pr.id(lv_i), pr.x(lv_i), pr.y(lv_i)
  next lv_i
  page copy PG_FIELD to PG_BACK
  page write PG_BACK
  scr.markAll()
end sub

sub lv.build()
  if lv.themeNow = 2 then
    lv.hospital()
  elseif lv.themeNow = 3 then
    lv.subway()
  else
    lv.street()
  end if
  lv.paint()
end sub

sub lv.openNext()
  for lv_i = 0 to door.n - 1
    if door.open(lv_i) = 0 then
      door.open(lv_i) = 1
      map.s(door.tx(lv_i), door.ty(lv_i)) = 0
      map.t(door.tx(lv_i), door.ty(lv_i)) = F_DOOR_OPEN
      page write PG_FIELD
      spr.at F_DOOR_OPEN, door.tx(lv_i) * TILE, door.ty(lv_i) * TILE
      page write PG_BACK
      spr.at F_DOOR_OPEN, door.tx(lv_i) * TILE, door.ty(lv_i) * TILE
      scr.mark drx, dry, drw, drh
      exit sub
    end if
  next lv_i
end sub

function pd.under()
  pd.under = 0
  pd_x = (pl.x \ 256) + 10
  pd_y = (pl.y \ 256) + 22
  for pd_i = 0 to pd.n - 1
    if pool(pd_i).alive = 1 then
      if pd_x >= pool(pd_i).x and pd_x < pool(pd_i).x + pool(pd_i).w then
        if pd_y >= pool(pd_i).y and pd_y < pool(pd_i).y + pool(pd_i).h then
          pd.under = 1
          exit function
        end if
      end if
    end if
  next pd_i
end function

function pd.nearest()
  pd_best = 2000000000
  pd.nearest = 0
  pd_x = pl.x \ 256
  pd_y = pl.y \ 256
  for pd_i = 0 to pd.n - 1
    if pool(pd_i).alive = 1 then
      pd_dx = pool(pd_i).w \ 2
      pd_dx = pool(pd_i).x + pd_dx - pd_x
      pd_dy = pool(pd_i).h \ 2
      pd_dy = pool(pd_i).y + pd_dy - pd_y
      pd_d2 = pd_dx * pd_dx + pd_dy * pd_dy
      if pd_d2 < pd_best then
        pd_best = pd_d2
        pd.nearest = pd_i
      end if
    end if
  next pd_i
end function

function pd.fid(i)
  pd.fid = F_PUDDLE_S_0 + pool(i).frm
  if pool(i).sz = 1 then pd.fid = F_PUDDLE_L_0 + pool(i).frm
  if pool(i).sz >= 2 then pd.fid = F_PUDDLE_W_0 + pool(i).frm
end function

sub pd.tick()
  for pd_i = 0 to pd.n - 1
    if pool(pd_i).alive = 1 then
      if gm.frame = (gm.frame \ 8) * 8 then
        pool(pd_i).frm = pool(pd_i).frm + 1
        if pool(pd_i).frm > 3 then pool(pd_i).frm = 0
      end if
    end if
  next pd_i
  if lv.rain = 0 then
    exit sub
  end if
  lv.rainT = lv.rainT + 1
  if lv.rainT < 220 then
    exit sub
  end if
  lv.rainT = 0
  if lv.raining = 1 then
    lv.raining = 0
    for pd_i = 0 to pd.n - 1
      if pool(pd_i).sz > 0 then
        pool(pd_i).sz = pool(pd_i).sz - 1
        pd.apply pd_i
      end if
    next pd_i
  else
    lv.raining = 1
    for pd_i = 0 to pd.n - 1
      if pool(pd_i).sz < 2 then
        pool(pd_i).sz = pool(pd_i).sz + 1
        pd.apply pd_i
      end if
    next pd_i
    if pd.n <= PD_LAST then
      lv.puddle 40 + rndn(520), 80 + rndn(200), 0
    end if
  end if
end sub

sub tr.tick()
  if tr.on = 0 then
    exit sub
  end if
  for tr_i = 0 to 1
    if tr.st(tr_i) = 0 then
      tr.t(tr_i) = tr.t(tr_i) - 1
      if tr.t(tr_i) <= 0 then
        tr.st(tr_i) = 1
        tr.t(tr_i) = 50
      end if
    elseif tr.st(tr_i) = 1 then
      tr.t(tr_i) = tr.t(tr_i) - 1
      if tr.t(tr_i) <= 0 then
        tr.st(tr_i) = 2
        if tr.vx(tr_i) > 0 then
          tr.x(tr_i) = -150 * 256
        else
          tr.x(tr_i) = 680 * 256
        end if
      end if
    else
      tr.x(tr_i) = tr.x(tr_i) + tr.vx(tr_i)
      if tr.vx(tr_i) > 0 and tr.x(tr_i) > 700 * 256 then
        tr.st(tr_i) = 0
        tr.t(tr_i) = 80
      end if
      if tr.vx(tr_i) < 0 and tr.x(tr_i) < -180 * 256 then
        tr.st(tr_i) = 0
        tr.t(tr_i) = 80
      end if
    end if
  next tr_i
  tr_px = (pl.x \ 256) + 10
  tr_py = (pl.y \ 256) + 29
  if trHits(tr_px, tr_py) = 1 then
    if tr_py < 200 then
      pl.y = 60 * 256
    else
      pl.y = 300 * 256
    end if
    gm.hurt 0
  end if
end sub

function trHits(px, py)
  trHits = 0
  for tr_i = 0 to 1
    if tr.st(tr_i) = 2 then
      tr_tx = tr.x(tr_i) \ 256
      if px >= tr_tx and px <= tr_tx + 120 then
        if py >= tr.y(tr_i) and py <= tr.y(tr_i) + 38 then
          trHits = 1
          exit function
        end if
      end if
    end if
  next tr_i
end function

function trHitsZed(px, py)
  trHitsZed = trHits(px, py)
end function
