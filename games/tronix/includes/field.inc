' field.inc - the grid: claimed land, open grid and the light wall (trail),
' how it is drawn, and the claim (fill) when the wall is closed.
'
' State: one string per field row, rw$(y), one character per cell:
'   "0" open grid   "1" claimed land   "2" light wall (trail being laid)
' Strings make runs of equal cells cheap to find: INSTR does the scanning in
' native code, so both the drawing and the fill work per RUN, not per cell.
'
' Drawing (on PG_FIELD): a run of claimed / open cells is ONE blit from the
' claimed-land / open-grid plane page (the whole field pre-tiled). Where land
' meets open grid a neon edge is laid on top: vertical pieces at run ends,
' horizontal strips (one blit per stretch) between rows. Wall cells get their
' wall piece on top of the open grid.
'
' Claim: every open run of the field is listed (a few per row); runs in
' neighbouring rows that overlap are connected. Starting from the runs the
' orbs are in, a breadth-first walk marks every run an orb can reach. All
' other open runs (no orb inside) become land, together with the wall. Only
' the bounding box of the change is redrawn and copied.
' hot-path helpers keep their scratch in globals: LOCAL costs O(variables)
' per call in mmcore (each LOCAL scans the whole variable table)
dim integer fld_rend_k, fld_rend_k2, fld_other_k, fld_other_k2, fld_vedge_s, fld_hedge_a, fld_hedge_b, fld_hedge_s
dim integer fld_hedge_hd, fld_hedge_sy, fld_hedge_dy, tr_piece_n, tr_piece_s, tr_piece_e, tr_piece_w

const FMAX_W = 100, FMAX_H = 64, RUN_MAX = 2400, TR_MAX = 4000
dim string rw$(FMAX_H - 1) length FMAX_W
dim fld.W, fld.H, fld.inner, fld.got   ' size, interior cells, interior cells claimed
dim fld.X0, fld.Y0, fld.c              ' field origin and cell size in screen px
' wall: cells in order, the side each came in from, its piece (tile index)
dim tr.x(TR_MAX - 1), tr.y(TR_MAX - 1), tr.in(TR_MAX - 1), tr.n
dim tr.shown   ' wall cells drawn so far (tr.show)
dim fld.tp(FMAX_W - 1, FMAX_H - 1)
' open runs for the claim
dim ru.s(RUN_MAX - 1), ru.e(RUN_MAX - 1), ru.y(RUN_MAX - 1), ru.v(RUN_MAX - 1), ru.q(RUN_MAX - 1)
dim ro.f(FMAX_H - 1), ro.n(FMAX_H - 1)
' change box (cells)
dim bb.x0, bb.y0, bb.x1, bb.y1
' cost of the last claim (ms): fill, redraw
dim fld.msFill, fld.msDraw, fld.lastGot
' directions 0 up 1 down 2 left 3 right
dim dir.x(3) = (0, 0, -1, 1), dir.y(3) = (-1, 1, 0, 0), dir.op(3) = (1, 0, 3, 2)
' wall piece for (side in, side out): T_TRAIL_* (sides 0 N 1 S 2 W 3 E)
dim tp.cap(3)

' ---- the two plane pages and the static parts, once at start-up
sub fld.planes()
  local s, c, x, y, t, pw, ph
  s = scr.sc
  c = 8 * s
  fld.c = c
  fld.W = lay.fw
  fld.H = lay.fh
  fld.X0 = lay.ox * s
  fld.Y0 = lay.oy * s
  fld.inner = (fld.W - 4) * (fld.H - 4)
  tp.cap(0) = T_TRAIL_N
  tp.cap(1) = T_TRAIL_S
  tp.cap(2) = T_TRAIL_W
  tp.cap(3) = T_TRAIL_E
  pw = fld.W * c
  ph = fld.H * c
  page write PG_OPEN
  cls rgb(0, 0, 0)
  for y = 0 to 3
    for x = 0 to 3
      t = T_OPEN_0
      if y = 0 and x = 0 then
        t = T_OPEN_X
      elseif y = 0 then
        t = T_OPEN_H
      elseif x = 0 then
        t = T_OPEN_V
      end if
      blit t * c, SH_TILE_Y * s, fld.X0 + x * c, fld.Y0 + y * c, c, c, PG_SHEET
    next x
  next y
  fld.grow(PG_OPEN, 4 * c, 4 * c, pw, ph)
  page write PG_CLAIM
  cls rgb(0, 0, 0)
  blit T_CLAIMED * c, SH_TILE_Y * s, fld.X0, fld.Y0, c, c, PG_SHEET
  fld.grow(PG_CLAIM, c, c, pw, ph)
  page write PG_BACK
end sub

' repeat the block at the field origin (bw x bh px) over fw x fh px: doubling copies
' (MMBasic names are not case sensitive: fw / fh are the full size, cw / ch the copied part)
sub fld.grow(pg, bw, bh, fw, fh)
  local cw, ch, n
  cw = bw
  do while cw < fw
    n = min(cw, fw - cw)
    blit fld.X0, fld.Y0, fld.X0 + cw, fld.Y0, n, bh, pg
    cw = cw + n
  loop
  ch = bh
  do while ch < fh
    n = min(ch, fh - ch)
    blit fld.X0, fld.Y0, fld.X0, fld.Y0 + ch, fw, n, pg
    ch = ch + n
  loop
end sub

' ---- state
' a fresh level: a claimed frame two cells wide round an open grid
sub fld.reset()
  local y, m$
  m$ = "11" + string$(fld.W - 4, "0") + "11"
  for y = 0 to fld.H - 1
    if y < 2 or y >= fld.H - 2 then
      rw$(y) = string$(fld.W, "1")
    else
      rw$(y) = m$
    end if
  next y
  fld.got = 0
  tr.n = 0
  tr.shown = 0
end sub

' cell state: 0 open, 1 claimed, 2 wall, 9 outside the field
function fld.at(x, y)
  if x < 0 or y < 0 or x >= fld.W or y >= fld.H then
    fld.at = 9
  else
    fld.at = asc(mid$(rw$(y), x + 1, 1)) - 48
  end if
end function

sub fld.set(x, y, v)
  mid$(rw$(y), x + 1, 1) = chr$(48 + v)
end sub

' claimed share of the interior (whole %)
function fld.pct()
  fld.pct = (fld.got * 100) \ fld.inner
end function

sub bb.none()
  bb.x0 = 9999
  bb.y0 = 9999
  bb.x1 = -1
  bb.y1 = -1
end sub

sub bb.add(x0, y0, x1, y1)
  if x0 < bb.x0 then bb.x0 = x0
  if y0 < bb.y0 then bb.y0 = y0
  if x1 > bb.x1 then bb.x1 = x1
  if y1 > bb.y1 then bb.y1 = y1
end sub

' ---- drawing
' last x of the run of class cl (1 land, 0 open or wall) that starts at x = p
' in row r$, capped at lim
function fld.rend(r$, p, cl, lim)
  if cl then
    fld_rend_k = instr(p + 1, r$, "0")
    fld_rend_k2 = instr(p + 1, r$, "2")
    if fld_rend_k = 0 or (fld_rend_k2 > 0 and fld_rend_k2 < fld_rend_k) then fld_rend_k = fld_rend_k2
  else
    fld_rend_k = instr(p + 1, r$, "1")
  end if
  if fld_rend_k = 0 then fld_rend_k = fld.W + 1
  fld.rend = min(lim, fld_rend_k - 2)
end function

' first x >= a in row r$ whose class is not cl (-1 none)
function fld.other(r$, a, cl)
  if cl then
    fld_other_k = instr(a + 1, r$, "0")
    fld_other_k2 = instr(a + 1, r$, "2")
    if fld_other_k = 0 or (fld_other_k2 > 0 and fld_other_k2 < fld_other_k) then fld_other_k = fld_other_k2
  else
    fld_other_k = instr(a + 1, r$, "1")
  end if
  fld.other = fld_other_k - 1
end function

' TRUE when segment o$ holds a cell whose class is not cl
function fld.mixed(o$, cl)
  if cl then
    fld.mixed = (instr(o$, "0") > 0) or (instr(o$, "2") > 0)
  else
    fld.mixed = (instr(o$, "1") > 0)
  end if
end function

' vertical edge between cells xb - 1 and xb of row y; rcl = class of cell xb
sub fld.vedge(xb, y, rcl)
  fld_vedge_s = scr.sc
  if rcl then
    blit SH_VL_X * fld_vedge_s, SH_TILE_Y * fld_vedge_s, fld.X0 + xb * fld.c - fld_vedge_s, fld.Y0 + y * fld.c, 3 * fld_vedge_s, fld.c, PG_SHEET, 4
  else
    blit SH_VR_X * fld_vedge_s, SH_TILE_Y * fld_vedge_s, fld.X0 + xb * fld.c - 2 * fld_vedge_s, fld.Y0 + y * fld.c, 3 * fld_vedge_s, fld.c, PG_SHEET, 4
  end if
end sub

' horizontal edges along cells p..q of a run of class cl, against the other
' row o$; yb = the lower row of the pair, up = TRUE when o$ is the upper row
sub fld.hedge(o$, p, q, yb, cl, up)
  fld_hedge_s = scr.sc
  ' HD strip: land above / open below, HU strip: open above / land below
  if up then
    fld_hedge_hd = not cl
  else
    fld_hedge_hd = cl
  end if
  if fld_hedge_hd then
    fld_hedge_sy = SH_EDGE_Y * fld_hedge_s
    fld_hedge_dy = fld.Y0 + yb * fld.c - 2 * fld_hedge_s
  else
    fld_hedge_sy = (SH_EDGE_Y + 4) * fld_hedge_s
    fld_hedge_dy = fld.Y0 + yb * fld.c - fld_hedge_s
  end if
  fld_hedge_a = p
  do while fld_hedge_a <= q
    fld_hedge_a = fld.other(o$, fld_hedge_a, cl)
    if fld_hedge_a < 0 or fld_hedge_a > q then exit do
    fld_hedge_b = fld.rend(o$, fld_hedge_a, not cl, q)
    blit fld_hedge_a * fld.c, fld_hedge_sy, fld.X0 + fld_hedge_a * fld.c, fld_hedge_dy, (fld_hedge_b - fld_hedge_a + 1) * fld.c, 3 * fld_hedge_s, PG_SHEET, 4
    fld_hedge_a = fld_hedge_b + 1
  loop
end sub

' redraw cells x0..x1, y0..y1 on PG_FIELD, then copy them to the back page
' and mark them for the screen. Callers set act.dirty (actors get redrawn).
sub fld.redraw(xa, ya, xb, yb)
  local x0, y0, x1, y1, y, p, q, cl, k, r$, o$, c, s, pg
  x0 = max(0, xa)
  y0 = max(0, ya)
  x1 = min(fld.W - 1, xb)
  y1 = min(fld.H - 1, yb)
  if x1 < x0 or y1 < y0 then exit sub
  c = fld.c
  s = scr.sc
  page write PG_FIELD
  for y = y0 to y1
    r$ = rw$(y)
    p = x0
    do while p <= x1
      cl = (mid$(r$, p + 1, 1) = "1")
      q = fld.rend(r$, p, cl, x1)
      if cl then
        pg = PG_CLAIM
      else
        pg = PG_OPEN
      end if
      blit fld.X0 + p * c, fld.Y0 + y * c, fld.X0 + p * c, fld.Y0 + y * c, (q - p + 1) * c, c, pg
      if p > 0 then
        if (mid$(r$, p, 1) = "1") <> cl then fld.vedge(p, y, cl)
      end if
      if q = x1 and q < fld.W - 1 then
        if (mid$(r$, q + 2, 1) = "1") <> cl then fld.vedge(q + 1, y, not cl)
      end if
      ' edges against the row above (and below on the last row) only when that
      ' row's segment holds a cell of the other class
      if y > 0 then
        o$ = mid$(rw$(y - 1), p + 1, q - p + 1)
        if fld.mixed(o$, cl) then fld.hedge(rw$(y - 1), p, q, y, cl, TRUE)
      end if
      if y = y1 and y < fld.H - 1 then
        o$ = mid$(rw$(y + 1), p + 1, q - p + 1)
        if fld.mixed(o$, cl) then fld.hedge(rw$(y + 1), p, q, y + 1, cl, FALSE)
      end if
      p = q + 1
    loop
    ' wall pieces on top
    k = instr(x0 + 1, r$, "2")
    do while k > 0 and k <= x1 + 1
      blit fld.tp(k - 1, y) * c, SH_TILE_Y * s, fld.X0 + (k - 1) * c, fld.Y0 + y * c, c, c, PG_SHEET, 4
      k = instr(k + 1, r$, "2")
    loop
  next y
  ' the thin frame round the whole field
  if x0 = 0 or y0 = 0 or x1 = fld.W - 1 or y1 = fld.H - 1 then fld.frame()
  page write PG_BACK
  blit fld.X0 + x0 * c, fld.Y0 + y0 * c, fld.X0 + x0 * c, fld.Y0 + y0 * c, (x1 - x0 + 1) * c, (y1 - y0 + 1) * c, PG_FIELD
  scr.mark(fld.X0 + x0 * c, fld.Y0 + y0 * c, (x1 - x0 + 1) * c, (y1 - y0 + 1) * c)
end sub

sub fld.frame()
  local w, h
  w = fld.W * 8
  h = fld.H * 8
  af lay.ox, lay.oy, w, 1, C_C1
  af lay.ox, lay.oy + h - 1, w, 1, C_C1
  af lay.ox, lay.oy, 1, h, C_C1
  af lay.ox + w - 1, lay.oy, 1, h, C_C1
end sub

' ---- the wall
' tr.add runs in the timer tick and only changes the state; the main loop
' draws the new cells with tr.show.
sub tr.clear()
  tr.n = 0
  tr.shown = 0
end sub

' add wall cell (x, y), entered moving in direction d; sets the pieces of it
' and of the cell before
sub tr.add(x, y, d)
  local i, a, b, px, py
  i = tr.n
  tr.x(i) = x
  tr.y(i) = y
  tr.in(i) = dir.op(d)
  tr.n = i + 1
  fld.set(x, y, 2)
  fld.tp(x, y) = tp.cap(dir.op(d))
  px = x
  py = y
  if i > 0 then
    px = tr.x(i - 1)
    py = tr.y(i - 1)
    fld.tp(px, py) = tr.piece(tr.in(i - 1), d)
  end if
end sub

' draw the wall cells added since the last call, and the cell before them
' (its piece changes when the wall turns). Main loop only.
sub tr.show()
  local i, n
  n = tr.n
  if tr.shown >= n then exit sub
  bb.none()
  for i = max(0, tr.shown - 1) to n - 1
    bb.add(tr.x(i), tr.y(i), tr.x(i), tr.y(i))
  next i
  tr.shown = n
  fld.redraw(bb.x0, bb.y0, bb.x1, bb.y1)
  act.dirty = TRUE
end sub

' wall piece through sides a and b (0 N 1 S 2 W 3 E)
function tr.piece(a, b)
  tr_piece_n = (a = 0 or b = 0)
  tr_piece_s = (a = 1 or b = 1)
  tr_piece_w = (a = 2 or b = 2)
  tr_piece_e = (a = 3 or b = 3)
  if tr_piece_n and tr_piece_s then
    tr.piece = T_TRAIL_NS
  elseif tr_piece_e and tr_piece_w then
    tr.piece = T_TRAIL_EW
  elseif tr_piece_n and tr_piece_e then
    tr.piece = T_TRAIL_NE
  elseif tr_piece_n and tr_piece_w then
    tr.piece = T_TRAIL_NW
  elseif tr_piece_s and tr_piece_e then
    tr.piece = T_TRAIL_SE
  else
    tr.piece = T_TRAIL_SW
  end if
end function

' the wall is cut (death): back to open grid
sub tr.remove()
  local i
  if tr.n = 0 then exit sub
  bb.none()
  for i = 0 to tr.n - 1
    fld.set(tr.x(i), tr.y(i), 0)
    bb.add(tr.x(i), tr.y(i), tr.x(i), tr.y(i))
  next i
  tr.n = 0
  tr.shown = 0
  fld.redraw(bb.x0 - 1, bb.y0 - 1, bb.x1 + 1, bb.y1 + 1)
end sub

' ---- the claim: the wall closed on land. Returns the number of cells claimed
function fld.close()
  local i, j, n, y, yy, p, k, e, qh, qt, x, r$, t0, gotNew, L, a, b
  t0 = timer
  bb.none()
  gotNew = tr.n
  for i = 0 to tr.n - 1
    fld.set(tr.x(i), tr.y(i), 1)
    bb.add(tr.x(i), tr.y(i), tr.x(i), tr.y(i))
  next i
  tr.n = 0
  tr.shown = 0
  ' every open run of the field
  n = 0
  for y = 0 to fld.H - 1
    ro.f(y) = n
    r$ = rw$(y)
    p = 1
    do
      k = instr(p, r$, "0")
      if k = 0 or n >= RUN_MAX then exit do
      e = instr(k, r$, "1")
      if e = 0 then e = fld.W + 1
      ru.s(n) = k - 1
      ru.e(n) = e - 2
      ru.y(n) = y
      ru.v(n) = FALSE
      n = n + 1
      p = e
    loop
    ro.n(y) = n - ro.f(y)
  next y
  ' seeds: the runs the orbs are in
  qh = 0
  qt = 0
  for i = 1 to act.nOrb
    x = o.x(i)
    y = o.y(i)
    for j = ro.f(y) to ro.f(y) + ro.n(y) - 1
      if ru.s(j) <= x and ru.e(j) >= x then
        if not ru.v(j) then
          ru.v(j) = TRUE
          ru.q(qt) = j
          qt = qt + 1
        end if
      end if
    next j
  next i
  ' walk: overlapping runs in the rows above and below are connected
  do while qh < qt
    i = ru.q(qh)
    qh = qh + 1
    a = ru.s(i)
    b = ru.e(i)
    for yy = ru.y(i) - 1 to ru.y(i) + 1 step 2
      if yy >= 0 and yy < fld.H then
        for j = ro.f(yy) to ro.f(yy) + ro.n(yy) - 1
          if not ru.v(j) then
            if ru.s(j) <= b and ru.e(j) >= a then
              ru.v(j) = TRUE
              ru.q(qt) = j
              qt = qt + 1
            end if
          end if
        next j
      end if
    next yy
  loop
  ' runs no orb can reach become land
  for j = 0 to n - 1
    if not ru.v(j) then
      y = ru.y(j)
      L = ru.e(j) - ru.s(j) + 1
      mid$(rw$(y), ru.s(j) + 1, L) = string$(L, "1")
      gotNew = gotNew + L
      bb.add(ru.s(j), y, ru.e(j), y)
    end if
  next j
  fld.got = fld.got + gotNew
  fld.msFill = timer - t0
  t0 = timer
  fld.redraw(bb.x0 - 1, bb.y0 - 1, bb.x1 + 1, bb.y1 + 1)
  fld.msDraw = timer - t0
  fld.lastGot = gotNew
  fld.close = gotNew
end function
