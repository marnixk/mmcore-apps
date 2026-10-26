' screen.inc - mode, pages, assets, the back page and changed-area copies
'
' Pages: 0 = the visible screen (only ever copied to, never drawn on),
'  PG_BACK   the back page where every frame is built
'  PG_SAVE   a copy of the play scene while a dialog covers it
'  PG_SHEET  the tiles (pre-composited, lay.ts px, 16 per row)
'  PG_TITLE / PG_SCORES  the title and high-score backdrops
'  PG_DIM    a dark checker for dimming the scene behind dialogs
'  PG_UI     the empty side panel at x 0 and the UI block (fonts, icons,
'            dither, name box) right of it at art x UI_X
' hot-path helpers keep their scratch in globals: LOCAL costs O(variables)
' per call in mmcore (each LOCAL scans the whole variable table)
dim integer scr_mark_x, scr_mark_y, scr_mark_w, scr_mark_h, scr_i

const PG_BACK = 1, PG_SAVE = 2, PG_SHEET = 3, PG_TITLE = 4, PG_SCORES = 5, PG_DIM = 6, PG_UI = 7
dim scr.dirty = 0

' changed rectangles waiting for scr.flush; scr.full = copy the whole page
const DR_MAX = 48
dim dr.x(DR_MAX - 1), dr.y(DR_MAX - 1), dr.w(DR_MAX - 1), dr.h(DR_MAX - 1)
dim dr.n = 0, scr.full = 0

dim float scr.oldMode
dim scr.W, scr.H, scr.sc, scr.lay

' current layout (filled by lay.set() in layouts.inc)
dim lay.mode, lay.W, lay.H, lay.sc, lay.ts, lay.vw, lay.vh, lay.px, lay.aw, lay.ah
dim lay.mx, lay.my, lay.mstep, lay.sx, lay.sy, lay.sstep
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

' PNGs go onto pages cleared to black: their see-through pixels stay black,
' and black is the see-through colour of BLIT ..., 4 (no sprite uses pure black)
sub scr.loadAssets()
  page write PG_SHEET
  cls rgb(0, 0, 0)
  load png "gfx/sheet_" + lay.res$ + ".png", 0, 0
  page write PG_UI
  cls rgb(0, 0, 0)
  load png "gfx/ui_" + lay.res$ + ".png", 0, 0
  page write PG_TITLE
  load png "gfx/title_" + lay.res$ + ".png", 0, 0
  page write PG_SCORES
  load png "gfx/scores_" + lay.res$ + ".png", 0, 0
  scr.makeDim()
  page write PG_BACK
  cls rgb(0, 0, 0)
end sub

' the dim overlay: the 8 x 8 dither tile from the UI block, doubled across
' the page (a handful of blits); black = see-through for PAGE COPY ..., B
sub scr.makeDim()
  local w, h, n, t
  t = 8 * scr.sc
  page write PG_DIM
  cls rgb(0, 0, 0)
  blit (UI_X + IC_DITHER) * scr.sc, IC_Y * scr.sc, 0, 0, t, t, PG_UI
  w = t
  do while w < scr.W
    n = min(w, scr.W - w)
    blit 0, 0, w, 0, n, t, PG_DIM
    w = w + n
  loop
  h = t
  do while h < scr.H
    n = min(h, scr.H - h)
    blit 0, 0, 0, h, scr.W, n, PG_DIM
    h = h + n
  loop
  page write PG_BACK
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

' the same in UI art units
sub scr.markA(x, y, w, h)
  scr.mark(x * scr.sc, y * scr.sc, w * scr.sc, h * scr.sc)
end sub

' show what changed: the marked rectangles, or the whole page after a redraw
sub scr.flush()
  if scr.full then
    scr.present()
    exit sub
  end if
  page write 0
  for scr_i = 0 to dr.n - 1
    blit dr.x(scr_i), dr.y(scr_i), dr.x(scr_i), dr.y(scr_i), dr.w(scr_i), dr.h(scr_i), PG_BACK
  next scr_i
  page write PG_BACK
  dr.n = 0
  scr.dirty = FALSE
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
