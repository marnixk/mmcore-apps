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

' draw the queued cells: back page, then the same cells to the screen
sub drw.cells()
  local n
  n = dqn
  if n = 0 then exit sub
  drw_ts = lay.ts
  page write PG_BACK
  for drw_k = 0 to n - 1
    drw_i = dq(drw_k)
    drw_c = g(drw_i) and 63
    drw.tile()
    drw_y = drw_i \ CV_W
    if drw_y >= lay.r0 then
      drw_x = lay.cx + (drw_i - drw_y * CV_W) * drw_ts
      drw_y = lay.cy + drw_y * drw_ts
      blit (drw_s mod 16) * drw_ts, (drw_s \ 16) * drw_ts, drw_x, drw_y, drw_ts, drw_ts, PG_SHEET
    end if
  next drw_k
  page write 0
  for drw_k = 0 to n - 1
    drw_i = dq(drw_k)
    dm(drw_i) = 0
    drw_y = drw_i \ CV_W
    if drw_y >= lay.r0 then
      drw_x = lay.cx + (drw_i - drw_y * CV_W) * drw_ts
      drw_y = lay.cy + drw_y * drw_ts
      blit drw_x, drw_y, drw_x, drw_y, drw_ts, drw_ts, PG_BACK
    end if
  next drw_k
  page write PG_BACK
  dqn = 0
end sub

' the whole scene on the back page: frame (dark steel + empty bar), every
' cell, the bar. Not shown yet (scr.present does that).
sub drw.all()
  local i, y, x, ts
  ts = lay.ts
  page copy PG_FRAME to PG_BACK
  page write PG_BACK
  for y = lay.r0 to CV_H - 1
    for x = 0 to CV_W - 1
      drw_i = y * CV_W + x
      drw_c = g(drw_i) and 63
      drw.tile()
      blit (drw_s mod 16) * ts, (drw_s \ 16) * ts, lay.cx + x * ts, lay.cy + y * ts, ts, ts, PG_SHEET
      dm(drw_i) = 0
    next x
  next y
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
sub hud.paint()
  local y, x, i, n, s$, st, bh
  bh = lay.bh \ scr.sc
  page write PG_BACK
  blit 0, 0, 0, 0, scr.W, lay.bh, PG_FRAME
  y = lay.bty
  ui.spr(IC_GEM, IC_Y, 8, 8, 6, y)
  s$ = hud.p2$(cv.got)
  st = F_GEM
  if cv.got >= cv.need then st = F_GOLD
  txt s$, 18, y, st
  txt "/", 18 + txtW(s$), y, F_GREY
  txt hud.p2$(cv.need), 26 + txtW(s$), y, F_WHITE
  x = 50 + txtW(s$)
  n = min(10, (cv.got * 10) \ cv.need)
  for i = 0 to 9
    if i < n then
      ui.spr(IC_TICK, IC_Y + 2, 3, 4, x + i * 4, y + 2)
    else
      ui.spr(IC_TICK0, IC_Y + 2, 3, 4, x + i * 4, y + 2)
    end if
  next i
  txtC hud.cave$, lay.aw \ 2, y, F_GOLD
  x = lay.aw - 8 - hud.lives * 10
  for i = 0 to hud.lives - 1
    ui.spr(IC_LIFE, IC_Y, 8, 8, x + i * 10, y)
  next i
  st = F_WHITE
  if cv.left < 20 then st = F_WARM
  txt "TIME", x - 76, y, F_GREY
  txtR str$(max(0, cv.left)), x - 12, y, st
  hud.dirty = FALSE
end sub

' repaint the bar and copy it to the screen
sub hud.draw()
  hud.paint()
  page write 0
  blit 0, 0, 0, 0, scr.W, lay.bh, PG_BACK
  page write PG_BACK
end sub
