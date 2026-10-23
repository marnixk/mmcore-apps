' game.inc - the play loop: cursor, placing digits, misses, timer, score, win

dim gm.level, gm.misses, gm.elapsed, gm.t0, gm.lastMin, gm.frame, gm.tPulse
dim gm.curR, gm.curC   ' where the cursor is drawn on the back buffer
dim gm.base(2) = (3000, 6000, 10000)
const GM_SEC_COST = 2, GM_MISS_COST = 150

function gm.secs()
  gm.secs = (gm.elapsed + (timer - gm.t0)) \ 1000
end function

function gm.score()
  local s
  s = gm.base(gm.level) - gm.secs() * GM_SEC_COST - gm.misses * GM_MISS_COST
  if s < 100 then s = 100
  gm.score = s
end function

' side panel on the current write page. The score is not shown while playing
' (it is still computed and shown on the win screen); the timer shows whole
' minutes only, so the panel changes at most once a minute.
sub game.panel()
  local sc = scr.sc, ix, iy, t$
  blit lay.panX, 0, lay.px, lay.py, lay.pw, lay.ph, PG_PANELS, 4
  ix = lay.px + 10 * sc
  iy = lay.py + 10 * sc
  dlg.icon(2, ix, iy)
  txt lv.name$(gm.level), ix + 11 * sc, iy, F_GREEN
  dlg.icon(0, ix, iy + 25 * sc)
  txt fmtMins$(gm.secs() \ 60), ix + 11 * sc, iy + 25 * sc, F_BROWN
  dlg.icon(1, ix, iy + 50 * sc)
  txt str$(gm.misses) + " MISS", ix + 11 * sc, iy + 50 * sc, F_BROWN
  dlg.icon(4, ix, iy + 75 * sc)
  txt str$(bd.empties()) + " LEFT", ix + 11 * sc, iy + 75 * sc, F_BROWN
  t$ = "ESC PAUSE"
  txt t$, lay.px + (lay.pw - txtW(t$)) \ 2, lay.py + lay.ph - 17 * sc, F_BROWN
end sub

' redraw just the side panel without flicker: rebuild it on the back buffer
' (forest behind it restored from the clean game scene first), then copy that
' one rectangle to the display with a single opaque BLIT
sub game.panelUpdate()
  page write PG_BACK
  blit lay.px, lay.py, lay.px, lay.py, lay.pw, lay.ph, PG_GAME
  game.panel()
  page write 0
  blit lay.px, lay.py, lay.px, lay.py, lay.pw, lay.ph, PG_BACK
end sub

' incremental, flicker-free update after moves, digits and the cursor pulse.
' Everything is rebuilt on the back buffer first: the old cursor rectangle is
' restored from the clean game scene, only the cells whose look changed are
' redrawn (unless onlyCursor), the cursor goes on top, the panel is rebuilt
' when asked, and then the board (and panel) rectangles are copied to the
' display with one opaque BLIT each.
sub game.update(onlyCursor, withPanel)
  local x, y, cell
  page write PG_BACK
  x = lay.bx + lay.off(gm.curC) - scr.sc
  y = lay.by + lay.off(gm.curR) - scr.sc
  blit x, y, x, y, lay.cur, lay.cur, PG_GAME
  cell = gm.curR * 9 + gm.curC
  bd.tShown(cell) = -1
  if onlyCursor then
    bd.drawCell(gm.curR, gm.curC)
  else
    bd.refresh(withPanel, gm.curR, gm.curC)
  end if
  bd.drawCursor(gm.frame)
  gm.curR = bd.r
  gm.curC = bd.c
  if withPanel then
    blit lay.px, lay.py, lay.px, lay.py, lay.pw, lay.ph, PG_GAME
    game.panel()
  end if
  page write 0
  blit lay.bx, lay.by, lay.bx, lay.by, lay.B, lay.B, PG_BACK
  if withPanel then blit lay.px, lay.py, lay.px, lay.py, lay.pw, lay.ph, PG_BACK
end sub

' full scene into the back buffer (forest, board, cells, cursor, panel)
sub game.compose()
  page write PG_BACK
  page copy PG_GAME to PG_BACK
  bd.drawAll()
  bd.drawCursor(gm.frame)
  gm.curR = bd.r
  gm.curC = bd.c
  game.panel()
end sub

sub game.redraw()
  game.compose()
  scr.present()
end sub

' move the cursor one cell for arrow code k (wraps around)
sub game.move(k)
  if k = K_UP then bd.r = (bd.r + 8) mod 9
  if k = K_DOWN then bd.r = (bd.r + 1) mod 9
  if k = K_LEFT then bd.c = (bd.c + 8) mod 9
  if k = K_RIGHT then bd.c = (bd.c + 1) mod 9
end sub

' place digit d (0 clears) at the cursor
sub game.put(d)
  local cell = bd.r * 9 + bd.c
  if bd.giv(cell) then exit sub
  if bd.g(cell) = d then exit sub
  bd.g(cell) = d
  bd.check()
  if d > 0 and bd.clash(cell) then gm.misses = gm.misses + 1
end sub

sub game.run(lv)
  local k, quitReq = FALSE, won = FALSE, secs, mins, score, rk, n$, moved, dig
  gm.level = lv
  gm.misses = 0
  gm.elapsed = 0
  gm.frame = 0
  gen.deal(lv)
  bd.check()
  bd.r = 4
  bd.c = 4
  gm.t0 = timer
  gm.lastMin = 0
  gm.tPulse = timer
  if not mus.on then music.start()
  game.redraw()
  key.flush()
  do
    music.poll()
    ' drain every queued key first, then draw once: held or fast repeated
    ' arrows never wait for a redraw per press
    moved = FALSE
    dig = FALSE
    do
      k = key.get()
      if k = 0 then exit do
      if k >= K_UP and k <= K_RIGHT then k = key.arrowKeep(k)
      if k >= K_UP and k <= K_RIGHT then
        game.move(k)
        moved = TRUE
      elseif k >= 49 and k <= 57 then
        game.put(k - 48)
        dig = TRUE
        if bd.empties() = 0 and bd.nClash = 0 then won = TRUE
      elseif k = 48 or k = K_DEL or k = K_BKSP then
        game.put(0)
        dig = TRUE
      elseif k = K_ESC then
        if moved or dig then game.update(FALSE, dig)
        moved = FALSE
        dig = FALSE
        gm.elapsed = gm.elapsed + (timer - gm.t0)
        quitReq = dlg.pause()
        gm.t0 = timer
        if not quitReq then game.redraw()
      end if
    loop until quitReq or won
    ' a held arrow keeps moving (KEYDOWN auto-repeat)
    k = 0
    if not (quitReq or won) then k = key.repeat()
    if k > 0 then game.move(k) : moved = TRUE
    if moved or dig then game.update(FALSE, dig)
    ' once a minute: rebuild the side panel off-screen and copy it in one go
    mins = gm.secs() \ 60
    if mins <> gm.lastMin then
      gm.lastMin = mins
      game.panelUpdate()
    end if
    ' gentle cursor pulse (also composed off-screen)
    if timer - gm.tPulse > 450 then
      gm.tPulse = timer
      gm.frame = 1 - gm.frame
      game.update(TRUE, FALSE)
    end if
    if not key.waiting() then pause 10
  loop until quitReq or won

  if won then
    secs = gm.secs()
    score = gm.score()
    music.jingle("sfx/win.wav")
    rk = hs.rank(lv, score)
    dlg.win(lv, secs, gm.misses, score, rk >= 0)
    if rk >= 0 then
      hs.insert(lv, rk, "", score, secs)
      n$ = dlg.enterName$(lv, rk)
      hs.name$(lv, rk) = n$
      hs.save()
      game.showTable(lv, rk)
    end if
  end if
  key.flush()
end sub

' show the table after entering a name, until a key is pressed
sub game.showTable(lv, rk)
  local k
  page write PG_BACK
  page copy PG_GAME to PG_BACK
  bd.drawAll()
  dlg.table(lv, rk, -1, "", FALSE)
  scr.present()
  key.flush()
  do
    k = key.get()
    pause 15
  loop until k <> 0
end sub
