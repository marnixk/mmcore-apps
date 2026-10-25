' game.inc - one game: levels, the snake, fruit, lives and the HUD.
'
' The snake is a ring buffer of segments from sn.t (tail) to sn.h (head);
' sn.d(i) is the direction the snake moved to enter segment i, which is all we
' need to pick the head, body, corner and tail sprites. Only the cells that
' change are redrawn on the back page; each loop pass then copies just those
' cells (and the HUD bar when it changed) to the screen.

const SN_MAX = 1024, GROW = 2, FRUIT_GOAL = 10, START_LIVES = 3, PULSE_MS = 420, TQ = 2, IDLE_MS = 4

dim sn.x(SN_MAX - 1), sn.y(SN_MAX - 1), sn.d(SN_MAX - 1)
dim sn.h, sn.t, sn.n, sn.grow
dim gm.diff, gm.lvn, gm.lay, gm.lives, gm.eaten, gm.fruits, gm.score, gm.speed, gm.due
dim gm.over, gm.quit, gm.clear, gm.dead
dim gm.tq(TQ - 1), gm.tn, gm.now, gm.early
dim gm.fx, gm.fy, gm.ft, gm.fp, gm.pt
' arrow key (Up Down Left Right) -> direction (0 right, 1 down, 2 left, 3 up)
dim kdir(3) = (3, 1, 2, 0)

function sn.nxt(i)
  sn.nxt = (i + 1) mod SN_MAX
end function

' sprite for snake segment i
function sn.piece(i)
  local a, b, t
  if i = sn.h then
    t = T_HEAD + sn.d(i)
    if gm.dead then t = T_DEAD + sn.d(i)
  elseif i = sn.t then
    t = T_TAIL + (sn.d(sn.nxt(i)) + 2) mod 4
  else
    ' a = edge towards the previous segment, b = edge towards the next one
    a = (sn.d(i) + 2) mod 4
    b = sn.d(sn.nxt(i))
    if a = (b + 2) mod 4 then
      t = T_BODYH
      if b mod 2 = 1 then t = T_BODYV
    else
      if a > b then
        t = a
        a = b
        b = t
      end if
      if a = 0 and b = 1 then t = T_RD
      if a = 1 and b = 2 then t = T_DL
      if a = 2 and b = 3 then t = T_LU
      if a = 0 and b = 3 then t = T_UR
    end if
  end if
  sn.piece = t
end function

sub sn.putAt(i)
  cel.put(sn.x(i), sn.y(i), sn.piece(i), -1)
end sub

function gm.speedFor()
  gm.speedFor = max(SPEED_MIN, df.start(gm.diff) - (gm.lvn - 1) * df.faster(gm.diff))
end function

' ---- status bar (top cell row):
'   LEVEL nn NAME | SCORE nnnnnn | LIVES (head icons, stone = lost) | fruit + 10 pips
sub gm.hud()
  local s = scr.sc, c = lay.cell, ty, x, i, bx, fx, lx
  ty = ((c \ s - 8) \ 2) * s
  rfill 0, 0, scr.W, c, C_K2
  rfill 0, 0, scr.W, s, C_K3
  rfill 0, c - s, scr.W, s, C_K
  x = 8 * s
  txt "LEVEL", x, ty, F_GOLD
  x = x + 48 * s
  txt zpad$(gm.lvn, 2), x, ty, F_CREAM
  x = x + txtW(zpad$(gm.lvn, 2)) + 8 * s
  txt lv.name$(gm.lay), x, ty, F_GREY
  ' score block: fixed place right of the longest level name
  x = 172 * s
  rfill x, ty - s, s, 10 * s, C_K3
  x = x + 10 * s
  txt "SCORE", x, ty, F_GOLD
  txt zpad$(gm.score, 6), x + 48 * s, ty, F_CREAM
  ' right: fruit + progress pips, lives to their left
  bx = scr.W - 86 * s
  for i = 0 to FRUIT_GOAL - 1
    x = bx + i * 8 * s
    rfill x, ty, 6 * s, 7 * s, C_K
    if i < gm.eaten then
      rfill x + s, ty + s, 4 * s, 5 * s, C_N3
      rfill x + s, ty + s, 4 * s, s, C_N5
    else
      rfill x + s, ty + s, 4 * s, 5 * s, C_K3
    end if
  next i
  fx = bx - 4 * s - c
  ui.tile(T_FRUIT + gm.ft, fx, 0)
  rfill fx - 6 * s, ty - s, s, 10 * s, C_K3
  lx = fx - 10 * s - 3 * c
  for i = 0 to START_LIVES - 1
    if i < gm.lives then
      ui.tile(T_LIFE, lx + i * c, 0)
    else
      ui.tile(T_DEAD, lx + i * c, 0)
    end if
  next i
  txt "LIVES", lx - 44 * s, ty, F_GOLD
  rfill lx - 52 * s, ty - s, s, 10 * s, C_K3
end sub

sub gm.hudShow()
  page write PG_BACK
  gm.hud()
  scr.mark(0, 0, scr.W, lay.cell)
end sub

' ---- level set-up
sub gm.placeFruit()
  local x, y, n = 0
  do
    x = 1 + int(rnd * (FW - 2))
    y = 1 + int(rnd * (FH - 2))
    n = n + 1
    if fg(x, y) = G_FREE then
      if abs(x - sn.x(sn.h)) + abs(y - sn.y(sn.h)) >= 5 or n > 400 then exit do
    end if
  loop
  gm.fx = x
  gm.fy = y
end sub

' whole live scene from scratch
sub gm.redrawAll()
  local i
  page copy PG_FIELD to PG_BACK
  page write PG_BACK
  gm.hud()
  i = sn.t
  do
    cel.draw(sn.x(i), sn.y(i), sn.piece(i), -1)
    if i = sn.h then exit do
    i = sn.nxt(i)
  loop
  if not gm.clear then cel.draw(gm.fx, gm.fy, T_GLOW + gm.fp, T_FRUIT + gm.ft)
  scr.markAll()
end sub

' (re)start the current level with a fresh snake; newLayout rebuilds the picture
sub gm.resetLevel(newLayout)
  local i, d, hx, hy
  gm.lay = (gm.lvn - 1) mod LV_COUNT
  gm.ft = (gm.lvn - 1) mod 8
  fld.setup(gm.lay)
  if newLayout then fld.build()
  gm.eaten = 0
  gm.dead = FALSE
  gm.clear = FALSE
  gm.tn = 0
  sn.grow = 0
  d = lv.hd(gm.lay)
  hx = lv.hx(gm.lay)
  hy = lv.hy(gm.lay)
  for i = 0 to 3
    sn.x(i) = hx - dx(d) * (3 - i)
    sn.y(i) = hy - dy(d) * (3 - i)
    sn.d(i) = d
    fg(hx - dx(d) * (3 - i), hy - dy(d) * (3 - i)) = G_SNAKE
  next i
  sn.t = 0
  sn.h = 3
  sn.n = 4
  gm.placeFruit()
  gm.redrawAll()
end sub

' a message panel over the field for ms milliseconds. Arrow presses (KEYDOWN)
' are kept as turns, and once one is in the banner ends early.
sub gm.banner(t1$, r1, t2$, t3$, ms)
  local s = scr.sc, x, k, t0
  scr.save()
  x = ui.centre(240, 84) + 120 * s
  bigC t1$, x, ui.py + 14 * s, r1
  txtC t2$, x, ui.py + 42 * s, F_CREAM
  txtC t3$, x, ui.py + 60 * s, F_GREEN
  scr.present()
  key.arrowReset()
  t0 = timer
  do
    k = gm.arrows()
    if gm.tn > 0 and timer - t0 > 300 then exit do
    pause 1
  loop until timer - t0 >= ms
  scr.unsave()
  gm.due = timer + gm.speed
  gm.early = FALSE
end sub

sub gm.levelBanner()
  gm.banner("LEVEL " + zpad$(gm.lvn, 2), B_GOLD, lv.name$(gm.lay), "GET READY", 1800)
end sub

' ---- steering: queue up to TQ (2) turns so a quick double tap counts on two
' successive steps; a repeat of the last direction or a 180 degree reversal is
' ignored
sub gm.turn(d)
  local last
  last = sn.d(sn.h)
  if gm.tn > 0 then last = gm.tq(gm.tn - 1)
  if d = last or d = (last + 2) mod 4 then exit sub
  if gm.tn < TQ then
    gm.tq(gm.tn) = d
    gm.tn = gm.tn + 1
  end if
end sub

' every arrow that went down since the last pass becomes a turn, oldest first.
' Returns TRUE when a turn went into an empty queue (the next step uses it).
function gm.arrows()
  local e, k, d, was
  gm.arrows = FALSE
  key.scan()
  for e = 0 to key.evn - 1
    k = key.ev(e)
    d = kdir(k - K_UP)
    was = gm.tn
    gm.turn(d)
    if was = 0 and gm.tn > 0 then gm.arrows = TRUE
  next e
end function

' ---- one move of the snake
sub gm.step()
  local d, nx, ny, eat, moves, ot, oh, tx, ty
  d = sn.d(sn.h)
  if gm.tn > 0 then
    d = gm.tq(0)
    gm.tq(0) = gm.tq(1)
    gm.tn = gm.tn - 1
  end if
  nx = sn.x(sn.h) + dx(d)
  ny = sn.y(sn.h) + dy(d)
  eat = (nx = gm.fx and ny = gm.fy)
  if eat then sn.grow = sn.grow + GROW
  moves = (sn.grow = 0)
  ot = sn.t
  tx = sn.x(ot)
  ty = sn.y(ot)
  ' the tail leaves its cell before the head arrives, so chasing it is fine
  if moves then fg(tx, ty) = G_FREE
  if fg(nx, ny) <> G_FREE then
    if moves then fg(tx, ty) = G_SNAKE
    gm.die()
    exit sub
  end if
  if moves then
    cel.put(tx, ty, -1, -1)
    sn.t = sn.nxt(ot)
  else
    sn.grow = sn.grow - 1
    sn.n = sn.n + 1
  end if
  oh = sn.h
  sn.h = sn.nxt(oh)
  sn.x(sn.h) = nx
  sn.y(sn.h) = ny
  sn.d(sn.h) = d
  fg(nx, ny) = G_SNAKE
  if moves then sn.putAt(sn.t)
  sn.putAt(oh)
  sn.putAt(sn.h)
  if eat then gm.ate()
end sub

sub gm.ate()
  gm.eaten = gm.eaten + 1
  gm.fruits = gm.fruits + 1
  gm.score = gm.score + 10 * gm.lvn * (gm.diff + 1)
  if gm.eaten >= FRUIT_GOAL then
    gm.clear = TRUE
  else
    gm.placeFruit()
    cel.put(gm.fx, gm.fy, T_GLOW + gm.fp, T_FRUIT + gm.ft)
  end if
  gm.hudShow()
end sub

' hit a wall or itself: stone head, lose a life, restart the level (or game over)
sub gm.die()
  local t$
  gm.dead = TRUE
  sn.putAt(sn.h)
  gm.lives = gm.lives - 1
  gm.hudShow()
  scr.flush()
  pause 1100
  if gm.lives <= 0 then
    gm.over = TRUE
    exit sub
  end if
  gm.resetLevel(FALSE)
  t$ = "LAST LIFE!"
  if gm.lives > 1 then t$ = str$(gm.lives) + " LIVES LEFT"
  gm.banner("OUCH!", B_RED, t$, "GET READY", 1800)
end sub

sub gm.levelUp()
  gm.score = gm.score + 50 * gm.lvn * (gm.diff + 1)
  gm.banner("LEVEL CLEAR!", B_GREEN, "LEVEL " + zpad$(gm.lvn, 2) + " DONE", "WELL PLAYED", 1300)
  gm.lvn = gm.lvn + 1
  gm.speed = gm.speedFor()
  gm.resetLevel(TRUE)
  gm.levelBanner()
end sub

sub gm.pauseGame()
  gm.quit = dlg.pause()
  if not gm.quit then
    scr.unsave()
    key.arrowReset()
    gm.due = timer + gm.speed
  end if
end sub

sub game.run(diff)
  local k$, st
  gm.diff = diff
  gm.lvn = 1
  gm.lives = START_LIVES
  gm.score = 0
  gm.fruits = 0
  gm.over = FALSE
  gm.quit = FALSE
  gm.fp = 0
  gm.speed = gm.speedFor()
  gm.resetLevel(TRUE)
  gm.levelBanner()
  gm.pt = timer + PULSE_MS
  gm.early = FALSE
  do
    ' P / Esc pause; anything else typed during play (e.g. the arrows, which
    ' steer via KEYDOWN) is ignored
    k$ = inkey$
    do while k$ <> "" and not gm.quit
      if key.isPause(k$) then gm.pauseGame()
      k$ = inkey$
    loop
    if gm.quit then exit do
    ' a first queued turn more than half a step after the last move is taken
    ' at once; never twice in a row, so the snake is not faster overall
    st = gm.arrows()
    gm.now = timer
    if st then st = (gm.due - gm.now < gm.speed \ 2)
    if st then st = not gm.early
    if st then
      gm.early = TRUE
      gm.due = gm.now + gm.speed
      gm.step()
      if gm.clear then gm.levelUp()
    elseif gm.now >= gm.due then
      gm.early = FALSE
      gm.due = gm.due + gm.speed
      if gm.due < gm.now then gm.due = gm.now + gm.speed
      gm.step()
      if gm.clear then gm.levelUp()
    end if
    if timer >= gm.pt and not gm.over then
      gm.fp = 1 - gm.fp
      cel.put(gm.fx, gm.fy, T_GLOW + gm.fp, T_FRUIT + gm.ft)
      gm.pt = timer + PULSE_MS
    end if
    ' one present per pass: only the cells (and HUD) that changed
    if scr.dirty then scr.flush()
    ' idle until the next step or pulse is due, waking every few ms to read
    ' the keys
    pause max(1, min(IDLE_MS, min(gm.due, gm.pt) - timer))
  loop until gm.over or gm.quit
  if not gm.quit then dlg.gameOver()
end sub
