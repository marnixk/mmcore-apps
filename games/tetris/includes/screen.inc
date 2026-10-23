' screen.inc - pick a screen mode + asset set from the current mode and
' resolution, load the art into off-screen pages, restore the mode on quit.
'
' Pages:  0 display     1 sprite sheet (fonts, tiles, mini tiles, arrows)
'         2 intro scene 3 high-score scene  4 dimmed game scene (pause/dialogs)
'         5 clean game scene (backdrop, frame, empty well, empty panels)
'         6 live game scene (5 + locked stack + HUD values)   7 back buffer

const PG_SHEET = 1, PG_INTRO = 2, PG_SCORES = 3, PG_DIM = 4, PG_GAME = 5, PG_STACK = 6, PG_BACK = 7

dim float scr.oldMode
dim scr.W, scr.H, scr.sc, scr.lay

' current layout (filled by lay.set() in layouts.inc; all in screen pixels)
dim lay.mode, lay.W, lay.H, lay.sc, lay.cell, lay.mini, lay.wx, lay.wy
dim lay.holdX, lay.holdY, lay.slotW, lay.slotH, lay.nextX, lay.nextY, lay.nextStep
dim lay.stX, lay.stY, lay.stW, lay.stH, lay.diffX, lay.diffY, lay.diffW, lay.diffH
dim lay.valR, lay.valY0, lay.valStep, lay.bestY
dim lay.fontY, lay.bigY, lay.tileY, lay.miniY, lay.arrowX
dim lay.tTitle, lay.mx, lay.my, lay.mw, lay.mh, lay.px, lay.py, lay.pw, lay.ph
dim lay.sx0, lay.spw, lay.sgap, lay.sy0, lay.sstep, lay.slv
dim string lay.res$, lay.sheet$

' choose the layout for a w x h screen:
'  - a size we have art for: that layout (MODE 20 keeps MODE 20 and uses the x1
'    art natively: the Chromebook panel doubles every pixel, so it looks exactly
'    like MODE 19 with the x2 art)
'  - anything else: the same-shaped (wide or 4:3) layout that is the smallest
'    one at least as big as the screen, or the biggest one if the screen is
'    larger than all of them. MODE retunes the display, so it fills the screen.
function scr.pickLayout(w, h)
  local i, wide, best, bw, lw
  for i = 0 to LAY_N - 1
    if lay.ws(i) = w and lay.hs(i) = h then
      scr.pickLayout = i
      exit function
    end if
  next i
  wide = (w * 10 >= h * 15)
  best = -1
  bw = 99999
  for i = 0 to LAY_N - 1
    lw = (lay.ws(i) * 10 >= lay.hs(i) * 15)
    if lw = wide and lay.ws(i) >= w and lay.hs(i) >= h and lay.ws(i) < bw then
      best = i
      bw = lay.ws(i)
    end if
  next i
  if best < 0 then
    bw = 0
    for i = 0 to LAY_N - 1
      lw = (lay.ws(i) * 10 >= lay.hs(i) * 15)
      if lw = wide and lay.ws(i) > bw then
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
  load png "gfx/sheet_" + lay.sheet$ + ".png", 0, 0
  page write PG_INTRO
  load png "gfx/intro_" + lay.res$ + ".png", 0, 0
  page write PG_SCORES
  load png "gfx/scores_" + lay.res$ + ".png", 0, 0
  page write PG_DIM
  load png "gfx/dim_" + lay.res$ + ".png", 0, 0
  page write PG_GAME
  load png "gfx/game_" + lay.res$ + ".png", 0, 0
  page write 0
end sub

' copy the back buffer to the display
sub scr.present()
  page copy PG_BACK to 0
  page write 0
end sub

' copy one rectangle of the back buffer to the display
sub scr.show(x, y, w, h)
  page write 0
  blit x, y, x, y, w, h, PG_BACK
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
  cls
end sub
