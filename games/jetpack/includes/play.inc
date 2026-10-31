' play.inc - levels, physics, phaser, enemies, the play loop.
' One logic frame is an original IRQ0 tick, about 55ms. The tick only
' changes state. The main loop draws.
'
' Phaser: one tile, the cell beside the player. No beam and no enemy hits.
' Light bricks hide on frame 26, dark on 56 (EXE counters). The picture is
' the original dissolve strip, fuller then sparser. Letting go before the
' brick hides puts the tile back, which is what the capture did.
' Generated brick_melt art is the title scene only.

const FRAME_MS = 55, TICK_MS = 10, IDLE_MS = 4
const FUEL_MAX = 10000, LIVES0 = 5
const GE_NONE = 0, GE_NEXT = 1, GE_DEAD = 2, GE_OVER = 3, GE_TITLE = 4

dim gm.level, gm.lives, gm.score, gm.fuel, gm.lifeAt, gm.frm
dim gm.res, gm.drawing, gm.due, gm.tiles, gm.moved, gm.force, gm.first
dim gm.vis, gm.enHid
dim integer g_now

dim pl.x, pl.y, pl.x0, pl.vx, pl.vy, pl.face, pl.hface, pl.frm
dim pl.ground, pl.dead, pl.age, pl.gems, pl.shield, pl.lock, pl.tx, pl.ty
dim pl.phasing, pl.jetting, pl.climbing, pl.tdir, pl.sw, pl.og, pl.oj, pl.fall

dim lv.dx, lv.dy, lv.doorT, lv.doorF
dim string lv.name$, lv.line$
dim lv.id(25, 15), lv.ch(25, 15)
dim lv.i, lv.r

dim mt.n, mt.hot
dim mt.x(7), mt.y(7), mt.tm(7), mt.cl(7), mt.hid(7)

dim en.n
dim en.ty(19), en.x(19), en.y(19), en.vx(19), en.vy(19), en.fr(19), en.st(19)
dim en.pr(11), en.cd(19), en.latch(19), en.dir(19)
dim float en.rf

dim cv.n
dim cv.c(127), cv.r(127)

dim sp.on, sp.x, sp.y

dim spr.fr(23), spr.kind(23), spr.on(23)
dim spr.x(23), spr.y(23), spr.w(23), spr.h(23)

dim hud.dirty, fld.mark, fld.n
dim fld.q(415)
dim integer p_c, p_r, p_c0, p_c1, p_r0, p_r1, p_n, p_id, p_k, p_x0, p_y0, p_w
dim integer p_a, p_b, p_d, p_c2, p_r2, p_hit, p_f, p_h, p_col, p_i, p_bit, p_qx, p_qy
dim integer mt_c, mt_r, mt_j, sx0, sy0

' ---------- level file ----------

function fld.word$(s$, n)
  local i, p, q
  p = 1
  i = 1
  do while i < n
    q = instr(p, s$, " ")
    if q = 0 then
      fld.word$ = ""
      exit function
    end if
    p = q + 1
    i = i + 1
  loop
  q = instr(p, s$, " ")
  if q = 0 then
    fld.word$ = mid$(s$, p)
  else
    fld.word$ = mid$(s$, p, q - p)
  end if
end function

function lv.nib(a)
  lv.nib = 0
  if a >= 48 and a <= 57 then lv.nib = a - 48
  if a >= 97 and a <= 102 then lv.nib = a - 87
end function

function lv.hex(h$)
  lv.hex = lv.nib(asc(mid$(h$, 1, 1))) * 16 + lv.nib(asc(mid$(h$, 2, 1)))
end function

sub lv.peek(n)
  open "data/levels.dat" for input as #1
  line input #1, lv.line$
  for lv.i = 0 to n
    line input #1, lv.name$
    line input #1, lv.line$
    line input #1, lv.line$
    line input #1, lv.line$
    for lv.r = 0 to 7
      line input #1, lv.line$
    next lv.r
  next lv.i
  close #1
  if lv.name$ = "-" then lv.name$ = ""
end sub

sub lv.storeRow(rowpair, s$)
  local i, b, c, r
  for i = 0 to 51
    b = lv.hex(mid$(s$, i * 2 + 1, 2))
    c = i mod 26
    r = rowpair * 2 + (i \ 26)
    lv.id(c, r) = b
    lv.ch(c, r) = 0
    if b >= 114 and b <= 119 then
      if cv.n < 128 then
        cv.c(cv.n) = c
        cv.r(cv.n) = r
        cv.n = cv.n + 1
      end if
    end if
  next i
end sub

sub lv.oneEn(s$)
  local ty, c1, c2, x, y
  if en.n >= 20 then
    exit sub
  end if
  ty = val(s$)
  if ty < 1 or ty > 8 then
    exit sub
  end if
  c1 = instr(s$, ",")
  if c1 = 0 then
    exit sub
  end if
  c2 = instr(c1 + 1, s$, ",")
  if c2 = 0 then
    exit sub
  end if
  x = val(mid$(s$, c1 + 1))
  y = val(mid$(s$, c2 + 1))
  en.ty(en.n) = ty
  en.x(en.n) = x * 384
  en.y(en.n) = y * 384
  en.vx(en.n) = 0
  en.vy(en.n) = 0
  en.st(en.n) = 0
  en.fr(en.n) = 0
  en.cd(en.n) = 0
  en.latch(en.n) = 0
  en.dir(en.n) = 0
  if ty = 4 then en.st(en.n) = 1
  en.n = en.n + 1
end sub

sub lv.parseEn(s$)
  local p, q
  en.n = 0
  if s$ = "-" or s$ = "" then
    exit sub
  end if
  p = 1
  do
    q = instr(p, s$, " ")
    if q = 0 then
      lv.oneEn(mid$(s$, p))
      exit do
    end if
    lv.oneEn(mid$(s$, p, q - p))
    p = q + 1
  loop
end sub

sub lv.countGems()
  pl.gems = 0
  for p_r = 0 to 15
    for p_c = 0 to 25
      if lv.id(p_c, p_r) = 4 then pl.gems = pl.gems + 1
    next p_c
  next p_r
end sub

sub lv.clearKind(k)
  for p_r = 0 to 15
    for p_c = 0 to 25
      p_id = lv.id(p_c, p_r)
      if p_id < 128 then
        if tf.k(p_id) = k then lv.id(p_c, p_r) = 0
      end if
    next p_c
  next p_r
end sub

sub lv.hooks()
  if TH_GEMS = 1 then
    lv.clearKind(4)
    en.n = 0
    p_c = (pl.x - 192) \ 384
    p_r = (pl.y - 192) \ 384
    if p_c >= 0 and p_r >= 0 and p_c <= 25 and p_r <= 15 then lv.id(p_c, p_r) = 4
    lv.dx = pl.x \ 384
    lv.dy = pl.y \ 384
    if lv.dx < 1 then lv.dx = 1
    if lv.dy < 1 then lv.dy = 1
  end if
  if TH_PHASE = 1 then
    en.n = 0
    mt_c = (pl.x \ 32) \ 12
    mt_r = ((pl.y - 384) \ 32) \ 12
    if mt_c >= 0 and mt_c <= 25 and mt_r >= 0 and mt_r <= 15 then lv.id(mt_c, mt_r) = 40
  end if
  if TH_KILL = 1 then
    en.n = 1
    en.ty(0) = 1
    en.x(0) = pl.x
    en.y(0) = pl.y
    en.vx(0) = 0
    en.vy(0) = 0
    en.st(0) = 0
    en.fr(0) = 4
  end if
  if TH_KILL = 2 then
    en.n = 0
    sp.on = 1
    sp.x = pl.x
    sp.y = pl.y - 192
  else
    sp.on = 0
  end if
  lv.countGems()
end sub

sub lv.load(n)
  local row
  cv.n = 0
  open "data/levels.dat" for input as #1
  line input #1, lv.line$
  for lv.i = 0 to n
    line input #1, lv.name$
    line input #1, lv.line$
    if lv.i = n then
      lv.dx = val(fld.word$(lv.line$, 1))
      lv.dy = val(fld.word$(lv.line$, 2))
      pl.x = val(fld.word$(lv.line$, 3)) * 384
      pl.y = val(fld.word$(lv.line$, 4)) * 384
    end if
    line input #1, lv.line$
    line input #1, lv.line$
    if lv.i = n then lv.parseEn(lv.line$)
    for row = 0 to 7
      line input #1, lv.line$
      if lv.i = n then lv.storeRow(row, lv.line$)
    next row
  next lv.i
  close #1
  if lv.name$ = "-" then lv.name$ = ""
  pl.vx = 0
  pl.vy = 0
  pl.face = 0
  pl.hface = 0
  pl.frm = 0
  pl.ground = FALSE
  pl.dead = FALSE
  pl.age = 0
  pl.shield = 0
  pl.lock = 0
  pl.phasing = FALSE
  pl.jetting = FALSE
  pl.climbing = FALSE
  pl.tdir = 0
  pl.sw = 0
  pl.og = 0
  pl.oj = 0
  pl.fall = 0
  lv.doorT = 0
  lv.doorF = 0
  mt.n = 0
  mt.hot = -1
  lv.hooks()
  spr.reset()
end sub

' ---------- map ----------

function fld.solid(c, r)
  fld.solid = TRUE
  if c < 0 or r < 0 or c > 25 or r > 15 then
    exit function
  end if
  p_id = lv.id(c, r)
  if p_id >= 128 then
    fld.solid = FALSE
    exit function
  end if
  ' Floor switches 0x21-0x23 are not walls. EXE support is a foot
  ' id >= 0x28 (file 0x1024D), so the body can overlap the plate.
  if p_id >= 33 and p_id <= 35 then
    fld.solid = FALSE
    exit function
  end if
  p_k = tf.k(p_id and 127)
  if p_k = 1 or p_k = 3 or p_k = 20 or p_k = 21 then
    fld.solid = TRUE
  else
    fld.solid = FALSE
  end if
end function

' Same as fld.solid, except a ladder or escalator is not a wall.
' The player walks into that column and climbs. Balls still land
' on an escalator through fld.solid.
function pl.blocks(c, r)
  pl.blocks = fld.solid(c, r)
  if pl.blocks = FALSE then
    exit function
  end if
  if c < 0 or r < 0 or c > 25 or r > 15 then
    exit function
  end if
  p_id = lv.id(c, r)
  if p_id >= 1 and p_id <= 3 then pl.blocks = FALSE
end function

function bod.hit(x, y)
  bod.hit = TRUE
  if x < 384 or y < 384 then
    exit function
  end if
  p_c0 = (x - 344) \ 384
  p_c1 = (x - 40) \ 384
  p_r0 = (y - 344) \ 384
  p_r1 = (y - 40) \ 384
  if p_c0 < 0 then p_c0 = 0
  if p_r0 < 0 then p_r0 = 0
  if p_c1 > 25 or p_r1 > 15 then
    exit function
  end if
  for p_r = p_r0 to p_r1
    for p_c = p_c0 to p_c1
      if fld.solid(p_c, p_r) then
        exit function
      end if
    next p_c
  next p_r
  bod.hit = FALSE
end function

sub fld.blit(c, r)
  local cell, id, dx, dy
  id = lv.id(c, r)
  cell = mt.visualAt(c, r)
  dx = lay.ox + c * lay.tile
  dy = lay.oy + r * lay.tile
  if cell >= 0 then
    blit cell * lay.tile, lay.syMelt, dx, dy, lay.tile, lay.tile, PG_SHEET
  else
    if id >= 128 then id = id and 127
    ' Palette steps of this belt, baked at column 20+phase, row id-114.
    if id >= 114 and id <= 119 then
      p_n = (gm.frm \ 3) mod 4
      blit (20 + p_n) * lay.tile, (id - 114) * lay.tile, dx, dy, lay.tile, lay.tile, PG_SHEET
    else
      blit (id mod 20) * lay.tile, (id \ 20) * lay.tile, dx, dy, lay.tile, lay.tile, PG_SHEET
    end if
  end if
  lv.ch(c, r) = 0
  if fld.mark <> 0 then scr.mark(dx, dy, lay.tile, lay.tile)
end sub

sub fld.touch(c, r)
  if c < 0 or r < 0 or c > 25 or r > 15 then
    exit sub
  end if
  if lv.ch(c, r) then
    exit sub
  end if
  lv.ch(c, r) = 1
  if fld.n >= 416 then
    exit sub
  end if
  fld.q(fld.n) = c + r * 26
  fld.n = fld.n + 1
end sub

sub cv.mark()
  for p_i = 0 to cv.n - 1
    fld.touch(cv.c(p_i), cv.r(p_i))
  next p_i
end sub

sub fld.flush()
  if fld.n = 0 then
    exit sub
  end if
  for p_i = 0 to fld.n - 1
    p_c = fld.q(p_i) mod 26
    p_r = fld.q(p_i) \ 26
    if lv.ch(p_c, p_r) then fld.blit(p_c, p_r)
  next p_i
  fld.n = 0
end sub

sub fld.paint()
  local x, y
  page write PG_BACK
  cls rgb(0, 0, 0)
  for y = 0 to scr.H - 1 step lay.tile
    for x = 0 to scr.W - 1 step lay.tile
      blit 0, 3 * lay.tile, x, y, lay.tile, lay.tile, PG_DIM
    next x
  next y
  fld.n = 0
  fld.mark = 0
  for p_r = 0 to 15
    for p_c = 0 to 25
      lv.ch(p_c, p_r) = 0
      fld.blit(p_c, p_r)
    next p_c
  next p_r
  fld.mark = 1
end sub

' ---------- phaser ----------

function mt.visual(i)
  local tm, cl
  tm = mt.tm(i)
  cl = mt.cl(i)
  mt.visual = -1
  if cl = 0 then
    if tm >= 21 and tm <= 25 then mt.visual = 25 - tm
    if tm >= 110 and tm <= 150 then
      if (tm mod 10) = 0 then mt.visual = (150 - tm) \ 10
    end if
  end if
  if cl = 1 then
    if tm >= 51 and tm <= 55 then mt.visual = 60 - tm
    if tm >= 260 and tm <= 300 then
      if (tm mod 10) = 0 then mt.visual = (300 - tm) \ 10 + 5
    end if
  end if
  if cl = 2 then
    if tm >= 1 and tm <= 5 then mt.visual = tm + 9
  end if
  if cl = 3 then
    if tm >= 1 and tm <= 5 then mt.visual = tm + 14
    if tm >= 20 and tm <= 40 then
      if (tm mod 5) = 0 then mt.visual = (40 - tm) \ 5 + 15
    end if
  end if
end function

function mt.visualAt(c, r)
  local i
  mt.visualAt = -1
  for i = 0 to mt.n - 1
    if mt.x(i) = c and mt.y(i) = r then
      mt.visualAt = mt.visual(i)
      exit function
    end if
  next i
end function

sub mt.drop(i)
  p_a = 0
  if mt.hot = i then p_a = 1
  fld.touch(mt.x(i), mt.y(i))
  gm.tiles = TRUE
  mt_j = mt.n - 1
  if i <> mt_j then
    mt.x(i) = mt.x(mt_j)
    mt.y(i) = mt.y(mt_j)
    mt.tm(i) = mt.tm(mt_j)
    mt.cl(i) = mt.cl(mt_j)
    mt.hid(i) = mt.hid(mt_j)
    if mt.hot = mt_j then mt.hot = i
  end if
  if p_a = 1 then mt.hot = -1
  mt.n = mt_j
end sub

sub mt.hide(i)
  p_id = lv.id(mt.x(i), mt.y(i))
  lv.id(mt.x(i), mt.y(i)) = (p_id and 127) or 128
  mt.hid(i) = 1
  fld.touch(mt.x(i), mt.y(i))
  gm.tiles = TRUE
end sub

sub mt.back(i)
  lv.id(mt.x(i), mt.y(i)) = lv.id(mt.x(i), mt.y(i)) and 127
  mt.hid(i) = 2
  fld.touch(mt.x(i), mt.y(i))
  gm.tiles = TRUE
end sub

sub mt.expire(i)
  if mt.cl(i) = 2 and mt.tm(i) >= 5 then
    lv.id(mt.x(i), mt.y(i)) = 0
    mt.hid(i) = 2
    fld.touch(mt.x(i), mt.y(i))
    gm.tiles = TRUE
    exit sub
  end if
  if mt.hid(i) = 0 then
    if mt.cl(i) = 0 and mt.tm(i) >= 26 then mt.hide(i)
    if mt.cl(i) = 1 and mt.tm(i) >= 56 then mt.hide(i)
    if mt.cl(i) = 3 and mt.tm(i) >= 5 then mt.hide(i)
  else
    if mt.cl(i) = 0 and mt.tm(i) > 150 then mt.back(i)
    if mt.cl(i) = 1 and mt.tm(i) > 300 then mt.back(i)
    if mt.cl(i) = 3 and mt.tm(i) > 40 then mt.back(i)
  end if
end sub

sub mt.aim()
  mt.hot = -1
  if pl.phasing = 0 or pl.dead then
    exit sub
  end if
  if pl.face = 0 then
    mt_c = (pl.x \ 32) \ 12
    mt_r = ((pl.y - 384) \ 32) \ 12
  end if
  if pl.face = 1 then
    mt_c = ((pl.x - 416) \ 32) \ 12
    mt_r = ((pl.y - 384) \ 32) \ 12
  end if
  if pl.face = 2 then
    mt_c = ((pl.x - 384) \ 32) \ 12
    mt_r = ((pl.y - 416) \ 32) \ 12
  end if
  if pl.face = 3 then
    mt_c = ((pl.x - 384) \ 32) \ 12
    mt_r = (pl.y \ 32) \ 12
  end if
  if mt_c < 0 or mt_r < 0 or mt_c > 25 or mt_r > 15 then
    exit sub
  end if
  p_id = lv.id(mt_c, mt_r)
  if p_id >= 128 then
    for p_i = 0 to mt.n - 1
      if mt.x(p_i) = mt_c and mt.y(p_i) = mt_r then mt.hot = p_i
    next p_i
    exit sub
  end if
  p_k = tf.c(p_id and 127)
  if p_k < 0 then
    exit sub
  end if
  p_bit = 1
  if pl.face = 1 then p_bit = 2
  if pl.face = 2 then p_bit = 4
  if pl.face = 3 then p_bit = 8
  if (tf.m(p_id and 127) and p_bit) = 0 then
    exit sub
  end if
  for p_i = 0 to mt.n - 1
    if mt.x(p_i) = mt_c and mt.y(p_i) = mt_r then
      mt.hot = p_i
      exit sub
    end if
  next p_i
  if mt.n >= 8 then
    exit sub
  end if
  mt.x(mt.n) = mt_c
  mt.y(mt.n) = mt_r
  mt.tm(mt.n) = 0
  mt.cl(mt.n) = p_k
  mt.hid(mt.n) = 0
  mt.hot = mt.n
  mt.n = mt.n + 1
end sub

sub mt.step()
  mt.aim()
  p_i = 0
  do while p_i < mt.n
    if mt.hid(p_i) = 0 then
      if p_i = mt.hot then
        mt.tm(p_i) = mt.tm(p_i) + 1
        fld.touch(mt.x(p_i), mt.y(p_i))
        gm.tiles = TRUE
        mt.expire(p_i)
        if mt.hid(p_i) = 2 then
          mt.drop(p_i)
        else
          p_i = p_i + 1
        end if
      else
        mt.drop(p_i)
      end if
    else
      mt.tm(p_i) = mt.tm(p_i) + 1
      fld.touch(mt.x(p_i), mt.y(p_i))
      gm.tiles = TRUE
      mt.expire(p_i)
      if mt.hid(p_i) = 2 then
        mt.drop(p_i)
      else
        p_i = p_i + 1
      end if
    end if
  loop
end sub

' ---------- player ----------

sub pl.addScore(n)
  gm.score = gm.score + n
  if (gm.score \ 1000) > gm.lifeAt then
    gm.lives = gm.lives + 1
    gm.lifeAt = gm.score \ 1000
    hud.dirty = TRUE
  end if
end sub

sub pl.die()
  if pl.dead then
    exit sub
  end if
  if pl.shield > 0 then
    exit sub
  end if
  pl.dead = TRUE
  pl.age = 0
  pl.vx = 0
  pl.vy = 0
  pl.jetting = FALSE
  pl.phasing = FALSE
end sub

sub pl.feet()
  pl.ground = FALSE
  p_c = (pl.x - 192) \ 384
  p_r = pl.y \ 384
  if fld.solid(p_c, p_r) and pl.vy >= 0 then pl.ground = TRUE
end sub

function pl.onLadder()
  pl.onLadder = FALSE
  p_c = (pl.x - 192) \ 384
  p_r = (pl.y - 192) \ 384
  if p_c < 0 or p_r < 0 or p_c > 25 or p_r > 15 then
    exit function
  end if
  p_id = lv.id(p_c, p_r)
  if p_id >= 128 then
    exit function
  end if
  if tf.k(p_id and 127) = 2 then pl.onLadder = TRUE
end function

sub pl.faceKeys()
  if key.left then
    pl.hface = 1
    pl.face = 1
  end if
  if key.right then
    pl.hface = 0
    pl.face = 0
  end if
  if pl.phasing then
    if key.up then pl.face = 2
    if key.dn and key.up = 0 then pl.face = 3
  else
    if pl.face > 1 then pl.face = pl.hface
  end if
end sub

sub pl.align()
  p_c = (pl.x - 1) \ 384
  if p_c < 0 then p_c = 0
  if p_c > 25 then p_c = 25
  p_w = (p_c + 1) * 384
  if pl.x < p_w then
    pl.x = pl.x + 32
    if pl.x > p_w then pl.x = p_w
  else
    if pl.x > p_w then
      pl.x = pl.x - 32
      if pl.x < p_w then pl.x = p_w
    end if
  end if
  p_r = (pl.y - 1) \ 384
  if p_r < 0 then p_r = 0
  if p_r > 15 then p_r = 15
  p_w = (p_r + 1) * 384
  if pl.y < p_w then
    pl.y = pl.y + 32
    if pl.y > p_w then pl.y = p_w
  else
    if pl.y > p_w then
      pl.y = pl.y - 32
      if pl.y < p_w then pl.y = p_w
    end if
  end if
end sub

sub pl.jet()
  pl.jetting = TRUE
  gm.fuel = gm.fuel - 10
  if gm.fuel < 0 then gm.fuel = 0
  hud.dirty = TRUE
  if key.jet and key.prevJ = 0 and pl.ground then
    pl.vy = -68
    if key.right then pl.vx = 62
    if key.left then pl.vx = -62
    if key.right = 0 and key.left = 0 then pl.vx = 0
  else
    if key.right then pl.vx = pl.vx + 6
    if key.left then pl.vx = pl.vx - 6
    if key.dn and pl.ground = 0 then
      if pl.vy > 5 then pl.vy = pl.vy - 5
      if pl.vy < -5 then pl.vy = pl.vy + 5
      if pl.vy >= -5 and pl.vy <= 5 then pl.vy = 0
    else
      pl.vy = pl.vy - 4
    end if
  end if
  if pl.vx > 64 then pl.vx = 64
  if pl.vx < -64 then pl.vx = -64
  if pl.vy > 64 then pl.vy = 64
  if pl.vy < -64 then pl.vy = -64
end sub

function pl.tile(c, r)
  pl.tile = 0
  if c < 0 or r < 0 or c > 25 or r > 15 then
    exit function
  end if
  p_id = lv.id(c, r)
  if p_id >= 128 then
    exit function
  end if
  pl.tile = p_id
end function

function pl.climbId(tid)
  pl.climbId = 0
  if tid >= 1 and tid <= 3 then pl.climbId = 1
end function

' ds:0x15E4 is the IEEE double 0.6. FIST of n*0.6, nearest.
function sc.ice(v)
  p_a = v * 3
  if p_a < 0 then
    sc.ice = -((2 - p_a) \ 5)
  else
    sc.ice = (p_a + 2) \ 5
  end if
end function

sub pl.ice(gid)
  if gid >= 97 and gid <= 99 then pl.vx = sc.ice(pl.vx)
end sub

' The one tile under the centre column. Left belt: against -60,
' with +25, stand -40. Right belt: with +60, against -25, stand +40.
sub pl.belt(gid)
  p_n = 0
  if key.right and key.left = 0 then p_n = 1
  if key.left and key.right = 0 then p_n = -1
  if gid >= 114 and gid <= 116 then
    pl.vx = -40
    if p_n < 0 then pl.vx = -60
    if p_n > 0 then pl.vx = 25
  end if
  if gid >= 117 and gid <= 119 then
    pl.vx = 40
    if p_n > 0 then pl.vx = 60
    if p_n < 0 then pl.vx = -25
  end if
end sub

' Ids 1..3 in the centre column are climbable. Id 2 and id 3 keep
' their own vy. Up beside a ladder walks at 55 until the sprite is
' centred on that column, then climbs. No pixel probe, no snap on exit.
sub pl.climb()
  p_c = (pl.x - 192) \ 384
  p_r = (pl.y - 192) \ 384
  p_b = pl.tile(p_c, p_r)
  p_h = pl.tile(p_c, pl.y \ 384)
  p_id = p_b
  p_f = 0
  if pl.climbId(p_id) = 1 then p_f = 1
  if pl.climbId(p_h) = 1 then p_f = 1
  if p_f = 0 then
    if pl.ground and key.up and key.dn = 0 and key.left = 0 and key.right = 0 then
      p_a = 0
      p_b = pl.tile(p_c - 1, p_r)
      if pl.climbId(p_b) = 1 then p_a = -1
      if p_a = 0 then
        p_b = pl.tile(p_c + 1, p_r)
        if pl.climbId(p_b) = 1 then p_a = 1
      end if
      if p_a <> 0 then
        pl.climbing = TRUE
        pl.vy = 0
        pl.vx = p_a * 55
        if p_a < 0 then
          pl.hface = 1
          pl.face = 1
        else
          pl.hface = 0
          pl.face = 0
        end if
      end if
    end if
    exit sub
  end if
  pl.climbing = TRUE
  p_n = 0
  if key.dn and key.up = 0 then p_n = 1
  if key.up and key.dn = 0 then p_n = -1
  p_k = 1
  if p_id = 2 or p_h = 2 then p_k = 2
  if p_k = 1 then
    if p_id = 3 or p_h = 3 then p_k = 3
  end if
  pl.vx = 0
  if key.left and key.right = 0 then
    pl.hface = 1
    pl.face = 1
    pl.vx = -55
  end if
  if key.right and key.left = 0 then
    pl.hface = 0
    pl.face = 0
    pl.vx = 55
  end if
  if pl.vx = 0 and p_n <> 0 then
    p_w = (p_c + 1) * 384
    if pl.x <> p_w then
      if pl.x < p_w then
        pl.vx = 32
        if pl.x + pl.vx > p_w then pl.vx = p_w - pl.x
      else
        pl.vx = -32
        if pl.x + pl.vx < p_w then pl.vx = p_w - pl.x
      end if
      pl.vy = 0
      exit sub
    end if
  end if
  if p_k = 2 then
    pl.vy = -25
    if p_n < 0 then pl.vy = -64
    if p_n > 0 then pl.vy = 20
  end if
  if p_k = 3 then
    pl.vy = 25
    if p_n > 0 then pl.vy = 64
    if p_n < 0 then pl.vy = -20
  end if
  if p_k = 1 then pl.vy = p_n * 45
end sub

sub pl.walk()
  if key.left and key.right = 0 then
    pl.hface = 1
    pl.face = 1
  end if
  if key.right and key.left = 0 then
    pl.hface = 0
    pl.face = 0
  end if
  if pl.ground then
    pl.fall = 0
    pl.vx = 0
    if key.left and key.right = 0 then pl.vx = -55
    if key.right and key.left = 0 then pl.vx = 55
    pl.vy = 0
  else
    ' 0x1030B: the frame a walk leaves the floor clears vx and
    ' sets vy to 64. Later fall frames add 6, cap 64.
    ' Horizontal keys add 5 and then decay by 3, clamped to ±20
    ' (0x103AC, 0x10407). Walk speed is not carried into the fall.
    ' A jet hop is state 2, not this path, so its vx is left alone.
    p_f = 0
    if pl.og <> 0 and pl.oj = 0 then
      pl.vx = 0
      pl.vy = 64
      pl.fall = 1
      p_f = 1
    end if
    if pl.fall = 0 then
      pl.vy = pl.vy + 6
      if pl.vy > 64 then pl.vy = 64
    else
      if p_f = 0 then
        pl.vy = pl.vy + 6
        if pl.vy > 64 then pl.vy = 64
      end if
      if key.right and key.left = 0 then pl.vx = pl.vx + 5
      if key.left and key.right = 0 then pl.vx = pl.vx - 5
      if pl.vx > 20 then pl.vx = 20
      if pl.vx < -20 then pl.vx = -20
      if pl.vx > 3 then
        pl.vx = pl.vx - 3
      else
        if pl.vx < -3 then
          pl.vx = pl.vx + 3
        else
          pl.vx = 0
        end if
      end if
    end if
  end if
  p_id = pl.tile((pl.x - 192) \ 384, pl.y \ 384)
  pl.ice(p_id)
  pl.belt(p_id)
end sub

sub pl.control()
  pl.feet()
  pl.oj = pl.jetting
  pl.phasing = FALSE
  pl.jetting = FALSE
  pl.climbing = FALSE
  if pl.lock > 0 then
    pl.lock = pl.lock - 1
    pl.vx = 0
    pl.vy = 0
    if pl.lock = 0 then
      pl.x = pl.tx
      pl.y = pl.ty
    end if
    exit sub
  end if
  if key.phase then pl.phasing = TRUE
  pl.faceKeys()
  if pl.phasing then
    pl.vx = 0
    pl.vy = 0
    pl.align()
    exit sub
  end if
  pl.climb()
  if pl.climbing then
    pl.og = 1
    exit sub
  end if
  if key.jet and gm.fuel > 0 then
    pl.fall = 0
    pl.jet()
    pl.og = 0
    exit sub
  end if
  pl.walk()
  pl.og = pl.ground
end sub

' Centre column only. A solid there is a wall. The sprite may overlap
' a curved neighbour; that neighbour is not a rail to slide along.
sub pl.moveX()
  if pl.vx = 0 then
    exit sub
  end if
  pl.x = pl.x + pl.vx
  if pl.x < 384 then
    pl.x = 384
    pl.vx = 0
  end if
  if pl.x > 9984 then
    pl.x = 9984
    pl.vx = 0
  end if
  p_c = (pl.x - 192) \ 384
  p_r = (pl.y - 192) \ 384
  if pl.blocks(p_c, p_r) then
    if pl.vx > 0 then
      pl.x = p_c * 384 + 191
    else
      pl.x = (p_c + 1) * 384 + 192
    end if
    pl.vx = 0
  end if
end sub

' Ground is the tile under the centre column. Ceiling is the tile
' above the sprite in that same column. Landing clamps to the tile
' edge and does not walk back along a corner.
sub pl.moveY()
  if pl.vy = 0 then
    exit sub
  end if
  pl.y = pl.y + pl.vy
  if pl.y < 384 then
    pl.y = 384
    pl.vy = 0
  end if
  if pl.y > 6144 then
    pl.y = 6144
    pl.vy = 0
  end if
  p_c = (pl.x - 192) \ 384
  if pl.vy > 0 then
    p_r = pl.y \ 384
    if pl.blocks(p_c, p_r) then
      pl.y = p_r * 384
      pl.vy = 0
      pl.ground = TRUE
    end if
  end if
  if pl.vy < 0 then
    p_r = (pl.y - 384) \ 384
    if pl.blocks(p_c, p_r) then
      pl.y = (p_r + 2) * 384
      pl.vy = 0
    end if
  end if
end sub

sub pl.take(c, r)
  if c < 0 or r < 0 or c > 25 or r > 15 then
    exit sub
  end if
  p_id = lv.id(c, r)
  if p_id <= 0 or p_id >= 128 then
    exit sub
  end if
  p_k = tf.k(p_id)
  p_a = 0
  if p_k = 4 then
    pl.gems = pl.gems - 1
    if pl.gems < 0 then pl.gems = 0
    pl.addScore(5)
    p_a = 1
  end if
  if p_k = 5 then
    gm.fuel = gm.fuel + 5000
    pl.addScore(10)
    p_a = 1
  end if
  if p_k = 6 then
    gm.fuel = gm.fuel + 10000
    pl.addScore(20)
    p_a = 1
  end if
  if p_k = 9 then
    pl.addScore(10)
    p_a = 1
  end if
  if p_k = 10 then
    pl.addScore(25)
    p_a = 1
  end if
  if p_k = 11 then
    pl.addScore(50)
    p_a = 1
  end if
  if p_k = 12 then
    gm.lives = gm.lives + 1
    pl.addScore(100)
    p_a = 1
  end if
  if p_k = 13 then
    pl.shield = 180
    pl.addScore(25)
    p_a = 1
  end if
  if p_a = 0 then
    exit sub
  end if
  if gm.fuel > FUEL_MAX then gm.fuel = FUEL_MAX
  lv.id(c, r) = 0
  fld.touch(c, r)
  gm.tiles = TRUE
  if p_k = 4 or p_k = 5 or p_k = 6 or p_k = 12 then hud.dirty = TRUE
end sub

sub pl.cellFuel(c, r)
  if c < 0 or r < 0 or c > 25 or r > 15 then
    exit sub
  end if
  p_id = lv.id(c, r)
  if p_id >= 128 then
    exit sub
  end if
  p_k = tf.k(p_id and 127)
  if p_k <> 7 and p_k <> 8 then
    exit sub
  end if
  p_h = gm.fuel
  if p_k = 7 then gm.fuel = gm.fuel + 30
  if p_k = 8 then gm.fuel = gm.fuel - 20
  if gm.fuel > FUEL_MAX then gm.fuel = FUEL_MAX
  if gm.fuel < 0 then gm.fuel = 0
  if gm.fuel <> p_h then hud.dirty = TRUE
end sub

sub pl.pickups()
  p_c0 = (pl.x - 344) \ 384
  p_c1 = (pl.x - 40) \ 384
  p_r0 = (pl.y - 344) \ 384
  p_r1 = (pl.y - 40) \ 384
  if p_c0 < 0 then p_c0 = 0
  if p_r0 < 0 then p_r0 = 0
  if p_c1 > 25 then p_c1 = 25
  if p_r1 > 15 then p_r1 = 15
  for p_r = p_r0 to p_r1
    for p_c = p_c0 to p_c1
      pl.take(p_c, p_r)
    next p_c
  next p_r
  pl.cellFuel((pl.x - 192) \ 384, pl.y \ 384)
  pl.cellFuel((pl.x - 192) \ 384, (pl.y - 192) \ 384)
end sub

sub lv.flip(k)
  p_a = 9
  p_b = 10
  if k = 18 then
    p_a = 11
    p_b = 12
  end if
  if k = 19 then
    p_a = 13
    p_b = 14
  end if
  for p_r = 0 to 15
    for p_c = 0 to 25
      if lv.id(p_c, p_r) = p_a then lv.id(p_c, p_r) = 200
    next p_c
  next p_r
  for p_r = 0 to 15
    for p_c = 0 to 25
      if lv.id(p_c, p_r) = p_b then
        lv.id(p_c, p_r) = p_a
        fld.touch(p_c, p_r)
      end if
      if lv.id(p_c, p_r) = 200 then
        lv.id(p_c, p_r) = p_b
        fld.touch(p_c, p_r)
      end if
    next p_c
  next p_r
  ' Tube mirage. EXE 0x11F28: the pair 0x4A+2*(signal-1) is plain
  ' (state 1) or already OR 0x80 (state 2). Plain becomes mirage;
  ' mirage is written back without the bit. en.tid and fld.solid
  ' already treat id >= 128 as empty, so the tube opens.
  p_d = 74
  if k = 18 then p_d = 76
  if k = 19 then p_d = 78
  p_f = 0
  for p_r = 0 to 15
    for p_c = 0 to 25
      if p_f = 0 then
        p_id = lv.id(p_c, p_r)
        p_h = p_id and 127
        if p_h = p_d or p_h = p_d + 1 then
          p_f = 1
          if p_id >= 128 then p_f = 2
        end if
      end if
    next p_c
  next p_r
  if p_f <> 0 then
    for p_r = 0 to 15
      for p_c = 0 to 25
        p_id = lv.id(p_c, p_r)
        p_h = p_id and 127
        if p_h = p_d or p_h = p_d + 1 then
          if p_f = 2 then
            lv.id(p_c, p_r) = p_h
          else
            lv.id(p_c, p_r) = p_h or 128
          end if
          fld.touch(p_c, p_r)
        end if
      next p_c
    next p_r
  end if
  gm.tiles = TRUE
end sub

sub pl.teleport(k)
  p_n = 0
  p_c0 = (pl.x - 192) \ 384
  p_r0 = (pl.y - 192) \ 384
  for p_r = 0 to 15
    for p_c = 0 to 25
      p_id = lv.id(p_c, p_r)
      if p_id < 128 then
        if tf.k(p_id) = k and (p_c <> p_c0 or p_r <> p_r0) then p_n = p_n + 1
      end if
    next p_c
  next p_r
  if p_n = 0 then
    exit sub
  end if
  p_w = (gm.frm + pl.x \ 64 + pl.y \ 64) mod p_n
  p_n = 0
  for p_r = 0 to 15
    for p_c = 0 to 25
      p_id = lv.id(p_c, p_r)
      if p_id < 128 then
        if tf.k(p_id) = k and (p_c <> p_c0 or p_r <> p_r0) then
          if p_n = p_w then
            pl.tx = (p_c + 1) * 384
            pl.ty = (p_r + 1) * 384
            pl.lock = 20
            exit sub
          end if
          p_n = p_n + 1
        end if
      end if
    next p_c
  next p_r
end sub

' Player floor switch. Centre-column tile 0x21-0x23.
sub pl.floor()
  if pl.phasing or pl.lock > 0 then
    exit sub
  end if
  p_id = pl.tile((pl.x - 192) \ 384, (pl.y - 192) \ 384)
  if p_id >= 33 and p_id <= 35 then
    if pl.sw = 0 then
      if p_id = 33 then lv.flip(17)
      if p_id = 34 then lv.flip(18)
      if p_id = 35 then lv.flip(19)
      pl.sw = 1
    end if
  else
    pl.sw = 0
  end if
end sub

sub pl.use()
  if pl.phasing or pl.lock > 0 then
    exit sub
  end if
  if key.dn = 0 or key.prevD <> 0 then
    exit sub
  end if
  p_c = (pl.x - 192) \ 384
  p_r = (pl.y - 192) \ 384
  if p_c < 0 or p_r < 0 or p_c > 25 or p_r > 15 then
    exit sub
  end if
  p_id = lv.id(p_c, p_r)
  if p_id >= 128 then
    exit sub
  end if
  p_k = tf.k(p_id and 127)
  if p_k >= 17 and p_k <= 19 then lv.flip(p_k)
  if p_k >= 14 and p_k <= 16 then pl.teleport(p_k)
end sub

sub pl.tube()
  if key.left or key.right or key.up or key.dn then
    exit sub
  end if
  if pl.phasing or pl.lock > 0 or pl.dead then
    exit sub
  end if
  p_c = (pl.x - 192) \ 384
  p_r = (pl.y - 192) \ 384
  if p_c < 0 or p_r < 0 or p_c > 25 or p_r > 15 then
    exit sub
  end if
  p_id = lv.id(p_c, p_r)
  if p_id >= 128 then
    exit sub
  end if
  if tf.k(p_id and 127) <> 22 then
    exit sub
  end if
  for p_n = 0 to 3
    p_d = (pl.tdir + p_n) mod 4
    p_c2 = p_c
    p_r2 = p_r
    if p_d = 0 then p_c2 = p_c + 1
    if p_d = 1 then p_c2 = p_c - 1
    if p_d = 2 then p_r2 = p_r - 1
    if p_d = 3 then p_r2 = p_r + 1
    if p_c2 >= 0 and p_c2 <= 25 and p_r2 >= 0 and p_r2 <= 15 then
      p_id = lv.id(p_c2, p_r2)
      if p_id < 128 then
        if tf.k(p_id and 127) = 22 then
          pl.tdir = p_d
          p_x0 = pl.x
          p_y0 = pl.y
          if p_d = 0 then pl.x = pl.x + 64
          if p_d = 1 then pl.x = pl.x - 64
          if p_d = 2 then pl.y = pl.y - 64
          if p_d = 3 then pl.y = pl.y + 64
          p_c = (pl.x - 192) \ 384
          p_r = (pl.y - 192) \ 384
          if fld.solid(p_c, p_r) then
            pl.x = p_x0
            pl.y = p_y0
          end if
          exit sub
        end if
      end if
    end if
  next p_n
end sub

sub pl.anim()
  if pl.dead then
    if pl.age < 36 then
      pl.frm = (pl.age \ 6) mod 6 + 72
    else
      pl.frm = 83
    end if
    exit sub
  end if
  if pl.phasing then
    p_n = (gm.frm \ 2) mod 3
    if pl.face = 0 then pl.frm = 10 + p_n
    if pl.face = 1 then pl.frm = 34 + p_n
    if pl.face = 2 then pl.frm = 52 + ((gm.frm \ 2) mod 6)
    if pl.face = 3 then pl.frm = 58 + (gm.frm mod 3)
    exit sub
  end if
  if pl.jetting then
    p_n = (gm.frm \ 2) mod 3
    if pl.hface = 1 then
      if pl.vx <> 0 then
        pl.frm = 31 + p_n
      else
        pl.frm = 28 + p_n
      end if
    else
      if pl.vx <> 0 then
        pl.frm = 7 + p_n
      else
        pl.frm = 4 + p_n
      end if
    end if
    exit sub
  end if
  if pl.climbing then
    pl.frm = 48 + ((pl.y \ 200) mod 4)
    exit sub
  end if
  if pl.ground = 0 then
    if pl.hface = 1 then
      if abs(pl.vx) > 20 then
        pl.frm = 40
      else
        pl.frm = 41
      end if
    else
      if abs(pl.vx) > 20 then
        pl.frm = 16
      else
        pl.frm = 17
      end if
    end if
    exit sub
  end if
  if pl.hface = 1 then
    pl.frm = 24 + ((pl.x0 \ 64) mod 4)
  else
    pl.frm = (pl.x0 \ 64) mod 4
  end if
end sub

sub lv.doorStep()
  if pl.dead then
    exit sub
  end if
  if pl.gems > 0 then
    exit sub
  end if
  lv.doorT = lv.doorT + 1
  if lv.doorT < 38 then
    exit sub
  end if
  if ((lv.doorT - 38) mod 3) = 0 and lv.doorF < 7 then lv.doorF = lv.doorF + 1
  if lv.doorF < 7 then
    exit sub
  end if
  p_c = (pl.x - 192) \ 384
  p_r = (pl.y - 192) \ 384
  if p_c >= lv.dx - 1 and p_c <= lv.dx and p_r >= lv.dy - 1 and p_r <= lv.dy then gm.res = GE_NEXT
end sub

' ---------- enemies ----------

function en.blocked(x, y)
  en.blocked = 1
  if x < 384 or y < 384 then
    exit function
  end if
  p_c0 = (x - 384) \ 384
  p_c1 = (x - 1) \ 384
  p_r0 = (y - 384) \ 384
  p_r1 = (y - 1) \ 384
  if p_c1 > 25 or p_r1 > 15 then
    exit function
  end if
  if p_c0 < 0 then p_c0 = 0
  if p_r0 < 0 then p_r0 = 0
  for p_r = p_r0 to p_r1
    for p_c = p_c0 to p_c1
      p_id = lv.id(p_c, p_r)
      if p_id < 128 then
        if p_id >= 40 then
          exit function
        end if
      end if
    next p_c
  next p_r
  en.blocked = 0
end function

sub en.bounceX(i)
  en.x(i) = en.x(i) - en.vx(i)
  en.vx(i) = -en.vx(i)
end sub

sub en.land(i)
  en.y(i) = en.y(i) - en.vy(i)
  en.vy(i) = 0
end sub

sub en.move(i, solid)
  en.x(i) = en.x(i) + en.vx(i)
  if solid then
    p_hit = bod.hit(en.x(i), en.y(i))
    if p_hit then en.bounceX(i)
  end if
  en.y(i) = en.y(i) + en.vy(i)
  if solid then
    p_hit = bod.hit(en.x(i), en.y(i))
    if p_hit then en.land(i)
  end if
end sub

function en.tid(xu, yu)
  en.tid = 255
  if xu < 0 or yu < 0 then
    exit function
  end if
  p_c = xu \ 384
  p_r = yu \ 384
  if p_c > 25 or p_r > 15 then
    exit function
  end if
  p_id = lv.id(p_c, p_r)
  if p_id >= 128 then p_id = 0
  en.tid = p_id
end function

function en.sar5(v)
  if v >= 0 then
    en.sar5 = v \ 32
    exit function
  end if
  en.sar5 = -((-v + 31) \ 32)
end function

sub en.probes(i)
  p_x0 = en.x(i) - 384
  p_y0 = en.y(i) - 384
  en.pr(0) = en.tid(p_x0 + 192, p_y0 + 192)
  en.pr(1) = en.tid(p_x0 + 64, p_y0 - 32)
  en.pr(2) = en.tid(p_x0 + 288, p_y0 - 32)
  en.pr(3) = en.tid(p_x0 + 384, p_y0)
  en.pr(4) = en.tid(p_x0 + 384, p_y0 + 352)
  en.pr(5) = en.tid(p_x0 + 288, p_y0 + 384)
  en.pr(6) = en.tid(p_x0 + 192, p_y0 + 352)
  en.pr(7) = en.tid(p_x0 + 64, p_y0 + 384)
  en.pr(8) = en.tid(p_x0 - 32, p_y0 + 352)
  en.pr(9) = en.tid(p_x0 - 32, p_y0)
  en.pr(10) = en.tid(p_x0 + 384, p_y0 + 384)
  en.pr(11) = en.tid(p_x0 - 32, p_y0 + 384)
end sub

' Trackbot element pass, after the director and before the integrator.
' EXE file 0x14C6B escalators, 0x14F58 conveyors, 0x15033 clamps,
' 0x150C7 floor switches, 0x15126 teleport. Steel balls share the
' pad table and the 0x17577 epilogue (velocity cleared).
sub en.belt(i)
  p_a = 0
  p_d = 0
  if en.pr(5) >= 114 and en.pr(5) <= 116 then p_a = 1
  if en.pr(7) >= 114 and en.pr(7) <= 116 then p_d = 1
  if p_a = 1 and p_d = 1 then
    if en.vx(i) < 0 then
      en.vx(i) = -55
    else
      if en.vx(i) > 0 then
        en.vx(i) = 20
      else
        en.vx(i) = -40
      end if
    end if
  end if
  p_a = 0
  p_d = 0
  if en.pr(5) >= 117 and en.pr(5) <= 119 then p_a = 1
  if en.pr(7) >= 117 and en.pr(7) <= 119 then p_d = 1
  if p_a = 1 and p_d = 1 then
    if en.vx(i) > 0 then
      en.vx(i) = 55
    else
      if en.vx(i) < 0 then
        en.vx(i) = -20
      else
        en.vx(i) = 40
      end if
    end if
  end if
end sub

sub en.cap(i)
  if en.vy(i) < -32 then
    p_h = en.tid(en.x(i) - 192, en.y(i) - 64)
    if p_h >= 1 and p_h <= 3 then
      p_id = en.tid(en.x(i) - 192, en.y(i) - 448)
      if p_id >= 40 then en.vy(i) = -32
    else
      en.vy(i) = -32
    end if
  end if
  if en.vy(i) > 32 then
    p_h = en.tid(en.x(i) - 192, en.y(i) + 32)
    if p_h >= 40 then
      en.vy(i) = 32
    else
      if en.pr(0) >= 1 and en.pr(0) <= 3 then
        p_a = 0
      else
        if p_h >= 1 and p_h <= 3 then en.vy(i) = 32
      end if
    end if
  end if
end sub

' Floor switch 0x21-0x23. Latch is bit 0x10 of the flags byte.
' Alignment is ((y >> 5) - 1) mod 12, except the homer, which
' does not decrement first (file 0x173CC).
'   type 3 spring: remainder < 4 (0x159C5)
'   type 5 spikes: remainder > 6 (0x16492)
'   type 8 homer: (y >> 5) mod 12 > 6 (0x173D2)
'   types 1, 2, 4, 6: remainder > 10
' Type 7 has no floor-switch test. Leaving the window clears
' the latch and keeps the cooldown nibble (and al, 0x0F).
sub en.sw(i)
  p_k = en.ty(i)
  if p_k = 7 then
    exit sub
  end if
  p_id = en.tid(en.x(i) - 192, en.y(i) - 192)
  p_f = 0
  if p_id >= 33 and p_id <= 35 then
    if p_k = 8 then
      p_n = en.sar5(en.y(i)) mod 12
      if p_n < 0 then p_n = p_n + 12
      if p_n > 6 then p_f = 1
    else
      p_n = en.sar5(en.y(i)) - 1
      p_n = p_n mod 12
      if p_n < 0 then p_n = p_n + 12
      if p_k = 3 then
        if p_n < 4 then p_f = 1
      else
        if p_k = 5 then
          if p_n > 6 then p_f = 1
        else
          if p_n > 10 then p_f = 1
        end if
      end if
    end if
  end if
  if p_f = 1 then
    if (en.cd(i) and 16) = 0 then
      if p_id = 33 then lv.flip(17)
      if p_id = 34 then lv.flip(18)
      if p_id = 35 then lv.flip(19)
      en.cd(i) = en.cd(i) or 16
    end if
    exit sub
  end if
  en.cd(i) = en.cd(i) and 15
end sub

sub en.pad(i)
  p_id = en.pr(0)
  p_a = 0
  for p_r = 0 to 15
    for p_c = 0 to 25
      p_h = lv.id(p_c, p_r)
      if p_h < 128 then
        if p_h = p_id then p_a = p_a + 1
      end if
    next p_c
  next p_r
  if p_a < 1 then
    exit sub
  end if
  p_c2 = 0
  p_r2 = 0
  p_qx = en.sar5(en.x(i) + 192) \ 12
  p_qy = en.sar5(en.y(i) + 352) \ 12
  p_n = 0
  do
    en.rf = rnd
    p_w = int(en.rf * p_a)
    if p_w < 0 then p_w = 0
    if p_w >= p_a then p_w = p_a - 1
    p_b = 0
    for p_r = 0 to 15
      for p_c = 0 to 25
        p_h = lv.id(p_c, p_r)
        if p_h < 128 then
          if p_h = p_id then
            if p_b = p_w then
              p_c2 = p_c
              p_r2 = p_r
            end if
            p_b = p_b + 1
          end if
        end if
      next p_c
    next p_r
    p_n = p_n + 1
    if (p_c2 + 1) <> p_qx or (p_r2 + 1) <> p_qy then p_n = 10
  loop until p_n >= 10
  en.x(i) = (p_c2 + 1) * 384
  en.y(i) = (p_r2 + 1) * 384
  en.vx(i) = 0
  en.vy(i) = 0
  ' EXE 0x151A5: keep the high nibble, set the cooldown nibble to 1.
  en.cd(i) = (en.cd(i) and 240) or 1
end sub

sub en.gate(i, wide)
  p_id = en.pr(0)
  if p_id < 6 or p_id > 8 then
    en.cd(i) = en.cd(i) and 240
    exit sub
  end if
  p_h = en.cd(i) and 15
  if p_h <> 0 then
    ' EXE 0x151EF increments the whole flags byte. A nibble of 15
    ' stores 0 (0x151E4), which also clears the latch bit.
    if p_h = 15 then
      en.cd(i) = 0
    else
      en.cd(i) = en.cd(i) + 1
    end if
    exit sub
  end if
  p_n = en.sar5(en.x(i)) mod 12
  if p_n < 0 then p_n = p_n + 12
  p_d = en.sar5(en.y(i)) mod 12
  if p_d < 0 then p_d = p_d + 12
  p_a = 0
  if wide = 0 then
    if p_n < 2 and p_d < 2 then p_a = 1
  else
    if p_d < 2 then
      if p_n < 2 or p_n >= 11 then p_a = 1
    end if
  end if
  if p_a = 0 then
    exit sub
  end if
  if wide = 0 then en.st(i) = 0
  en.pad(i)
end sub

sub en.track(i)
  en.probes(i)
  p_n = (en.x(i) \ 32) mod 12
  if p_n < 0 then p_n = p_n + 12
  p_b = 0
  if en.pr(8) < 40 and en.pr(9) < 40 then p_b = p_b or 1
  if en.pr(3) < 40 and en.pr(4) < 40 then p_b = p_b or 2
  if en.pr(1) < 40 and en.pr(2) < 40 and p_n < 2 then
    if en.pr(6) >= 1 and en.pr(6) <= 3 then p_b = p_b or 4
  end if
  if en.pr(5) < 40 and en.pr(7) < 40 and p_n < 2 then
    p_d = 0
    if en.pr(0) >= 1 and en.pr(0) <= 3 then p_d = 1
    if en.pr(5) >= 1 and en.pr(5) <= 3 and en.pr(7) >= 1 and en.pr(7) <= 3 then p_d = 1
    if p_d = 1 then p_b = p_b or 8
  end if
  p_c2 = en.sar5(en.x(i) - pl.x)
  p_r2 = en.sar5(en.y(i) - pl.y)
  p_k = en.st(i)
  p_h = 0
  if p_k = 4 and (p_b and 4) <> 0 then p_h = 1
  if p_k = 8 and (p_b and 8) <> 0 then p_h = 1
  if p_h = 1 then
    p_a = 0
    if p_r2 > 0 and p_k = 8 then p_a = 1
    if p_r2 < 0 and p_k = 4 then p_a = 1
    p_f = 0
    if p_a = 0 then
      en.rf = rnd
      if int(en.rf * 3) <> 0 then p_f = 1
    end if
    if p_f = 0 then
      p_d = 0
      if p_c2 > 0 then p_d = 1
      if p_d = 0 then
        en.rf = rnd
        if int(en.rf * 20) = 0 then p_d = 1
      end if
      if p_d = 1 and (p_b and 1) <> 0 then
        if en.pr(11) >= 40 or (en.pr(11) >= 1 and en.pr(11) <= 3) then
          p_k = 1
          p_f = 1
        end if
      end if
      if p_f = 0 then
        p_d = 0
        if p_c2 < 0 then p_d = 1
        if p_d = 0 then
          en.rf = rnd
        if int(en.rf * 20) = 0 then p_d = 1
        end if
        if p_d = 1 and (p_b and 2) <> 0 then
          if en.pr(10) >= 40 or (en.pr(10) >= 1 and en.pr(10) <= 3) then p_k = 2
        end if
      end if
    end if
  else
    p_h = 0
    if p_k = 1 and (p_b and 1) <> 0 then p_h = 1
    if p_k = 2 and (p_b and 2) <> 0 then p_h = 1
    if p_h = 1 then
      p_a = 0
      if p_c2 > 0 and p_k = 2 then p_a = 1
      if p_c2 < 0 and p_k = 1 then p_a = 1
      p_f = 0
      if p_a = 0 then
        en.rf = rnd
      if int(en.rf * 3) <> 0 then p_f = 1
      end if
      if p_f = 0 then
        p_d = 0
        if p_r2 > 0 then p_d = 1
        if p_d = 0 then
          en.rf = rnd
        if int(en.rf * 20) = 0 then p_d = 1
        end if
        if p_d = 1 and (p_b and 4) <> 0 then
          p_k = 4
          p_f = 1
        end if
        if p_f = 0 then
          p_d = 0
          if p_r2 < 0 then p_d = 1
          if p_d = 0 then
            en.rf = rnd
        if int(en.rf * 20) = 0 then p_d = 1
          end if
          if p_d = 1 and (p_b and 8) <> 0 then p_k = 8
        end if
      end if
    else
      p_bit = p_k and 12
      p_col = p_k and 3
      if p_bit = 0 and p_r2 > 0 and (p_b and 4) <> 0 then
        p_k = 4
      else
        if p_bit = 0 and p_r2 < 0 and (p_b and 8) <> 0 then
          p_k = 8
        else
          if p_col = 0 and p_c2 > 0 and (p_b and 1) <> 0 then
            p_k = 1
          else
            if p_col = 0 and p_c2 < 0 and (p_b and 2) <> 0 then
              p_k = 2
            else
              if p_bit = 0 and (p_b and 4) <> 0 then
                p_k = 4
              else
                if p_bit = 0 and (p_b and 8) <> 0 then
                  p_k = 8
                else
                  if p_col = 0 and (p_b and 1) <> 0 then
                    p_k = 1
                  else
                    if p_col = 0 and (p_b and 2) <> 0 then
                      p_k = 2
                    else
                      p_k = 0
                    end if
                  end if
                end if
              end if
            end if
          end if
        end if
      end if
    end if
  end if
  en.st(i) = p_k
  if en.pr(0) >= 40 then
    en.vx(i) = 0
    en.vy(i) = 0
  else
    p_w = 0
    if en.pr(0) >= 1 and en.pr(0) <= 3 then p_w = 1
    if en.pr(5) >= 40 or (en.pr(5) >= 1 and en.pr(5) <= 3) then p_w = 1
    if en.pr(7) >= 40 or (en.pr(7) >= 1 and en.pr(7) <= 3) then p_w = 1
    if p_w = 0 then
      en.st(i) = 0
      en.vx(i) = 0
      en.vy(i) = 64
    else
      en.vx(i) = 0
      en.vy(i) = 0
      if p_k = 1 then en.vx(i) = -43
      if p_k = 2 then en.vx(i) = 43
      if p_k = 4 then en.vy(i) = -33
      if p_k = 8 then en.vy(i) = 33
      if p_k = 4 or p_k = 8 then
        if p_n >= 6 then en.vx(i) = 32
        if p_n >= 1 and p_n < 6 then en.vx(i) = -32
      end if
      if en.vy(i) <> 0 then
        p_d = 0
        if en.pr(0) = 2 or en.pr(5) = 2 or en.pr(7) = 2 then p_d = 1
        if p_d = 1 and (p_b and 4) <> 0 then
          if en.vy(i) < 0 then
            en.vy(i) = -61
          else
            en.vy(i) = 16
          end if
        end if
        p_d = 0
        if en.pr(0) = 3 or en.pr(5) = 3 or en.pr(7) = 3 then p_d = 1
        if p_d = 1 and (p_b and 8) <> 0 then
          if en.vy(i) > 0 then
            en.vy(i) = 61
          else
            en.vy(i) = -16
          end if
        end if
      end if
    end if
  end if
  if en.vy(i) <> 0 and en.vy(i) <> 64 then
    en.fr(i) = 10 + ((en.y(i) \ 64) and 1)
  else
    if en.vx(i) <> 0 then
      en.fr(i) = 4 + ((en.x(i) \ 64) mod 3)
      if en.vx(i) < 0 then en.fr(i) = en.fr(i) + 3
    else
      en.fr(i) = 4
    end if
  end if
  ' Moss 0x5E-0x60, both feet (0x14ECC): frame only.
  if en.pr(5) >= 94 and en.pr(5) <= 96 then
    if en.pr(7) >= 94 and en.pr(7) <= 96 then
      if en.vx(i) >= 0 then
        en.fr(i) = 4
      else
        en.fr(i) = 7
      end if
    end if
  end if
  ' Ice 0x61-0x63, both feet. Qword at ds:0x15E4 is 0.6.
  if en.pr(5) >= 97 and en.pr(5) <= 99 then
    if en.pr(7) >= 97 and en.pr(7) <= 99 then
      en.vx(i) = sc.ice(en.vx(i))
    end if
  end if
  en.belt(i)
  en.cap(i)
  en.sw(i)
  en.gate(i, 0)
  if en.pr(0) >= 40 then
    en.vx(i) = 0
    en.vy(i) = 0
    en.fr(i) = 10
  end if
  en.x(i) = en.x(i) + en.vx(i)
  en.y(i) = en.y(i) + en.vy(i)
end sub

' Tile clamp. A full-step revert leaves a 64-unit fall hovering
' above the floor, so the step stops on the tile edge instead.
sub en.slide(i)
  en.x(i) = en.x(i) + en.vx(i)
  if en.x(i) < 384 then
    en.x(i) = 384
    en.vx(i) = 0
  end if
  if en.x(i) > 9984 then
    en.x(i) = 9984
    en.vx(i) = 0
  end if
  p_c = (en.x(i) - 192) \ 384
  p_r = (en.y(i) - 192) \ 384
  if fld.solid(p_c, p_r) then
    if en.vx(i) >= 0 then
      en.x(i) = p_c * 384 + 191
    else
      en.x(i) = (p_c + 1) * 384 + 192
    end if
    en.vx(i) = 0
  end if
  en.y(i) = en.y(i) + en.vy(i)
  if en.y(i) < 384 then
    en.y(i) = 384
    en.vy(i) = 0
  end if
  if en.y(i) > 6144 then
    en.y(i) = 6144
    en.vy(i) = 0
  end if
  p_c = (en.x(i) - 192) \ 384
  if en.vy(i) > 0 then
    p_r = en.y(i) \ 384
    if fld.solid(p_c, p_r) then
      en.y(i) = p_r * 384
      en.vy(i) = 0
    end if
  end if
  if en.vy(i) < 0 then
    p_r = (en.y(i) - 384) \ 384
    if fld.solid(p_c, p_r) then
      en.y(i) = (p_r + 2) * 384
      en.vy(i) = 0
    end if
  end if
end sub

' Steel ball, file 0x1527A. Probes are its own, from the sprite
' top-left: (6,11) pad, (12,11) right, (1,12) and (10,12) support,
' (-1,11) foot. Support empty means fall at 64. The direction byte
' starts at 0 (0xE4BE), not forced right.
sub en.ball(i)
  p_x0 = en.x(i) - 384
  p_y0 = en.y(i) - 384
  en.pr(0) = en.tid(p_x0 + 192, p_y0 + 352)
  p_a = en.tid(p_x0 + 384, p_y0 + 352)
  p_d = en.tid(p_x0 + 32, p_y0 + 384)
  p_h = en.tid(p_x0 + 320, p_y0 + 384)
  p_f = en.tid(p_x0 - 32, p_y0 + 352)
  p_b = 0
  if p_d < 40 and (p_d < 1 or p_d > 3) then
    if p_h < 40 and (p_h < 1 or p_h > 3) then p_b = 1
  end if
  if p_b = 1 then
    en.vx(i) = 0
    en.vy(i) = 64
    en.fr(i) = 3
  else
    ' Direction advances only while supported (file 0x1527A).
    en.dir(i) = en.dir(i) + 1
    if en.dir(i) >= 256 then en.dir(i) = 0
    p_n = en.dir(i) mod 64
    if p_n = 63 then
      if en.dir(i) < 64 then
        en.dir(i) = 0
      else
        en.dir(i) = 64
      end if
    end if
    if p_f >= 40 then en.dir(i) = 64
    if p_f < 40 and p_a >= 40 then en.dir(i) = 0
    if en.dir(i) < 64 and p_f < 40 then
      en.vx(i) = -59
    else
      if p_a < 40 then
        en.vx(i) = 59
      else
        en.vx(i) = 0
      end if
    end if
    en.vy(i) = 0
    p_n = (en.dir(i) \ 2) mod 3
    if p_n < 0 then p_n = p_n + 3
    if en.dir(i) < 64 then
      en.fr(i) = 2 - p_n
    else
      en.fr(i) = p_n
    end if
  end if
  if p_d >= 97 and p_d <= 99 then en.vx(i) = sc.ice(en.vx(i))
  if p_d >= 114 and p_d <= 116 then
    if en.vx(i) < 0 then
      en.vx(i) = -62
    else
      if en.vx(i) > 0 then
        en.vx(i) = 27
      else
        en.vx(i) = -40
      end if
    end if
  end if
  if p_d >= 117 and p_d <= 119 then
    if en.vx(i) > 0 then
      en.vx(i) = 62
    else
      if en.vx(i) < 0 then
        en.vx(i) = -27
      else
        en.vx(i) = 40
      end if
    end if
  end if
  en.sw(i)
  en.gate(i, 1)
  ' Probes already decided vx. Do not also reject the centre cell:
  ' the belt carries the ball along under a wall the centre overlaps.
  en.x(i) = en.x(i) + en.vx(i)
  if en.x(i) < 384 then en.x(i) = 384
  if en.x(i) > 9984 then en.x(i) = 9984
  en.y(i) = en.y(i) + en.vy(i)
  if en.y(i) < 384 then
    en.y(i) = 384
    en.vy(i) = 0
  end if
  if en.y(i) > 6144 then
    en.y(i) = 6144
    en.vy(i) = 0
  end if
  if en.vy(i) > 0 then
    p_c = (en.x(i) - 192) \ 384
    p_r = en.y(i) \ 384
    if fld.solid(p_c, p_r) then
      en.y(i) = p_r * 384
      en.vy(i) = 0
    end if
  end if
end sub

' Spring, file 0x157CA. dir 0 falls at +50, dir 1 rises at -50.
' Hitting the floor sets 10 and counts to 14, then 1. Hitting the
' ceiling sets 20 and counts to 24, then 0. Other values hold still.
sub en.spring(i)
  p_x0 = en.x(i) - 384
  p_y0 = en.y(i) - 384
  en.pr(0) = en.tid(p_x0 + 192, p_y0 + 128)
  p_a = en.tid(p_x0 + 192, p_y0 - 32)
  p_id = en.tid(p_x0 + 192, p_y0 + 256)
  p_d = en.dir(i) \ 10
  if p_d >= 1 then
    p_n = en.dir(i) mod 10
    if p_n = 4 then
      en.dir(i) = 2 - p_d
    else
      en.dir(i) = en.dir(i) + 1
    end if
    en.vx(i) = 0
    en.vy(i) = 0
  else
    en.vx(i) = 0
    if en.dir(i) = 0 then
      if p_id >= 40 then
        en.dir(i) = 10
        en.fr(i) = 12
        en.vy(i) = 0
      else
        en.vy(i) = 50
        en.fr(i) = 13 + ((en.y(i) \ 200) and 1)
      end if
    end if
    if en.dir(i) = 1 then
      if p_a >= 40 then
        en.dir(i) = 20
        en.fr(i) = 15
        en.vy(i) = 0
      else
        en.vy(i) = -50
        en.fr(i) = 13 + ((en.y(i) \ 200) and 1)
      end if
    end if
  end if
  en.sw(i)
  en.gate(i, 0)
  en.slide(i)
end sub

sub en.think(i)
  p_k = en.ty(i)
  if p_k = 1 then
    en.track(i)
    p_k = 1
  end if
  if p_k = 2 then en.ball(i)
  if p_k = 3 then en.spring(i)
  if p_k = 4 then
    if en.st(i) = 0 then
      en.vx(i) = 0
      en.vy(i) = -64
    end if
    if en.st(i) = 1 then
      en.vx(i) = 64
      en.vy(i) = 0
    end if
    if en.st(i) = 2 then
      en.vx(i) = 0
      en.vy(i) = 64
    end if
    if en.st(i) = 3 then
      en.vx(i) = -64
      en.vy(i) = 0
    end if
    p_x0 = en.x(i)
    p_y0 = en.y(i)
    p_c2 = en.vx(i)
    p_r2 = en.vy(i)
    en.sw(i)
    en.move(i, TRUE)
    if p_c2 <> 0 and en.x(i) = p_x0 then en.st(i) = (en.st(i) + 1) mod 4
    if p_r2 <> 0 and en.y(i) = p_y0 then en.st(i) = (en.st(i) + 1) mod 4
    en.fr(i) = 21 + en.st(i) * 3 + ((gm.frm \ 2) mod 3)
  end if
  if p_k = 5 then
    if en.st(i) = 0 then
      en.rf = rnd
      en.st(i) = int(en.rf * 4) + 1
      en.vx(i) = 20
      en.vy(i) = 20
      if en.st(i) = 1 or en.st(i) = 4 then en.vx(i) = -20
      if en.st(i) = 1 or en.st(i) = 2 then en.vy(i) = -20
    end if
    if en.vx(i) < 0 then
      en.vx(i) = -20
    else
      en.vx(i) = 20
    end if
    if en.vy(i) < 0 then
      en.vy(i) = -20
    else
      en.vy(i) = 20
    end if
    en.sw(i)
    en.x(i) = en.x(i) + en.vx(i)
    p_hit = en.blocked(en.x(i), en.y(i))
    if p_hit then
      en.x(i) = en.x(i) - en.vx(i)
      en.vx(i) = -en.vx(i)
    end if
    en.y(i) = en.y(i) + en.vy(i)
    p_hit = en.blocked(en.x(i), en.y(i))
    if p_hit then
      en.y(i) = en.y(i) - en.vy(i)
      en.vy(i) = -en.vy(i)
    end if
    en.fr(i) = 16 + ((en.x(i) \ 32) mod 3)
  end if
  if p_k = 6 then
    if en.dir(i) < 1 or en.dir(i) > 8 then en.dir(i) = 1
    if (gm.frm mod 8) = 0 then
      en.rf = rnd
      if int(en.rf * 10) = 0 then
        en.dir(i) = 1
      else
        en.rf = rnd
        if int(en.rf * 2) = 0 then
          en.dir(i) = en.dir(i) - 1
        else
          en.dir(i) = en.dir(i) + 1
        end if
        if en.dir(i) < 1 then en.dir(i) = 8
        if en.dir(i) > 8 then en.dir(i) = 1
      end if
    end if
    p_d = en.dir(i)
    en.vx(i) = 0
    en.vy(i) = 0
    if p_d = 1 or p_d = 2 or p_d = 8 then en.vx(i) = 11
    if p_d = 4 or p_d = 5 or p_d = 6 then en.vx(i) = -11
    if p_d = 2 or p_d = 3 or p_d = 4 then en.vy(i) = -11
    if p_d = 6 or p_d = 7 or p_d = 8 then en.vy(i) = 11
    en.sw(i)
    en.move(i, TRUE)
    en.fr(i) = 33 + ((p_d - 1) mod 8)
  end if
  if p_k = 7 then
    if abs(pl.x - en.x(i)) < 80 then
      en.vx(i) = 0
    else
      if pl.x > en.x(i) then
        en.vx(i) = 4
      else
        en.vx(i) = -4
      end if
    end if
    if abs(pl.y - en.y(i)) < 80 then
      en.vy(i) = 0
    else
      if pl.y > en.y(i) then
        en.vy(i) = 4
      else
        en.vy(i) = -4
      end if
    end if
    if en.vx(i) <> 0 and en.vy(i) <> 0 then
      if abs(pl.x - en.x(i)) < abs(pl.y - en.y(i)) then
        en.vx(i) = 0
      else
        en.vy(i) = 0
      end if
    end if
    en.move(i, FALSE)
    en.fr(i) = 19 + ((gm.frm \ 8) and 1)
  end if
  if p_k = 8 then
    p_c = (en.x(i) - 192) \ 384
    p_r = en.y(i) \ 384
    p_hit = fld.solid(p_c, p_r)
    if en.st(i) = 1 then
      en.vy(i) = en.vy(i) + 5
      if en.vy(i) > 17 then en.vy(i) = 17
      en.vx(i) = 0
    else
      en.vy(i) = 0
      if pl.x >= en.x(i) then
        en.vx(i) = 10
      else
        en.vx(i) = -10
      end if
      if ((gm.frm + i * 3) mod 48) = 0 then
        en.vy(i) = -50
        en.st(i) = 1
      end if
    end if
    en.sw(i)
    en.move(i, TRUE)
    if p_hit and en.vy(i) >= 0 then en.st(i) = 0
    if en.st(i) = 1 and en.vy(i) < 0 then
      en.fr(i) = 42
    else
      en.fr(i) = 42 + ((gm.frm \ 4) mod 5)
    end if
  end if
  if en.fr(i) < 0 then en.fr(i) = 0
  if en.fr(i) > 47 then en.fr(i) = 47
  if pl.dead = 0 then
    if abs(en.x(i) - pl.x) < 280 and abs(en.y(i) - pl.y) < 280 then pl.die()
  end if
end sub

sub en.step()
  for p_i = 0 to en.n - 1
    en.think(p_i)
  next p_i
  if sp.on and pl.dead = 0 then
    if abs(sp.x - pl.x) < 300 and abs(sp.y - pl.y) < 300 then pl.die()
  end if
end sub

' ---------- draw ----------

sub hud.draw()
  local gx, gy, gw, gh, bev, well, fill, row, c0, c1, c2
  if lay.ox < 20 then
    exit sub
  end if
  box 0, 0, lay.ox, scr.H, 1, rgb(16, 12, 14), rgb(16, 12, 14)
  bev = lay.fsc
  gx = 2 * bev
  gw = lay.ox - gx * 2
  if gw > 91 * lay.fsc then gw = 91 * lay.fsc
  gh = bev * 2 + 3 * lay.fsc
  gy = 4
  if lay.ox >= 4 * 8 * lay.fsc + gx then
    font 1, lay.fsc
    text gx, gy, "FUEL", rgb(73, 251, 203)
    font 1, 1
    gy = gy + 16 * lay.fsc
  end if
  box gx, gy, gw, gh, 1, rgb(96, 100, 108), rgb(96, 100, 108)
  box gx, gy, gw, bev, 1, rgb(220, 224, 232), rgb(220, 224, 232)
  box gx, gy + gh - bev, gw, bev, 1, rgb(32, 34, 40), rgb(32, 34, 40)
  box gx, gy, bev, gh, 1, rgb(220, 224, 232), rgb(220, 224, 232)
  box gx + gw - bev, gy, bev, gh, 1, rgb(32, 34, 40), rgb(32, 34, 40)
  well = gw - bev * 2
  row = 3 * lay.fsc
  box gx + bev, gy + bev, well, row, 1, rgb(0, 0, 0), rgb(0, 0, 0)
  c0 = rgb(170, 0, 0)
  c1 = rgb(199, 0, 0)
  c2 = rgb(227, 0, 0)
  if gm.fuel >= 1500 then
    c0 = rgb(170, 158, 0)
    c1 = rgb(199, 186, 0)
    c2 = rgb(227, 219, 0)
  end if
  if gm.fuel >= 3000 then
    c0 = rgb(0, 142, 0)
    c1 = rgb(0, 170, 0)
    c2 = rgb(0, 199, 0)
  end if
  fill = well * gm.fuel \ FUEL_MAX
  if fill > well then fill = well
  if fill > 0 then
    box gx + bev, gy + bev, fill, lay.fsc, 1, c0, c0
    box gx + bev, gy + bev + lay.fsc, fill, lay.fsc, 1, c1, c1
    box gx + bev, gy + bev + lay.fsc * 2, fill, lay.fsc, 1, c2, c2
  end if
  ui.text(3, scr.H - 20 * lay.fsc, num$(pl.gems), 2)
  ui.text(3, scr.H - 10 * lay.fsc, num$(gm.lives), 2)
  scr.mark(0, 0, lay.ox, scr.H)
end sub

sub spr.reset()
  for p_i = 1 to 23
    if spr.on(p_i) = 1 then sprite hide p_i
    spr.on(p_i) = 0
    spr.fr(p_i) = -1
    spr.kind(p_i) = -1
  next p_i
end sub

sub spr.hide(n)
  if spr.on(n) = 1 then sprite hide n
  spr.on(n) = 0
end sub

sub spr.put(n, fr, kind, x, y, w, h, layer)
  ' Same pose and rectangle, and the back page was not rebuilt: the
  ' pixels from last time are still there. A tile flush sets gm.force.
  if gm.force = 0 then
    if spr.on(n) = 1 then
      if spr.fr(n) = fr then
        if spr.kind(n) = kind then
          if spr.x(n) = x then
            if spr.y(n) = y then
              if spr.w(n) = w then
                if spr.h(n) = h then
                  exit sub
                end if
              end if
            end if
          end if
        end if
      end if
    end if
  end if
  if x < -w or y < -h or x > scr.W or y > scr.H then
    spr.hide(n)
    exit sub
  end if
  if spr.fr(n) <> fr or spr.kind(n) <> kind then
    spr.hide(n)
    if kind = 0 then
      sx0 = (fr mod 24) * lay.tile
      sy0 = lay.syPlayer + (fr \ 24) * lay.tile
    end if
    if kind = 1 then
      sx0 = (fr mod 24) * lay.tile
      sy0 = lay.syEnemy + (fr \ 24) * lay.tile
    end if
    if kind = 2 then
      sx0 = fr * lay.tile * 2
      sy0 = lay.syDoor
    end if
    sprite read n, sx0, sy0, w, h, PG_SHEET
    spr.fr(n) = fr
    spr.kind(n) = kind
  end if
  if gm.force then spr.on(n) = 0
  if spr.on(n) = 0 then
    sprite show n, x, y, layer
    spr.on(n) = 1
  else
    if x <> spr.x(n) or y <> spr.y(n) then
      sprite next n, x, y
      gm.moved = TRUE
    end if
  end if
  if spr.on(n) = 1 then
    scr.mark(spr.x(n), spr.y(n), spr.w(n), spr.h(n))
    scr.mark(x, y, w, h)
  end if
  spr.x(n) = x
  spr.y(n) = y
  spr.w(n) = w
  spr.h(n) = h
end sub

sub gm.sprites()
  local x, y, i
  x = lay.ox + (pl.x - 384) * lay.tile \ 384
  y = lay.oy + (pl.y - 384) * lay.tile \ 384
  spr.put(1, pl.frm, 0, x, y, lay.tile, lay.tile, 3)
  if en.n < gm.enHid then
    for i = en.n to gm.enHid - 1
      spr.hide(2 + i)
    next i
  end if
  gm.enHid = en.n
  for i = 0 to en.n - 1
    x = lay.ox + (en.x(i) - 384) * lay.tile \ 384
    y = lay.oy + (en.y(i) - 384) * lay.tile \ 384
    spr.put(2 + i, en.fr(i), 1, x, y, lay.tile, lay.tile, 2)
  next i
  x = lay.ox + (lv.dx - 1) * lay.tile
  y = lay.oy + (lv.dy - 1) * lay.tile
  spr.put(22, lv.doorF, 2, x, y, lay.tile * 2, lay.tile * 2, 1)
  if sp.on then
    x = lay.ox + (sp.x - 384) * lay.tile \ 384
    y = lay.oy + (sp.y - 384) * lay.tile \ 384
    spr.put(23, 19, 1, x, y, lay.tile, lay.tile, 2)
  else
    spr.hide(23)
  end if
end sub

sub gm.step()
  if gm.res <> GE_NONE then
    exit sub
  end if
  gm.frm = gm.frm + 1
  key.logic()
  if pl.shield > 0 and pl.dead = 0 then pl.shield = pl.shield - 1
  if pl.dead then
    pl.age = pl.age + 1
    pl.anim()
    en.step()
    if pl.age >= 48 then gm.res = GE_DEAD
    key.prevJ = key.jet
    key.prevD = key.dn
    gm.vis = TRUE
    exit sub
  end if
  pl.x0 = pl.x
  pl.control()
  if pl.phasing = 0 and pl.lock = 0 then
    pl.moveX()
    pl.moveY()
    pl.tube()
  end if
  pl.pickups()
  pl.floor()
  pl.use()
  if (gm.frm mod 3) = 0 then cv.mark()
  mt.step()
  en.step()
  lv.doorStep()
  pl.anim()
  key.prevJ = key.jet
  key.prevD = key.dn
  gm.vis = TRUE
end sub

sub gm.tick()
  if TH_FAST <> 0 then
    exit sub
  end if
  if gm.drawing or gm.res <> GE_NONE then
    exit sub
  end if
  g_now = timer
  if g_now < gm.due then
    exit sub
  end if
  gm.due = gm.due + FRAME_MS
  if gm.due <= g_now then gm.due = g_now + FRAME_MS
  gm.step()
end sub

sub gm.handlers(on)
  if on then
    key.reset()
    gm.due = timer + FRAME_MS
    on key key.onKey
    settick TICK_MS, gm.tick
  else
    settick 0, gm.tick
    on key 0
  end if
end sub

sub gm.hideAll()
  for p_i = 1 to 23
    spr.hide(p_i)
  next p_i
end sub

sub gm.draw()
  gm.drawing = TRUE
  page write PG_BACK
  if gm.tiles then gm.hideAll()
  if fld.n <> 0 then gm.force = TRUE
  fld.flush()
  if gm.tiles then gm.force = TRUE
  gm.tiles = FALSE
  if hud.dirty then hud.draw()
  hud.dirty = FALSE
  ' Sprites are already on the back page. Redraw only after a logic
  ' tick or a tile rebuild. Quiet presents leave the display alone.
  if gm.vis or gm.force then
    gm.sprites()
    if gm.moved then sprite move
    gm.moved = FALSE
    gm.vis = FALSE
    gm.force = FALSE
  end if
  scr.flush()
  gm.drawing = FALSE
end sub

sub gm.pauseDraw(sel)
  local px, py
  page copy PG_HOLD to PG_BACK
  px = (scr.W - lay.pw) \ 2
  py = (scr.H - lay.ph) \ 2
  if sel = 0 then
    blit lay.p0x, lay.p0y, px, py, lay.pw, lay.ph, PG_UI
  else
    blit lay.p1x, lay.p1y, px, py, lay.pw, lay.ph, PG_UI
  end if
  scr.present()
end sub

function gm.paused()
  local k$, k, sel
  gm.handlers(FALSE)
  gm.hideAll()
  page copy PG_BACK to PG_HOLD
  sel = 0
  key.drain()
  gm.pauseDraw(sel)
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      k = asc(k$)
      if k = K_UP or k = K_LEFT then sel = 0
      if k = K_DOWN or k = K_RIGHT then sel = 1
      if k = 113 or k = 81 then
        gm.paused = TRUE
        exit function
      end if
      if k = K_ESC or k = 112 or k = 80 then
        gm.paused = FALSE
        exit function
      end if
      if k = K_ENTER or k = K_LF then
        if sel = 0 then
          gm.paused = FALSE
        else
          gm.paused = TRUE
        end if
        exit function
      end if
      gm.pauseDraw(sel)
    end if
  loop
end function

sub gm.play()
  gm.res = GE_NONE
  gm.enHid = 0
  gm.vis = FALSE
  gm.drawing = FALSE
  gm.tiles = FALSE
  gm.moved = FALSE
  gm.force = FALSE
  fld.paint()
  hud.dirty = TRUE
  hud.draw()
  gm.sprites()
  scr.present()
  gm.handlers(TRUE)
  do
    if key.pause then
      key.pause = FALSE
      p_hit = gm.paused()
      if p_hit then
        gm.res = GE_TITLE
        exit do
      end if
      page copy PG_HOLD to PG_BACK
      gm.force = TRUE
      gm.sprites()
      scr.present()
      gm.force = FALSE
      gm.handlers(TRUE)
    end if
    if TH_FAST <> 0 then
      gm.step()
      gm.due = timer + FRAME_MS
    end if
    gm.draw()
    if gm.res <> GE_NONE then
      exit do
    end if
    if TH_FAST <> 0 then
      pause 0
    else
      pause IDLE_MS
    end if
  loop
  gm.handlers(FALSE)
  gm.hideAll()
end sub

sub gm.over()
  local rk
  rk = hs.rank(gm.score)
  if rk >= 0 then
    hs.ask()
    hs.insert(rk, hs.entered$, gm.score, gm.level + 1)
    hs.save()
  end if
  hs.show(rk)
end sub

sub gm.run()
  gm.level = menu.level
  if gm.level < 0 then gm.level = 0
  if gm.level > 99 then gm.level = 99
  gm.lives = LIVES0
  if TH_LIVES > 0 then gm.lives = TH_LIVES
  gm.score = 0
  gm.fuel = 0
  gm.lifeAt = 0
  gm.frm = 0
  do
    lv.load(gm.level)
    gm.play()
    if gm.res = GE_TITLE then
      exit sub
    end if
    if gm.res = GE_DEAD then
      gm.lives = gm.lives - 1
      if gm.lives <= 0 then
        gm.over()
        exit sub
      end if
    end if
    if gm.res = GE_NEXT then
      gm.level = gm.level + 1
      if gm.level > 99 then
        exit sub
      end if
    end if
  loop
end sub
