' intro.inc - title screen with the forest clearing, logo, menu and fireflies
'
' Menu input is one simple INKEY$ loop: each key press moves the selection once
' (a held arrow repeats through INKEY$) and redraws just the rows that changed
' on the back buffer before copying those rows to the display. No full title
' compose and no sound on navigation.

const MENU_START = 0, MENU_LEVEL = 1, MENU_SCORES = 2, MENU_QUIT = 3
dim menu.level = LV_MEDIUM, menu.sel = 0
dim intro.titleY, intro.titleH, intro.mx, intro.my

' menu rows inside the panel: art-unit offsets from the panel top
const MENU_ROW0 = 12, MENU_ROWH = 15

const N_FLY = 18
dim fly.x(N_FLY - 1), fly.y(N_FLY - 1), fly.ph(N_FLY - 1)
dim float fly.dx(N_FLY - 1), fly.dy(N_FLY - 1), fly.fx(N_FLY - 1), fly.fy(N_FLY - 1)
dim fly.ready = FALSE
dim fly.ax, fly.ay, fly.aw, fly.ah   ' rectangle the fireflies keep away from

' label for menu row i; LEVEL shows the current level
function intro.item$(i)
  if i = MENU_START then intro.item$ = "START GAME"
  if i = MENU_LEVEL then intro.item$ = "LEVEL: < " + lv.name$(menu.level) + " >"
  if i = MENU_SCORES then intro.item$ = "HIGH SCORES"
  if i = MENU_QUIT then intro.item$ = "QUIT"
end function

function intro.rowY(i)
  intro.rowY = intro.my + (MENU_ROW0 + i * MENU_ROWH) * scr.sc
end function

' paint one menu row (clean panel slice, icon when selected, label) on the
' current write page. blitting the panel slice first erases the old highlight.
sub intro.row(i)
  local sc = scr.sc, y, r
  y = intro.rowY(i)
  blit 0, lay.menuY + (MENU_ROW0 + i * MENU_ROWH) * sc, intro.mx, y, lay.menuW, MENU_ROWH * sc, PG_PANELS, 4
  if i = menu.sel then
    r = F_GREEN
    dlg.icon(2, intro.mx + 11 * sc, y)
  else
    r = F_BROWN
  end if
  txt intro.item$(i), intro.mx + 22 * sc, y, r
end sub

sub intro.compose()
  local i
  page write PG_BACK
  page copy PG_INTRO to PG_BACK
  intro.mx = (scr.W - lay.menuW) \ 2
  intro.my = intro.titleY + intro.titleH + scr.H * 5 \ 100
  blit 0, lay.menuY, intro.mx, intro.my, lay.menuW, lay.menuH, PG_PANELS, 4
  fly.ax = intro.mx
  fly.ay = intro.my
  fly.aw = lay.menuW
  fly.ah = lay.menuH
  for i = 0 to 3
    intro.row(i)
  next i
  scr.present()
end sub

' redraw only the rows whose look changed: the row that lost the selection,
' the row that gained it, and the LEVEL row when the level changed. The rows
' are rebuilt off-screen on PG_BACK, then copied to the display with one
' small opaque BLIT each, so the title screen and the fireflies never flash.
sub intro.refresh(sel0, sel1, lvChanged)
  local i, y, draw
  page write PG_BACK
  for i = 0 to 3
    draw = (i = sel1)
    if i = sel0 and sel0 <> sel1 then draw = TRUE
    if i = MENU_LEVEL and lvChanged then draw = TRUE
    if draw then intro.row(i)
  next i
  page write 0
  for i = 0 to 3
    draw = (i = sel1)
    if i = sel0 and sel0 <> sel1 then draw = TRUE
    if i = MENU_LEVEL and lvChanged then draw = TRUE
    if draw then
      y = intro.rowY(i)
      blit intro.mx, y, intro.mx, y, lay.menuW, MENU_ROWH * scr.sc, PG_BACK
    end if
  next i
end sub

' little drifting fireflies; drawn straight onto the display, restored from
' the back buffer so nothing smears (a dot that did not move is left alone)
sub intro.flyInit(i)
  fly.fx(i) = rnd * scr.W
  fly.fy(i) = scr.H * (0.35 + rnd * 0.6)
  fly.dx(i) = (rnd - 0.5) * 0.8
  fly.dy(i) = (rnd - 0.5) * 0.5
  fly.ph(i) = int(rnd * 60)
  fly.x(i) = -100
  fly.y(i) = -100
end sub

sub intro.fireflies()
  local i, s = scr.sc, c, ox, oy
  if not fly.ready then
    for i = 0 to N_FLY - 1
      intro.flyInit(i)
    next i
    fly.ready = TRUE
  end if
  page write 0
  for i = 0 to N_FLY - 1
    ox = fly.x(i)
    oy = fly.y(i)
    fly.fx(i) = fly.fx(i) + fly.dx(i) + sin(fly.ph(i) / 9) * 0.3
    fly.fy(i) = fly.fy(i) + fly.dy(i)
    fly.ph(i) = (fly.ph(i) + 1) mod 120
    if fly.fx(i) < 0 or fly.fx(i) > scr.W - s or fly.fy(i) < scr.H * 0.3 or fly.fy(i) > scr.H - s then intro.flyInit(i)
    fly.x(i) = int(fly.fx(i))
    fly.y(i) = int(fly.fy(i))
    ' erase the old dot only when it moved, so a resting firefly never blinks
    if ox >= 0 and (ox <> fly.x(i) or oy <> fly.y(i)) then blit ox, oy, ox, oy, s, s, PG_BACK
    ' stay off the menu panel
    if fly.x(i) + s > fly.ax and fly.x(i) < fly.ax + fly.aw and fly.y(i) + s > fly.ay and fly.y(i) < fly.ay + fly.ah then
      if ox >= 0 then blit ox, oy, ox, oy, s, s, PG_BACK
      fly.x(i) = -100
    elseif fly.ph(i) < 80 then
      if fly.ph(i) mod 20 < 10 then c = rgb(255, 247, 176) else c = rgb(232, 224, 112)
      box fly.x(i), fly.y(i), s, s, 1, c, c
    else
      if ox >= 0 then blit ox, oy, ox, oy, s, s, PG_BACK
      fly.x(i) = -100
    end if
  next i
end sub

' returns MENU_START, MENU_SCORES or MENU_QUIT. One key press = one move; only
' the rows that changed are redrawn and copied to the screen.
function intro.run()
  local k$, sel0, lv0, pick = -1
  if not mus.on then music.start()
  intro.compose()
  do
    music.poll()
    intro.fireflies()
    k$ = inkey$
    if k$ = "" then
      pause 15
    else
      sel0 = menu.sel
      lv0 = menu.level
      if k$ = chr$(K_UP) then
        menu.sel = (menu.sel + 3) mod 4
      elseif k$ = chr$(K_DOWN) then
        menu.sel = (menu.sel + 1) mod 4
      elseif k$ = chr$(K_LEFT) and menu.sel = MENU_LEVEL then
        menu.level = (menu.level + 2) mod 3
      elseif k$ = chr$(K_RIGHT) and menu.sel = MENU_LEVEL then
        menu.level = (menu.level + 1) mod 3
      elseif key.isEnter(k$) and menu.sel = MENU_LEVEL then
        menu.level = (menu.level + 1) mod 3
      elseif key.isEnter(k$) then
        pick = menu.sel
        exit do
      elseif k$ = chr$(K_ESC) then
        menu.sel = MENU_QUIT
      end if
      if menu.sel <> sel0 or menu.level <> lv0 then
        intro.refresh(sel0, menu.sel, menu.level <> lv0)
      end if
    end if
  loop
  intro.run = pick
end function
