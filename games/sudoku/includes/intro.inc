' intro.inc - title screen with the forest clearing, logo, menu and fireflies

const MENU_START = 0, MENU_LEVEL = 1, MENU_SCORES = 2, MENU_QUIT = 3
dim menu.level = LV_MEDIUM, menu.sel = 0
dim intro.titleY, intro.titleH, intro.mx, intro.my

const N_FLY = 18
dim fly.x(N_FLY - 1), fly.y(N_FLY - 1), fly.ph(N_FLY - 1)
dim float fly.dx(N_FLY - 1), fly.dy(N_FLY - 1), fly.fx(N_FLY - 1), fly.fy(N_FLY - 1)
dim fly.ready = FALSE
dim fly.ax, fly.ay, fly.aw, fly.ah   ' rectangle the fireflies keep away from

sub intro.compose()
  local sc = scr.sc, i, y, r
  local string items$(3)
  items$(0) = "START GAME"
  items$(1) = "LEVEL: < " + lv.name$(menu.level) + " >"
  items$(2) = "HIGH SCORES"
  items$(3) = "QUIT"
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
    y = intro.my + 12 * sc + i * 15 * sc
    if i = menu.sel then
      r = F_GREEN
      dlg.icon(2, intro.mx + 11 * sc, y)
    else
      r = F_BROWN
    end if
    txt items$(i), intro.mx + 22 * sc, y, r
  next i
  scr.present()
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

' returns MENU_START, MENU_SCORES or MENU_QUIT
function intro.run()
  local k
  if not mus.on then music.start()
  intro.compose()
  key.flush()
  do
    music.poll()
    intro.fireflies()
    k = key.get()
    if k = K_UP then
      menu.sel = (menu.sel + 3) mod 4
      intro.compose()
    elseif k = K_DOWN then
      menu.sel = (menu.sel + 1) mod 4
      intro.compose()
    elseif k = K_LEFT and menu.sel = MENU_LEVEL then
      menu.level = (menu.level + 2) mod 3
      intro.compose()
    elseif k = K_RIGHT and menu.sel = MENU_LEVEL then
      menu.level = (menu.level + 1) mod 3
      intro.compose()
    elseif (k = K_ENTER or k = K_LF) and menu.sel = MENU_LEVEL then
      menu.level = (menu.level + 1) mod 3
      intro.compose()
    elseif k = K_ENTER or k = K_LF then
      intro.run = menu.sel
      exit do
    elseif k = K_ESC then
      menu.sel = MENU_QUIT
      intro.compose()
    end if
    if not key.waiting() then pause 15
  loop
  key.flush()
end function
