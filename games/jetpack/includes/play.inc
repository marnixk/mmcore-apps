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
dim integer g_now

dim pl.x, pl.y, pl.x0, pl.vx, pl.vy, pl.face, pl.hface, pl.frm
dim pl.ground, pl.dead, pl.age, pl.gems, pl.shield, pl.lock, pl.tx, pl.ty
dim pl.phasing, pl.jetting, pl.climbing, pl.tdir

dim lv.dx, lv.dy, lv.doorT, lv.doorF
dim string lv.name$, lv.line$
dim lv.id(25, 15), lv.ch(25, 15)
dim lv.i, lv.r

dim mt.n, mt.hot
dim mt.x(7), mt.y(7), mt.tm(7), mt.cl(7), mt.hid(7)

dim en.n
dim en.ty(19), en.x(19), en.y(19), en.vx(19), en.vy(19), en.fr(19), en.st(19)

dim sp.on, sp.x, sp.y

dim spr.fr(23), spr.kind(23), spr.on(23)
dim spr.x(23), spr.y(23), spr.w(23), spr.h(23)

dim hud.dirty, fld.mark
dim integer p_c, p_r, p_c0, p_c1, p_r0, p_r1, p_n, p_id, p_k, p_x0, p_y0, p_w
dim integer p_a, p_b, p_d, p_c2, p_r2, p_hit, p_f, p_h, p_col, p_i, p_bit
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
  if ty = 2 then en.vx(en.n) = 59
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
  p_k = tf.k(p_id and 127)
  if p_k = 1 or p_k = 3 or p_k = 20 or p_k = 21 then
    fld.solid = TRUE
  else
    fld.solid = FALSE
  end if
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
    if id >= 128 then id = 0
    blit (id mod 20) * lay.tile, (id \ 20) * lay.tile, dx, dy, lay.tile, lay.tile, PG_SHEET
  end if
  lv.ch(c, r) = 0
  if fld.mark <> 0 then scr.mark(dx, dy, lay.tile, lay.tile)
end sub

sub fld.flush()
  for p_r = 0 to 15
    for p_c = 0 to 25
      if lv.ch(p_c, p_r) then fld.blit(p_c, p_r)
    next p_c
  next p_r
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
  for p_r = 0 to 15
    for p_c = 0 to 25
      lv.ch(p_c, p_r) = 1
    next p_c
  next p_r
  fld.mark = 0
  fld.flush()
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
  lv.ch(mt.x(i), mt.y(i)) = 1
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
  lv.ch(mt.x(i), mt.y(i)) = 1
  gm.tiles = TRUE
end sub

sub mt.back(i)
  lv.id(mt.x(i), mt.y(i)) = lv.id(mt.x(i), mt.y(i)) and 127
  mt.hid(i) = 2
  lv.ch(mt.x(i), mt.y(i)) = 1
  gm.tiles = TRUE
end sub

sub mt.expire(i)
  if mt.cl(i) = 2 and mt.tm(i) >= 5 then
    lv.id(mt.x(i), mt.y(i)) = 0
    mt.hid(i) = 2
    lv.ch(mt.x(i), mt.y(i)) = 1
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
        lv.ch(mt.x(p_i), mt.y(p_i)) = 1
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
      lv.ch(mt.x(p_i), mt.y(p_i)) = 1
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
  p_x0 = pl.x
  p_y0 = pl.y
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
  p_hit = bod.hit(pl.x, p_y0)
  if p_hit then pl.x = p_x0
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
  p_hit = bod.hit(pl.x, pl.y)
  if p_hit then pl.y = p_y0
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

sub pl.walk()
  p_a = 0
  if pl.ground then
    p_c = (pl.x - 192) \ 384
    p_r = pl.y \ 384
    if p_c >= 0 and p_r >= 0 and p_c <= 25 and p_r <= 15 then
      p_id = lv.id(p_c, p_r)
      if p_id < 128 then
        if tf.k(p_id) = 20 then p_a = -60
        if tf.k(p_id) = 21 then p_a = 60
      end if
    end if
  end if
  if key.left and key.right = 0 then
    pl.hface = 1
    pl.face = 1
    if p_a > 0 then
      pl.vx = -25
    else
      pl.vx = -55
    end if
  end if
  if key.right and key.left = 0 then
    pl.hface = 0
    pl.face = 0
    if p_a < 0 then
      pl.vx = 25
    else
      pl.vx = 55
    end if
  end if
  if key.left = 0 and key.right = 0 then pl.vx = p_a
  if pl.ground then
    pl.vy = 0
  else
    pl.vy = pl.vy + 5
    if pl.vy > 64 then pl.vy = 64
    if pl.vx > 20 then pl.vx = 20
    if pl.vx < -20 then pl.vx = -20
  end if
end sub

sub pl.control()
  pl.feet()
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
  if pl.onLadder() and (key.up or key.dn) then
    pl.climbing = TRUE
    pl.vx = 0
    pl.vy = 0
    if key.up then pl.vy = -45
    if key.dn then pl.vy = 45
    if key.left then pl.vx = -45
    if key.right then pl.vx = 45
    exit sub
  end if
  if key.jet and gm.fuel > 0 then
    pl.jet()
    exit sub
  end if
  pl.walk()
end sub

sub pl.moveX()
  p_x0 = pl.x
  pl.x = pl.x + pl.vx
  p_hit = bod.hit(pl.x, pl.y)
  if p_hit = 0 then
    exit sub
  end if
  p_n = 0
  if pl.vx > 0 then
    do while p_n < 24
      p_hit = bod.hit(pl.x, pl.y)
      if p_hit = 0 then
        exit do
      end if
      pl.x = pl.x - 16
      p_n = p_n + 1
    loop
  else
    do while p_n < 24
      p_hit = bod.hit(pl.x, pl.y)
      if p_hit = 0 then
        exit do
      end if
      pl.x = pl.x + 16
      p_n = p_n + 1
    loop
  end if
  if p_n >= 24 then pl.x = p_x0
  pl.vx = 0
end sub

sub pl.moveY()
  p_y0 = pl.y
  pl.y = pl.y + pl.vy
  p_hit = bod.hit(pl.x, pl.y)
  if p_hit = 0 then
    exit sub
  end if
  p_n = 0
  if pl.vy >= 0 then
    do while p_n < 24
      p_hit = bod.hit(pl.x, pl.y)
      if p_hit = 0 then
        exit do
      end if
      pl.y = pl.y - 16
      p_n = p_n + 1
    loop
    if p_n < 24 then pl.ground = TRUE
  else
    do while p_n < 24
      p_hit = bod.hit(pl.x, pl.y)
      if p_hit = 0 then
        exit do
      end if
      pl.y = pl.y + 16
      p_n = p_n + 1
    loop
  end if
  if p_n >= 24 then pl.y = p_y0
  pl.vy = 0
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
  lv.ch(c, r) = 1
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
        lv.ch(p_c, p_r) = 1
      end if
      if lv.id(p_c, p_r) = 200 then
        lv.id(p_c, p_r) = p_b
        lv.ch(p_c, p_r) = 1
      end if
    next p_c
  next p_r
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
          p_hit = bod.hit(pl.x, pl.y)
          if p_hit then
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

sub en.think(i)
  p_k = en.ty(i)
  if p_k = 1 then
    p_c = (en.x(i) - 192) \ 384
    p_r = (en.y(i) - 192) \ 384
    p_a = 0
    if p_c >= 0 and p_r >= 0 and p_c <= 25 and p_r <= 15 then
      p_id = lv.id(p_c, p_r)
      if p_id < 128 then
        if tf.k(p_id) = 2 then p_a = 1
      end if
    end if
    if p_a = 1 and abs(pl.y - en.y(i)) > 200 then
      en.vx(i) = 0
      if pl.y < en.y(i) then
        en.vy(i) = -33
      else
        en.vy(i) = 33
      end if
    else
      en.vy(i) = en.vy(i) + 5
      if en.vy(i) > 64 then en.vy(i) = 64
      if pl.x >= en.x(i) then
        en.vx(i) = 43
      else
        en.vx(i) = -43
      end if
    end if
    en.move(i, TRUE)
    if p_a = 1 then
      en.fr(i) = 10 + ((en.y(i) \ 64) and 1)
    else
      en.fr(i) = 4 + ((en.x(i) \ 64) mod 3)
      if en.vx(i) < 0 then en.fr(i) = en.fr(i) + 3
    end if
  end if
  if p_k = 2 then
    if en.vx(i) = 0 then en.vx(i) = 59
    en.vy(i) = en.vy(i) + 5
    if en.vy(i) > 64 then en.vy(i) = 64
    en.move(i, TRUE)
    if en.st(i) = 0 and en.vy(i) = 0 then en.st(i) = 4
    if en.st(i) > 0 then
      en.fr(i) = 3
      en.st(i) = en.st(i) - 1
    else
      en.fr(i) = 2 - ((gm.frm \ 2) mod 3)
    end if
  end if
  if p_k = 3 then
    if en.vy(i) = 0 then en.vy(i) = -50
    p_w = en.vy(i)
    en.move(i, TRUE)
    if en.vy(i) = 0 then
      if p_w > 0 then
        en.vy(i) = -50
      else
        en.vy(i) = 50
      end if
    end if
    en.fr(i) = 13 + ((en.y(i) \ 200) and 1)
  end if
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
    en.move(i, TRUE)
    if p_c2 <> 0 and en.x(i) = p_x0 then en.st(i) = (en.st(i) + 1) mod 4
    if p_r2 <> 0 and en.y(i) = p_y0 then en.st(i) = (en.st(i) + 1) mod 4
    en.fr(i) = 21 + en.st(i) * 3 + ((gm.frm \ 2) mod 3)
  end if
  if p_k = 5 then
    if en.st(i) = 0 then
      en.st(i) = int(rnd * 4) + 1
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
    p_d = ((gm.frm \ 8) + i) mod 8
    en.vx(i) = 0
    en.vy(i) = 0
    if p_d = 0 or p_d = 1 or p_d = 7 then en.vx(i) = 11
    if p_d = 3 or p_d = 4 or p_d = 5 then en.vx(i) = -11
    if p_d = 1 or p_d = 2 or p_d = 3 then en.vy(i) = -11
    if p_d = 5 or p_d = 6 or p_d = 7 then en.vy(i) = 11
    en.move(i, TRUE)
    en.fr(i) = 33 + ((gm.frm \ 20) mod 8)
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
  for i = 0 to 19
    if i < en.n then
      x = lay.ox + (en.x(i) - 384) * lay.tile \ 384
      y = lay.oy + (en.y(i) - 384) * lay.tile \ 384
      spr.put(2 + i, en.fr(i), 1, x, y, lay.tile, lay.tile, 2)
    else
      spr.hide(2 + i)
    end if
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
  pl.use()
  mt.step()
  en.step()
  lv.doorStep()
  pl.anim()
  key.prevJ = key.jet
  key.prevD = key.dn
  hud.dirty = TRUE
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
  fld.flush()
  if gm.tiles then gm.force = TRUE
  gm.tiles = FALSE
  if hud.dirty then hud.draw()
  hud.dirty = FALSE
  gm.sprites()
  if gm.moved then sprite move
  gm.moved = FALSE
  gm.force = FALSE
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
