' game.inc - the play loop: gravity, lock delay, moves, SRS rotation, hold,
' hard drop, line clears, scoring, levels and the HUD.
'
' Scoring (modern guideline): 1/2/3/4 lines = 100/300/500/800 x level,
' hard drop 2 points per cell. Difficulty sets the lines per level
' (easy 15, medium 10, hard 5). Gravity follows the guideline curve
' (0.8 - (level - 1) * 0.007) ^ (level - 1) seconds per row, capped at
' GM_MINGRAV ms so the top levels stay playable.

const LOCK_MS = 500, LOCK_RESETS = 15, GM_MINGRAV = 50, GM_MAXLEVEL = 99
const CLEAR_STEP = 35, TETRIS_HOLD = 450

dim gm.lv, gm.level, gm.lines, gm.score, gm.grav
dim gm.next(2), gm.hold, gm.holdUsed
dim gm.fallT, gm.lockT, gm.resets, gm.over, gm.quit, gm.dirty
dim p.k, p.r, p.x, p.y, p.lowest
' values currently shown in the HUD (redraw only on change)
dim sh.level, sh.lines, sh.score

function gm.gravity(lvl)
  local float t
  t = 1000 * (0.8 - (lvl - 1) * 0.007) ^ (lvl - 1)
  if t < GM_MINGRAV then t = GM_MINGRAV
  gm.gravity = int(t)
end function

' ---- HUD (drawn on PG_STACK, then that rectangle is copied to the display)
sub gm.fromStack(x, y, w, h)
  page write 0
  blit x, y, x, y, w, h, PG_STACK
end sub

sub gm.hudValue(i, v)
  local s = scr.sc, x, y, w, h, v$
  v$ = str$(v)
  x = lay.stX + 6 * s
  y = lay.valY0 + i * lay.valStep
  w = lay.valR + 2 * s - x
  h = 18 * s
  page write PG_STACK
  blit x, y, x, y, w, h, PG_GAME
  if len(v$) <= 6 then
    bigR v$, lay.valR, y, B_CREAM
  else
    txtR v$, lay.valR, y + 4 * s, F_CREAM
  end if
  gm.fromStack(x, y, w, h)
end sub

sub gm.hudBest()
  local s = scr.sc, x, w, b
  b = max(hs.score(gm.lv, 0), gm.score)
  x = lay.stX + 50 * s
  w = lay.valR + 2 * s - x
  page write PG_STACK
  blit x, lay.bestY, x, lay.bestY, w, 9 * s, PG_GAME
  if b > 0 then
    txtR str$(b), lay.valR, lay.bestY, F_GREY
  else
    txtR "-", lay.valR, lay.bestY, F_GREY
  end if
  gm.fromStack(x, lay.bestY, w, 9 * s)
end sub

sub gm.hudHold()
  page write PG_STACK
  blit lay.holdX, lay.holdY, lay.holdX, lay.holdY, lay.slotW, lay.slotH, PG_GAME
  if gm.hold >= 0 then ui.mini(gm.hold, lay.holdX, lay.holdY, lay.slotW, lay.slotH)
  gm.fromStack(lay.holdX, lay.holdY, lay.slotW, lay.slotH)
end sub

sub gm.hudNext()
  local i, y
  page write PG_STACK
  for i = 0 to 2
    y = lay.nextY + i * lay.nextStep
    blit lay.nextX, y, lay.nextX, y, lay.slotW, lay.slotH, PG_GAME
    ui.mini(gm.next(i), lay.nextX, y, lay.slotW, lay.slotH)
  next i
  gm.fromStack(lay.nextX, lay.nextY, lay.slotW, 2 * lay.nextStep + lay.slotH)
end sub

' level / lines / score / best, only what changed
sub gm.hudStats(force)
  if force or sh.level <> gm.level then
    gm.hudValue(0, gm.level)
    sh.level = gm.level
  end if
  if force or sh.lines <> gm.lines then
    gm.hudValue(1, gm.lines)
    sh.lines = gm.lines
  end if
  if force or sh.score <> gm.score then
    gm.hudValue(2, gm.score)
    if force or gm.score > hs.score(gm.lv, 0) then gm.hudBest()
    sh.score = gm.score
  end if
end sub

sub gm.hudDiff()
  local r
  if gm.lv = LV_EASY then r = F_GREEN
  if gm.lv = LV_MEDIUM then r = F_GOLD
  if gm.lv = LV_HARD then r = F_RED
  page write PG_STACK
  txtC lv.name$(gm.lv), lay.stX + lay.stW \ 2, lay.diffY, r
end sub

' ---- well
' compose the stack + ghost + falling piece on PG_BACK, copy the well rectangle
sub gm.drawWell()
  local gy, w, h
  w = 10 * lay.cell
  h = 20 * lay.cell
  page write PG_BACK
  blit lay.wx, lay.wy, lay.wx, lay.wy, w, h, PG_STACK
  if not gm.over then
    gy = bd.dropY(p.k, p.r, p.x, p.y)
    if gy <> p.y then bd.piece(p.k, p.r, p.x, gy, TRUE)
    bd.piece(p.k, p.r, p.x, p.y, FALSE)
  end if
  scr.show(lay.wx, lay.wy, w, h)
  gm.dirty = FALSE
end sub

' everything (after a pause / dialog)
sub gm.redrawAll()
  page copy PG_STACK to PG_BACK
  gm.drawWell()
  page copy PG_BACK to 0
end sub

' ---- piece life cycle
' put kind k at the top; FALSE when it does not fit (block out)
function gm.spawn(k)
  local i, mny = 9
  for i = 0 to 3
    mny = min(mny, pc.cy(k, 0, i))
  next i
  p.k = k
  p.r = 0
  p.x = 3
  p.y = BD_HID - mny
  p.lowest = p.y
  gm.fallT = timer + gm.grav
  gm.lockT = 0
  gm.resets = 0
  gm.dirty = TRUE
  gm.spawn = bd.fits(p.k, p.r, p.x, p.y)
end function

sub gm.spawnNext()
  local k
  k = gm.next(0)
  gm.next(0) = gm.next(1)
  gm.next(1) = gm.next(2)
  gm.next(2) = pc.next()
  gm.hudNext()
  if not gm.spawn(k) then gm.over = TRUE
end sub

' a successful move or rotation: lock-delay bookkeeping (move reset, capped)
sub gm.moved()
  gm.dirty = TRUE
  if p.y > p.lowest then
    p.lowest = p.y
    gm.resets = 0
  end if
  if bd.fits(p.k, p.r, p.x, p.y + 1) then
    gm.lockT = 0
  elseif gm.lockT <> 0 then
    if gm.resets < LOCK_RESETS then
      gm.lockT = timer
      gm.resets = gm.resets + 1
    end if
  end if
end sub

sub gm.shift(dx)
  if bd.fits(p.k, p.r, p.x + dx, p.y) then
    p.x = p.x + dx
    gm.moved()
  end if
end sub

' dir 0 = clockwise, 1 = counter-clockwise; SRS kicks (I has its own table)
sub gm.rotate(dir)
  local nr, t, tr, i, nx, ny
  if p.k = 1 then exit sub
  if dir = 0 then nr = (p.r + 1) mod 4 else nr = (p.r + 3) mod 4
  if p.k = 0 then t = 1 else t = 0
  tr = p.r * 2 + dir
  for i = 0 to 4
    nx = p.x + pc.kx(t, tr, i)
    ny = p.y - pc.ky(t, tr, i)
    if bd.fits(p.k, nr, nx, ny) then
      p.r = nr
      p.x = nx
      p.y = ny
      gm.moved()
      exit sub
    end if
  next i
end sub

sub gm.doHold()
  local k
  if gm.holdUsed then exit sub
  k = p.k
  if gm.hold < 0 then
    gm.hold = k
    gm.hudHold()
    gm.spawnNext()
  else
    p.k = gm.hold
    gm.hold = k
    gm.hudHold()
    if not gm.spawn(p.k) then gm.over = TRUE
  end if
  gm.holdUsed = TRUE
end sub

sub gm.hardDrop()
  local ny
  ny = bd.dropY(p.k, p.r, p.x, p.y)
  gm.score = gm.score + 2 * (ny - p.y)
  p.y = ny
  gm.lockPiece()
end sub

' TETRIS! callout across the well (current write page)
sub gm.banner(t$, r, row)
  local s = scr.sc, y, w
  w = 10 * lay.cell
  y = lay.wy + row * lay.cell
  rfill lay.wx, y, w, 25 * s, C_INK
  rfill lay.wx, y + s, w, s, C_G3
  rfill lay.wx, y + 23 * s, w, s, C_G3
  bigC t$, lay.wx + w \ 2, y + 5 * s, r
end sub

' line-clear wipe from the centre outwards using the 4 flash frames
sub gm.clearAnim()
  local st, i, r, c, d, f, row, t0
  row = bd.full(0) - BD_HID - 3
  if row < 1 then row = bd.full(bd.nFull - 1) - BD_HID + 2
  for st = 0 to 8
    t0 = timer
    page write PG_BACK
    blit lay.wx, lay.wy, lay.wx, lay.wy, 10 * lay.cell, 20 * lay.cell, PG_STACK
    for i = 0 to bd.nFull - 1
      r = bd.full(i)
      if r >= BD_HID then
        for c = 0 to BD_W - 1
          d = min(abs(c - 4), abs(c - 5))
          f = st - d
          if f >= 0 then
            if f > 3 then f = -1
            bd.flashTile(f, c, r)
          end if
        next c
      end if
    next i
    if bd.nFull = 4 then gm.banner("TETRIS!", B_GOLD, row)
    scr.show(lay.wx, lay.wy, 10 * lay.cell, 20 * lay.cell)
    do while timer - t0 < CLEAR_STEP
    loop
  next st
  if bd.nFull = 4 then pause TETRIS_HOLD
end sub

sub gm.lockPiece()
  local n, i, lv0
  if bd.lock(p.k, p.r, p.x, p.y) then gm.over = TRUE
  page write PG_STACK
  bd.piece(p.k, p.r, p.x, p.y, FALSE)
  bd.findFull()
  n = bd.nFull
  if n > 0 then
    gm.clearAnim()
    bd.removeFull()
    bd.drawStack(FALSE)
    if n = 1 then gm.score = gm.score + 100 * gm.level
    if n = 2 then gm.score = gm.score + 300 * gm.level
    if n = 3 then gm.score = gm.score + 500 * gm.level
    if n = 4 then gm.score = gm.score + 800 * gm.level
    gm.lines = gm.lines + n
    lv0 = gm.level
    gm.level = min(GM_MAXLEVEL, 1 + gm.lines \ lv.lines(gm.lv))
    if gm.level <> lv0 then gm.grav = gm.gravity(gm.level)
  end if
  gm.hudStats(FALSE)
  gm.holdUsed = FALSE
  if not gm.over then gm.spawnNext()
  gm.dirty = TRUE
end sub

' gravity + lock delay, called every loop pass
sub gm.tick()
  local now = timer
  if now >= gm.fallT then
    do while now >= gm.fallT and bd.fits(p.k, p.r, p.x, p.y + 1)
      p.y = p.y + 1
      gm.fallT = gm.fallT + gm.grav
      gm.dirty = TRUE
    loop
    if p.y > p.lowest then
      p.lowest = p.y
      gm.resets = 0
    end if
    if gm.fallT <= now then gm.fallT = now + gm.grav
  end if
  if bd.fits(p.k, p.r, p.x, p.y + 1) then
    gm.lockT = 0
  else
    if gm.lockT = 0 then
      gm.lockT = now
      if gm.resets >= LOCK_RESETS then gm.lockT = now - LOCK_MS
    end if
    if now - gm.lockT >= LOCK_MS then gm.lockPiece()
  end if
end sub

' ---- one game; returns when it is over or the player quit
sub game.run(lv)
  local k, t0, rk, n$
  gm.lv = lv
  gm.level = 1
  gm.lines = 0
  gm.score = 0
  gm.grav = gm.gravity(1)
  gm.hold = -1
  gm.holdUsed = FALSE
  gm.over = FALSE
  gm.quit = FALSE
  bd.clear()
  pc.bagN = 0
  gm.next(0) = pc.next()
  gm.next(1) = pc.next()
  gm.next(2) = pc.next()
  ' build the live scene: clean game scene + difficulty + HUD
  page copy PG_GAME to PG_STACK
  gm.hudDiff()
  sh.level = -1
  sh.lines = -1
  sh.score = -1
  page write PG_STACK
  gm.hudStats(TRUE)
  gm.hudHold()
  gm.spawnNext()
  gm.redrawAll()
  key.flush()
  do
    key.nrPoll()
    do
      k = key.get()
      if k = 0 then exit do
      if k = K_LEFT or k = K_RIGHT then k = key.arrowKeep(k)
      if k = K_LEFT then
        gm.shift(-1)
      elseif k = K_RIGHT then
        gm.shift(1)
      elseif k = K_UP then
        if key.once(k, K_UP) then gm.rotate(0)
      elseif k = asc("z") or k = asc("Z") then
        if key.once(k, asc("z")) then gm.rotate(1)
      elseif k = asc("c") or k = asc("C") then
        if key.once(k, asc("c")) then gm.doHold()
      elseif k = K_DOWN then
        if key.once(k, K_DOWN) then gm.hardDrop()
      elseif k = K_ESC then
        if gm.dirty then gm.drawWell()
        t0 = timer
        gm.quit = dlg.pause()
        if not gm.quit then
          t0 = timer - t0
          gm.fallT = gm.fallT + t0
          if gm.lockT <> 0 then gm.lockT = gm.lockT + t0
          gm.redrawAll()
        end if
      end if
      if gm.over or gm.quit then exit do
    loop
    if gm.over or gm.quit then exit do
    k = key.repeat()
    if k = K_LEFT then gm.shift(-1)
    if k = K_RIGHT then gm.shift(1)
    gm.tick()
    if gm.dirty and not gm.over then gm.drawWell()
    if not key.waiting() then pause 2
  loop until gm.over or gm.quit
  if gm.quit then
    key.flush()
    exit sub
  end if
  ' ---- game over
  bd.drawStack(TRUE)
  page write PG_STACK
  dlg.gameOver()
  page write 0
  blit lay.wx, lay.wy, lay.wx, lay.wy, 10 * lay.cell, 20 * lay.cell, PG_STACK
  pause 700
  key.flush()
  do
    k = key.get()
    if k = 0 then pause 15
  loop until key.isEnter(k) or k = K_ESC or k = asc(" ")
  rk = hs.rank(lv, gm.score)
  if rk >= 0 then
    n$ = dlg.enterName$(lv, rk)
    hs.insert(lv, rk, n$, gm.score, gm.level)
    hs.save()
    dlg.scores(lv, rk)
  end if
  key.flush()
end sub
