' intro.inc - title screen with the main menu
' Up/Down pick a row, Left/Right change the difficulty (from any row),
' Enter selects, Esc / Q jump to QUIT.

const MENU_START = 0, MENU_DIFF = 1, MENU_SCORES = 2, MENU_QUIT = 3
dim menu.diff = DF_NORMAL
dim menu.row = MENU_START

sub intro.draw()
  local s = scr.sc, r, ty, t$, i, x = lay.mx, y = lay.my, w = lay.mw
  page copy PG_INTRO to PG_TMP
  page write PG_TMP
  ui.panel(x, y, w, lay.mh)
  for r = MENU_START to MENU_QUIT
    if r = MENU_START then
      t$ = "START GAME"
      ty = y + 16 * s
    elseif r = MENU_DIFF then
      t$ = "DIFFICULTY"
      ty = y + 38 * s
    elseif r = MENU_SCORES then
      t$ = "HIGH SCORES"
      ty = y + 80 * s
    else
      t$ = "QUIT"
      ty = y + 100 * s
    end if
    if r = menu.row then
      rfill x + 10 * s, ty - 5 * s, w - 20 * s, 17 * s, C_K3
      ui.tile(T_LIFE, x + 18 * s, ty + 4 * s - lay.cell \ 2)
      txt t$, x + 40 * s, ty, F_GOLD
    else
      txt t$, x + 40 * s, ty, F_CREAM
    end if
  next r
  for i = 0 to 2
    ui.pill(x + (22 + i * 66) * s, y + 54 * s, 60 * s, df.name$(i), i = menu.diff, df.row(i))
  next i
  scr.presentTmp()
end sub

' returns MENU_START, MENU_SCORES or MENU_QUIT
function intro.run()
  local k
  intro.draw()
  key.flush()
  do
    k = key.get()
    if k = 0 then
      pause 15
    else
      if k = K_UP then menu.row = (menu.row + 3) mod 4
      if k = K_DOWN then menu.row = (menu.row + 1) mod 4
      if k = K_LEFT then menu.diff = max(DF_EASY, menu.diff - 1)
      if k = K_RIGHT then menu.diff = min(DF_HARD, menu.diff + 1)
      if k = K_ESC or k = asc("q") or k = asc("Q") then menu.row = MENU_QUIT
      if key.isEnter(k) then
        if menu.row = MENU_DIFF then
          intro.run = MENU_START
        else
          intro.run = menu.row
        end if
        exit do
      end if
      intro.draw()
    end if
  loop
  key.flush()
end function
