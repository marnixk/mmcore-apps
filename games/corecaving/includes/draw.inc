' draw.inc - drawing the cave and the status bar
'
' Every cell is one opaque tile from the sheet (pre-composited on the cave
' floor), so a changed cell is two blits: sheet -> back page, then the same
' cell back page -> screen. A full scene (cave start, after a dialog) is built
' on the back page from the frame page plus all 880 cells and shown with one
' PAGE COPY.

' tile for each cell code (-1 = animated, worked out in drw.tile)
dim drw.t(E_LAST)
' explosion stages 0..3
dim drw.boom(3) = (T_BOOM_0, T_BOOM_1, T_BOOM_2, T_BOOM_1)
' scratch (globals: LOCAL is slow in mmcore)
dim integer drw_i, drw_c, drw_k, drw_m, drw_s, drw_x, drw_y, drw_ts, drw_tx, drw_ty

sub drw.init()
  local c
  for c = 0 to E_LAST
    drw.t(c) = -1
  next c
  drw.t(E_SPACE) = T_SPACE
  drw.t(E_DIRT) = T_DIRT
  drw.t(E_BRICK) = T_BRICK
  drw.t(E_STEEL) = T_STEEL
  drw.t(E_EXITC) = T_EXIT_CLOSED
  drw.t(E_BOULDER) = T_BOULDER
  drw.t(E_BOULDERF) = T_BOULDER
  drw.t(E_DIAMONDF) = T_DIAMOND_0
  for c = E_EXS to E_LAST
    drw.t(c) = drw.boom((c - E_EXS) mod 4)
  next c
end sub

' the tiles that change with the frame but not with the cell (drw.t; the
' diamond twinkle and the enemies also depend on the cell: drw.tile)
sub drw.frameTiles()
  drw.t(E_PLAYER) = cv.spr
  drw.t(E_AMOEBA) = T_AMOEBA_0 + (cv.afr \ 4) mod 2
  drw.t(E_EXITO) = T_EXIT_OPEN_0 + (cv.afr \ 3) mod 2
  drw.t(E_MAGIC) = T_MAGIC_0
  if cv.mw = 1 then drw.t(E_MAGIC) = T_MAGIC_1 + (cv.afr \ 2) mod 2
end sub

' the tile for cell drw_i (code drw_c) -> drw_s
sub drw.tile()
  drw_s = drw.t(drw_c)
  if drw_s >= 0 then exit sub
  if drw_c = E_DIAMOND then
    drw_m = (cv.afr + drw_i * 7) mod 24
    drw_s = T_DIAMOND_0
    if drw_m >= 21 then drw_s = T_DIAMOND_1 + drw_m - 21
  elseif drw_c = E_PLAYER then
    drw_s = cv.spr
  elseif drw_c >= E_FF and drw_c < E_BF then
    drw_s = T_FIREFLY_0 + (cv.afr + drw_i) mod 4
  elseif drw_c >= E_BF and drw_c < E_EXS then
    drw_s = T_BUTTERFLY_0 + (cv.afr + drw_i) mod 4
  elseif drw_c >= E_EXS then
    drw_s = drw.boom((drw_c - E_EXS) mod 4)
  elseif drw_c = E_MAGIC then
    drw_s = T_MAGIC_0
    if cv.mw = 1 then drw_s = T_MAGIC_1 + (cv.afr \ 2) mod 2
  elseif drw_c = E_AMOEBA then
    drw_s = T_AMOEBA_0 + (cv.afr \ 4) mod 2
  elseif drw_c = E_EXITO then
    drw_s = T_EXIT_OPEN_0 + (cv.afr \ 3) mod 2
  else
    drw_s = T_STEEL
  end if
end sub

' draw the queued cells: back page, then the same cells to the screen.
' Cell and tile positions come from tables (drw.geom), and BLIT gets plain
' variables: arithmetic in BLIT arguments is slow in mmcore.
' drw.px / drw.py: each cell's screen position (drw.pv = 0: under the bar);
' drw.sx / drw.sy: each tile's sheet position; drw.m63 = code without the tag
dim integer drw.px(CV_N - 1), drw.py(CV_N - 1), drw.pv(CV_N - 1)
dim integer drw.sx(127), drw.sy(127), drw.m63(127)
dim integer dc_n, drw_n1, drw_a, drw_b
sub drw.cells()
  dc_n = dqn
  if dc_n = 0 then exit sub
  drw_ts = lay.ts : drw_n1 = dc_n - 1
  drw.frameTiles()
  page write PG_BACK
  for drw_k = 0 to drw_n1
    drw_i = dq(drw_k)
    if drw.pv(drw_i) = 0 then continue for
    drw_c = g(drw_i) : drw_c = drw.m63(drw_c) : drw_s = drw.t(drw_c)
    if drw_s < 0 then drw.tile()
    drw_a = drw.sx(drw_s) : drw_b = drw.sy(drw_s) : drw_x = drw.px(drw_i) : drw_y = drw.py(drw_i)
    blit drw_a, drw_b, drw_x, drw_y, drw_ts, drw_ts, PG_SHEET
  next drw_k
  page write 0
  for drw_k = 0 to drw_n1
    drw_i = dq(drw_k)
    dm(drw_i) = 0
    if drw.pv(drw_i) = 0 then continue for
    drw_x = drw.px(drw_i) : drw_y = drw.py(drw_i)
    blit drw_x, drw_y, drw_x, drw_y, drw_ts, drw_ts, PG_BACK
  next drw_k
  page write PG_BACK
  dqn = 0
end sub

' the tables for drw.cells, for the current layout
sub drw.geom()
  local i, y
  for i = 0 to CV_N - 1
    y = i \ CV_W
    drw.px(i) = lay.cx + (i - y * CV_W) * lay.ts
    drw.py(i) = lay.cy + y * lay.ts
    drw.pv(i) = (y >= lay.r0)
  next i
  for i = 0 to 127
    drw.sx(i) = (i mod 16) * lay.ts
    drw.sy(i) = (i \ 16) * lay.ts
    drw.m63(i) = i and 63
  next i
end sub

' the whole scene on the back page: frame (dark steel + empty bar), every
' cell, the bar. Not shown yet (scr.present does that).
' scratch for the sub below (globals: LOCAL clears mmcore's fast-path cache)
dim da_i, da_y, da_x, da_ts
sub drw.all()
  da_ts = lay.ts
  drw.geom()
  drw.frameTiles()
  page copy PG_FRAME to PG_BACK
  page write PG_BACK
  for da_y = lay.r0 to CV_H - 1
    for da_x = 0 to CV_W - 1
      drw_i = da_y * CV_W + da_x
      drw_c = g(drw_i) and 63
      drw.tile()
      blit (drw_s mod 16) * da_ts, (drw_s \ 16) * da_ts, lay.cx + da_x * da_ts, lay.cy + da_y * da_ts, da_ts, da_ts, PG_SHEET
      dm(drw_i) = 0
    next da_x
  next da_y
  dqn = 0
  hud.paint()
end sub

' ---- status bar: diamonds got / needed with ten progress ticks, the cave,
' the seconds left and the lives. hud.dirty (set by the cave frame) asks the
' main loop for a redraw.
dim hud.dirty, hud.lives
dim string hud.cave$ length 12

' a number with at least two digits
function hud.p2$(n)
  hud.p2$ = str$(n)
  if n < 10 then hud.p2$ = "0" + str$(n)
end function

' paint the bar on the back page (UI art units)
' scratch for the sub below (globals: LOCAL clears mmcore's fast-path cache)
dim hp_y, hp_x, hp_i, hp_n, hp_st, hp_bh
dim string hp_s$
sub hud.paint()
  hp_bh = lay.bh \ scr.sc
  page write PG_BACK
  blit 0, 0, 0, 0, scr.W, lay.bh, PG_FRAME
  hp_y = lay.bty
  ui.spr(IC_GEM, IC_Y, 8, 8, 6, hp_y)
  hp_s$ = hud.p2$(cv.got)
  hp_st = F_GEM
  if cv.got >= cv.need then hp_st = F_GOLD
  txt hp_s$, 18, hp_y, hp_st
  txt "/", 18 + txtW(hp_s$), hp_y, F_GREY
  txt hud.p2$(cv.need), 26 + txtW(hp_s$), hp_y, F_WHITE
  hp_x = 50 + txtW(hp_s$)
  hp_n = min(10, (cv.got * 10) \ cv.need)
  for hp_i = 0 to 9
    if hp_i < hp_n then
      ui.spr(IC_TICK, IC_Y + 2, 3, 4, hp_x + hp_i * 4, hp_y + 2)
    else
      ui.spr(IC_TICK0, IC_Y + 2, 3, 4, hp_x + hp_i * 4, hp_y + 2)
    end if
  next hp_i
  txtC hud.cave$, lay.aw \ 2, hp_y, F_GOLD
  hp_x = lay.aw - 8 - hud.lives * 10
  for hp_i = 0 to hud.lives - 1
    ui.spr(IC_LIFE, IC_Y, 8, 8, hp_x + hp_i * 10, hp_y)
  next hp_i
  hp_st = F_WHITE
  if cv.left < 20 then hp_st = F_WARM
  txt "TIME", hp_x - 76, hp_y, F_GREY
  txtR str$(max(0, cv.left)), hp_x - 12, hp_y, hp_st
  hud.dirty = FALSE
end sub

' repaint the bar and copy it to the screen
sub hud.draw()
  hud.paint()
  page write 0
  blit 0, 0, 0, 0, scr.W, lay.bh, PG_BACK
  page write PG_BACK
end sub
