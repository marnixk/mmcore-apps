' bonus.inc - the Salsa Shuffle, after every BONUS_EVERY-th round.
' Three salsa bowls; one hides the golden chip. It is shown, then the bowls
' swap places, faster each time; the player picks one with Left / Right and
' Space (or Enter). Found: bonus points and an extra chef hat (max 5).
' The bowls, the chip and the pointer are SPRITEs read from bowl_<res>.png
' (loaded onto PG_SAVE, which no dialog needs meanwhile). The swaps come
' from the engine's random numbers. Timing: one frame per 20 ms.
const BONUS_PTS = 2000
dim bw.slot(2), bw.cx(2), bw.top, bw.sp, bw.cb
dim integer bw_i, bw_f, bw_t
' pause: bw.res = BR_AGAIN (R: shuffle again from the start) or BR_QUIT (Q);
' bw.tofs = ms spent paused (the frame clock skips it); bw.rng = the engine's
' random state at the start, so a fresh shuffle is the same shuffle
const BR_NONE = 0, BR_AGAIN = 1, BR_QUIT = 2
dim bw.res, bw.picked, bw.tofs, bw.rng, bw_r, bw_k$
dim float bw_t0

' art x of slot s's centre
sub bonus.layout()
  bw.sp = min(100, (lay.artw - BW_W) \ 2 - 4)
  for bw_i = 0 to 2
    bw.cx(bw_i) = lay.artw \ 2 + (bw_i - 1) * bw.sp
  next bw_i
  bw.top = lay.ty - BW_H + 3
end sub

' wait for the next 20 ms frame (t = start time, f = frame number)
sub bonus.frame(t, f)
  if scr.dirty then scr.flush()
  do while timer < t + bw.tofs + 20 * f
    bonus.keys()
    if bw.res <> BR_NONE then exit sub
    pause 2
  loop
end sub

' P / Esc opens the pause dialog (handlers are off between rounds: plain INKEY$)
sub bonus.keys()
  bw_k$ = inkey$
  if bw_k$ = "" then exit sub
  if key.isPause(bw_k$) then bonus.pause()
end sub

sub bonus.pause()
  bw_t0 = timer
  bw_r = dlg.pause()
  scr.present()
  bw.tofs = bw.tofs + timer - bw_t0
  ' no second try once the chip has been shown
  if bw_r = DLG_RESTART and not bw.picked then bw.res = BR_AGAIN
  if bw_r = DLG_QUIT then bw.res = BR_QUIT
end sub

' wait ms (keys still checked)
sub bonus.hold(ms)
  bw_t0 = timer + ms
  do while timer < bw_t0 + bw.tofs
    bonus.keys()
    if bw.res <> BR_NONE then exit sub
    pause 5
  loop
end sub

' bowl i at art centre x, lifted by up art px (queued with SPRITE NEXT)
sub bonus.put(i, x, up)
  sprite next SPR_BOWL + i, (x - BW_W \ 2) * scr.k, (bw.top - up) * scr.k
end sub

sub bonus.markBand()
  scr.mark(0, (bw.top - 40) * scr.k, scr.W, (BW_H + 42) * scr.k)
end sub

' the line of text under the table (UI px)
sub bonus.line(s$, st)
  local y
  y = lay.ah - 16
  blit 0, y * scr.sc, 0, y * scr.sc, scr.W, 12 * scr.sc, PG_SCN
  if s$ <> "" then txtC s$, lay.aw \ 2, y, st
  scr.mark(0, y * scr.sc, scr.W, 12 * scr.sc)
end sub

' lift bowl i (at its slot) up and down: frames, held for ms at the top
sub bonus.lift(i, hold)
  local t, f, up
  bw.tofs = 0
  t = timer
  for f = 1 to 10
    up = f * 2
    bonus.put(i, bw.cx(bw.slot(i)), up)
    sprite move
    bonus.markBand()
    bonus.frame(t, f)
    if bw.res <> BR_NONE then exit sub
  next f
  if hold > 0 then
    bw.tofs = 0
    bonus.hold(hold)
    if bw.res <> BR_NONE then exit sub
    bw.tofs = 0
    t = timer
    for f = 1 to 10
      up = 20 - f * 2
      bonus.put(i, bw.cx(bw.slot(i)), up)
      sprite move
      bonus.markBand()
      bonus.frame(t, f)
      if bw.res <> BR_NONE then exit sub
    next f
  end if
end sub

' the golden chip under bowl bw.cb (drawn below the bowls: bowl hidden and re-shown)
sub bonus.showChip()
  local s
  s = bw.slot(bw.cb)
  sprite hide SPR_BOWL + bw.cb
  sprite show SPR_CHIP, (bw.cx(s) - GC_W \ 2) * scr.k, (lay.ty - GC_H + 1) * scr.k, 1
  sprite show SPR_BOWL + bw.cb, (bw.cx(s) - BW_W \ 2) * scr.k, bw.top * scr.k, 1
  bonus.markBand()
end sub

' the Salsa Shuffle; afterwards bw.res = BR_QUIT if the player quit from the pause
sub bonus.run(n)
  bw.rng = en.rng
  do
    bw.res = BR_NONE
    bw.picked = FALSE
    bonus.play(n)
    if bw.res <> BR_AGAIN then exit do
    en.rng = bw.rng
  loop
  vw.clear()
end sub

sub bonus.play(n)
  local i, j, a, b, ia, ib, d, nsw, t, f, x, sel, k$, won, ox
  vw.clear()
  scr.loadScn("bonus")
  page copy PG_SCN to PG_BACK
  page write PG_BACK
  hud.draw(en.lives, "BONUS", -1)
  page write PG_SAVE
  cls rgb(0, 0, 0)
  load png "gfx/bowl_" + lay.res$ + ".png", 0, 0
  page write PG_BACK
  for i = 0 to 2
    sprite read SPR_BOWL + i, 0, 0, BW_W * scr.k, BW_H * scr.k, PG_SAVE
  next i
  sprite read SPR_CHIP, GC_X * scr.k, 0, GC_W * scr.k, GC_H * scr.k, PG_SAVE
  sprite read SPR_PTR, PT_X * scr.k, 0, PT_W * scr.k, PT_H * scr.k, PG_SAVE
  bonus.layout()
  bw.cb = en.rnd(3)
  sprite show SPR_CHIP, (bw.cx(bw.cb) - GC_W \ 2) * scr.k, (lay.ty - GC_H + 1) * scr.k, 1
  for i = 0 to 2
    bw.slot(i) = i
    sprite show SPR_BOWL + i, (bw.cx(i) - BW_W \ 2) * scr.k, bw.top * scr.k, 1
  next i
  bonus.line("WATCH THE GOLDEN CHIP!", F_GOLD)
  scr.present()
  bw.tofs = 0
  bonus.hold(600)
  if bw.res <> BR_NONE then exit sub
  bonus.lift(bw.cb, 1100)
  if bw.res <> BR_NONE then exit sub
  sprite hide SPR_CHIP
  ' the shuffle
  nsw = min(14, 4 + 2 * n + en.diff)
  for j = 1 to nsw
    bonus.line("SHUFFLE " + str$(j) + " OF " + str$(nsw), F_CREAM)
    a = en.rnd(3)
    b = (a + 1 + en.rnd(2)) mod 3
    for i = 0 to 2
      if bw.slot(i) = a then ia = i
      if bw.slot(i) = b then ib = i
    next i
    d = max(8, 30 - 2 * j - 2 * n - 2 * en.diff)
    bw.tofs = 0
    t = timer
    for f = 1 to d
      x = bw.cx(a) + (bw.cx(b) - bw.cx(a)) * f \ d
      bonus.put(ia, x, 112 * f * (d - f) \ (d * d))
      x = bw.cx(b) + (bw.cx(a) - bw.cx(b)) * f \ d
      bonus.put(ib, x, 0)
      sprite move
      bonus.markBand()
      bonus.frame(t, f)
      if bw.res <> BR_NONE then exit sub
    next f
    bw.slot(ia) = b
    bw.slot(ib) = a
  next j
  ' the pick
  bonus.line("WHERE IS THE GOLDEN CHIP?", F_GOLD)
  sel = 1
  sprite show SPR_PTR, (bw.cx(sel) - PT_W \ 2) * scr.k, (bw.top - PT_H - 6) * scr.k, 1
  bonus.markBand()
  scr.flush()
  key.drain()
  do
    k$ = inkey$
    if k$ = "" then
      pause 5
    else
      ox = sel
      if key.isPause(k$) then
        bonus.pause()
        if bw.res <> BR_NONE then exit sub
      end if
      if asc(k$) = K_LEFT then sel = max(0, sel - 1)
      if asc(k$) = K_RIGHT then sel = min(2, sel + 1)
      if k$ = " " or key.isEnter(k$) then exit do
      if sel <> ox then
        sprite move SPR_PTR, (bw.cx(sel) - PT_W \ 2) * scr.k, (bw.top - PT_H - 6) * scr.k
        bonus.markBand()
        scr.flush()
      end if
    end if
  loop
  sprite hide SPR_PTR
  bonus.markBand()
  bw.picked = TRUE
  won = (bw.slot(bw.cb) = sel)
  bonus.showChip()
  for i = 0 to 2
    if bw.slot(i) = sel then j = i
  next i
  bonus.lift(j, 0)
  if won then
    en.add(BONUS_PTS)
    if en.lives < MAX_LIVES then en.lives = en.lives + 1
    bonus.line("GOLDEN! +" + str$(BONUS_PTS) + " AND AN EXTRA CHEF HAT", F_GOLD)
    hud.draw(en.lives, "BONUS", -1)
  else
    bonus.lift(bw.cb, 0)
    bonus.line("SO CLOSE! IT WAS OVER HERE", F_CREAM)
  end if
  scr.flush()
  key.drain()
  dlg.waitFor(2400)
  key.drain()
  vw.clear()
end sub
