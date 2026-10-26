' board.inc - the board on the back page: cells, the cursor, the picked gem,
' the hint, and the animations (swap, pops / beams / blasts, falling gems).
' Only changed cells are drawn and marked (scr.mark) for scr.flush; the
' animations are time based (a slow frame skips ahead, it never lags).
' Cell (x, y) is at screen px lay.bx + x * ts, lay.by + y * ts.
const SWAP_MS = 130, POP_MS = 45, FALL_MS = 38, PULSE_MS = 220, CUR_MS = 420
dim bv.sx(99), bv.sy(99)            ' sheet position of cell tile k
dim bv.ts
dim cur.x = 4, cur.y = 4, cur.pk = -1   ' cursor cell, picked gem (-1 none)
dim bv.cf, bv.pf, bv.hon            ' cursor frame, pick pulse frame, hint shown
dim bv.hide(80)                     ' 1 = draw the cell without its gem (it is moving)
dim integer bv_x, bv_y, bv_i, bv_t, bv_k, bv_c, bv_px, bv_py, bv_r, bv_y0, bv_y1, bv_n, bv_j
dim integer bv_x0, bv_x1, bv_ya, bv_yb, bv_f

sub bv.init()
  local k
  bv.ts = lay.ts
  for k = 0 to 99
    bv.sx(k) = (k mod SH_COLS) * bv.ts
    bv.sy(k) = (k \ SH_COLS) * bv.ts
  next k
end sub

' a level starts: the clean scene with the board frame hugging the shape
' (three rings: dark, mid gold, light gold), then every cell on PG_BACK
sub bv.start()
  local i, x, y, d, c
  page copy PG_SCENE to PG_CLEAN
  page write PG_CLEAN
  for d = 3 to 1 step -1
    if d = 3 then c = C_K
    if d = 2 then c = C_AU1
    if d = 1 then c = C_AU2
    for i = 0 to 80
      if ac(i) then
        x = lay.bx + (i mod 9) * bv.ts - d * lay.fr \ 3
        y = lay.by + (i \ 9) * bv.ts - d * lay.fr \ 3
        box x, y, bv.ts + 2 * d * lay.fr \ 3, bv.ts + 2 * d * lay.fr \ 3, 1, c, c
      end if
    next i
  next d
  page copy PG_CLEAN to PG_BACK
  page write PG_BACK
  for i = 0 to 80
    bv.hide(i) = 0
  next i
  cur.pk = -1
  bv.hon = 0
  bv.all()
end sub

sub bv.all()
  for bv_j = 0 to 80
    bv.cell(bv_j)
  next bv_j
end sub

' the gem tile for colour c, kind k
function bv.gem(c, k)
  if k = K_PR or c = PRISM_C then
    bv.gem = TL_BOMB
  else
    bv.gem = c + 6 * k
  end if
end function

' the cell background: frost or the plain checker cell
sub bv.back(i)
  bv_px = lay.bx + (i mod 9) * bv.ts
  bv_py = lay.by + (i \ 9) * bv.ts
  if fr(i) = 2 then
    bv_t = TL_LAYER_2
  elseif fr(i) = 1 then
    bv_t = TL_LAYER_1
  else
    bv_t = TL_CELL_0 + ((i mod 9) + (i \ 9)) mod 2
  end if
  blit bv.sx(bv_t), bv.sy(bv_t), bv_px, bv_py, bv.ts, bv.ts, PG_SHEET
end sub

sub bv.spr(k, x, y)
  blit bv.sx(k), bv.sy(k), x, y, bv.ts, bv.ts, PG_SHEET, 4
end sub

' one cell as it is now (no mark)
sub bv.paint(i)
  if ac(i) = 0 then
    bv_px = lay.bx + (i mod 9) * bv.ts
    bv_py = lay.by + (i \ 9) * bv.ts
    blit bv_px, bv_py, bv_px, bv_py, bv.ts, bv.ts, PG_CLEAN
    exit sub
  end if
  bv.back(i)
  if bd(i) then
    bv_t = TL_STONE
    if bd(i) = 1 and bd0(i) = 2 then bv_t = TL_STONE_CRACKED
    bv.spr(bv_t, bv_px, bv_py)
  elseif gc(i) >= 0 and bv.hide(i) = 0 then
    bv.gemAt(i)
  end if
  if i = cur.y * 9 + cur.x then bv.spr(TL_CURSOR_0 + bv.cf, bv_px, bv_py)
end sub

' the gem in cell i with its outline (picked / hint) and chain. A sub of its
' own: a block IF nested in a block IF must not be skipped while the play
' handlers may interrupt (mmcore loses the nesting count, see gm.handlers)
sub bv.gemAt(i)
  if i = cur.pk then
    bv.outline(i, bv.pf)
  elseif bv.hon and (i = en.ha or i = en.hb) then
    bv.outline(i, 1)
  end if
  bv.spr(bv.gem(gc(i), gk(i)), bv_px, bv_py)
  if ch(i) then bv.spr(TL_LOCK, bv_px, bv_py)
end sub

' the select outline (frame f) of the gem in cell i
sub bv.outline(i, f)
  if gk(i) = K_PR or gc(i) = PRISM_C then
    bv.spr(TL_SEL_X_0 + f, bv_px, bv_py)
  else
    bv.spr(TL_SEL_R_0 + 2 * gc(i) + f, bv_px, bv_py)
  end if
end sub

' draw cell i and note it for the next flush
sub bv.cell(i)
  bv.paint(i)
  scr.mark(lay.bx + (i mod 9) * bv.ts, lay.by + (i \ 9) * bv.ts, bv.ts, bv.ts)
end sub

' ---- swap: the gems in cells a and b trade places (and back when back = 1)
sub bv.swap(a, b, back)
  local ta, tb, ax, ay, dx, dy, t0, t, d, ph, o
  ta = bv.gem(gc(a), gk(a))
  tb = bv.gem(gc(b), gk(b))
  ax = lay.bx + (a mod 9) * bv.ts
  ay = lay.by + (a \ 9) * bv.ts
  dx = ((b mod 9) - (a mod 9)) * bv.ts
  dy = ((b \ 9) - (a \ 9)) * bv.ts
  bv.hide(a) = 1
  bv.hide(b) = 1
  for ph = 0 to back
    t0 = timer
    do
      t = timer - t0
      if t > SWAP_MS then t = SWAP_MS
      d = (t * bv.ts) \ SWAP_MS
      if ph = 1 then d = bv.ts - d
      gm.drawing = TRUE
      bv.paint(a)
      bv.paint(b)
      o = (d * dx) \ bv.ts
      bv.spr(tb, ax + dx - (d * dx) \ bv.ts, ay + dy - (d * dy) \ bv.ts)
      bv.spr(ta, ax + o, ay + (d * dy) \ bv.ts)
      scr.mark(min(ax, ax + dx), min(ay, ay + dy), abs(dx) + bv.ts, abs(dy) + bv.ts)
      bv.frameEnd()
      if t >= SWAP_MS then exit do
      pause 2
    loop
  next ph
  bv.hide(a) = 0
  bv.hide(b) = 0
end sub

' end of an animation frame: cursor changes from the tick, then the flush
sub bv.frameEnd()
  gm.cursor()
  if scr.dirty then scr.flush()
  gm.drawing = FALSE
end sub

' ---- clear: pops on the cells hit, beams along the lines of striped gems,
' blasts round charged gems, rays from prisms; 3 frames
dim bv.tm(80)
sub bv.clearAnim()
  local f, t0, k, i, x, y, d, xa, xb, ya, yb, dx, dy
  for i = 0 to 80
    bv.tm(i) = 0
  next i
  for k = 0 to en.hn - 1
    bv.tm(ev.i(k)) = 1
  next k
  for k = 0 to en.fn - 1
    bv.fxCells(fx.i(k), fx.k(k))
  next k
  xa = 9 : xb = -1 : ya = 9 : yb = -1
  for i = 0 to 80
    if bv.tm(i) then
      x = i mod 9
      y = i \ 9
      xa = min(xa, x) : xb = max(xb, x) : ya = min(ya, y) : yb = max(yb, y)
    end if
  next i
  if xb < 0 then
    exit sub
  end if
  t0 = timer
  for f = 0 to 2
    gm.drawing = TRUE
    for i = 0 to 80
      if bv.tm(i) then bv.paint(i)
    next i
    for k = 0 to en.hn - 1
      i = ev.i(k)
      x = lay.bx + (i mod 9) * bv.ts
      y = lay.by + (i \ 9) * bv.ts
      d = ev.c(k)
      if d >= 0 and d <= 5 then
        bv.spr(TL_POP_R_0 + 3 * d + f, x, y)
      elseif d = PRISM_C then
        bv.spr(TL_BOMBFX_0 + f, x, y)
      else
        bv.spr(TL_POP_0 + f, x, y)
      end if
    next k
    for k = 0 to en.fn - 1
      bv.fxDraw(fx.i(k), fx.k(k), f)
    next k
    scr.mark(lay.bx + xa * bv.ts, lay.by + ya * bv.ts, (xb - xa + 1) * bv.ts, (yb - ya + 1) * bv.ts)
    bv.frameEnd()
    do while timer < t0 + (f + 1) * POP_MS
      pause 2
    loop
  next f
  gm.drawing = TRUE
  for i = 0 to 80
    if bv.tm(i) then bv.paint(i)
  next i
  scr.mark(lay.bx + xa * bv.ts, lay.by + ya * bv.ts, (xb - xa + 1) * bv.ts, (yb - ya + 1) * bv.ts)
  bv.frameEnd()
end sub

' the cells an effect draws over (bv.tm)
sub bv.fxCells(i, k)
  local x, y, j, r
  x = i mod 9
  y = i \ 9
  r = 0
  if k = K_WR then r = 1
  if k = 7 then r = 2
  if k = K_SH or k = 5 or k = 6 then
    for j = max(0, y - (k = 6)) to min(8, y + (k = 6))
      for bv_k = 0 to 8
        bv.tm(j * 9 + bv_k) = 1
      next bv_k
    next j
  end if
  if k = K_SV or k = 5 or k = 6 then
    for j = max(0, x - (k = 6)) to min(8, x + (k = 6))
      for bv_k = 0 to 8
        bv.tm(bv_k * 9 + j) = 1
      next bv_k
    next j
  end if
  if r > 0 then
    for bv_y = max(0, y - r) to min(8, y + r)
      for bv_x = max(0, x - r) to min(8, x + r)
        bv.tm(bv_y * 9 + bv_x) = 1
      next bv_x
    next bv_y
  end if
  bv.tm(i) = 1
end sub

' draw effect k at cell i, frame f
sub bv.fxDraw(i, k, f)
  local x, y, j
  x = i mod 9
  y = i \ 9
  if k = K_SH or k = 5 or k = 6 then
    for j = max(0, y - (k = 6)) to min(8, y + (k = 6))
      bv.beam(j, 0, f)
    next j
  end if
  if k = K_SV or k = 5 or k = 6 then
    for j = max(0, x - (k = 6)) to min(8, x + (k = 6))
      bv.beam(j, 1, f)
    next j
  end if
  if k = K_WR or k = 7 then bv.blast(x, y, f)
  if k = K_PR or k = 9 then bv.spr(TL_BOMBFX_0 + f, lay.bx + x * bv.ts, lay.by + y * bv.ts)
end sub

' a beam along row (v = 0) or column (v = 1) j over its active cells
sub bv.beam(j, v, f)
  for bv_k = 0 to 8
    if v = 0 then
      bv_i = j * 9 + bv_k
    else
      bv_i = bv_k * 9 + j
    end if
    if ac(bv_i) then
      if v = 0 then
        bv.spr(TL_BEAM_H_0 + f, lay.bx + bv_k * bv.ts, lay.by + j * bv.ts)
      else
        bv.spr(TL_BEAM_V_0 + f, lay.bx + j * bv.ts, lay.by + bv_k * bv.ts)
      end if
    end if
  next bv_k
end sub

' the blast ring (3 x 3 cells) centred on cell x, y, clipped to the board
sub bv.blast(x, y, f)
  local sx, sy, dx, dy, w, h, ox, oy
  sx = SH_COLS * bv.ts
  sy = 3 * f * bv.ts
  dx = lay.bx + (x - 1) * bv.ts
  dy = lay.by + (y - 1) * bv.ts
  w = 3 * bv.ts
  h = 3 * bv.ts
  ox = 0
  oy = 0
  if x = 0 then ox = bv.ts
  if y = 0 then oy = bv.ts
  if x = 8 then w = 2 * bv.ts
  if y = 8 then h = 2 * bv.ts
  blit sx + ox, sy + oy, dx + ox, dy + oy, w - ox, h - oy, PG_SHEET, 4
end sub

' ---- falling: every gem that moved drops from row fl.y to its cell; new
' gems come in from above the board. Gems are only drawn over the cells
' they may fall through (holes, boulders and chained gems hide them)
dim bv.cb(8)
sub bv.fallAnim()
  local t0, t, x, i, dist, done, py, k
  for x = 0 to 8
    bv.cb(x) = -1
  next x
  for i = 0 to 80
    if fl.mv(i) then
      bv.hide(i) = 1
      bv.cb(i mod 9) = max(bv.cb(i mod 9), i \ 9)
    end if
  next i
  t0 = timer
  do
    t = timer - t0
    done = 1
    gm.drawing = TRUE
    for x = 0 to 8
      if bv.cb(x) >= 0 then
        for bv_y = 0 to bv.cb(x)
          bv.paint(bv_y * 9 + x)
        next bv_y
        for bv_y = 0 to bv.cb(x)
          i = bv_y * 9 + x
          if fl.mv(i) then
            dist = (bv_y - fl.y(i)) * bv.ts
            k = (t * bv.ts) \ FALL_MS
            if k < dist then
              done = 0
              py = fl.y(i) * bv.ts + k
            else
              py = bv_y * bv.ts
            end if
            bv.drop(i, x, py)
          end if
        next bv_y
        if cur.x = x and cur.y <= bv.cb(x) then bv.spr(TL_CURSOR_0 + bv.cf, lay.bx + x * bv.ts, lay.by + cur.y * bv.ts)
        scr.mark(lay.bx + x * bv.ts, lay.by, bv.ts, (bv.cb(x) + 1) * bv.ts)
      end if
    next x
    bv.frameEnd()
    if done then exit do
    pause 2
  loop
  for i = 0 to 80
    if fl.mv(i) then
      bv.hide(i) = 0
      bv.cell(i)
    end if
  next i
end sub

' the gem now in cell i, drawn at board px y py in column x, clipped to the
' cells a gem can pass
sub bv.drop(i, x, py)
  bv_t = bv.gem(gc(i), gk(i))
  bv_px = lay.bx + x * bv.ts
  if py >= 0 then
    bv_r = py \ bv.ts
  else
    bv_r = -1 - ((-py - 1) \ bv.ts)
  end if
  for bv_n = bv_r to bv_r + 1
    if bv_n >= 0 and bv_n <= 8 then
      bv_j = bv_n * 9 + x
      if ac(bv_j) and bd(bv_j) = 0 and ch(bv_j) = 0 then
        bv_y0 = max(py, bv_n * bv.ts)
        bv_y1 = min(py + bv.ts, (bv_n + 1) * bv.ts)
        if bv_y1 > bv_y0 then blit bv.sx(bv_t), bv.sy(bv_t) + bv_y0 - py, bv_px, lay.by + bv_y0, bv.ts, bv_y1 - bv_y0, PG_SHEET, 4
      end if
    end if
  next bv_n
end sub
