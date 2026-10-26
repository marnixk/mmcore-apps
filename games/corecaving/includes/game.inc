' game.inc - one game: caves in order, the miner's input, the cave beat,
' deaths, time-outs, the cave clear with its time bonus, lives and score.
'
' How a cave runs - two interrupt handlers and the main loop:
'  key.onKey (ON KEY, input.inc) notes arrow presses and the pause key.
'  gm.tick (SETTICK, every TICK_MS) is the beat: it notices released keys
'    and, every cv.tick ms (one cave frame, on fixed "due" times), takes the
'    miner's move from the keys and runs one cave frame (cv.step). It only
'    changes the game state and never draws. What needs the main loop (the
'    miner died, the cave was cleared, the time ran out) is flagged in
'    gm.res; the tick then stops stepping until the main loop has dealt with
'    it.
'  The main loop (gm.playCave) draws: the changed cells (draw.inc) and the
'    status bar, and opens the pause dialog. While it draws (gm.drawing) the
'    tick skips its cave frame, so a frame never shows a half-done scan.
' Both handlers are only on during play; menus and dialogs read INKEY$.
'
' Moves: one tap = one step on the next cave frame; a held arrow keeps
' walking, one cell per cave frame, after REPEAT_MS. Space held with an arrow
' grabs the dirt or diamond next to the miner without moving.

const IDLE_MS = 4, TICK_MS = 5, REPEAT_MS = 170
' after a death the cave keeps going this many frames (the explosion plays)
const DEATH_FRAMES = 14
const GE_NONE = 0, GE_WON = 1, GE_DEAD = 2, GE_TIMEOUT = 3, GE_GIVEUP = 4, GE_QUIT = 5

dim gm.diff, gm.ci, gm.lives, gm.score, gm.reached
dim gm.res, gm.drawing, gm.deadT, cv.due
' key directions (up down left right) -> cave directions (up right down left)
dim gm.kd(3) = (0, 2, 3, 1)
dim integer gm_now, gm_d   ' the tick's scratch (globals: LOCAL is slow)

' ---- the beat (timer interrupt, every TICK_MS)
sub gm.tick()
  key.poll()
  if gm.drawing or gm.res <> GE_NONE then exit sub
  gm_now = timer
  if gm_now < cv.due then exit sub
  cv.due = cv.due + cv.tick
  if cv.due <= gm_now then cv.due = gm_now + cv.tick
  gm.input()
  cv.step()
  if cv.won then
    gm.res = GE_WON
  elseif cv.dead then
    gm.deadT = gm.deadT + 1
    if gm.deadT >= DEATH_FRAMES then gm.res = GE_DEAD
  elseif cv.timeout then
    gm.res = GE_TIMEOUT
  end if
end sub

' keys -> the miner's move for this cave frame (cv.pd, cv.snap)
sub gm.input()
  cv.pd = -1
  cv.snap = FALSE
  if cv.dead or cv.won or cv.timeout then exit sub
  gm_d = key.pop()
  if gm_d < 0 and key.held >= 0 then
    if gm_now >= key.t0 + REPEAT_MS then gm_d = key.held
  end if
  if gm_d >= 0 then
    cv.pd = gm.kd(gm_d)
    cv.snap = key.spaceDown()
  end if
end sub

' the key handler and the beat: on while a cave is played, off for dialogs,
' banners and between caves. Switching on starts afresh: no keys typed before
' count, the next cave frame is one cave tick from now.
sub gm.handlers(on)
  if on then
    key.reset()
    cv.due = timer + cv.tick
    on key key.onKey
    settick TICK_MS, gm.tick
  else
    settick 0, gm.tick
    on key 0
  end if
end sub

' draw what the cave frames changed (the tick waits meanwhile)
sub gm.draw()
  gm.drawing = TRUE
  if dqn > 0 then drw.cells()
  if hud.dirty then hud.draw()
  gm.drawing = FALSE
end sub

' pause: the dialog covers the scene; afterwards the scene comes back
function gm.pauseGame()
  gm.handlers(FALSE)
  gm.pauseGame = dlg.pause()
  scr.present()
  if gm.pauseGame = DLG_RESUME then gm.handlers(TRUE)
end function

' ---- one cave: returns GE_WON, GE_DEAD, GE_TIMEOUT, GE_GIVEUP or GE_QUIT.
' The main loop only draws and handles what the handlers flagged; it wakes
' every IDLE_MS (the tick and ON KEY also run during its PAUSE).
function gm.playCave()
  local r
  gm.res = GE_NONE
  gm.deadT = 0
  gm.drawing = FALSE
  gm.handlers(TRUE)
  do
    if key.pause then
      key.pause = FALSE
      r = gm.pauseGame()
      if r = DLG_QUIT then
        gm.playCave = GE_QUIT
        exit function
      elseif r = DLG_GIVEUP then
        gm.playCave = GE_GIVEUP
        exit function
      end if
    end if
    gm.draw()
    if gm.res <> GE_NONE then exit do
    pause IDLE_MS
  loop
  gm.handlers(FALSE)
  gm.draw()
  gm.playCave = gm.res
end function

' set up and show cave gm.ci, with the intro banner
sub gm.startCave()
  local k
  k = gm.ci
  cv.load(k, gm.diff)
  if cvd.bonus(k) then
    hud.cave$ = "BONUS " + cvd.id$(k)
  else
    hud.cave$ = "CAVE " + cvd.id$(k)
  end if
  hud.lives = gm.lives
  drw.all()
  scr.present()
  dlg.banner(hud.cave$, cvd.title$(k), F_TAN, "COLLECT " + str$(cv.need) + " DIAMONDS", F_GEM, 1800)
  scr.present()
end sub

' cave cleared: the seconds left count down into the score, then the banner
sub gm.caveWon()
  local t, n
  pause 300
  do while cv.left > 0
    n = max(1, cv.left \ 25)
    cv.left = cv.left - n
    gm.score = gm.score + n * TIME_PTS
    hud.draw()
    pause 15
  loop
  if cvd.bonus(gm.ci) then
    if gm.lives < LIVES_MAX then
      gm.lives = gm.lives + 1
      hud.lives = gm.lives
      hud.draw()
      dlg.banner("BONUS CLEARED", "EXTRA LIFE!", F_GEM, "", 0, 2200)
    else
      dlg.banner("BONUS CLEARED", "", 0, "", 0, 2000)
    end if
  else
    dlg.banner("CAVE " + cvd.id$(gm.ci) + " CLEARED", cvd.title$(gm.ci), F_TAN, "", 0, 2000)
  end if
end sub

sub game.run(diff, start)
  local r, gQuit, gOver, gWon
  gm.diff = diff
  gm.ci = start
  gm.lives = LIVES0
  gm.score = 0
  gm.reached = start
  gQuit = FALSE
  gOver = FALSE
  gWon = FALSE
  do
    if not cvd.bonus(gm.ci) then hs.reach(diff) = max(hs.reach(diff), gm.ci)
    gm.reached = max(gm.reached, gm.ci)
    gm.startCave()
    r = gm.playCave()
    gm.score = gm.score + cv.score
    if r = GE_QUIT then
      gQuit = TRUE
      exit do
    elseif r = GE_WON then
      gm.caveWon()
      gm.ci = gm.ci + 1
    else
      if r = GE_TIMEOUT then
        pause 500
        dlg.banner("OUT OF TIME", "", 0, "", 0, 1800)
      end if
      if cvd.bonus(gm.ci) then
        dlg.banner("BONUS OVER", "ON TO THE NEXT CAVE", F_TAN, "", 0, 1800)
        gm.ci = gm.ci + 1
      else
        gm.lives = gm.lives - 1
        hud.lives = gm.lives
        hud.draw()
        if gm.lives <= 0 then
          gOver = TRUE
          pause 500
          exit do
        end if
      end if
    end if
    if gm.ci >= CAVE_N then
      gWon = TRUE
      gm.reached = CAVE_N
      exit do
    end if
  loop
  ' keep the caves reached even without a high score
  hs.save()
  if not gQuit then dlg.gameOver(gWon)
end sub
