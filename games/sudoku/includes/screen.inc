' screen.inc - pick a screen mode + asset set from MM.HRES/MM.VRES, load the art
' into off-screen pages and keep the layout numbers in one place.
'
' Pages:  0 display      2 sprite sheet (tiles, digits, cursor, icons)
'         3 font sheet   4 panels (dialog, menu, side panel)
'         5 intro scene  6 game scene (forest + empty board)   7 back buffer

const PG_CLEAN = 1, PG_SHEET = 2, PG_FONT = 3, PG_PANELS = 4, PG_INTRO = 5, PG_GAME = 6, PG_BACK = 7

dim float scr.oldMode
dim scr.W, scr.H, scr.sc
dim string scr.res$, scr.sc$

' layout (pixels)
dim lay.cs, lay.B, lay.bx, lay.by, lay.px, lay.py, lay.pw, lay.ph
dim lay.off(8)
dim lay.curY, lay.iconY, lay.cur
dim lay.dlgW, lay.dlgH, lay.menuY, lay.menuW, lay.menuH, lay.panX

' layouts we have art for (index 6 is a spare: mmcore evaluates IF conditions
' inside skipped blocks, so a loop variable that ran past 5 must stay in range)
dim scr.mm(6) = (19, 11, 10, 9, 1, 8, 0)
dim scr.mw(6) = (1366, 1280, 848, 1024, 800, 640, 0)
dim scr.mh(6) = (768, 720, 480, 768, 600, 480, 0)
dim scr.mwide(6) = (1, 1, 1, 0, 0, 0, 9)

' choose the MODE to play in from the current mode and resolution:
'  - MODE 19 / MODE 20 (Chromebook panel 1366x768 and its exact half 683x384)
'    both play in MODE 19, the panel's native resolution (as AFK does), so the
'    board fills the screen with the crisp x3 art; quitting restores MODE 20
'  - a resolution we have art for keeps that mode
'  - anything else: the same-shaped (wide or 4:3) layout that is the smallest
'    one at least as big as the screen, or the biggest one if the screen is
'    larger than all of them. MODE retunes the display, so it fills the screen.
function scr.pickMode(w, h, cm)
  local i, wide, best, bw
  if cm = 19 or cm = 20 then
    scr.pickMode = 19
    exit function
  end if
  if w = 683 and h = 384 then
    scr.pickMode = 19
    exit function
  end if
  for i = 0 to 5
    if scr.mw(i) = w and scr.mh(i) = h then
      scr.pickMode = scr.mm(i)
      exit function
    end if
  next i
  wide = 0
  if w * 10 >= h * 15 then wide = 1
  best = -1
  bw = 99999
  for i = 0 to 5
    if scr.mwide(i) = wide and scr.mw(i) >= w and scr.mh(i) >= h and scr.mw(i) < bw then
      best = i
      bw = scr.mw(i)
    end if
  next i
  if best < 0 then
    bw = 0
    for i = 0 to 5
      if scr.mwide(i) = wide and scr.mw(i) > bw then
        best = i
        bw = scr.mw(i)
      end if
    next i
  end if
  scr.pickMode = scr.mm(best)
end function

sub scr.init()
  local w, h, m, cm
  scr.oldMode = MM.INFO("MODE")
  cm = int(scr.oldMode)
  w = MM.HRES
  h = MM.VRES
  m = scr.pickMode(w, h, cm)
  mode m, 16
  locate , , 0
  scr.W = MM.HRES
  scr.H = MM.VRES
  if scr.H >= 720 then scr.sc = 3 else scr.sc = 2
  scr.res$ = str$(scr.W) + "x" + str$(scr.H)
  scr.sc$ = str$(scr.sc)

  ' board + side panel layout (art units * scale)
  local i, sc = scr.sc
  local arts(8) = (12, 33, 54, 76, 97, 118, 140, 161, 182)
  lay.cs = 20 * sc
  lay.B = 214 * sc
  lay.pw = 88 * sc
  lay.ph = 124 * sc
  lay.bx = (scr.W - (lay.B + 12 * sc + lay.pw)) \ 2
  lay.by = (scr.H - lay.B) \ 2
  lay.px = lay.bx + lay.B + 12 * sc
  lay.py = lay.by + (lay.B - lay.ph) \ 2
  for i = 0 to 8
  lay.off(i) = arts(i) * sc
  next i
  ' sprite sheet rows on PG_SHEET
  lay.cur = lay.cs + 2 * sc
  lay.curY = 4 * lay.cs
  lay.iconY = lay.curY + lay.cur
  ' panels on PG_PANELS
  lay.dlgW = 190 * sc
  lay.dlgH = 140 * sc
  lay.menuY = lay.dlgH
  lay.menuW = 150 * sc
  lay.menuH = 84 * sc
  lay.panX = lay.dlgW
end sub

sub scr.loadAssets()
  local sc = scr.sc, tw, ty, t$
  page write PG_SHEET
  cls rgb(0, 0, 0)
  load png "gfx/tiles_x" + scr.sc$ + ".png", 0, 0
  load png "gfx/digits_x" + scr.sc$ + ".png", 0, lay.cs
  load png "gfx/cursor_x" + scr.sc$ + ".png", 0, lay.curY
  load png "gfx/icons_x" + scr.sc$ + ".png", 0, lay.iconY

  page write PG_FONT
  cls rgb(0, 0, 0)
  load png "gfx/font_x" + scr.sc$ + ".png", 0, 0

  page write PG_PANELS
  cls rgb(0, 0, 0)
  load png "gfx/dialog_x" + scr.sc$ + ".png", 0, 0
  load png "gfx/menu_x" + scr.sc$ + ".png", 0, lay.menuY
  load png "gfx/panel_x" + scr.sc$ + ".png", lay.panX, 0

  ' intro scene: forest clearing + logo
  page write PG_CLEAN
  load png "gfx/intro_" + scr.res$ + ".png", 0, 0
  page write PG_INTRO
  page copy PG_CLEAN to PG_INTRO
  if scr.W < 1000 then
    t$ = "gfx/title_x3.png"
    tw = 417
  else
    t$ = "gfx/title_x4.png"
    tw = 556
  end if
  intro.titleY = scr.H * 6 \ 100
  load png t$, (scr.W - tw) \ 2, intro.titleY
  if scr.W < 1000 then intro.titleH = 153 else intro.titleH = 204

  ' game scene: forest + empty board
  page write PG_GAME
  load png "gfx/bg_" + scr.res$ + ".png", 0, 0
  load png "gfx/board_x" + scr.sc$ + ".png", lay.bx, lay.by

  page write 0
end sub

' copy the back buffer to the display
sub scr.present()
  page copy PG_BACK to 0
  page write 0
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
