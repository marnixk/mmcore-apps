' screen.inc - pick a screen mode + asset set from the current mode and
' resolution, load the art into off-screen pages, restore the mode on quit.
'
' Nothing is ever drawn on the visible page 0. Every frame is built on the
' back page 1 (PAGE WRITE 1 stays selected). A scene change is shown with one
' PAGE COPY 1 TO 0 (scr.present). During play only the changed rectangles
' (scr.mark) are blitted across, opaque and whole (scr.flush): the boxes of the
' moving orbs, sentinels and player, the cells of the growing wall and the
' bounding box of a claim. Far cheaper on a Pi than full-screen copies.
'
' Pages:  0 display      1 back page (the frame being built)
'         2 the field as it is now (HUD, claimed land, open grid, wall), no actors:
'           the source that erases an actor's old place
'         3 open-grid plane, 4 claimed-land plane (the whole field in one tile
'           kind each: a run of cells is redrawn with one blit)
'         5 sprite sheet (actors, tiles, wall pieces, edge strips, font, pills)
'         6 title scene
'         7 high-score scene in the menus, dim overlay (checker of dark ink,
'           black = see-through) during a game
' hot-path helpers keep their scratch in globals: LOCAL costs O(variables)
' per call in mmcore (each LOCAL scans the whole variable table)
dim integer scr_mark_x, scr_mark_y, scr_mark_w, scr_mark_h, scr_copyA_s, scr_copyA_x, scr_copyA_y, scr_copyA_w, scr_copyA_h

const PG_BACK = 1, PG_FIELD = 2, PG_OPEN = 3, PG_CLAIM = 4, PG_SHEET = 5, PG_TITLE = 6, PG_P7 = 7
const P7_NONE = 0, P7_SCORES = 1, P7_DIM = 2
dim scr.p7 = P7_NONE
dim scr.dirty = 0

' changed rectangles waiting for scr.flush; scr.full = copy the whole page
const DR_MAX = 32
dim dr.x(DR_MAX - 1), dr.y(DR_MAX - 1), dr.w(DR_MAX - 1), dr.h(DR_MAX - 1)
dim dr.n = 0, scr.full = 0

dim float scr.oldMode
dim scr.W, scr.H, scr.sc, scr.lay

' current layout (filled by lay.set() in layouts.inc; art units)
dim lay.mode, lay.W, lay.H, lay.sc, lay.aw, lay.ah, lay.fw, lay.fh, lay.ox, lay.oy
dim lay.mx, lay.my, lay.hbx, lay.hbw, lay.sx, lay.sy, lay.sw, lay.sh, lay.sstep
dim string lay.res$

' choose the layout for a w x h screen:
'  - a size we have art for: that layout
'  - anything else: the smallest layout at least as big as the screen, or the
'    biggest one. MODE retunes the display, so the game always fills the screen.
function scr.pickLayout(w, h)
  local i, best, bw
  best = -1
  bw = 99999
  for i = 0 to LAY_N - 1
    if lay.ws(i) = w and lay.hs(i) = h then
      scr.pickLayout = i
      exit function
    end if
  next i
  for i = 0 to LAY_N - 1
    if lay.ws(i) >= w and lay.hs(i) >= h and lay.ws(i) < bw then
      best = i
      bw = lay.ws(i)
    end if
  next i
  if best < 0 then
    bw = 0
    for i = 0 to LAY_N - 1
      if lay.ws(i) > bw then
        best = i
        bw = lay.ws(i)
      end if
    next i
  end if
  scr.pickLayout = best
end function

sub scr.init()
  local i, cm, bits
  scr.oldMode = MM.INFO(MODE)
  cm = int(scr.oldMode)
  bits = cint((scr.oldMode - cm) * 100)
  i = scr.pickLayout(MM.HRES, MM.VRES)
  scr.lay = i
  lay.set(i)
  ' the art needs at least 16-bit colour
  if bits < 16 then bits = 16
  mode lay.mode, bits
  locate , , 0
  scr.W = MM.HRES
  scr.H = MM.VRES
  scr.sc = lay.sc
end sub

sub scr.loadAssets()
  page write PG_SHEET
  cls rgb(0, 0, 0)
  load png "gfx/sheet_" + lay.res$ + ".png", 0, 0
  page write PG_TITLE
  load png "gfx/title_" + lay.res$ + ".png", 0, 0
  scr.useScores()
  page write PG_BACK
end sub

' page 7 holds the high-score scene in the menus ...
sub scr.useScores()
  if scr.p7 = P7_SCORES then exit sub
  page write PG_P7
  load png "gfx/scores_" + lay.res$ + ".png", 0, 0
  page write PG_BACK
  scr.p7 = P7_SCORES
end sub

' ... and the dim overlay during a game: the 8 x 8 art dither tile from the
' sheet, doubled across the page (a handful of blits)
sub scr.useDim()
  local w, h, n, t
  if scr.p7 = P7_DIM then exit sub
  t = 8 * scr.sc
  page write PG_P7
  cls rgb(0, 0, 0)
  blit SH_DITHER_X * scr.sc, SH_DITHER_Y * scr.sc, 0, 0, t, t, PG_SHEET
  w = t
  do while w < scr.W
    n = min(w, scr.W - w)
    blit 0, 0, w, 0, n, t, PG_P7
    w = w + n
  loop
  h = t
  do while h < scr.H
    n = min(h, scr.H - h)
    blit 0, 0, 0, h, scr.W, n, PG_P7
    h = h + n
  loop
  page write PG_BACK
  scr.p7 = P7_DIM
end sub

' show the finished back page (one opaque PAGE COPY = one whole frame)
sub scr.present()
  page copy PG_BACK to 0
  page write PG_BACK
  scr.dirty = FALSE
  scr.full = FALSE
  dr.n = 0
end sub

' note a changed back-page rectangle (screen px); too many = one full copy
sub scr.mark(x0, y0, w0, h0)
  scr_mark_x = x0
  scr_mark_y = y0
  scr_mark_w = w0
  scr_mark_h = h0
  if scr_mark_x < 0 then
    scr_mark_w = scr_mark_w + scr_mark_x
    scr_mark_x = 0
  end if
  if scr_mark_y < 0 then
    scr_mark_h = scr_mark_h + scr_mark_y
    scr_mark_y = 0
  end if
  if scr_mark_x + scr_mark_w > scr.W then scr_mark_w = scr.W - scr_mark_x
  if scr_mark_y + scr_mark_h > scr.H then scr_mark_h = scr.H - scr_mark_y
  if scr_mark_w <= 0 or scr_mark_h <= 0 then exit sub
  scr.dirty = TRUE
  if scr.full then exit sub
  if dr.n >= DR_MAX then
    scr.full = TRUE
    exit sub
  end if
  dr.x(dr.n) = scr_mark_x
  dr.y(dr.n) = scr_mark_y
  dr.w(dr.n) = scr_mark_w
  dr.h(dr.n) = scr_mark_h
  dr.n = dr.n + 1
end sub

' the same in art units
sub scr.markA(x, y, w, h)
  scr.mark(x * scr.sc, y * scr.sc, w * scr.sc, h * scr.sc)
end sub

sub scr.markAll()
  scr.dirty = TRUE
  scr.full = TRUE
end sub

' show what changed: the marked rectangles, or the whole page after a redraw
sub scr.flush()
  local i
  if scr.full then
    scr.present()
    exit sub
  end if
  page write 0
  for i = 0 to dr.n - 1
    blit dr.x(i), dr.y(i), dr.x(i), dr.y(i), dr.w(i), dr.h(i), PG_BACK
  next i
  page write PG_BACK
  dr.n = 0
  scr.dirty = FALSE
end sub

' copy an art rectangle from page pg onto the back page (clipped to the screen)
sub scr.copyA(x0, y0, w0, h0, pg)
  scr_copyA_s = scr.sc
  scr_copyA_x = x0 * scr_copyA_s
  scr_copyA_y = y0 * scr_copyA_s
  scr_copyA_w = w0 * scr_copyA_s
  scr_copyA_h = h0 * scr_copyA_s
  if scr_copyA_x < 0 then
    scr_copyA_w = scr_copyA_w + scr_copyA_x
    scr_copyA_x = 0
  end if
  if scr_copyA_y < 0 then
    scr_copyA_h = scr_copyA_h + scr_copyA_y
    scr_copyA_y = 0
  end if
  if scr_copyA_x + scr_copyA_w > scr.W then scr_copyA_w = scr.W - scr_copyA_x
  if scr_copyA_y + scr_copyA_h > scr.H then scr_copyA_h = scr.H - scr_copyA_y
  if scr_copyA_w <= 0 or scr_copyA_h <= 0 then exit sub
  blit scr_copyA_x, scr_copyA_y, scr_copyA_x, scr_copyA_y, scr_copyA_w, scr_copyA_h, pg
end sub

sub scr.restore()
  local m, b
  m = int(scr.oldMode)
  b = cint((scr.oldMode - m) * 100)
  page write 0
  if b = 8 or b = 12 or b = 16 or b = 32 then
    mode m, b
  else
    mode m
  end if
end sub
