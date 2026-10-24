' screen.inc - pick a screen mode + asset set from the current mode and
' resolution, load the art into off-screen pages, restore the mode on quit.
'
' Page flipping: nothing is ever drawn on the visible page 0. Every frame is
' built on the back page 1 (PAGE WRITE 1 stays selected) and shown in one go with
' PAGE COPY 1 TO 0 (scr.present), so a half-drawn frame can never be seen.
'
' Pages:  0 display      1 back page (the frame being built)
'         2 intro scene  3 high-score scene
'         4 clean level (floor, walls)   5 sprite sheet (tiles, small + big font)
'         6 dim overlay (checker of dark ink, black = see-through)
'         7 the game scene kept while a banner or dialog covers the back page

const PG_BACK = 1, PG_INTRO = 2, PG_SCORES = 3, PG_FIELD = 4, PG_SHEET = 5, PG_DIM = 6, PG_SAVE = 7
dim scr.dirty = 0

dim float scr.oldMode
dim scr.W, scr.H, scr.sc, scr.lay

' current layout (filled by lay.set() in layouts.inc; screen pixels)
dim lay.mode, lay.W, lay.H, lay.sc, lay.cell, lay.ox, lay.smallY, lay.bigY
dim lay.mx, lay.my, lay.mw, lay.mh, lay.sx, lay.sy, lay.sw, lay.sh, lay.sstep
dim string lay.res$

' choose the layout for a w x h screen:
'  - a size we have art for: that layout (MODE 20 keeps MODE 20 and uses the x1
'    art natively; the Chromebook panel doubles every pixel)
'  - anything else: the smallest layout at least as big as the screen, or the
'    biggest one. MODE retunes the display, so the game always fills the screen.
function scr.pickLayout(w, h)
  local i, best = -1, bw = 99999
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
  local x, y, c = lay.cell, st
  page write PG_SHEET
  cls rgb(0, 0, 0)
  load png "gfx/sheet_" + lay.res$ + ".png", 0, 0
  page write PG_INTRO
  load png "gfx/intro_" + lay.res$ + ".png", 0, 0
  page write PG_SCORES
  load png "gfx/scores_" + lay.res$ + ".png", 0, 0
  ' dim overlay: the dither tile repeated at an even step so the checker
  ' stays seamless for odd cell sizes too
  page write PG_DIM
  cls rgb(0, 0, 0)
  st = c - (c mod 2)
  for y = 0 to scr.H - 1 step st
    for x = 0 to scr.W - 1 step st
      blit (T_DITHER mod 16) * c, (T_DITHER \ 16) * c, x, y, min(c, scr.W - x), min(c, scr.H - y), PG_SHEET
    next x
  next y
  page write PG_BACK
end sub

' show the finished back page (one opaque PAGE COPY = one whole frame)
sub scr.present()
  page copy PG_BACK to 0
  page write PG_BACK
  scr.dirty = FALSE
end sub

' keep the game scene aside while a banner or dialog is drawn over it
sub scr.save()
  page copy PG_BACK to PG_SAVE
  page write PG_BACK
end sub

sub scr.unsave()
  page copy PG_SAVE to PG_BACK
  scr.present()
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
