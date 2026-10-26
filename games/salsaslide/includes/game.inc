' game.inc - one game: rounds in order, the player's input, the beat, lives
' lost, round clears, the Salsa Shuffle every 4th round and the score.
'
' How a round runs - two interrupt handlers and the main loop:
'  key.onKey (ON KEY, input.inc) notes arrow taps, Space, P / Esc and R.
'  gm.tick (SETTICK, every TICK_RATE ms) is the beat: it notices released
'    keys and, every TICK_MS (on fixed "due" times), turns the keys into the
'    cook's input and runs one engine tick (en.step). It never draws. When
'    the round is won or a life is lost it sets gm.res and stops stepping
'    until the main loop has dealt with it.
'  The main loop (gm.play) draws: the sprites (view.inc) and the status bar
'    (hud.inc), and opens the pause dialog. While it draws (gm.drawing) the
'    tick skips its step, so a frame never shows a half-done tick.
' Both handlers are only on during play; menus and dialogs read INKEY$.
'
' Controls: Up / Down jump to the next counter (wrapping round), Left / Right
' walk along it (a tap = one stride, held = keeps walking after REPEAT_MS),
' Space slides a full plate (only at the pumps; one plate per press).
const IDLE_MS = 4, TICK_RATE = 5, TICK_MS = 20, REPEAT_MS = 160
const GE_NONE = 0, GE_WON = 1, GE_LOST = 2, GE_RESTART = 3, GE_QUIT = 4
const BONUS_EVERY = 4

dim gm.res, gm.drawing, gm.due, gm.drawn, gm.start, gm.rd
dim integer gm_now, gm_d
dim gm.bit(3) = (1, 2, 4, 8)

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
  if en.lost then
    gm.res = GE_LOST
  elseif en.won then
    gm.res = GE_WON
  end if
end sub

' keys -> en.inp for this tick. Taps wait until the cook may act; a held
' Left / Right keeps walking; Space waits for the cook at the pumps (and is
' dropped while he is away from them).
sub gm.input()
  en.inp = 0
  if en.show > 0 then exit sub
  if ck.x <> HOME and ck.wn = 0 then key.sv = 0
  if ck.sv <> 0 or ck.wn <> 0 then exit sub
  gm_d = key.pop()
  if gm_d < 0 and key.held >= 2 then
    if gm_now >= key.t0 + REPEAT_MS then gm_d = key.held
  end if
  if gm_d >= 0 then
    en.inp = gm.bit(gm_d)
  elseif key.sv > 0 and ck.x = HOME then
    en.inp = IN_SERVE
    key.sv = key.sv - 1
  end if
end sub

' the key handler and the beat: on while a round is played, off for dialogs
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
  if en.tk = gm.drawn then exit sub
  gm.drawing = TRUE
  gm.drawn = en.tk
  vw.update()
  hud.update()
  if scr.dirty then scr.flush()
  gm.drawing = FALSE
end sub

function gm.pauseGame()
  gm.handlers(FALSE)
  gm.pauseGame = dlg.pause()
  scr.present()
  if gm.pauseGame = DLG_RESUME then gm.handlers(TRUE)
end function

' ---- play until the round is won, a life is lost, or restart / quit
function gm.play()
  local r
  gm.res = GE_NONE
  gm.drawing = FALSE
  gm.drawn = -1
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

' a clean cantina: no sprites, the backdrop, the status bar
sub gm.board()
  vw.clear()
  page copy PG_BG to PG_BACK
  page write PG_BACK
  hud.all()
  gm.drawn = -1
  vw.update()
  scr.present()
end sub

' the life-lost scene: the cook reels, the culprit stamps / the plate smashes
sub gm.lostAnim()
  local k, t
  t = timer
  for k = 0 to 75
    vw.lostFrame(k)
    if scr.dirty then scr.flush()
    do while timer < t + 20 * (k + 1)
      pause 2
    loop
  next k
end sub

function gm.cause$()
  select case en.lost
    case L_REACH
      gm.cause$ = "A HUNGRY CUSTOMER GOT TO YOU"
    case L_FULL
      gm.cause$ = "A FULL PLATE SLID OFF THE END"
    case else
      gm.cause$ = "AN EMPTY PLATE HIT THE FLOOR"
  end select
end function

function gm.livesLine$()
  if en.lives = 1 then
    gm.livesLine$ = "LAST CHEF HAT!"
  else
    gm.livesLine$ = str$(en.lives) + " CHEF HATS LEFT"
  end if
end function

sub game.run(dif, start)
  local r, lv0, sc0, cleared, over, qt
  gm.start = start
  gm.rd = start
  cleared = 0
  over = FALSE
  qt = FALSE
  en.newGame(timer mod 2147483647 + 1, dif, 3, 0)
  do
    hs.reach(dif) = max(hs.reach(dif), min(RD_MAX, gm.rd))
    lv0 = en.lives
    sc0 = en.score
    en.start(gm.rd)
    gm.board()
    dlg.banner("ROUND " + str$(gm.rd + 1), "SERVE " + str$(en.total) + " HUNGRY CUSTOMERS", F_CREAM, "", F_CREAM, 1800)
    do
      r = gm.play()
      if r = GE_LOST then
        gm.lostAnim()
        en.lifeLost()
        if en.lives <= 0 then
          over = TRUE
          exit do
        end if
        gm.board()
        dlg.banner("OOPS!", gm.cause$(), F_CREAM, gm.livesLine$(), F_GOLD, 1600)
      elseif r = GE_RESTART then
        en.lives = lv0
        en.score = sc0
        en.start(gm.rd)
        gm.board()
        dlg.banner("ROUND " + str$(gm.rd + 1), "START AGAIN", F_CREAM, "", F_CREAM, 1200)
      else
        exit do
      end if
    loop
    if over then exit do
    if r = GE_QUIT then
      qt = TRUE
      exit do
    end if
    ' round cleared
    pause 400
    dlg.clear(gm.rd + 1, en.plates, en.tips, 2600)
    cleared = cleared + 1
    gm.rd = gm.rd + 1
    if gm.rd mod BONUS_EVERY = 0 then
      bonus.run(gm.rd \ BONUS_EVERY)
      if bw.res = BR_QUIT then
        qt = TRUE
        exit do
      end if
    end if
  loop
  vw.clear()
  ' the next round counts as reached, so it can be picked on the title
  hs.reach(dif) = max(hs.reach(dif), min(RD_MAX, gm.rd))
  hs.save()
  dlg.gameOver(cleared, gm.rd + 1)
end sub
