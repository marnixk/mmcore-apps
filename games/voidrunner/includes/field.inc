' field.inc - the obstacles: asteroid pillars and laser gates.
'
' An obstacle is a 64-unit wide column with a gap: rock or beam from the top
' edge down to the gap and from the gap down to the bottom edge, so it acts like
' a pillar. When one spawns, its full-height picture is rendered ONCE into its
' own slot on PG_COLS (slot i at art x = i * SLOT_W, black = see-through) and
' read into SPRITE SPR_COL + i. From then on the column only moves: SPRITE NEXT
' queues its new place and one SPRITE MOVE per frame restores and redraws all
' sprites in mmcore's native code (background saved and restored automatically,
' no full-screen work). Its crystal is sprite SPR_GEM + i. All numbers here are
' art units unless noted.

const NOB = 7, SLOT_W = 64
const OK_ROCK = 0, OK_LASER = 1
dim ob.on(NOB - 1), ob.x(NOB - 1), ob.ox(NOB - 1), ob.k(NOB - 1), ob.drawn(NOB - 1)
dim ob.gt(NOB - 1), ob.gb(NOB - 1), ob.gem(NOB - 1), ob.gy(NOB - 1), ob.pass(NOB - 1)
dim ob.last = -1, ob.lastC, ob.count

sub ob.clear()
  local i
  for i = 0 to NOB - 1
    ob.on(i) = FALSE
    ob.gem(i) = FALSE
  next i
  ob.last = -1
  ob.count = 0
end sub

' a new obstacle at art x; gap size gap, centre within maxShift of the last one
sub ob.spawn(x, gap, maxShift, gemsOk)
  local i, f, c, lo, hi, k
  f = -1
  for i = 0 to NOB - 1
    if not ob.on(i) and f < 0 then f = i
  next i
  if f < 0 then exit sub
  lo = 26 + 26 + gap \ 2
  hi = lay.ah - lo
  if ob.last < 0 then
    c = lay.ah \ 2 + int(rnd * 61) - 30
  else
    c = ob.lastC + int(rnd * (2 * maxShift + 1)) - maxShift
  end if
  c = max(lo, min(hi, c))
  ob.on(f) = TRUE
  ob.x(f) = x
  ob.ox(f) = x
  ob.drawn(f) = FALSE
  ob.pass(f) = FALSE
  ob.k(f) = OK_ROCK
  if ob.count > 0 and rnd < 0.4 then ob.k(f) = OK_LASER
  ob.gt(f) = c - gap \ 2
  ob.gb(f) = c - gap \ 2 + gap
  ' crystal: in about a third of the gaps, close to the top or bottom edge of
  ' the gap, so taking it is a risk you choose
  ob.gem(f) = FALSE
  if gemsOk and ob.count >= 2 and rnd < 0.35 then
    ob.gem(f) = TRUE
    if rnd < 0.5 then
      ob.gy(f) = ob.gt(f) + 8
    else
      ob.gy(f) = ob.gb(f) - 24
    end if
  end if
  ob.last = f
  ob.lastC = c
  ob.count = ob.count + 1
  ob.render(f)
  k = SLOT_W * scr.sc
  sprite read SPR_COL + f, f * k, 0, k, scr.H, PG_COLS
  sprite show SPR_COL + f, x * scr.sc, 0, 1
  if ob.gem(f) then sprite show SPR_GEM + f, (x + 24) * scr.sc, ob.gy(f) * scr.sc, 2
end sub

' an obstacle that scrolled off the left edge
sub ob.remove(i)
  ob.on(i) = FALSE
  sprite hide SPR_COL + i
  if ob.gem(i) then sprite hide SPR_GEM + i
  ob.gem(i) = FALSE
end sub

' render obstacle i into its slot on PG_COLS
sub ob.render(i)
  local s, sx, y, cy, gt, gb
  s = scr.sc
  sx = i * SLOT_W
  gt = ob.gt(i)
  gb = ob.gb(i)
  page write PG_COLS
  box sx * s, 0, SLOT_W * s, scr.H, 1, 0, 0
  cl.set(sx * s, 0, (sx + SLOT_W) * s, scr.H)
  if ob.k(i) = OK_ROCK then
    cy = gt - 26
    y = cy + 6
    do while y > -32
      y = y - 32
      ui.sprO(SP_ROCK_BODY_X, SP_ROCK_BODY_Y, 48, 32, sx + 8, y)
    loop
    ui.spr(SP_CAP_DOWN_X, SP_CAP_DOWN_Y, 64, 32, sx, cy)
    cy = gb - 6
    y = cy + 26
    do while y < lay.ah
      ui.sprO(SP_ROCK_BODY_X, SP_ROCK_BODY_Y, 48, 32, sx + 8, y)
      y = y + 32
    loop
    ui.spr(SP_CAP_UP_X, SP_CAP_UP_Y, 64, 32, sx, cy)
  else
    cy = gt - 24
    y = cy + 2
    do while y > -16
      y = y - 16
      ui.sprO(SP_BEAM_0_X, SP_BEAM_0_Y, 48, 16, sx + 8, y)
    loop
    ui.spr(SP_EMIT_TOP_X, SP_EMIT_TOP_Y, 64, 24, sx, cy)
    y = gb + 22
    do while y < lay.ah
      ui.sprO(SP_BEAM_1_X, SP_BEAM_1_Y, 48, 16, sx + 8, y)
      y = y + 16
    loop
    ui.spr(SP_EMIT_BOT_X, SP_EMIT_BOT_Y, 64, 24, sx, gb)
  end if
  cl.all()
  page write PG_BACK
end sub

' do art boxes [ax0, ax1) x [ay0, ay1) and [bx0, bx1) x [by0, by1) overlap?
function ovl(ax0, ay0, ax1, ay1, bx0, by0, bx1, by1)
  ovl = (ax0 < bx1 and bx0 < ax1 and ay0 < by1 and by0 < ay1)
end function

' does the ship hull box hit obstacle i? Solid parts are the rock body / beam
' core and the caps / emitters, slightly inside the drawn outline (fair)
function ob.hit(i, hx0, hy0, hx1, hy1)
  local X, gt, gb, h
  h = lay.ah
  ob.hit = FALSE
  X = ob.x(i)
  if hx1 <= X or hx0 >= X + SLOT_W then exit function
  gt = ob.gt(i)
  gb = ob.gb(i)
  if ob.k(i) = OK_ROCK then
    if ovl(hx0, hy0, hx1, hy1, X + 11, 0, X + 52, gt - 24) then ob.hit = TRUE
    if ovl(hx0, hy0, hx1, hy1, X + 5, gt - 24, X + 59, gt - 1) then ob.hit = TRUE
    if ovl(hx0, hy0, hx1, hy1, X + 5, gb + 1, X + 59, gb + 24) then ob.hit = TRUE
    if ovl(hx0, hy0, hx1, hy1, X + 11, gb + 24, X + 52, h) then ob.hit = TRUE
  else
    if ovl(hx0, hy0, hx1, hy1, X + 23, 0, X + 41, gt - 22) then ob.hit = TRUE
    if ovl(hx0, hy0, hx1, hy1, X + 2, gt - 22, X + 62, gt - 2) then ob.hit = TRUE
    if ovl(hx0, hy0, hx1, hy1, X + 2, gb + 2, X + 62, gb + 22) then ob.hit = TRUE
    if ovl(hx0, hy0, hx1, hy1, X + 23, gb + 22, X + 41, h) then ob.hit = TRUE
  end if
end function
