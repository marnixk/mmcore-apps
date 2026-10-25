' intro.inc - title screen with the main menu
' Up/Down pick a row, Left/Right change the difficulty (from any row), Enter
' selects (on DIFFICULTY it steps to the next skill), Esc / Q jump to QUIT.

const MENU_START = 0, MENU_DIFF = 1, MENU_SCORES = 2, MENU_QUIT = 3
dim menu.diff = DF_NORMAL
dim menu.row = MENU_START

' art y of menu row r (text row)
function intro.ty(r)
  if r = MENU_START then intro.ty = lay.my + 16
  if r = MENU_DIFF then intro.ty = lay.my + 38
  if r = MENU_SCORES then intro.ty = lay.my + 80
  if r = MENU_QUIT then intro.ty = lay.my + 100
end function

' draw one menu row on the back page (selected = bar + bullet + cyan text)
sub intro.row(r)
  local x, w, ty, t$
  x = lay.mx
  w = 236
  ty = intro.ty(r)
  t$ = "START GAME"
  if r = MENU_DIFF then t$ = "DIFFICULTY"
  if r = MENU_SCORES then t$ = "HIGH SCORES"
  if r = MENU_QUIT then t$ = "QUIT"
  scr.copyA(x + 10, ty - 5, w - 20, 17, PG_TITLE)
  if r = menu.row then
    ui.bar(x + 10, ty - 5, w - 20)
    ui.spr(8 * T_LIFE, SH_TILE_Y, 8, 8, x + 22, ty)
    txt t$, x + 42, ty, F_CYAN
  else
    txt t$, x + 42, ty, F_WHITE
  end if
  scr.markA(x + 10, ty - 5, w - 20, 17)
end sub

' the three difficulty pills
sub intro.pills()
  local i
  scr.copyA(lay.mx + 22, lay.my + 54, 192, 15, PG_TITLE)
  for i = 0 to 2
    ui.pill(lay.mx + 22 + i * 66, lay.my + 54, i, i = menu.diff)
  next i
  scr.markA(lay.mx + 22, lay.my + 54, 192, 15)
end sub

' whole title screen (once, on entry)
sub intro.draw()
  local r
  page copy PG_TITLE to PG_BACK
  page write PG_BACK
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
  key.drain()
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      old = menu.row
      od = menu.diff
      select case asc(k$)
        case K_UP
          menu.row = (menu.row + 3) mod 4
        case K_DOWN
          menu.row = (menu.row + 1) mod 4
        case K_LEFT
          menu.diff = max(DF_EASY, menu.diff - 1)
        case K_RIGHT
          menu.diff = min(DF_HARD, menu.diff + 1)
        case K_ESC, asc("q"), asc("Q")
          menu.row = MENU_QUIT
        case K_ENTER, K_LF
          if menu.row = MENU_DIFF then
            menu.diff = (menu.diff + 1) mod 3
          else
            intro.run = menu.row
            exit do
          end if
      end select
      if menu.row <> old then
        intro.row(old)
        intro.row(menu.row)
      end if
      if menu.diff <> od then intro.pills()
      if scr.dirty then scr.flush()
    end if
  loop
end function
