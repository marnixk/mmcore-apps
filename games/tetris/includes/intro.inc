' intro.inc - title screen with the main menu
' Up/Down pick a row, Left/Right change the difficulty (from any row),
' Enter selects, Esc jumps to QUIT.

const MENU_START = 0, MENU_LEVEL = 1, MENU_SCORES = 2, MENU_QUIT = 3
dim menu.level = LV_MEDIUM
dim menu.row = MENU_START

function intro.rowY(r)
  local s = scr.sc
  if r = MENU_START then intro.rowY = lay.my + 16 * s
  if r = MENU_LEVEL then intro.rowY = lay.my + 38 * s
  if r = MENU_SCORES then intro.rowY = lay.my + 98 * s
  if r = MENU_QUIT then intro.rowY = lay.my + 118 * s
end function

function intro.lvColour(lv)
  if lv = LV_EASY then intro.lvColour = F_GREEN
  if lv = LV_MEDIUM then intro.lvColour = F_GOLD
  if lv = LV_HARD then intro.lvColour = F_RED
end function

' menu panel interior + prompt panel, drawn on PG_BACK
sub intro.drawMenu()
  local s = scr.sc, r, y, i, c, t$, mn = lay.mini
  page write PG_BACK
  blit lay.mx, lay.my, lay.mx, lay.my, lay.mw, lay.mh, PG_INTRO
  for r = MENU_START to MENU_QUIT
    y = intro.rowY(r)
    if r = MENU_START then t$ = "START GAME"
    if r = MENU_LEVEL then t$ = "DIFFICULTY"
    if r = MENU_SCORES then t$ = "HIGH SCORES"
    if r = MENU_QUIT then t$ = "QUIT"
    if r = menu.row then
      c = F_GOLD
    else
      c = F_CREAM
    end if
    txt t$, lay.mx + 40 * s, y, c
    if r = menu.row then
      blit 2 * mn, lay.miniY, lay.mx + 18 * s, y - s, mn, mn, PG_SHEET
    end if
  next r
  for i = 0 to 2
    ui.pill(lay.mx + 20 * s + i * 72 * s, lay.my + 54 * s, 66 * s, 16 * s, lv.name$(i), i = menu.level, intro.lvColour(i))
  next i
  txtC "LEVEL UP EVERY " + str$(lv.lines(menu.level)) + " LINES", lay.mx + lay.mw \ 2, lay.my + 76 * s, F_GREY
  ' prompt panel
  blit lay.px, lay.py, lay.px, lay.py, lay.pw, lay.ph, PG_INTRO
  txtC "PRESS ENTER TO START", lay.px + lay.pw \ 2, lay.py + 11 * s, F_GOLD
  if hs.score(menu.level, 0) > 0 then
    t$ = "BEST " + lv.name$(menu.level) + "  " + str$(hs.score(menu.level, 0)) + "  " + hs.name$(menu.level, 0)
    txtC t$, lay.px + lay.pw \ 2, lay.py + 27 * s, F_CREAM
  else
    txtC "NO " + lv.name$(menu.level) + " SCORES YET", lay.px + lay.pw \ 2, lay.py + 27 * s, F_GREY
  end if
end sub

sub intro.show()
  scr.show(lay.mx, lay.my, lay.mw, lay.mh)
  scr.show(lay.px, lay.py, lay.pw, lay.ph)
end sub

' returns MENU_START, MENU_SCORES or MENU_QUIT. One key press = one move; the
' menu panel is redrawn on the back page and only that panel (and the prompt
' below it) is copied to the screen.
function intro.run()
  local k$, old, ol
  page copy PG_INTRO to PG_BACK
  intro.drawMenu()
  scr.present()
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      old = menu.row
      ol = menu.level
      if k$ = chr$(K_UP) then menu.row = (menu.row + 3) mod 4
      if k$ = chr$(K_DOWN) then menu.row = (menu.row + 1) mod 4
      if k$ = chr$(K_LEFT) then menu.level = max(LV_EASY, menu.level - 1)
      if k$ = chr$(K_RIGHT) then menu.level = min(LV_HARD, menu.level + 1)
      if k$ = chr$(K_ESC) or key.isQuit(k$) then menu.row = MENU_QUIT
      if key.isEnter(k$) then
        intro.run = menu.row
        if menu.row = MENU_LEVEL then intro.run = MENU_START
        exit do
      end if
      if menu.row <> old or menu.level <> ol then
        intro.drawMenu()
        intro.show()
      end if
    end if
  loop
end function
