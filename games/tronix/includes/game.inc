' game.inc - one game: levels, the player, orbs and sentinels, claims,
' deaths, the level clear and the HUD status bar.
'
' Timing: the player steps every PLAYER_MS while it rides the open grid (it
' keeps going in its last direction, classic Xonix); on claimed land it only
' moves on a key press (one step per tap, a held arrow repeats after
' LAND_DELAY). Orbs step together every gm.orbMs, sentinels every gm.enMs.
'
' How a level runs - two interrupt handlers and the main loop:
'  key.onKey (ON KEY, input.inc) notes arrow presses and the pause key.
'  gm.tick (SETTICK, every TICK_MS) is the beat: it notices released keys and
'    moves the player, orbs and sentinels when their step is due. It only
'    changes the game state and never draws, so it cannot upset the page or
'    the scratch variables the main loop is drawing with. Work too big for it
'    is flagged for the main loop: a claim (gm.claim) or a kill (gm.dead);
'    the tick then waits until the main loop has dealt with it.
'  The main loop (gm.play) draws: the new wall cells, a frame of the actors
'    every FRAME_MS, the claim, the derez, the pause dialog. While it draws
'    the actors (gm.drawing) the tick skips its moves, so a frame never shows
'    an actor half way through a step.
' Both handlers are only on during play; menus and dialogs read INKEY$.
'
' Difficulty ramp per level L (numbers per skill in the main program):
'   orbs      = min(ORB_MAX, df.orbs0 + (L - 1) \ df.orbEvery)
'   sentinels = 1, a 2nd from level df.en2, a 3rd from level df.en3
'   orb step  = max(df.orbMin, df.orbMs - ORB_FASTER * ((L - 1) \ df.fastEvery)) ms
'   target    = TARGET1 %, TARGET2 % from level TARGET2_LEVEL

const FRAME_MS = 20, IDLE_MS = 4, TICK_MS = 5

dim gm.diff, gm.level, gm.lives, gm.score, gm.quit, gm.over, gm.target
dim gm.orbMs, gm.enMs, gm.lvPts, gm.cleared
dim gm.dead, gm.hx, gm.hy, gm.hitOrb
dim gm.claim, gm.drawing   ' a claim is waiting for the main loop; actors being drawn
dim pl.land, pl.d, pl.due, pl.sx, pl.sy
dim integer gm_tick_now   ' the tick's clock (a global: LOCAL is slow, see actors.inc)
dim orb.due, en.due, fr.due

' ---- status bar (on PG_FIELD, copied to the back page): level, claimed %
' with a bar and the target mark, lives
sub hud.draw()
  local W, bx, bw, p, fw, tx, i
  page write PG_FIELD
  W = lay.aw
  af 0, 0, W, 16, C_N1
  af 0, 14, W, 1, C_N3
  af 0, 15, W, 1, C_C1
  txt "LEVEL " + str$(gm.level), 8, 4, F_CYAN
  bx = lay.hbx
  bw = lay.hbw
  p = min(100, fld.pct())
  txtR str$(p) + "%", bx - 6, 4, F_HOT
  af bx, 4, bw, 8, C_N3
  af bx + 1, 5, bw - 2, 6, C_N0
  fw = ((bw - 2) * p) \ 100
  if fw > 0 then
    af bx + 1, 5, fw, 6, C_CL3
    af bx + 1, 5, fw, 1, C_C3
  end if
  af bx + 1 + fw, 5, 1, 6, C_WW
  tx = bx + 1 + ((bw - 2) * gm.target) \ 100
  af tx, 3, 1, 10, C_O2
  af tx, 2, 1, 1, C_O3
  af tx, 13, 1, 1, C_O3
  txt str$(gm.target) + "%", bx + bw + 6, 4, F_ORANGE
  for i = 0 to gm.lives - 1
    ui.spr(8 * T_LIFE, SH_TILE_Y, 8, 8, W - 16 - i * 11, 4)
  next i
  page write PG_BACK
  scr.copyA(0, 0, W, 16, PG_FIELD)
  scr.markA(0, 0, W, 16)
  act.dirty = TRUE
end sub

' ---- level set-up
sub gm.setupLevel()
  local L, i, n, x, y, ok, j, ne
  L = gm.level
  gm.target = TARGET1
  if L >= TARGET2_LEVEL then gm.target = TARGET2
  gm.orbMs = max(df.orbMin(gm.diff), df.orbMs(gm.diff) - ORB_FASTER * ((L - 1) \ df.fastEvery(gm.diff)))
  gm.enMs = gm.orbMs + EN_SLOWER
  gm.lvPts = 0
  gm.cleared = FALSE
  gm.dead = FALSE
  gm.claim = FALSE
  fld.reset()
  act.clear()
  page write PG_FIELD
  cls rgb(0, 0, 0)
  page write PG_BACK
  fld.redraw(0, 0, fld.W - 1, fld.H - 1)
  hud.draw()
  ' orbs anywhere in the open grid, apart from each other
  n = min(ORB_MAX, df.orbs0(gm.diff) + (L - 1) \ df.orbEvery(gm.diff))
  for i = 1 to n
    do
      x = 4 + int(rnd * (fld.W - 8))
      y = 4 + int(rnd * (fld.H - 8))
      ok = TRUE
      for j = 1 to i - 1
        if abs(o.x(j) - x) < 4 and abs(o.y(j) - y) < 4 then ok = FALSE
      next j
    loop until ok
    act.place(i, x, y)
    o.dx(i) = 1 - 2 * int(rnd * 2)
    o.dy(i) = 1 - 2 * int(rnd * 2)
  next i
  act.nOrb = n
  ' sentinels on the frame: bottom middle, then the left and right sides
  ne = 1
  if L >= df.en2(gm.diff) then ne = 2
  if L >= df.en3(gm.diff) then ne = 3
  for i = 0 to ne - 1
    if i = 0 then
      act.place(A_EN, fld.W \ 2, fld.H - 1)
    elseif i = 1 then
      act.place(A_EN + 1, 0, fld.H \ 2)
    else
      act.place(A_EN + 2, fld.W - 1, fld.H \ 2 + 1)
    end if
    o.dx(A_EN + i) = 1 - 2 * int(rnd * 2)
    o.dy(A_EN + i) = -1
  next i
  act.nEn = ne
  ' the player: top middle of the frame
  act.place(0, fld.W \ 2, 0)
  pl.land = TRUE
  pl.d = -1
  page copy PG_FIELD to PG_BACK
  page write PG_BACK
  act.invalidate()
  act.dirty = TRUE
  act.render()
  scr.present()
end sub

' ---- the player (runs in the tick)
' one step in direction d; the next step is due PLAYER_MS later, on the beat
' while it keeps moving, or PLAYER_MS from now after a rest
sub pl.go(d, now)
  local nx, ny, c, i
  nx = o.x(0) + dir.x(d)
  ny = o.y(0) + dir.y(d)
  c = fld.at(nx, ny)
  if c = 9 then exit sub
  pl.due = pl.due + PLAYER_MS
  if pl.due <= now then pl.due = now + PLAYER_MS
  if pl.land then
    act.move(0, nx, ny, PLAYER_MS, now)
    if c = 0 then
      ' off the land: start a wall
      pl.land = FALSE
      pl.sx = o.lx(0)
      pl.sy = o.ly(0)
      pl.d = d
      tr.clear()
      tr.add(nx, ny, d)
    end if
  else
    if c = 2 then
      ' ran into its own wall
      act.move(0, nx, ny, PLAYER_MS, now)
      gm.kill(nx, ny, FALSE)
      exit sub
    end if
    act.move(0, nx, ny, PLAYER_MS, now)
    pl.d = d
    if c = 0 then
      tr.add(nx, ny, d)
    else
      ' back on land: the wall closes; the main loop does the claim
      pl.land = TRUE
      pl.d = -1
      gm.claim = TRUE
    end if
  end if
  ' ran into an orb / a sentinel
  if pl.land then
    for i = A_EN to A_EN + act.nEn - 1
      if o.x(i) = nx and o.y(i) = ny then gm.kill(nx, ny, FALSE)
    next i
  else
    for i = 1 to act.nOrb
      if o.x(i) = nx and o.y(i) = ny then gm.kill(nx, ny, TRUE)
    next i
  end if
end sub

' keys -> a player step (from the tick, when a step is due)
sub pl.input(now)
  local d
  if pl.land then
    d = key.pop()
    if d < 0 and key.held >= 0 then
      if now >= key.t0 + LAND_DELAY then d = key.held
    end if
    if d >= 0 then pl.go(d, now)
  else
    ' riding the grid: queued taps turn; repeats and reversals are dropped
    do
      d = key.pop()
      if d < 0 then exit do
      if d <> pl.d and d <> dir.op(pl.d) then exit do
    loop
    if d >= 0 then pl.d = d
    pl.go(pl.d, now)
  end if
end sub

' ---- orbs: bounce diagonally off claimed land; touching the wall is a kill
sub orb.step(now)
  local i, x, y, dx, dy, h, v, g, nx, ny
  for i = 1 to act.nOrb
    x = o.x(i)
    y = o.y(i)
    dx = o.dx(i)
    dy = o.dy(i)
    h = fld.at(x + dx, y)
    v = fld.at(x, y + dy)
    if h = 1 or h = 9 then dx = -dx
    if v = 1 or v = 9 then dy = -dy
    if dx = o.dx(i) and dy = o.dy(i) then
      g = fld.at(x + dx, y + dy)
      if g = 1 or g = 9 then
        dx = -dx
        dy = -dy
      end if
    end if
    o.dx(i) = dx
    o.dy(i) = dy
    h = fld.at(x + dx, y)
    v = fld.at(x, y + dy)
    g = fld.at(x + dx, y + dy)
    if g = 2 then
      gm.kill(x + dx, y + dy, TRUE)
    elseif h = 2 then
      gm.kill(x + dx, y, TRUE)
    elseif v = 2 then
      gm.kill(x, y + dy, TRUE)
    end if
    if g = 0 or g = 2 then
      nx = x + dx
      ny = y + dy
    else
      nx = x
      ny = y
    end if
    act.move(i, nx, ny, gm.orbMs, now)
    if not pl.land and nx = o.x(0) and ny = o.y(0) then gm.kill(nx, ny, TRUE)
  next i
end sub

' ---- sentinels: bounce diagonally inside the claimed land
sub en.step(now)
  local i, x, y, dx, dy, h, v, g, nx, ny, px, py
  px = o.x(0)
  py = o.y(0)
  for i = A_EN to A_EN + act.nEn - 1
    x = o.x(i)
    y = o.y(i)
    dx = o.dx(i)
    dy = o.dy(i)
    h = fld.at(x + dx, y)
    v = fld.at(x, y + dy)
    if h <> 1 then dx = -dx
    if v <> 1 then dy = -dy
    if dx = o.dx(i) and dy = o.dy(i) then
      g = fld.at(x + dx, y + dy)
      if g <> 1 then
        dx = -dx
        dy = -dy
      end if
    end if
    o.dx(i) = dx
    o.dy(i) = dy
    if fld.at(x + dx, y + dy) = 1 then
      nx = x + dx
      ny = y + dy
    else
      nx = x
      ny = y
    end if
    act.move(i, nx, ny, gm.enMs, now)
    if pl.land then
      if (nx = px and ny = py) or (x = px and y = py) then gm.kill(px, py, FALSE)
    end if
  next i
end sub

' ---- the wall closed (flagged by the tick): claim, points, status bar,
' level target; then the tick carries on
sub gm.claimNow()
  local n, p
  n = fld.close()
  act.dirty = TRUE
  p = (fld.got * 100 * PTS_PER_PCT) \ fld.inner
  gm.score = gm.score + p - gm.lvPts
  gm.lvPts = p
  hud.draw()
  if fld.pct() >= gm.target then gm.cleared = TRUE
  gm.claim = FALSE
end sub

' ---- a kill: remember where it hit (the derez runs in gm.die)
sub gm.kill(x, y, byOrb)
  if gm.dead then exit sub
  gm.dead = TRUE
  gm.hx = x
  gm.hy = y
  gm.hitOrb = byOrb
end sub

' derez, the wall is cut, one life less, the player back where it left land
sub gm.die()
  local f, i, best, bd, d, cx(3), cy(3), j
  gm.handlers(FALSE)
  o.fs(0) = -1
  if gm.hitOrb then
    act.place(A_FX, gm.hx, gm.hy)
  end if
  for f = 0 to 2
    o.fs(0) = S_DEREZ_0 + f
    o.fs(A_FX) = S_SPARK_0 + f
    act.render()
    scr.flush()
    pause 110
  next f
  o.on(0) = FALSE
  o.on(A_FX) = FALSE
  o.fs(0) = -1
  o.fs(A_FX) = -1
  act.render()
  scr.flush()
  pause 250
  tr.remove()
  gm.lives = gm.lives - 1
  hud.draw()
  act.render()
  scr.flush()
  if gm.lives <= 0 then
    gm.over = TRUE
    pause 500
    exit sub
  end if
  if not pl.land then
    act.place(0, pl.sx, pl.sy)
  else
    ' killed on land: respawn on the frame spot farthest from the sentinels
    cx(0) = fld.W \ 2 : cy(0) = 0
    cx(1) = fld.W \ 2 : cy(1) = fld.H - 1
    cx(2) = 0 : cy(2) = fld.H \ 2
    cx(3) = fld.W - 1 : cy(3) = fld.H \ 2
    best = 0
    bd = -1
    for j = 0 to 3
      d = 9999
      for i = A_EN to A_EN + act.nEn - 1
        d = min(d, abs(o.x(i) - cx(j)) + abs(o.y(i) - cy(j)))
      next i
      if d > bd then
        bd = d
        best = j
      end if
    next j
    act.place(0, cx(best), cy(best))
  end if
  pl.land = TRUE
  pl.d = -1
  gm.dead = FALSE
  act.render()
  scr.flush()
  pause 600
  gm.handlers(TRUE)
end sub

' the key handler and the beat: on while a level is played, off for the
' derez, the pause dialog and between levels. Switching on starts afresh:
' no keys typed before count, the steps are timed from now.
sub gm.handlers(on)
  local now
  if on then
    key.reset()
    now = timer
    orb.due = now + gm.orbMs
    en.due = now + gm.enMs
    fr.due = now
    pl.due = now
    on key key.onKey
    settick TICK_MS, gm.tick
  else
    settick 0, gm.tick
    on key 0
  end if
end sub

' ---- the beat (timer interrupt, every TICK_MS): releases, then whatever
' step is due. Nothing moves while a kill, a claim or the level clear waits
' for the main loop, or while the main loop draws the actors.
sub gm.tick()
  key.poll()
  if gm.dead or gm.claim or gm.cleared or gm.drawing then exit sub
  gm_tick_now = timer
  if gm_tick_now >= pl.due then pl.input(gm_tick_now)
  if gm.dead or gm.claim then exit sub
  if gm_tick_now >= orb.due then
    orb.step(gm_tick_now)
    orb.due = orb.due + gm.orbMs
    if orb.due < gm_tick_now then orb.due = gm_tick_now + gm.orbMs
  end if
  if not gm.dead and gm_tick_now >= en.due then
    en.step(gm_tick_now)
    en.due = en.due + gm.enMs
    if en.due < gm_tick_now then en.due = gm_tick_now + gm.enMs
  end if
end sub

' pause: the dialog covers the back page; afterwards the scene is rebuilt
' from the clean field page and the actors
sub gm.pauseGame()
  gm.handlers(FALSE)
  gm.quit = dlg.pause()
  if gm.quit then exit sub
  page copy PG_FIELD to PG_BACK
  page write PG_BACK
  act.invalidate()
  act.render()
  scr.present()
  gm.handlers(TRUE)
end sub

' ---- one level: returns when it is cleared, the game is over or quit.
' The main loop only draws and handles what the handlers flagged; it wakes
' every IDLE_MS (the tick and ON KEY also run during its PAUSE).
sub gm.play()
  local now
  gm.handlers(TRUE)
  do
    if key.pause then
      gm.pauseGame()
      if gm.quit then exit do
    end if
    if gm.claim then gm.claimNow()
    tr.show()
    if gm.dead then
      gm.die()
      if gm.over then exit do
    end if
    now = timer
    if now >= fr.due or gm.cleared then
      gm.drawing = TRUE
      act.render()
      gm.drawing = FALSE
      if scr.dirty then scr.flush()
      fr.due = now + FRAME_MS
    end if
    if gm.cleared then exit do
    pause max(1, min(IDLE_MS, fr.due - timer))
  loop
  gm.handlers(FALSE)
end sub

' ---- level clear: bonuses, the panel, an extra life every EXTRA_EVERY levels
sub gm.levelClear()
  local p, over, lb, extra
  ' let the last moves settle on screen
  fr.due = timer + 350
  do while timer < fr.due
    act.render()
    if scr.dirty then scr.flush()
    pause 10
  loop
  p = fld.pct()
  over = max(0, p - gm.target) * OVER_BONUS
  lb = gm.lives * LIFE_BONUS
  gm.score = gm.score + over + lb
  extra = (gm.level mod EXTRA_EVERY = 0) and gm.lives < LIVES_MAX
  dlg.levelClear(p, over, lb, extra)
  if extra then gm.lives = gm.lives + 1
  gm.level = gm.level + 1
end sub

sub game.run(diff)
  gm.diff = diff
  gm.level = 1
  gm.lives = LIVES0
  gm.score = 0
  gm.quit = FALSE
  gm.over = FALSE
  scr.useDim()
  do
    gm.setupLevel()
    gm.play()
    if gm.quit or gm.over then exit do
    gm.levelClear()
    if gm.quit then exit do
  loop
  if gm.over then dlg.gameOver()
end sub
