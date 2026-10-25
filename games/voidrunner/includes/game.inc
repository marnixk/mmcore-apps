' game.inc - one run: the ship, gravity and thrust, the scrolling obstacles,
' crystals, the speed ramp and the crash.
'
' Fixed timestep: the world updates every TICK ms (50 per second) in whole art
' pixels; the scroll speed (art px per tick, fractional) feeds a sub-pixel
' accumulator. After the updates that were due, ONE frame is drawn:
'   - every moving thing is a SPRITE on the back page (columns, crystals, the
'     ship, the cue text, the explosion). SPRITE NEXT queues the new places and
'     a single SPRITE MOVE restores the backdrop under all of them and redraws
'     them in mmcore's native code (the backdrop itself never changes in play,
'     so the star field stays static and costs nothing),
'   - then only the changed rectangles go to the screen: one full-height strip
'     per moving column (old + new place = column width + scroll step) and the
'     ship's old + new box (scr.mark / scr.flush).
' Between ticks the loop idles with a short PAUSE and reads KEYDOWN on every wake.

const TICK = 20, IDLE_MS = 4, CATCHUP = 5
' ship: frame top-left at art x SHX; hull box (hit box) inside the 64 x 48 frame
const SHX = 84, HB_X0 = 30, HB_X1 = 56, HB_Y0 = 18, HB_Y1 = 30
' physics in art px per tick
const GRAV = 0.30, FLAP = -4.6, VMAX = 6.5
const GEM_BONUS = 2, CUE_MS = 1100
' sprite numbers: columns + crystals per obstacle slot, 7 ship poses, 2 cue
' texts, 3 explosion puffs x 4 frames
const SPR_COL = 1, SPR_GEM = 11, SPR_SHIP = 21, SPR_CUE = 31, SPR_BOOM = 41
' ship poses: 0 idle0 1 idle1 2 thrust0 3 thrust1 4 fall 5 dive 6 hit
dim sp.px(6), sp.py(6)
' cue texts on PG_COLS (art), right of the column slots
const CUE_X = NOB * SLOT_W + 1, CUE_W = 150, CUE_H = 22

dim gm.diff, gm.gaps, gm.gems, gm.score, gm.over, gm.quit, gm.dead, gm.ready
dim gm.due, gm.tk, gm.flap, gm.lv, gm.moved
dim float gm.speed, gm.acc, gm.target
dim float sh.y, sh.vy
dim sh.dy, sh.p, sh.cur, sh.oy, sh.thr, sh.by
' cue text (0 GET READY, 1 SPEED UP!)
dim cue.on, cue.t0, cue.vis, cue.k, cue.y
' explosion: three puffs, frame -1 = not shown
dim bm.f(2), bm.dx(2) = (13, 49, 63), bm.dy(2) = (-6, 6, 20)
dim gm.rects

' ---- sprites read once at start-up
sub gm.sprites()
  local s, i, j, f, x0
  s = scr.sc
  sp.px(0) = SP_A_IDLE0_X : sp.py(0) = SP_A_IDLE0_Y
  sp.px(1) = SP_A_IDLE1_X : sp.py(1) = SP_A_IDLE1_Y
  sp.px(2) = SP_A_THRUST0_X : sp.py(2) = SP_A_THRUST0_Y
  sp.px(3) = SP_A_THRUST1_X : sp.py(3) = SP_A_THRUST1_Y
  sp.px(4) = SP_A_FALL_X : sp.py(4) = SP_A_FALL_Y
  sp.px(5) = SP_A_DIVE_X : sp.py(5) = SP_A_DIVE_Y
  sp.px(6) = SP_A_HIT_X : sp.py(6) = SP_A_HIT_Y
  for i = 0 to 6
    sprite read SPR_SHIP + i, sp.px(i) * s, sp.py(i) * s, 64 * s, 48 * s, PG_SHEET
  next i
  for j = 0 to 2
    for f = 0 to 3
      sprite read SPR_BOOM + 4 * j + f, (SP_BOOM_0_X + 32 * f) * s, SP_BOOM_0_Y * s, 32 * s, 32 * s, PG_SHEET
    next f
  next j
  ' crystal (glow + gem) and the two cue texts, composed right of the slots
  page write PG_COLS
  x0 = CUE_X - 1
  box x0 * s, 0, scr.W - x0 * s, 80 * s, 1, 0, 0
  ui.spr(SP_GLOW_0_X, SP_GLOW_0_Y, 16, 16, x0 + 160, 60)
  ui.spr(SP_CRYSTAL_0_X, SP_CRYSTAL_0_Y, 16, 16, x0 + 160, 60)
  bigC "GET READY", CUE_X + 75, 2, B_GOLD
  bigC "SPEED UP!", CUE_X + 75, 30, B_CYAN
  page write PG_BACK
  for i = 0 to NOB - 1
    sprite read SPR_GEM + i, (x0 + 160) * s, 60 * s, 16 * s, 16 * s, PG_COLS
  next i
  sprite read SPR_CUE, (CUE_X - 2) * s, 0, CUE_W * s, CUE_H * s, PG_COLS
  sprite read SPR_CUE + 1, (CUE_X - 2) * s, 28 * s, CUE_W * s, CUE_H * s, PG_COLS
end sub

' ---- the ship pose for the current state (sh.p)
sub sh.pose()
  if gm.dead then
    sh.p = 6
  elseif sh.thr > 0 then
    sh.p = 2 + (gm.tk \ 3) mod 2
  elseif sh.vy > 3.5 and not gm.ready then
    sh.p = 5
  elseif sh.vy > 1.5 and not gm.ready then
    sh.p = 4
  else
    sh.p = (gm.tk \ 6) mod 2
  end if
end sub

' ---- cue text: shown as sprite SPR_CUE + k, centred, art y
sub cue.show(k, y)
  local s
  s = scr.sc
  cue.hide()
  cue.k = k
  cue.y = y
  cue.on = TRUE
  cue.vis = TRUE
  cue.t0 = timer
  sprite show SPR_CUE + k, (lay.aw \ 2 - 77) * s, (y - 2) * s, 3
  cue.mark()
end sub

sub cue.mark()
  local s
  s = scr.sc
  scr.mark((lay.aw \ 2 - 77) * s, (cue.y - 2) * s, CUE_W * s, CUE_H * s)
end sub

sub cue.hide()
  if not cue.on then exit sub
  cue.on = FALSE
  if cue.vis then
    sprite hide SPR_CUE + cue.k
    cue.mark()
  end if
  cue.vis = FALSE
end sub

' SPEED UP! shows for ~0.6 s, blinks out and is gone after CUE_MS
sub cue.update()
  local a, v
  if not cue.on or gm.ready then exit sub
  a = timer - cue.t0
  if a >= CUE_MS then
    cue.hide()
    exit sub
  end if
  v = (a < 600) or ((a \ 100) mod 2 = 0)
  if v <> cue.vis then
    cue.vis = v
    if v then
      sprite show SPR_CUE + cue.k, (lay.aw \ 2 - 77) * scr.sc, (cue.y - 2) * scr.sc, 3
    else
      sprite hide SPR_CUE + cue.k
    end if
    cue.mark()
  end if
end sub

' ---- the speed for the current number of gaps passed
function gm.speedFor(gaps)
  gm.speedFor = min(df.vmax(gm.diff), df.v0(gm.diff) + df.dv(gm.diff) * (gaps \ RAMP_GAPS))
end function

' ---- one fixed update
sub gm.tick()
  local i, d, y, hx0, hx1, hy0, hy1, lx
  gm.tk = gm.tk + 1
  if gm.ready then
    if gm.flap then
      gm.ready = FALSE
      cue.hide()
    else
      sh.y = sh.by + 3 * sin(gm.tk / 9)
      exit sub
    end if
  end if
  if gm.flap then
    sh.vy = FLAP
    sh.thr = 8
    gm.flap = FALSE
  elseif sh.thr > 0 then
    sh.thr = sh.thr - 1
  end if
  sh.vy = min(VMAX, sh.vy + GRAV)
  sh.y = sh.y + sh.vy
  if sh.y + HB_Y0 < 0 then
    sh.y = -HB_Y0
    sh.vy = 0
  end if
  ' speed: ease towards the target of the current ramp step
  if gm.speed < gm.target then gm.speed = min(gm.target, gm.speed + 0.02)
  gm.acc = gm.acc + gm.speed
  d = int(gm.acc)
  gm.acc = gm.acc - d
  lx = -9999
  for i = 0 to NOB - 1
    if ob.on(i) then
      ob.x(i) = ob.x(i) - d
      if ob.x(i) > lx then lx = ob.x(i)
    end if
  next i
  if lx < lay.aw + 8 then
    if lx = -9999 then lx = lay.aw - df.space(gm.diff)
    ob.spawn(lx + df.space(gm.diff), df.gap(gm.diff), df.shift(gm.diff), TRUE)
  end if
  ' passed gaps, crystals, collisions (hull box only, never the flame)
  y = int(sh.y)
  hx0 = SHX + HB_X0
  hx1 = SHX + HB_X1
  hy0 = y + HB_Y0
  hy1 = y + HB_Y1
  for i = 0 to NOB - 1
    if ob.on(i) then
      if not ob.pass(i) and ob.x(i) + 56 < hx0 then
        ob.pass(i) = TRUE
        gm.passed()
      end if
      if ob.x(i) < hx1 and ob.x(i) + SLOT_W > hx0 then
        if ob.gem(i) then
          if ovl(hx0 - 3, hy0 - 3, hx1 + 3, hy1 + 3, ob.x(i) + 25, ob.gy(i) + 1, ob.x(i) + 39, ob.gy(i) + 15) then
            ob.gem(i) = FALSE
            gm.gems = gm.gems + 1
            sprite hide SPR_GEM + i
            scr.mark((ob.x(i) + 24) * scr.sc, ob.gy(i) * scr.sc, 16 * scr.sc, 16 * scr.sc)
          end if
        end if
        if ob.hit(i, hx0, hy0, hx1, hy1) then gm.dead = TRUE
      end if
    end if
  next i
  if hy1 > lay.ah then gm.dead = TRUE
end sub

sub gm.passed()
  local float t
  gm.gaps = gm.gaps + 1
  t = gm.speedFor(gm.gaps)
  if t > gm.target + 0.001 then
    gm.target = t
    gm.lv = gm.lv + 1
    cue.show(1, 30)
  end if
end sub

' ---- draw what changed since the last frame
sub gm.render()
  local s, i, x0, x1, y, X, H
  s = scr.sc
  H = scr.H
  gm.moved = FALSE
  for i = 0 to NOB - 1
    if ob.on(i) then
      if ob.x(i) <> ob.ox(i) then
        X = ob.x(i)
        x0 = min(X, ob.ox(i))
        x1 = max(X, ob.ox(i)) + SLOT_W
        ob.ox(i) = X
        if X + SLOT_W <= 0 then
          ob.remove(i)
        elseif X < lay.aw then
          sprite next SPR_COL + i, X * s, 0
          if ob.gem(i) then sprite next SPR_GEM + i, (X + 24) * s, ob.gy(i) * s
          gm.moved = TRUE
        else
          sprite next SPR_COL + i, X * s, 0
          if ob.gem(i) then sprite next SPR_GEM + i, (X + 24) * s, ob.gy(i) * s
        end if
        if x0 < lay.aw then scr.mark(x0 * s, 0, (x1 - x0) * s, H)
      end if
    end if
  next i
  ' ship: a new pose is a different sprite (the old one hides first)
  y = int(sh.y)
  sh.pose()
  if sh.p <> sh.cur then
    sprite hide SPR_SHIP + sh.cur
    sprite show SPR_SHIP + sh.p, SHX * s, y * s, 4
    sh.cur = sh.p
    scr.mark(SHX * s, min(y, sh.oy) * s, 64 * s, (abs(y - sh.oy) + 48) * s)
    sh.oy = y
  elseif y <> sh.oy then
    sprite next SPR_SHIP + sh.cur, SHX * s, y * s
    gm.moved = TRUE
    scr.mark(SHX * s, min(y, sh.oy) * s, 64 * s, (abs(y - sh.oy) + 48) * s)
    sh.oy = y
  end if
  sh.dy = y
  if gm.moved then sprite move
  cue.update()
end sub

' ---- the crash: hit pose, three puffs, then the game-over panel
sub gm.crash()
  local s, t, j, f
  s = scr.sc
  cue.hide()
  for j = 0 to 2
    bm.f(j) = -1
  next j
  gm.render()
  for t = 0 to 4
    for j = 0 to 2
      f = min(3 - j, t - j)
      if f < 0 then f = -1
      if f <> bm.f(j) then
        if bm.f(j) >= 0 then sprite hide SPR_BOOM + 4 * j + bm.f(j)
        if f >= 0 then sprite show SPR_BOOM + 4 * j + f, (SHX + bm.dx(j)) * s, (sh.dy + bm.dy(j)) * s, 5
        bm.f(j) = f
      end if
    next j
    scr.mark(SHX * s, (sh.dy - 8) * s, 100 * s, 62 * s)
    scr.flush()
    pause 110
  next t
  pause 500
end sub

' pause: the scene is kept on PG_INTRO (sprite backgrounds stay valid) while the
' dialog covers the back page, and comes back unchanged
sub gm.pauseGame()
  page copy PG_BACK to PG_INTRO
  page write PG_BACK
  gm.quit = dlg.pause()
  page copy PG_INTRO to PG_BACK
  page write PG_BACK
  if not gm.quit then
    scr.present()
    key.thrustReset()
    gm.due = timer + TICK
  end if
end sub

sub game.run(diff)
  local k, k$, n, now, s
  s = scr.sc
  gm.diff = diff
  gm.gaps = 0
  gm.gems = 0
  gm.score = 0
  gm.over = FALSE
  gm.quit = FALSE
  gm.dead = FALSE
  gm.ready = TRUE
  gm.flap = FALSE
  gm.tk = 0
  gm.lv = 0
  gm.speed = df.v0(diff)
  gm.target = gm.speed
  gm.acc = 0
  cue.on = FALSE
  for k = 0 to 2
    bm.f(k) = -1
  next k
  ' clean scene: backdrop, then the sprites on top
  for k = 1 to 60
    sprite hide k
  next k
  page copy PG_BG to PG_BACK
  page write PG_BACK
  ob.clear()
  ' first column 2.5 s of flying ahead, the next ones a spacing apart
  ob.spawn(SHX + 60 + int(df.v0(diff) * 125), df.gap(diff), df.shift(diff), FALSE)
  do while ob.x(ob.last) < lay.aw + 8
    ob.spawn(ob.x(ob.last) + df.space(diff), df.gap(diff), df.shift(diff), TRUE)
  loop
  sh.by = lay.ah \ 2 - 30
  sh.y = sh.by
  sh.vy = 0
  sh.thr = 0
  sh.dy = sh.by
  sh.oy = sh.by
  sh.pose()
  sh.cur = sh.p
  sprite show SPR_SHIP + sh.cur, SHX * s, sh.oy * s, 4
  cue.show(0, lay.ah \ 2 - 70)
  scr.present()
  key.thrustReset()
  gm.due = timer + TICK
  do
    ' P / Esc pause; anything else typed during play (e.g. Space) is ignored
    k$ = inkey$
    do while k$ <> "" and not gm.quit
      if key.isPause(k$) then gm.pauseGame()
      k$ = inkey$
    loop
    if gm.quit then exit do
    if key.scan() then gm.flap = TRUE
    now = timer
    n = 0
    do while now >= gm.due and n < CATCHUP
      gm.tick()
      gm.due = gm.due + TICK
      n = n + 1
      if gm.dead then exit do
    loop
    if n >= CATCHUP and gm.due < now then gm.due = now + TICK
    if n > 0 then
      gm.render()
      if scr.dirty then scr.flush()
    end if
    if gm.dead then exit do
    pause max(1, min(IDLE_MS, gm.due - timer))
  loop
  gm.score = gm.gaps + GEM_BONUS * gm.gems
  if gm.dead then
    gm.crash()
    dlg.gameOver()
  end if
  sprite close all
  gm.sprites()
  scr.reloadIntro()
end sub
