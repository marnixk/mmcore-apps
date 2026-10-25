' intro.inc - title screen with the main menu
' Up/Down pick a row, Left/Right change the difficulty (from any row),
' Enter selects, Esc / Q jump to QUIT.

const MENU_START = 0, MENU_DIFF = 1, MENU_SCORES = 2, MENU_QUIT = 3
dim menu.diff = DF_NORMAL
dim menu.row = MENU_START

' screen y of menu row r (text top)
function intro.ty(r)
  local y = lay.my, s = scr.sc
  if r = MENU_START then intro.ty = y + 16 * s
  if r = MENU_DIFF then intro.ty = y + 38 * s
  if r = MENU_SCORES then intro.ty = y + 80 * s
  if r = MENU_QUIT then intro.ty = y + 100 * s
end function

' draw one menu row on the back page (selected = bar + life tile + gold text)
sub intro.row(r)
  local s = scr.sc, x = lay.mx, w = lay.mw, ty, t$
  ty = intro.ty(r)
  t$ = "START GAME"
  if r = MENU_DIFF then t$ = "DIFFICULTY"
  if r = MENU_SCORES then t$ = "HIGH SCORES"
  if r = MENU_QUIT then t$ = "QUIT"
  rfill x + 10 * s, ty - 5 * s, w - 20 * s, 17 * s, C_K2
  if r = menu.row then
    rfill x + 10 * s, ty - 5 * s, w - 20 * s, 17 * s, C_K3
    ui.tile(T_LIFE, x + 18 * s, ty + 4 * s - lay.cell \ 2)
    txt t$, x + 40 * s, ty, F_GOLD
  else
    txt t$, x + 40 * s, ty, F_CREAM
  end if
  scr.mark(x + 10 * s, ty - 5 * s, w - 20 * s, 17 * s)
end sub

' the three difficulty pills
sub intro.pills()
  local s = scr.sc, i
  for i = 0 to 2
    ui.pill(lay.mx + (22 + i * 66) * s, lay.my + 54 * s, 60 * s, df.name$(i), i = menu.diff, df.row(i))
  next i
  scr.mark(lay.mx + 22 * s, lay.my + 54 * s, 192 * s, 15 * s)
end sub

' whole title screen (once, on entry)
sub intro.draw()
  local r
  page copy PG_INTRO to PG_BACK
  page write PG_BACK
  ui.panel(lay.mx, lay.my, lay.mw, lay.mh)
  for r = MENU_START to MENU_QUIT
    intro.row(r)
  next r
  intro.pills()
  scr.present()
end sub

' returns MENU_START, MENU_SCORES or MENU_QUIT. One key press = one move; only
' the rows / pills that changed are redrawn and copied to the screen.
function intro.run()
  local k$, old, od
  intro.draw()
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      old = menu.row
      od = menu.diff
      if k$ = chr$(K_UP) then menu.row = (menu.row + 3) mod 4
      if k$ = chr$(K_DOWN) then menu.row = (menu.row + 1) mod 4
      if k$ = chr$(K_LEFT) then menu.diff = max(DF_EASY, menu.diff - 1)
      if k$ = chr$(K_RIGHT) then menu.diff = min(DF_HARD, menu.diff + 1)
      if k$ = chr$(K_ESC) or key.isQuit(k$) then menu.row = MENU_QUIT
      if key.isEnter(k$) then
        intro.run = menu.row
        if menu.row = MENU_DIFF then intro.run = MENU_START
        exit do
      end if
      if menu.row <> old then
        intro.row(old)
        intro.row(menu.row)
      end if
      if menu.diff <> od then intro.pills()
      scr.flush()
    end if
  loop
end function
