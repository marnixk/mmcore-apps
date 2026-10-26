' game.inc - one game: levels in order, the player's input, the board's
' beat, deaths and retries, the level clear and the score.
'
' How a level runs - two interrupt handlers and the main loop:
'  key.onKey (ON KEY, input.inc) notes arrow presses, P / Esc and R.
'  gm.tick (SETTICK, every TICK_RATE ms) is the beat: it notices released
'    keys and, every TICK_MS (on fixed "due" times), takes the player's move
'    from the keys and runs one board tick (en.step). It only changes the
'    game state and never draws. When the attempt is over (won, or died) it
'    sets gm.res and stops stepping until the main loop has dealt with it.
'  The main loop (gm.play) draws: the changed cells and the scroll
'    (view.inc), the side panel (hud.inc), and opens the pause dialog. While
'    it draws (gm.drawing) the tick skips its step, so a frame never shows
'    a half-done tick.
' Both handlers are only on during play; menus and dialogs read INKEY$.
'
' Moves: one tap = one step as soon as the player may move (a step takes
' MOVE_T ticks); a held arrow keeps walking after REPEAT_MS.

const IDLE_MS = 4, TICK_RATE = 5, REPEAT_MS = 250
const GE_NONE = 0, GE_WON = 1, GE_DEAD = 2, GE_RESTART = 3, GE_QUIT = 4

dim gm.diff, gm.li, gm.score, gm.retries, gm.reached, gm.start
dim gm.res, gm.drawing, gm.due
' key directions (up down left right) -> board directions (up right down left)
dim gm.kd(3) = (0, 2, 3, 1)
dim integer gm_now, gm_d   ' the tick's scratch (globals: LOCAL is slow)

' ---- the beat (timer interrupt, every TICK_RATE ms)
sub gm.tick()
  key.poll()
  if gm.drawing or gm.res <> GE_NONE then exit sub
  gm_now = timer
  if gm_now < gm.due then exit sub
  gm.due = gm.due + TICK_MS
  if gm.due <= gm_now then gm.due = gm_now + TICK_MS
  gm.input()
  en.step()
  if en.won then
    gm.res = GE_WON
  elseif en.dead then
    gm.res = GE_DEAD
  end if
end sub

' keys -> the player's move for this tick (en.inp). Taps wait in the queue
' until the player may move; while sliding only a held arrow counts (it
' steers off when the slide is blocked).
sub gm.input()
  en.inp = -1
  if not en.ready() then exit sub
  if en.slide >= 0 then
    if key.held >= 0 then en.inp = gm.kd(key.held)
    exit sub
  end if
  gm_d = key.pop()
  if gm_d < 0 and key.held >= 0 then
    if gm_now >= key.t0 + REPEAT_MS then gm_d = key.held
  end if
  if gm_d >= 0 then en.inp = gm.kd(gm_d)
end sub

' the key handler and the beat: on while a level is played, off for
' dialogs and banners. Switching on starts afresh: no keys typed before
' count, the next tick is TICK_MS from now.
sub gm.handlers(hon)
  if hon then
    key.reset()
    gm.due = timer + TICK_MS
    on key key.onKey
    settick TICK_RATE, gm.tick
  else
    settick 0, gm.tick
    on key 0
  end if
end sub

' draw what the ticks changed (the tick waits meanwhile)
sub gm.draw()
  gm.drawing = TRUE
  if timer >= vw.anext then vw.anim()
  if vw.ak < vw.aend then vw.animStep()
  vw.update()
  hud.update()
  if scr.dirty then scr.flush()
  gm.drawing = FALSE
end sub

' pause: the dialog covers the scene; afterwards the scene comes back
function gm.pauseGame()
  gm.handlers(FALSE)
  gm.pauseGame = dlg.pause()
  scr.present()
  if gm.pauseGame = DLG_RESUME then gm.handlers(TRUE)
end function

' ---- one attempt at the level: GE_WON, GE_DEAD, GE_RESTART or GE_QUIT.
' The main loop only draws and handles what the handlers flagged; it wakes
' every IDLE_MS (the tick and ON KEY also run during its PAUSE).
function gm.play()
  local r
  gm.res = GE_NONE
  gm.drawing = FALSE
  gm.handlers(TRUE)
  do
    if key.pause then
      key.pause = FALSE
      r = gm.pauseGame()
      if r = DLG_QUIT then
        gm.play = GE_QUIT
        exit function
      elseif r = DLG_RESTART then
        gm.play = GE_RESTART
        exit function
      end if
    end if
    if key.restart then
      gm.res = GE_RESTART
      exit do
    end if
    gm.draw()
    if gm.res <> GE_NONE then exit do
    pause IDLE_MS
  loop
  gm.handlers(FALSE)
  gm.draw()
  gm.play = gm.res
end function

' set up level gm.li and show it (view + panel on PG_BACK, then the screen)
sub gm.setup()
  en.load(gm.li, gm.diff)
  vw.start()
  hud.all()
  scr.present()
end sub

' what ended the attempt, for the banner
function gm.cause$()
  select case en.dead
    case D_DROWN
      gm.cause$ = "SHORTED OUT IN THE COOLANT"
    case D_BURN
      gm.cause$ = "BURNED UP IN THE FIRE"
    case D_BOMB
      gm.cause$ = "BOOM! THAT WAS A BOMB"
    case D_TIME
      gm.cause$ = "OUT OF TIME"
    case else
      gm.cause$ = "CAUGHT BY A " + gm.monAt$()
  end select
end function

' the monster on (or next to) the player's cell
function gm.monAt$()
  local m
  m = lm(en.pi)
  gm.monAt$ = "MONSTER"
  if m > 0 then gm.monAt$ = en.mname$(mk(m - 1))
end function

' points for a cleared level: seconds left, minus a little per retry
function gm.points()
  gm.points = max(0, PTS_SEC * en.tl - PTS_RETRY * gm.retries)
end function

sub game.run(dif, start)
  local r, pts, gQuit, gWon, first
  gm.diff = dif
  gm.li = start
  gm.start = start
  gm.score = 0
  gm.reached = start
  gQuit = FALSE
  gWon = FALSE
  do
    hs.reach(dif) = max(hs.reach(dif), gm.li)
    gm.reached = max(gm.reached, gm.li)
    gm.retries = 0
    first = TRUE
    do
      gm.setup()
      if first then dlg.banner("LEVEL " + hud.n2$(gm.li + 1), lv.name$(gm.li), F_CYAN, gm.goal$(), F_GREY, 2000)
      first = FALSE
      r = gm.play()
      if r = GE_WON or r = GE_QUIT then exit do
      if r = GE_DEAD then
        pause 700
        dlg.banner("TRY AGAIN", gm.cause$(), F_COPPER, "RETRIES SO FAR: " + str$(gm.retries + 1), F_GREY, 1800)
      end if
      gm.retries = gm.retries + 1
    loop
    if r = GE_QUIT then
      gQuit = TRUE
      exit do
    end if
    pts = gm.points()
    gm.score = gm.score + pts
    pause 300
    dlg.banner("LEVEL COMPLETE", "TIME LEFT " + str$(en.tl) + "  RETRIES " + str$(gm.retries), F_CYAN, "+" + str$(pts) + " POINTS", F_GOLD, 2200)
    gm.li = gm.li + 1
    if gm.li >= LV_N then
      gWon = TRUE
      gm.reached = LV_N
      exit do
    end if
  loop
  ' the next level counts as reached, so it can be picked on the title
  if gm.li < LV_N then hs.reach(dif) = max(hs.reach(dif), gm.li)
  hs.save()
  dlg.gameOver(gWon, gQuit)
end sub

function gm.goal$()
  if en.cl = 1 then
    gm.goal$ = "COLLECT 1 CHIP"
  else
    gm.goal$ = "COLLECT " + str$(en.cl) + " CHIPS"
  end if
end function
