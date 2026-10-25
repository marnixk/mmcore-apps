' screen.inc - pick a screen mode + asset set from the current mode and
' resolution, load the art into off-screen pages, restore the mode on quit.
'
' Nothing is ever drawn on the visible page 0. Every frame is built on the
' back page 1 (PAGE WRITE 1 stays selected). A scene change is shown with one
' PAGE COPY 1 TO 0 (scr.present). During play the moving things are SPRITEs on
' the back page over the static backdrop; after their one SPRITE MOVE per frame
' only the changed rectangles (scr.mark) are blitted across, opaque and whole
' (scr.flush): a full-height strip per moving column and the ship's box, far
' cheaper on a Pi than copying the full screen every frame.
'
' Pages:  0 display      1 back page (the frame being built)
'         2 clean playfield backdrop (static nebula + stars)
'         3 sprite sheet (sprites, small + big font, dither tile)
'         4 obstacle columns: one pre-rendered column per obstacle slot, then
'           the crystal and the cue texts (sources for SPRITE READ)
'         5 title scene (also holds the paused game scene; reloaded after play)
'         6 high-score scene
'         7 dim overlay (checker of dark ink, black = see-through)

const PG_BACK = 1, PG_BG = 2, PG_SHEET = 3, PG_COLS = 4, PG_INTRO = 5, PG_SCORES = 6, PG_DIM = 7
dim scr.dirty = 0

' changed rectangles waiting for scr.flush; scr.full = copy the whole page
const DR_MAX = 20
dim dr.x(DR_MAX - 1), dr.y(DR_MAX - 1), dr.w(DR_MAX - 1), dr.h(DR_MAX - 1)
dim dr.n = 0, scr.full = 0

dim float scr.oldMode
dim scr.W, scr.H, scr.sc, scr.lay

' current layout (filled by lay.set() in layouts.inc; panel numbers in art units)
dim lay.mode, lay.W, lay.H, lay.sc, lay.aw, lay.ah
dim lay.mx, lay.my, lay.mw, lay.mh, lay.sx, lay.sy, lay.sw, lay.sh, lay.sstep
dim string lay.res$

' clip rectangle for bl() in screen pixels (x0 <= x < x1)
dim cl.x0, cl.y0, cl.x1, cl.y1

' choose the layout for a w x h screen:
'  - a size we have art for: that layout (MODE 20 keeps MODE 20 and uses the x1
'    art natively; the Chromebook panel doubles every pixel)
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
  cl.all()
end sub

sub scr.loadAssets()
  local x, y, s, t
  s = scr.sc
  page write PG_SHEET
  cls rgb(0, 0, 0)
  load png "gfx/sheet_" + lay.res$ + ".png", 0, 0
  page write PG_BG
  load png "gfx/play_" + lay.res$ + ".png", 0, 0
  page write PG_INTRO
  load png "gfx/intro_" + lay.res$ + ".png", 0, 0
  page write PG_SCORES
  load png "gfx/scores_" + lay.res$ + ".png", 0, 0
  page write PG_COLS
  cls rgb(0, 0, 0)
  ' dim overlay: the dither tile repeated (an even art step keeps it seamless)
  page write PG_DIM
  cls rgb(0, 0, 0)
  t = SP_DITHER_S * s
  for y = 0 to scr.H - 1 step t
    for x = 0 to scr.W - 1 step t
      blit SP_DITHER_X * s, SP_DITHER_Y * s, x, y, min(t, scr.W - x), min(t, scr.H - y), PG_SHEET
    next x
  next y
  page write PG_BACK
end sub

' ---- clipped blit: the current write page gets source page pg's rectangle,
' cut to the clip rectangle; tr = 1 skips black (transparent) pixels
sub cl.all()
  cl.x0 = 0
  cl.y0 = 0
  cl.x1 = scr.W
  cl.y1 = scr.H
end sub

sub cl.set(x0, y0, x1, y1)
  cl.x0 = max(0, x0)
  cl.y0 = max(0, y0)
  cl.x1 = min(scr.W, x1)
  cl.y1 = min(scr.H, y1)
end sub

sub bl(sx0, sy0, dx0, dy0, w0, h0, pg, tr)
  ' works on copies (MMBasic passes variables by reference). The copies are
  ' assigned on their own lines: "local x = x0" picked up a wrong value in
  ' mmcore here (see the report), plain assignments are fine.
  local d, sx, sy, dx, dy, w, h
  sx = sx0
  sy = sy0
  dx = dx0
  dy = dy0
  w = w0
  h = h0
  if dx < cl.x0 then
    d = cl.x0 - dx
    sx = sx + d
    w = w - d
    dx = cl.x0
  end if
  if dy < cl.y0 then
    d = cl.y0 - dy
    sy = sy + d
    h = h - d
    dy = cl.y0
  end if
  if dx + w > cl.x1 then w = cl.x1 - dx
  if dy + h > cl.y1 then h = cl.y1 - dy
  if w <= 0 or h <= 0 then exit sub
  if tr then
    blit sx, sy, dx, dy, w, h, pg, 4
  else
    blit sx, sy, dx, dy, w, h, pg
  end if
end sub

' the title scene page doubles as the pause save area during a game
sub scr.reloadIntro()
  page write PG_INTRO
  load png "gfx/intro_" + lay.res$ + ".png", 0, 0
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
  local x, y, w, h
  x = x0
  y = y0
  w = w0
  h = h0
  if x < 0 then
    w = w + x
    x = 0
  end if
  if y < 0 then
    h = h + y
    y = 0
  end if
  if x + w > scr.W then w = scr.W - x
  if y + h > scr.H then h = scr.H - y
  if w <= 0 or h <= 0 then exit sub
  scr.dirty = TRUE
  if scr.full then exit sub
  if dr.n >= DR_MAX then
    scr.full = TRUE
    exit sub
  end if
  dr.x(dr.n) = x
  dr.y(dr.n) = y
  dr.w(dr.n) = w
  dr.h(dr.n) = h
  dr.n = dr.n + 1
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
